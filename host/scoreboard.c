/* scoreboard.c -- a trace-driven timing model for the CDC 6000 style core.
 *
 *   cdcemu runtime.img program.img ... -T trace.bin       (4 byte records, see cdcemu.c)
 *   scoreboard [key=value ...] < trace.bin
 *
 * It does not execute anything.  For every instruction of the trace it works out when it can issue, when it can start
 * (its operands are there), when its result exists, under a model of a core with in-order issue, several functional units
 * and a scoreboard in the manner of the machine's own organisation, which is described in the open literature (J. E.
 * Thornton, "Design of a Computer: the Control Data 6600", 1970): an instruction is issued if its functional unit is free and
 * its destination register is not reserved by an earlier instruction; it then waits in the unit for its operands; a result
 * is not written while an earlier instruction still has to read the old value of the register.
 *
 * mode=seq reproduces the present core (rtl/cdc6000_core.sv): one instruction at a time, two cycles to fetch every 60 bit
 * word, and the extra cycles of a load, a floating operation and a division.  Compare its answer with the cycles the
 * RTL really takes to check the model, then use mode=sb for the proposals.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { U_BR, U_BOOL, U_SHIFT, U_FADD, U_LADD, U_MUL, U_DIV, U_INC, U_NONE, NU };
static const char *uname[NU] = {"branch", "boolean", "shift", "float add", "long add", "multiply", "divide", "increment", "-"};

/* configuration: units of a kind, result latency (cycles from start to result), busy (cycles before the unit takes another) */
static struct { int n, lat, busy; } cfg[NU] = {
    [U_BR] = {1, 1, 1}, [U_BOOL] = {1, 1, 1}, [U_SHIFT] = {1, 2, 2}, [U_FADD] = {1, 3, 3}, [U_LADD] = {1, 2, 2},
    [U_MUL] = {1, 4, 1}, [U_DIV] = {1, 52, 52}, [U_INC] = {2, 2, 1}, [U_NONE] = {1, 1, 1}};
static int inorder = 0, dropno = 0, mode_sb = 1, width = 1, memlat = 3, memports = 1, brpen = 3, spec = 0, ibuf = 0, fwd = 0, trace_n = 0;
static long long limit = 0;

#define NREG 24            /* X0-7, A0-7, B0-7 */
struct ins { int unit, nd, d[3], ns, s[3], load, store, br, taken, newword; };

static int X(int i) { return i; }
static int A(int i) { return 8 + i; }
static int B(int i) { return 16 + i; }

static void dst(struct ins *t, int r) { t->d[t->nd++] = r; }
static void src(struct ins *t, int r) { if (r != B(0)) t->s[t->ns++] = r; }

static void decode(struct ins *t, int f, int i, int j, int k, int flags)
{
    memset(t, 0, sizeof *t);
    t->unit = U_NONE; t->taken = (flags >> 1) & 1; t->newword = (flags >> 2) & 1;
    switch (f) {
    case 0: case 046: break;
    case 1: t->unit = U_BR; t->br = 1; t->store = 1; break;
    case 2: t->unit = U_BR; t->br = 1; src(t, B(i)); break;
    case 3: t->unit = U_BR; t->br = 1; src(t, X(j)); break;
    case 4: case 5: case 6: case 7: t->unit = U_BR; t->br = 1; src(t, B(i)); src(t, B(j)); break;
    case 010: t->unit = U_BOOL; dst(t, X(i)); src(t, X(j)); break;
    case 011: case 012: case 013: case 015: case 016: case 017: t->unit = U_BOOL; dst(t, X(i)); src(t, X(j)); src(t, X(k)); break;
    case 014: t->unit = U_BOOL; dst(t, X(i)); src(t, X(k)); break;
    case 043: t->unit = U_BOOL; dst(t, X(i)); break;
    case 020: case 021: t->unit = U_SHIFT; dst(t, X(i)); src(t, X(i)); break;
    case 022: case 023: t->unit = U_SHIFT; dst(t, X(i)); src(t, B(j)); src(t, X(k)); break;
    case 024: case 025: case 026: t->unit = U_SHIFT; dst(t, X(i)); if (j) dst(t, B(j)); src(t, X(k)); break;
    case 027: t->unit = U_SHIFT; dst(t, X(i)); src(t, B(j)); src(t, X(k)); break;
    case 047: t->unit = U_SHIFT; dst(t, X(i)); src(t, X(k)); break;
    case 030: case 031: case 032: case 033: case 034: case 035: t->unit = U_FADD; dst(t, X(i)); src(t, X(j)); src(t, X(k)); break;
    case 036: case 037: t->unit = U_LADD; dst(t, X(i)); src(t, X(j)); src(t, X(k)); break;
    case 040: case 041: case 042: t->unit = U_MUL; dst(t, X(i)); src(t, X(j)); src(t, X(k)); break;
    case 044: case 045: t->unit = U_DIV; dst(t, X(i)); src(t, X(j)); src(t, X(k)); break;
    default:
        if (f >= 050 && f <= 077) {
            int g = f & 7, base = f >> 3;           /* 5: A, 6: B, 7: X */
            t->unit = U_INC;
            if (base == 5) { dst(t, A(i)); if (i >= 1 && i <= 5) { dst(t, X(i)); t->load = 1; } else if (i >= 6) { src(t, X(i)); t->store = 1; } }
            else if (base == 6) { if (i) dst(t, B(i)); }
            else dst(t, X(i));
            switch (g) {
            case 0: src(t, A(j)); break;
            case 1: src(t, B(j)); break;
            case 2: src(t, X(j)); break;
            case 3: src(t, X(j)); src(t, B(k)); break;
            case 4: case 5: src(t, A(j)); src(t, B(k)); break;
            default: src(t, B(j)); src(t, B(k)); break;
            }
        }
    }
}

