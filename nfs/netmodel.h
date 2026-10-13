/* netmodel.h -- the Ethernet device of the CDC core as the NFS driver (nfsdrv.pas) sees it, and the server behind it.
 *
 * Device: the ethmin I/O space (fpga/vc707-ethmin/ethmin_vm_core.v) with one byte per word:
 *   0x0000..0x07FF RX window   0x0800..0x0FFF TX window
 *   0x1000 status (1 frame waiting, 2 transmitter busy, 4 truncated)   0x1002 received length / write: release
 *   0x1003 write: send the TX window, this long   0x1005 write: console character   0x1006 milliseconds
 *
 * Server (192.168.1.109): answers ARP, and over UDP portmap (111), mountd (20048) and nfsd (2049), ONC RPC / NFS v3:
 * GETPORT, MNT, LOOKUP, READ, CREATE, WRITE.  The export is the directory net_root; every MNT path maps to it, file names are
 * taken from the root.  File handles are 8 bytes (an index).  Time is simulated: ten milliseconds per status read.
 */
#ifndef NETMODEL_H
#define NETMODEL_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define NET_DEVSIZE 0x1010
static const char *net_root = ".";
static int net_verbose;

static const uint8_t nm_srv_mac[6] = {0x10, 0xe2, 0xd5, 0x00, 0x00, 0x01};
static const uint8_t nm_srv_ip[4] = {192, 168, 1, 109};

static uint8_t nm_rx[2048], nm_tx[2048];
static int nm_rxlen, nm_rxvalid;
static struct { int len; uint8_t b[1536]; } nm_q[16];
static int nm_qh, nm_qn;
static long nm_ms;
static int nm_dip, nm_nodhcp;            /* the DIP switches (-D n); -N: no DHCP server on the network */
static long nm_frames_tx, nm_frames_rx;

static char nm_names[256][512];
static int nm_nn = 1;               /* names[0] is the root */

static uint32_t nm_r32(const uint8_t *p) { return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }
static void nm_w32(uint8_t *p, uint32_t v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

static void nm_load_rx(void)
{
    if (!nm_rxvalid && nm_qn) {
        memset(nm_rx, 0, sizeof nm_rx);
        memcpy(nm_rx, nm_q[nm_qh].b, nm_q[nm_qh].len);
        nm_rxlen = nm_q[nm_qh].len;
        nm_qh = (nm_qh + 1) % 16; nm_qn--; nm_rxvalid = 1; nm_frames_rx++;
    }
}
static void nm_enqueue(const uint8_t *b, int len)
{
    int i = (nm_qh + nm_qn) % 16;
    if (nm_qn == 16) { fprintf(stderr, "netmodel: reply queue full\n"); return; }
    memcpy(nm_q[i].b, b, len); nm_q[i].len = len < 60 ? 60 : len; nm_qn++;
    if (len < 60) memset(nm_q[i].b + len, 0, 60 - len);
}

static uint16_t nm_csum(const uint8_t *p, int len)
{
    uint32_t s = 0; int i;
    for (i = 0; i + 1 < len; i += 2) s += p[i] << 8 | p[i + 1];
    if (len & 1) s += p[len - 1] << 8;
    while (s >> 16) s = (s & 0xFFFF) + (s >> 16);
    return (uint16_t)~s;
}

static int nm_fh(const char *rel)
{
    int i;
    for (i = 0; i < nm_nn; i++) if (!strcmp(nm_names[i], rel)) return i;
    if (nm_nn == 256) return 0;
    snprintf(nm_names[nm_nn], sizeof nm_names[0], "%s", rel);
    return nm_nn++;
}
static void nm_path(int k, char *out, size_t n)
{
    snprintf(out, n, "%s%s%s", net_root, nm_names[k][0] ? "/" : "", nm_names[k]);
}

/* append `n` bytes as an XDR opaque */
static int nm_xopaque(uint8_t *p, const uint8_t *d, int n)
{
    int pad = (4 - (n & 3)) & 3;
    nm_w32(p, n); memcpy(p + 4, d, n); memset(p + 4 + n, 0, pad);
    return 4 + n + pad;
}
static void nm_fillattr(uint8_t *q, const struct stat *st)      /* fattr3, 84 bytes, RFC 1813 */
{
    long long size = st->st_size;
    memset(q, 0, 84);
    nm_w32(q, S_ISDIR(st->st_mode) ? 2 : 1);
    nm_w32(q + 4, st->st_mode & 07777); nm_w32(q + 8, st->st_nlink);
    nm_w32(q + 20, size >> 32); nm_w32(q + 24, size & 0xffffffff);
    nm_w32(q + 28, 0); nm_w32(q + 32, (st->st_blocks * 512) & 0xffffffff);
    nm_w32(q + 48, 1); nm_w32(q + 56, st->st_ino & 0xffffffff);
    nm_w32(q + 60, st->st_atime); nm_w32(q + 68, st->st_mtime); nm_w32(q + 76, st->st_ctime);
}
static int nm_fattr(uint8_t *p, int fh)   /* post_op_attr with attributes (flag 1 + 84 bytes) */
{
    char path[512]; struct stat st;
    nm_path(fh, path, sizeof path);
    if (stat(path, &st)) { nm_w32(p, 0); return 4; }
    nm_w32(p, 1); nm_fillattr(p + 4, &st);
    return 88;
}
static int nm_pathfh(const char *rel, uint8_t *p)    /* post_op_fh3: flag + handle */
{
    uint8_t fh[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    nm_w32(p, 1); nm_w32(fh + 4, nm_fh(rel));
    return 4 + nm_xopaque(p + 4, fh, 8);
}
/* the relative path of name in directory handle k ("." and ".." are understood), into out */
static void nm_join(int k, const char *name, char *out, size_t n)
{
    if (!strcmp(name, ".")) snprintf(out, n, "%s", nm_names[k]);
    else if (!strcmp(name, "..")) {
        char *sl; snprintf(out, n, "%s", nm_names[k]);
        sl = strrchr(out, '/'); if (sl) *sl = 0; else out[0] = 0;
    } else snprintf(out, n, "%s%s%s", nm_names[k], nm_names[k][0] ? "/" : "", name);
}
static int nm_getname(const uint8_t *p, char *name)   /* an XDR string; returns the bytes it used */
{
    uint32_t nl = nm_r32(p); if (nl > 127) nl = 127;
    memcpy(name, p + 4, nl); name[nl] = 0;
    return 4 + (nm_r32(p) + 3) / 4 * 4;
}
static int nm_cmpname(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }
static int nm_dirfh(const uint8_t *a) { return (int)nm_r32(a + 8); }      /* the handle's second word: the index */

/* handle one RPC call; the reply body (after the accept_stat) goes to r, its length returned; -1 = no reply */
static int nm_rpc(uint32_t prog, uint32_t proc, const uint8_t *a, int alen, uint8_t *r)
{
    int n = 0;
    (void)alen;
    if (prog == 100000) {                               /* portmap */
        if (proc == 3) {
            uint32_t p = nm_r32(a);
            nm_w32(r, p == 100005 ? 20048 : p == 100003 ? 2049 : 0); return 4;
        }
        return -1;
    }
    if (prog == 100005) {                               /* mount */
        if (proc == 1) {
            uint8_t fh[8] = {0, 0, 0, 0, 0, 0, 0, 0};
            nm_w32(r, 0); n = 4;
            n += nm_xopaque(r + n, fh, 8);
            nm_w32(r + n, 1); nm_w32(r + n + 4, 1); n += 8;     /* auth flavors: AUTH_UNIX */
            return n;
        }
        return -1;
    }
    if (prog == 100003) {                               /* nfs v3 */
        uint32_t fl = nm_r32(a); int dfh;
        char name[128], rel[512], path[1024]; int off;
        if (fl != 8) return -1;
        dfh = nm_dirfh(a);
        off = 4 + 8;                                     /* past the handle (length word + 8 bytes) */
        switch (proc) {
        case 1: {                                       /* GETATTR */
            struct stat st;
            nm_path(dfh, path, sizeof path);
            if (stat(path, &st)) { nm_w32(r, 2); return 4; }
            nm_w32(r, 0); nm_fillattr(r + 4, &st); return 88; }
        case 3: {                                       /* LOOKUP */
            struct stat st;
            nm_getname(a + off, name); nm_join(dfh, name, rel, sizeof rel);
            snprintf(path, sizeof path, "%s%s%s", net_root, rel[0] ? "/" : "", rel);
            if (stat(path, &st)) { nm_w32(r, 2); nm_w32(r + 4, 0); return 8; }      /* NFS3ERR_NOENT, no dir attr */
            nm_w32(r, 0); n = 4;
            { uint8_t fh[8] = {0, 0, 0, 0, 0, 0, 0, 0}; nm_w32(fh + 4, nm_fh(rel)); n += nm_xopaque(r + n, fh, 8); }
            n += nm_fattr(r + n, nm_fh(rel));
            nm_w32(r + n, 0); n += 4;
            return n; }
        case 6: {                                       /* READ */
            int fd; uint32_t cnt = nm_r32(a + off + 8);
            uint64_t o = (uint64_t)nm_r32(a + off + 0) << 32 | nm_r32(a + off + 4);
            long got = 0; struct stat st; int eof;
            if (cnt > 1024) cnt = 1024;
            nm_path(dfh, path, sizeof path);
            fd = open(path, O_RDONLY);
            if (fd < 0) { nm_w32(r, 2); nm_w32(r + 4, 0); return 8; }
            { uint8_t buf[1024]; got = pread(fd, buf, cnt, o); if (got < 0) got = 0;
              fstat(fd, &st); eof = (long)(o + got) >= st.st_size;
              nm_w32(r, 0); n = 4;
              n += nm_fattr(r + n, dfh);
              nm_w32(r + n, got); nm_w32(r + n + 4, eof); n += 8;
              n += nm_xopaque(r + n, buf, got); }
            close(fd);
            return n; }
        case 8: {                                       /* CREATE */
            int fd;
            nm_getname(a + off, name); nm_join(dfh, name, rel, sizeof rel);
            snprintf(path, sizeof path, "%s%s%s", net_root, rel[0] ? "/" : "", rel);
            fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { nm_w32(r, 13); nm_w32(r + 4, 0); nm_w32(r + 8, 0); return 12; }
            close(fd);
            nm_w32(r, 0); nm_w32(r + 4, 1); n = 8;
            { uint8_t fh[8] = {0, 0, 0, 0, 0, 0, 0, 0}; nm_w32(fh + 4, nm_fh(rel)); n += nm_xopaque(r + n, fh, 8); }
            nm_w32(r + n, 0); n += 4;                    /* obj_attributes: none */
            nm_w32(r + n, 0); nm_w32(r + n + 4, 0); n += 8;   /* dir_wcc */
            return n; }
        case 7: {                                       /* WRITE */
            int fd;
            uint64_t o = (uint64_t)nm_r32(a + off + 0) << 32 | nm_r32(a + off + 4);
            uint32_t cnt = nm_r32(a + off + 8); long put;
            nm_path(dfh, path, sizeof path);
            fd = open(path, O_WRONLY);
            if (fd < 0) { nm_w32(r, 2); nm_w32(r + 4, 0); nm_w32(r + 8, 0); return 12; }
            put = pwrite(fd, a + off + 20, cnt, o);
            close(fd);
            nm_w32(r, 0); n = 4;
            nm_w32(r + n, 1); memset(r + n + 4, 0, 24); n += 28;      /* pre_op_attr: size, mtime, ctime */
            nm_w32(r + n, 0); n += 4;                                  /* post_op_attr: none */
            nm_w32(r + n, put); nm_w32(r + n + 4, 2); memset(r + n + 8, 0x5a, 8); n += 16;
            return n; }
        case 9: case 12: case 13: {                     /* MKDIR, REMOVE, RMDIR */
            int rc;
            nm_getname(a + off, name); nm_join(dfh, name, rel, sizeof rel);
            snprintf(path, sizeof path, "%s%s%s", net_root, rel[0] ? "/" : "", rel);
            rc = proc == 9 ? mkdir(path, 0755) : proc == 12 ? unlink(path) : rmdir(path);
            if (rc) { nm_w32(r, errno == ENOENT ? 2 : errno == EEXIST ? 17 : errno == ENOTEMPTY ? 66 : 13); n = 4; }
            else { nm_w32(r, 0); n = 4; }
            if (proc == 9) { if (rc) { nm_w32(r + n, 0); n += 4; } else n += nm_pathfh(rel, r + n); nm_w32(r + n, 0); n += 4; }
            nm_w32(r + n, 0); nm_w32(r + n + 4, 0); n += 8;           /* dir_wcc */
            return n; }
        case 14: {                                      /* RENAME */
            char name2[128], rel2[512], path2[1024]; int d2, c, rc;
            c = off + nm_getname(a + off, name);
            d2 = (int)nm_r32(a + c + 8);
            c += 4 + 8;
            nm_getname(a + c, name2);
            nm_join(dfh, name, rel, sizeof rel); nm_join(d2, name2, rel2, sizeof rel2);
            snprintf(path, sizeof path, "%s%s%s", net_root, rel[0] ? "/" : "", rel);
            snprintf(path2, sizeof path2, "%s%s%s", net_root, rel2[0] ? "/" : "", rel2);
            rc = rename(path, path2);
            nm_w32(r, rc ? (errno == ENOENT ? 2 : 13) : 0);
            nm_w32(r + 4, 0); nm_w32(r + 8, 0); nm_w32(r + 12, 0); nm_w32(r + 16, 0);   /* two wcc_data */
            return 20; }
        case 17: {                                      /* READDIRPLUS */
            DIR *dp; struct dirent *de; char *names[512]; int nn = 0, i; uint64_t ck;
            uint32_t maxc = nm_r32(a + off + 8 + 8 + 4);
            struct stat st; int used;
            ck = (uint64_t)nm_r32(a + off) << 32 | nm_r32(a + off + 4);
            nm_path(dfh, path, sizeof path);
            dp = opendir(path);
            if (!dp) { nm_w32(r, 20); nm_w32(r + 4, 0); return 8; }
            while ((de = readdir(dp)) && nn < 512) names[nn++] = strdup(de->d_name);
            closedir(dp);
            qsort(names, nn, sizeof names[0], nm_cmpname);
            nm_w32(r, 0); n = 4;
            n += nm_fattr(r + n, dfh);
            memset(r + n, 0, 8); n += 8;                  /* cookieverf */
            if (maxc > 1400) maxc = 1400;
            used = 4 + 88 + 8 + 8;
            for (i = (int)ck; i < nn; i++) {
                int en; char rl[512], pa[1024]; uint8_t ent[512]; int el = 0;
                nm_join(dfh, names[i], rl, sizeof rl);
                snprintf(pa, sizeof pa, "%s%s%s", net_root, rl[0] ? "/" : "", rl);
                nm_w32(ent, 1); memset(ent + 4, 0, 4); nm_w32(ent + 8, i + 1); el = 12;    /* value follows, fileid */
                el += nm_xopaque(ent + el, (const uint8_t *)names[i], (int)strlen(names[i]));
                nm_w32(ent + el, 0); nm_w32(ent + el + 4, i + 1); el += 8;                  /* cookie */
                if (!strcmp(names[i], ".") || !strcmp(names[i], "..") || stat(pa, &st)) { nm_w32(ent + el, 0); el += 4; nm_w32(ent + el, 0); el += 4; }
                else { nm_w32(ent + el, 1); nm_fillattr(ent + el + 4, &st); el += 88; el += nm_pathfh(rl, ent + el); }
                en = el;
                if (used + en + 8 > (int)maxc) break;
                memcpy(r + n, ent, en); n += en; used += en;
            }
            nm_w32(r + n, 0); n += 4;                       /* no more entries in this reply */
            nm_w32(r + n, i >= nn ? 1 : 0); n += 4;         /* eof */
            for (i = 0; i < nn; i++) free(names[i]);
            return n; }
        case 18: {                                      /* FSSTAT */
            nm_w32(r, 0); n = 4; n += nm_fattr(r + n, dfh);
            { uint64_t tot = 200ULL << 30, fre = 50ULL << 30;
              nm_w32(r + n, tot >> 32); nm_w32(r + n + 4, tot & 0xffffffff);
              nm_w32(r + n + 8, fre >> 32); nm_w32(r + n + 12, fre & 0xffffffff);
              nm_w32(r + n + 16, fre >> 32); nm_w32(r + n + 20, fre & 0xffffffff);
              nm_w32(r + n + 24, 0); nm_w32(r + n + 28, 1000000); nm_w32(r + n + 32, 0); nm_w32(r + n + 36, 900000);
              nm_w32(r + n + 40, 0); nm_w32(r + n + 44, 900000); nm_w32(r + n + 48, 0); n += 52; }
            return n; }
        }
    }
    return -1;
}

/* a frame from the program */
static void nm_frame(const uint8_t *f, int len)
{
    nm_frames_tx++;
    if (len < 42) return;
    if (f[12] == 0x08 && f[13] == 0x06 && f[21] == 1 && !memcmp(f + 38, nm_srv_ip, 4)) {       /* ARP request for the server */
        uint8_t r[64]; memset(r, 0, sizeof r);
        memcpy(r, f + 22, 6); memcpy(r + 6, nm_srv_mac, 6); r[12] = 8; r[13] = 6;
        r[14] = 0; r[15] = 1; r[16] = 8; r[17] = 0; r[18] = 6; r[19] = 4; r[20] = 0; r[21] = 2;
        memcpy(r + 22, nm_srv_mac, 6); memcpy(r + 28, nm_srv_ip, 4); memcpy(r + 32, f + 22, 6); memcpy(r + 38, f + 28, 4);
        nm_enqueue(r, 42);
        return;
    }
    if (f[12] == 0x08 && f[13] == 0x00 && f[23] == 17 && f[36] == 0 && f[37] == 67 && len >= 282) {      /* DHCP */
        static const uint8_t offer[4] = {192, 168, 1, 178};            /* what the model hands out: not the static address */
        uint8_t r[400]; int mt = 0, c = 282, n, ipl; uint16_t cs;
        while (c + 1 < len && f[c] != 255) { if (f[c] == 0) { c++; continue; } if (f[c] == 53) mt = f[c + 2]; c += 2 + f[c + 1]; }
        if ((mt != 1 && mt != 3) || nm_nodhcp) return;
        memset(r, 0, sizeof r);
        memcpy(r, f + 70, 6); memcpy(r + 6, nm_srv_mac, 6); r[12] = 8; r[13] = 0;
        r[14] = 0x45; r[22] = 64; r[23] = 17; memcpy(r + 26, nm_srv_ip, 4); memset(r + 30, 255, 4);
        r[34] = 0; r[35] = 67; r[36] = 0; r[37] = 68;
        r[42] = 2; r[43] = 1; r[44] = 6; memcpy(r + 46, f + 46, 4); memcpy(r + 52, f + 52, 2);
        memcpy(r + 58, offer, 4); memcpy(r + 62, nm_srv_ip, 4); memcpy(r + 70, f + 70, 6);
        r[278] = 99; r[279] = 130; r[280] = 83; r[281] = 99;
        n = 282;
        r[n++] = 53; r[n++] = 1; r[n++] = mt == 1 ? 2 : 5;
        r[n++] = 54; r[n++] = 4; memcpy(r + n, nm_srv_ip, 4); n += 4;
        r[n++] = 51; r[n++] = 4; nm_w32(r + n, 86400); n += 4;
        r[n++] = 1; r[n++] = 4; r[n++] = 255; r[n++] = 255; r[n++] = 255; r[n++] = 0;
        r[n++] = 3; r[n++] = 4; memcpy(r + n, nm_srv_ip, 4); n += 4;
        r[n++] = 255;
        if (n < 300) n = 300;
        ipl = n - 14;
        r[16] = ipl >> 8; r[17] = ipl & 255;
        cs = nm_csum(r + 14, 20); r[24] = cs >> 8; r[25] = cs & 255;
        r[38] = (n - 34) >> 8; r[39] = (n - 34) & 255;
        nm_enqueue(r, n);
        return;
    }
    if (f[12] == 0x08 && f[13] == 0x00 && f[14] == 0x45 && f[23] == 17 && !memcmp(f + 30, nm_srv_ip, 4)) {
        int ipl = f[16] << 8 | f[17], dport = f[36] << 8 | f[37], sport = f[34] << 8 | f[35];
        uint8_t rep[1536]; uint8_t *x = rep + 42; int n;
        const uint8_t *call = f + 42;
        uint32_t prog, vers, proc; int c;
        (void)vers;
        if (nm_csum(f + 14, 20) != 0) { fprintf(stderr, "netmodel: bad IP header checksum\n"); return; }
        if (!(dport == 111 || dport == 20048 || dport == 2049)) return;
        if (nm_r32(call + 4) != 0 || nm_r32(call + 8) != 2) { fprintf(stderr, "netmodel: not an RPC call\n"); return; }
        prog = nm_r32(call + 12); vers = nm_r32(call + 16); proc = nm_r32(call + 20);
        c = 24;
        c += 8 + (nm_r32(call + c + 4) + 3) / 4 * 4;               /* credential */
        c += 8 + (nm_r32(call + c + 4) + 3) / 4 * 4;               /* verifier */
        nm_w32(x, nm_r32(call)); nm_w32(x + 4, 1); nm_w32(x + 8, 0);       /* xid, REPLY, MSG_ACCEPTED */
        nm_w32(x + 12, 0); nm_w32(x + 16, 0); nm_w32(x + 20, 0);            /* verifier, SUCCESS */
        n = nm_rpc(prog, proc, call + c, ipl - 28 - c, x + 24);
        if (n < 0) { fprintf(stderr, "netmodel: unsupported RPC %u/%u\n", prog, proc); return; }
        n += 24;
        memcpy(rep, f + 6, 6); memcpy(rep + 6, nm_srv_mac, 6); rep[12] = 8; rep[13] = 0;
        rep[14] = 0x45; rep[15] = 0; rep[16] = (28 + n) >> 8; rep[17] = (28 + n) & 255; memset(rep + 18, 0, 4);
        rep[22] = 64; rep[23] = 17; rep[24] = rep[25] = 0; memcpy(rep + 26, nm_srv_ip, 4); memcpy(rep + 30, f + 26, 4);
        { uint16_t s = nm_csum(rep + 14, 20); rep[24] = s >> 8; rep[25] = s & 255; }
        rep[34] = dport >> 8; rep[35] = dport & 255; rep[36] = sport >> 8; rep[37] = sport & 255;
        rep[38] = (8 + n) >> 8; rep[39] = (8 + n) & 255; rep[40] = rep[41] = 0;
        nm_enqueue(rep, 42 + n);
    }
}

/* frame level, for a testbench that drives the hardware's MAC stream: the next reply frame (length, 0 if none), and a frame the
 * hardware sent */
static int net_pop_reply(uint8_t *buf)
{
    int n;
    if (!nm_qn) return 0;
    n = nm_q[nm_qh].len; memcpy(buf, nm_q[nm_qh].b, n);
    nm_qh = (nm_qh + 1) % 16; nm_qn--;
    return n;
}
static void net_push_frame(const uint8_t *f, int len) { nm_frame(f, len); }

/* the console UART: bytes for the program come from nm_uart_in (-u file); when they are used up and the program asks for
   more, nm_uart_eof is set and the harness stops (the shell is idle).  What the program sends goes to stdout. */
static FILE *nm_uart_in;
static int nm_uart_eof;
static uint64_t nm_cyc;
static uint64_t nm_uart_get(void)
{
    /* a person types slowly: the next byte is there after the program has asked a few hundred times */
    static int polls;
    int c;
    if (++polls < 400) return 0;
    polls = 0;
    c = nm_uart_in ? fgetc(nm_uart_in) : EOF;
    if (c == EOF) { nm_uart_eof = 1; return 0; }
    return 256 | (uint64_t)c;
}

static uint64_t net_read(unsigned off)
{
    if (off < 0x800) return nm_rx[off];
    if (off < 0x1000) return nm_tx[off - 0x800];
    switch (off) {
    case 0x1000:
        nm_ms += 10; nm_load_rx();
        return nm_rxvalid ? 1 : 0;
    case 0x1002: return nm_rxlen;
    case 0x1006: return nm_ms & 0x3fffffff;
    case 0x1008: return nm_uart_get();               /* UART receive: 256 + byte, 0 if none */
    case 0x1009: return nm_dip;
    case 0x100a: nm_cyc += 211; return nm_cyc & 0xffffffffULL;   /* clock cycle count */
    }
    return 0;
}
static void net_write(unsigned off, uint64_t v)
{
    if (off < 0x800) { nm_rx[off] = v & 255; return; }
    if (off < 0x1000) { nm_tx[off - 0x800] = v & 255; return; }
    switch (off) {
    case 0x1002: nm_rxvalid = 0; nm_rxlen = 0; nm_load_rx(); break;
    case 0x1003: nm_frame(nm_tx, (int)v); break;
    case 0x1005: fputc((int)(v & 255), stdout); fflush(stdout); break;
    }
}
#endif