int main(int argc, char **argv)
{
    unsigned char rec[4];
    long long n = 0, cyc_seq = 0, cyc_fe = 0, c[NU] = {0}, nloads = 0, nstores = 0, nbr = 0, ntaken = 0, nwords = 0, nfp = 0, ndiv = 0;
    int lastu[NREG] = {0}; long long waw_by[NU + 1] = {0};
    long long st_unit = 0, st_waw = 0, st_br = 0, st_raw = 0, st_war = 0;
    long long rdy[NREG] = {0}, wrdone[NREG] = {0}, rdtime[NREG] = {0};
    long long ufree[NU][4] = {{0}}, prevstart = 0;
    long long last = 0, nextfetch = 0, memfree = 0, issued_now = 0, cycle_of_issue = -1, end = 0;
    int a;
    for (a = 1; a < argc; a++) {
        char *e = strchr(argv[a], '=');
        if (!e) continue;
        *e++ = 0;
        if (!strcmp(argv[a], "mode")) mode_sb = !strcmp(e, "sb");
        else if (!strcmp(argv[a], "width")) width = atoi(e);
        else if (!strcmp(argv[a], "memlat")) memlat = atoi(e);
        else if (!strcmp(argv[a], "brpen")) brpen = atoi(e);
        else if (!strcmp(argv[a], "spec")) spec = atoi(e);
        else if (!strcmp(argv[a], "fwd")) fwd = atoi(e);
        else if (!strcmp(argv[a], "dropno")) dropno = atoi(e);
        else if (!strcmp(argv[a], "inorder")) inorder = atoi(e);
        else if (!strcmp(argv[a], "limit")) limit = atoll(e);
        else {
            int u;
            for (u = 0; u < NU; u++) {
                char nm[32];
                snprintf(nm, sizeof nm, "%s", uname[u]);
                { char *p; for (p = nm; *p; p++) if (*p == ' ') *p = '_'; }
                if (!strcmp(argv[a], nm)) {         /* e.g. increment=2,2,1 : count, latency, busy */
                    int cn, cl, cb;
                    if (sscanf(e, "%d,%d,%d", &cn, &cl, &cb) == 3) { cfg[u].n = cn; cfg[u].lat = cl; cfg[u].busy = cb; }
                }
            }
        }
    }
    (void)trace_n; (void)ibuf;
    while (fread(rec, 1, 4, stdin) == 4) {
        struct ins t;
        int f = rec[0], ijk = rec[1] << 8 | rec[2], i = ijk >> 6 & 7, j = ijk >> 3 & 7, k = ijk & 7, flags = rec[3], q, u;
        long long tissue, tstart, comp, tr;
        decode(&t, f, i, j, k, flags);
        n++;
        if (limit && n > limit) break;
        c[t.unit]++; nloads += t.load; nstores += t.store; nbr += t.br; ntaken += t.br && t.taken; nwords += t.newword;
        nfp += (f >= 030 && f <= 035) || (f >= 040 && f <= 042); ndiv += f == 044 || f == 045;
        /* ---- the present core: every instruction one cycle, two to fetch a word, one more for a load or a floating operation ---- */
        cyc_seq += 1 + (t.newword ? 2 : 0) + (t.load ? 1 : 0) + (((f >= 030 && f <= 035) || (f >= 040 && f <= 042)) ? 1 : 0)
                 + ((f == 044 || f == 045) ? 52 : 0);
        /* the present execute stage behind a front end: an instruction stack takes the fetch off the instruction, no-operations are
           not executed, and a taken branch costs two cycles to the first parcel of its target */
        if (!(t.unit == U_NONE && !t.br))
            cyc_fe += 1 + (t.load ? 1 : 0) + (((f >= 030 && f <= 035) || (f >= 040 && f <= 042)) ? 1 : 0) + ((f == 044 || f == 045) ? 52 : 0)
                    + ((t.br && t.taken) ? 2 : 0);
        if (!mode_sb) continue;
        if (dropno && t.unit == U_NONE && !t.br) continue;               /* the decoder throws no-operations away */

        /* ---- the scoreboard model ---- */
        tissue = last;                                           /* in order: not before the previous one */
        if (tissue < nextfetch) { st_br += nextfetch - tissue; tissue = nextfetch; }
        if (tissue == cycle_of_issue && issued_now >= width) tissue++;      /* at most `width` a cycle */
        else if (tissue < cycle_of_issue) tissue = cycle_of_issue;
        /* a free unit of the kind */
        { int best = 0, bu;
          for (bu = 1; bu < cfg[t.unit].n; bu++) if (ufree[t.unit][bu] < ufree[t.unit][best]) best = bu;
          if (ufree[t.unit][best] > tissue) { st_unit += ufree[t.unit][best] - tissue; tissue = ufree[t.unit][best]; }
          u = best; }
        /* the destination must not be reserved by a result that has not been written (write after write) */
        for (q = 0; q < t.nd; q++) if (t.d[q] != B(0) && wrdone[t.d[q]] > tissue) { st_waw += wrdone[t.d[q]] - tissue; waw_by[lastu[t.d[q]]] += wrdone[t.d[q]] - tissue; tissue = wrdone[t.d[q]]; }
        if (tissue != cycle_of_issue) { cycle_of_issue = tissue; issued_now = 0; }
        issued_now++;
        /* inorder=1: a plain pipeline without a scoreboard -- no issue before the operands exist, and execution in order */
        if (inorder) { for (q = 0; q < t.ns; q++) if (rdy[t.s[q]] - 1 > tissue) tissue = rdy[t.s[q]] - 1;
                       if (prevstart + 0 > tissue + 1) tissue = prevstart - 1; }
        if (tissue != cycle_of_issue) { cycle_of_issue = tissue; issued_now = 0; }
        /* it waits in the unit for its operands */
        tstart = tissue + 1;
        for (q = 0; q < t.ns; q++) if (rdy[t.s[q]] > tstart) { st_raw += rdy[t.s[q]] - tstart; tstart = rdy[t.s[q]]; }
        if (t.load || t.store) { if (memfree > tstart) tstart = memfree; memfree = tstart + 1; }
        comp = tstart + cfg[t.unit].lat;
        prevstart = tstart;
        /* a result is not written while an earlier instruction still has to read the old value (write after read) */
        for (q = 0; q < t.nd; q++) if (t.d[q] != B(0) && rdtime[t.d[q]] + 1 > comp) { st_war += rdtime[t.d[q]] + 1 - comp; comp = rdtime[t.d[q]] + 1; }
        for (q = 0; q < t.ns; q++) if (rdtime[t.s[q]] < tstart) rdtime[t.s[q]] = tstart;
        for (q = 0; q < t.nd; q++) {
            int r = t.d[q]; long long when = comp;
            if (r < 8 && t.load) when = comp + memlat;              /* the X register of a load is written after the memory read */
            if (r == B(0)) continue;
            rdy[r] = when + fwd; wrdone[r] = when; lastu[r] = (r < 8 && t.load) ? NU : t.unit;
        }
        ufree[t.unit][u] = (cfg[t.unit].busy == 1) ? tstart + 1 : tstart + cfg[t.unit].busy;
        if (t.unit == U_DIV || cfg[t.unit].busy >= cfg[t.unit].lat) ufree[t.unit][u] = comp;
        if (t.br) {
            tr = tstart + cfg[U_BR].lat;
            if (t.taken) nextfetch = tr + brpen; else nextfetch = spec ? tissue + 1 : tr;
            if (nextfetch < tissue + 1) nextfetch = tissue + 1;
        } else nextfetch = tissue;
        last = tissue;
        if (comp > end) end = comp;
        { int q2; for (q2 = 0; q2 < t.nd; q2++) if (wrdone[t.d[q2]] > end) end = wrdone[t.d[q2]]; }
    }
    printf("instructions %lld   words entered %lld   loads %lld stores %lld   branches %lld (taken %lld)   float %lld   divide %lld\n",
           n, nwords, nloads, nstores, nbr, ntaken, nfp, ndiv);
    printf("mix:"); for (a = 0; a < NU; a++) printf("  %s %.1f%%", uname[a], 100.0 * c[a] / n); printf("\n");
    printf("present core (model): %lld cycles, %.3f per instruction\n", cyc_seq, (double)cyc_seq / n);
    printf("present execute stage behind a front end (stack, no-operations dropped): %lld cycles, speed-up %.2fx\n", cyc_fe, (double)cyc_seq / cyc_fe);
    if (mode_sb) {
        printf("scoreboard model: %lld cycles, %.3f per instruction (IPC %.3f), speed-up %.2fx over the present core\n",
               end, (double)end / n, (double)n / end, (double)cyc_seq / end);
        printf("destination reserved, by the unit that holds it:"); for (a = 0; a <= NU; a++) if (waw_by[a]) printf("  %s %lld", a == NU ? "load" : uname[a], waw_by[a]); printf("\n");
        printf("issue delays (cycles summed): fetch/branch %lld  unit busy %lld  dest reserved (WAW) %lld   waiting for operands %lld   result held (WAR) %lld\n",
               st_br, st_unit, st_waw, st_raw, st_war);
    }
    return 0;
}
