// A CDC 6000 series central processor with an instruction stack, in-order issue, a scoreboard and several functional units.
//
// The instruction set and every result are those of cdc6000_core.sv (the reference RTL) and host/cdcemu.c (the reference model); this
// file is a different organisation of the same machine, described in rtl/SUPERSCALAR.md.  The organisation follows the one in the
// open literature for this machine (J. E. Thornton, "Design of a Computer: the Control Data 6600", 1970):
//
//   instruction stack  ->  decode register  ->  issue  ->  functional units (boolean, shift, long add, floating add, multiply,
//                                                          divide, increment x NINC, branch)  ->  result buses  ->  registers
//
// * Issue is in order, one instruction a cycle.  An instruction issues if its unit is free and its destination registers are not
//   reserved by an unfinished instruction.  Issue then goes on to the next instruction.
// * The instruction waits in its unit for operands that are not yet written.  A reservation names the instruction that will write the
//   register (a tag); an operand that is reserved is taken from the result bus when the tag appears there.  An operand that is not
//   reserved is read at issue.  (The original reads the registers from the unit when both operands are ready and holds a result back
//   until earlier readers have read; here the value is captured, which gives the same results without that check.)
// * A-register instructions are the memory instructions: SA1-SA5 load X1-X5, SA6-SA7 store X6-X7.  The address comes from an increment
//   unit; the reference goes through a queue that keeps program order, so memory needs no further checking.
// * A conditional branch holds issue until it is resolved; an unconditional one to a known place redirects the instruction fetch at
//   issue.  Results are never written before the instructions before them are issued, so nothing has to be undone.
// * Exceptions (PS, a 30 bit instruction in the last parcel of a word) stop the machine once everything before has finished.
//
// The memory has two read ports: the instruction port fetches whole words into the instruction stack, the data port serves the
// loads and stores.  EXT_MEM=1: the memory is outside (fpga/vc707-eth/genmem.py makes it); otherwise it is an array in here.
module cdc6000_sb #(
    parameter int MEM_AW = 17,
    parameter MEM_INIT = "",
    parameter bit EXT_MEM = 1'b0,
    parameter int NSK = 8,            // words in the instruction stack
    parameter int NINC = 2,           // address increment units
    parameter int LAT_BOOL = 1,       // result latency of each kind of unit: cycles from the start of execution to the result on the bus
    parameter int LAT_SHIFT = 1,
    parameter int LAT_LADD = 1,
    parameter int LAT_FADD = 1,
    parameter int LAT_MUL = 1,
    parameter int LAT_INC = 1,
    parameter int MEMLAT = 1,         // 1: the word read is on the bus the cycle after the request; 2: one register more
    parameter int HALT_WAIT = 80      // cycles that older instructions get to finish after the store that halts
) (
    input logic clk,
    input logic reset,

    // instruction port and data port of an outside memory
    output logic [MEM_AW-1:0] ext_iaddr,
    input  logic [59:0]       ext_irdata,
    output logic [MEM_AW-1:0] ext_addr,
    output logic              ext_we,
    output logic [59:0]       ext_wdata,
    input  logic [59:0]       ext_rdata,

    input  logic                load_we,
    input  logic [MEM_AW-1:0]   load_addr,
    input  logic [59:0]         load_data,

    input  logic        rx_valid,
    input  logic [5:0]  rx_char,
    input  logic        rx_eof,
    output logic        rx_take,

    output logic        tx_valid,
    output logic [5:0]  tx_char,
    input  logic        tx_ready,

    output logic        fo_valid,
    output logic [59:0] fo_name,
    output logic [59:0] fo_word,
    input  logic        fo_ready,

    output logic        dev_req,
    output logic        dev_we,
    output logic [12:0] dev_off,
    output logic [59:0] dev_wdata,
    input  logic        dev_ack,
    input  logic [59:0] dev_rdata,

    output logic        halted,
    output logic        fault,

    output logic [17:0]  dbg_pc,
    output logic [63:0]  icount,
    output logic [479:0] dbg_x,
    output logic [143:0] dbg_a,
    output logic [143:0] dbg_b
);
  import cdc_pkg::*;

  localparam int U_BR = 0, U_BOOL = 1, U_SHIFT = 2, U_LADD = 3, U_FADD = 4, U_MUL = 5, U_DIV = 6, U_INC0 = 7;
  localparam int NU = 7 + NINC;                       // units that have an entry
  localparam int NB = 2 * (NU + 1);                   // result buses: two for each unit and two for the memory
  localparam int Q = 4;                               // memory queue
  localparam logic [17:0] DEV_BASE = 18'o120000;

  // unit kinds of an instruction
  localparam logic [3:0] UK_BR = 4'd0, UK_BOOL = 4'd1, UK_SHIFT = 4'd2, UK_LADD = 4'd3, UK_FADD = 4'd4, UK_MUL = 4'd5,
                         UK_DIV = 4'd6, UK_INC = 4'd7, UK_NONE = 4'd8;

  // registers by number: X0-7 are 0-7, A0-7 are 8-15, B0-7 are 16-23
  function automatic logic [4:0] rx_(input logic [2:0] n); rx_ = {2'b00, n}; endfunction
  function automatic logic [4:0] ra_(input logic [2:0] n); ra_ = {2'b01, n}; endfunction
  function automatic logic [4:0] rb_(input logic [2:0] n); rb_ = {2'b10, n}; endfunction

  function automatic int lat_of(input int u);
    if (u == U_BOOL) lat_of = LAT_BOOL;
    else if (u == U_SHIFT) lat_of = LAT_SHIFT;
    else if (u == U_LADD) lat_of = LAT_LADD;
    else if (u == U_FADD) lat_of = LAT_FADD;
    else if (u == U_MUL) lat_of = LAT_MUL;
    else if (u >= U_INC0) lat_of = LAT_INC;
    else lat_of = 1;
  endfunction

  // ------------------------------------------------------------------ memory
  logic [59:0] mem[0:(1<<MEM_AW)-1];
  logic [MEM_AW-1:0] iaddr, daddr;
  logic ld_v_next;
  logic [4:0] ld_t_next;
  logic [59:0] irdata, drdata, dwdata;
  logic dwe;

  generate
    if (EXT_MEM) begin : g_ext
      assign ext_iaddr = iaddr;
      assign ext_addr  = daddr;
      assign ext_we    = dwe;
      assign ext_wdata = dwdata;
      assign irdata    = ext_irdata;
      assign drdata    = ext_rdata;
    end else begin : g_int
      assign ext_iaddr = '0; assign ext_addr = '0; assign ext_we = 1'b0; assign ext_wdata = 60'd0;
      initial begin
        if (MEM_INIT != "") $readmemh(MEM_INIT, mem);
      end
      always_ff @(posedge clk) begin
        if (reset) begin
          if (load_we) mem[load_addr] <= load_data;
        end else if (dwe) mem[daddr] <= dwdata;
        irdata <= mem[iaddr];
        drdata <= mem[daddr];
      end
    end
  endgenerate

  // ------------------------------------------------------------------ architectural state
  logic [59:0] xr[0:7];
  logic [17:0] ar[0:7];
  logic [17:0] br[0:7];
  logic [59:0] io_file;
  logic [2:0]  boot;                                   // 0-3: reading the boot block, 4: running

  // reservations: who will write each register
  logic [23:0] rsv_v;
  logic [4:0]  rsv_t[0:23];                            // {tag, port}
  logic [1:0]  tag_pend[0:15];                         // results outstanding for each tag, one bit for each port

  // ------------------------------------------------------------------ the instruction decoder
  typedef struct packed {
    logic [3:0] uk;
    logic       d0v;  logic [4:0] d0r;                 // results: the first (also the A of a load), the second (B of a normalise)
    logic       d1v;  logic [4:0] d1r;
    logic       s0v;  logic [4:0] s0r;                 // operands
    logic       s1v;  logic [4:0] s1r;
    logic       mld;                                   // SA1-5: loads X(i), reserved by the memory
    logic       mst;                                   // SA6-7: stores X(i) (source sdr)
    logic [4:0] sdr;
    logic       rj;                                    // RJ: stores the return jump, jumps to K+1
    logic       jimm;                                  // JP with no register: jumps at issue
    logic       special;                               // PS: stops the machine
  } dec_t;

  function automatic dec_t decode(input logic [5:0] f, input logic [2:0] i, input logic [2:0] j, input logic [2:0] k);
    dec_t d;
    logic [2:0] g;
    d = '0;
    d.uk = UK_NONE;
    g = f[2:0];
    case (f)
      6'o00: d.special = 1'b1;
      6'o01: begin d.rj = 1'b1; end
      6'o02: begin
        if (i == 3'd0) d.jimm = 1'b1;
        else begin d.uk = UK_BR; d.s0v = 1'b1; d.s0r = rb_(i); end
      end
      6'o03: begin d.uk = UK_BR; d.s0v = 1'b1; d.s0r = rx_(j); end
      6'o04, 6'o05, 6'o06, 6'o07: begin
        d.uk = UK_BR;
        if (i != 0) begin d.s0v = 1'b1; d.s0r = rb_(i); end
        if (j != 0) begin d.s1v = 1'b1; d.s1r = rb_(j); end
      end
      6'o10: begin d.uk = UK_BOOL; d.d0v = 1; d.d0r = rx_(i); d.s0v = 1; d.s0r = rx_(j); end
      6'o11, 6'o12, 6'o13, 6'o15, 6'o16, 6'o17: begin
        d.uk = UK_BOOL; d.d0v = 1; d.d0r = rx_(i); d.s0v = 1; d.s0r = rx_(j); d.s1v = 1; d.s1r = rx_(k);
      end
      6'o14: begin d.uk = UK_BOOL; d.d0v = 1; d.d0r = rx_(i); d.s0v = 1; d.s0r = rx_(k); end
      6'o43: begin d.uk = UK_BOOL; d.d0v = 1; d.d0r = rx_(i); end
      6'o20, 6'o21: begin d.uk = UK_SHIFT; d.d0v = 1; d.d0r = rx_(i); d.s0v = 1; d.s0r = rx_(i); end
      6'o22, 6'o23: begin
        d.uk = UK_SHIFT; d.d0v = 1; d.d0r = rx_(i);
        if (j != 0) begin d.s0v = 1; d.s0r = rb_(j); end
        d.s1v = 1; d.s1r = rx_(k);
      end
      6'o24, 6'o25, 6'o26: begin
        d.uk = UK_SHIFT; d.d0v = 1; d.d0r = rx_(i); d.s0v = 1; d.s0r = rx_(k);
        if (j != 0) begin d.d1v = 1; d.d1r = rb_(j); end
      end
      6'o27: begin
        d.uk = UK_SHIFT; d.d0v = 1; d.d0r = rx_(i);
        if (j != 0) begin d.s0v = 1; d.s0r = rb_(j); end
        d.s1v = 1; d.s1r = rx_(k);
      end
      6'o47: begin d.uk = UK_SHIFT; d.d0v = 1; d.d0r = rx_(i); d.s0v = 1; d.s0r = rx_(k); end
      6'o30, 6'o31, 6'o32, 6'o33, 6'o34, 6'o35: begin
        d.uk = UK_FADD; d.d0v = 1; d.d0r = rx_(i); d.s0v = 1; d.s0r = rx_(j); d.s1v = 1; d.s1r = rx_(k);
      end
      6'o36, 6'o37: begin
        d.uk = UK_LADD; d.d0v = 1; d.d0r = rx_(i); d.s0v = 1; d.s0r = rx_(j); d.s1v = 1; d.s1r = rx_(k);
      end
      6'o40, 6'o41, 6'o42: begin
        d.uk = UK_MUL; d.d0v = 1; d.d0r = rx_(i); d.s0v = 1; d.s0r = rx_(j); d.s1v = 1; d.s1r = rx_(k);
      end
      6'o44, 6'o45: begin
        d.uk = UK_DIV; d.d0v = 1; d.d0r = rx_(i); d.s0v = 1; d.s0r = rx_(j); d.s1v = 1; d.s1r = rx_(k);
      end
      6'o46: ;
      default: begin
        // 50-57 SA (A(i)), 60-67 SB (B(i)), 70-77 SX (X(i)): one sum of A/B/X operands and a constant
        d.uk = UK_INC;
        if (f[5:3] == 3'd5) begin
          d.d0v = 1; d.d0r = ra_(i);
          if (i >= 3'd1 && i <= 3'd5) d.mld = 1'b1;
          else if (i >= 3'd6) begin d.mst = 1'b1; d.sdr = rx_(i); end
        end else if (f[5:3] == 3'd6) begin
          if (i != 0) begin d.d0v = 1; d.d0r = rb_(i); end
        end else begin
          d.d0v = 1; d.d0r = rx_(i);
        end
        case (g)
          3'd0: begin d.s0v = 1; d.s0r = ra_(j); end
          3'd1: begin if (j != 0) begin d.s0v = 1; d.s0r = rb_(j); end end
          3'd2: begin d.s0v = 1; d.s0r = rx_(j); end
          3'd3: begin d.s0v = 1; d.s0r = rx_(j); if (k != 0) begin d.s1v = 1; d.s1r = rb_(k); end end
          3'd4, 3'd5: begin d.s0v = 1; d.s0r = ra_(j); if (k != 0) begin d.s1v = 1; d.s1r = rb_(k); end end
          default: begin
            if (j != 0) begin d.s0v = 1; d.s0r = rb_(j); end
            if (k != 0) begin d.s1v = 1; d.s1r = rb_(k); end
          end
        endcase
      end
    endcase
    decode = d;
  endfunction

  function automatic logic is_long_f(input logic [5:0] f);
    is_long_f = (f >= 6'o01 && f <= 6'o07) || f == 6'o50 || f == 6'o51 || f == 6'o52 ||
                f == 6'o60 || f == 6'o61 || f == 6'o62 || f == 6'o70 || f == 6'o71 || f == 6'o72;
  endfunction

  function automatic logic is_dev(input logic [17:0] ad);
    is_dev = (ad >= DEV_BASE) && (ad < DEV_BASE + 18'd4112);
  endfunction

  function automatic logic is_io(input logic [17:0] ad);
    is_io = (ad[17:5] == 13'd0) && ad[4] && !ad[3] && (ad[2:0] <= 3'd4);
  endfunction

  function automatic logic cond(input logic [2:0] c, input logic [59:0] v);
    logic [11:0] e;
    logic inr, ind;
    e = v[59:48];
    ind = (e == 12'o1777) || (e == 12'o6000);
    inr = (e == 12'o3777) || (e == 12'o4000) || ind;
    case (c)
      3'd0: cond = (v == 60'd0) || (v == M60);
      3'd1: cond = !((v == 60'd0) || (v == M60));
      3'd2: cond = !v[59];
      3'd3: cond = v[59];
      3'd4: cond = !inr;
      3'd5: cond = inr;
      3'd6: cond = !ind;
      default: cond = ind;
    endcase
  endfunction

  // ------------------------------------------------------------------ front end: instruction stack, fetch, decode register
  logic [59:0]       sk_d[0:NSK-1];
  logic [MEM_AW-1:0] sk_a[0:NSK-1];
  logic [NSK-1:0]    sk_v;
  logic [$clog2(NSK)-1:0] sk_wp;
  logic [17:0]       dp_w;                              // the word, and the parcel in it, that decode takes next
  logic [1:0]        dp_s;
  logic [4:0]        dp_nskip;                          // no-operations passed since the last instruction
  logic              f_v, f_drop;                       // a word was asked for last cycle
  logic [MEM_AW-1:0] f_a;

  logic              dr_v;                              // decode register
  logic [17:0]       dr_w;
  logic [5:0]        dr_f;
  logic [2:0]        dr_i, dr_j, dr_k;
  logic [17:0]       dr_K;
  logic [4:0]        dr_ns;
  logic              dr_bad;                            // a 30 bit instruction in the last parcel

  // data written into memory this cycle (keeps the instruction stack right)
  logic              st_v;
  logic [MEM_AW-1:0] st_a;
  logic [59:0]       st_d;

  logic redirect_v;
  logic [17:0] redirect_t;
  logic issue_fire;

  logic [MEM_AW-1:0] dpa;
  assign dpa = dp_w[MEM_AW-1:0];
  assign iaddr = (boot == 3'd4 && want_v) ? dpa + MEM_AW'(want_n) : dpa;
  logic [59:0] ret_data;
  assign ret_data = (st_v && st_a == f_a) ? st_d : irdata;

  logic [NSK-1:0] hit_n;
  logic [59:0] dw;
  logic dec_hit;
  logic [3:0] present;
  logic [1:0] want_n;
  logic want_v;
  always_comb begin
    dw = 60'd0;
    for (int n = 0; n < NSK; n++) begin
      hit_n[n] = sk_v[n] && (sk_a[n] == dpa);
      if (hit_n[n]) dw = dw | sk_d[n];
    end
    if (f_v && !f_drop && f_a == dpa) begin
      dw = ret_data;
      dec_hit = 1'b1;
    end else dec_hit = |hit_n;
    // prefetch: the first of the next four words that is neither in the stack nor on its way
    for (int q = 0; q < 4; q++) begin
      logic inst;
      inst = 1'b0;
      for (int n = 0; n < NSK; n++) if (sk_v[n] && sk_a[n] == dpa + MEM_AW'(q)) inst = 1'b1;
      if (f_v && !f_drop && f_a == dpa + MEM_AW'(q)) inst = 1'b1;
      present[q] = inst;
    end
    want_v = 1'b0; want_n = 2'd0;
    for (int q = 3; q >= 0; q--) if (!present[q]) begin want_v = 1'b1; want_n = q[1:0]; end
  end

  // decode: skip the no-operations, find the instruction
  logic [14:0] pp[0:3];
  logic [2:0]  spos;
  logic [4:0]  skipc;
  logic        found;
  logic [5:0]  pf0;
  logic [14:0] pc0, pc1;
  logic        pl;
  logic        nextw;
  logic [2:0]  newpos;
  always_comb begin
    pp[0] = dw[59:45]; pp[1] = dw[44:30]; pp[2] = dw[29:15]; pp[3] = dw[14:0];
    found = 1'b0; spos = 3'd4; skipc = 5'd0;
    for (int q = 0; q < 4; q++) begin
      if (!found && q >= dp_s) begin
        if (pp[q][14:9] == 6'o46) skipc = skipc + 5'd1;
        else begin found = 1'b1; spos = q[2:0]; end
      end
    end
    pc0 = (spos < 3'd4) ? pp[spos[1:0]] : 15'd0;
    pf0 = pc0[14:9];
    pl = found && is_long_f(pf0);
    pc1 = (spos < 3'd3) ? pp[spos[1:0] + 2'd1] : 15'd0;
    newpos = found ? (spos + (pl ? 3'd2 : 3'd1)) : 3'd4;
    nextw = (newpos >= 3'd4);
  end

  // ------------------------------------------------------------------ issue
  dec_t D;
  dec_t Dbad;
  always_comb begin
    Dbad = '0; Dbad.uk = UK_NONE; Dbad.special = 1'b1;
    D = dr_bad ? Dbad : decode(dr_f, dr_i, dr_j, dr_k);
  end

  // units: entries
  logic [NU-1:0]  e_v;
  logic [5:0]     e_f[0:NU-1];
  logic [8:0]     e_ijk[0:NU-1];
  logic [17:0]    e_K[0:NU-1];
  logic [3:0]     e_tag[0:NU-1];
  logic           e_d0v[0:NU-1];
  logic           e_d1v[0:NU-1];
  logic [59:0]    e_s0[0:NU-1];
  logic [59:0]    e_s1[0:NU-1];
  logic           e_w0[0:NU-1];
  logic           e_w1[0:NU-1];
  logic [4:0]     e_t0[0:NU-1];
  logic [4:0]     e_t1[0:NU-1];
  logic           e_mf[0:NU-1];
  logic [1:0]     e_mq[0:NU-1];
  logic [17:0]    e_wd[0:NU-1];                         // word address of the instruction (branch unit: not used)
  logic [NU-1:0]  launch;
  logic [NU-1:0]  accept;

  // result buses
  logic        rb_v[0:NB-1];
  logic [4:0]  rb_t[0:NB-1];
  logic [59:0] rb_d[0:NB-1];

  function automatic logic [60:0] busf(input logic [4:0] t);
    logic [60:0] r;
    r = 61'd0;
    for (int b = 0; b < NB; b++) if (rb_v[b] && rb_t[b] == t) begin r[60] = 1'b1; r[59:0] = rb_d[b]; end
    busf = r;
  endfunction

  function automatic logic [59:0] regval(input logic [4:0] r);
    if (r[4:3] == 2'b00) regval = xr[r[2:0]];
    else if (r[4:3] == 2'b01) regval = {42'd0, ar[r[2:0]]};
    else regval = {42'd0, br[r[2:0]]};
  endfunction

  // the three operands an instruction may read at issue: s0, s1 and the data of a store
  logic [59:0] o_v[0:2];
  logic        o_w[0:2];
  logic [4:0]  o_t[0:2];
  logic        o_use[0:2];
  logic [4:0]  o_r[0:2];
  always_comb begin
    o_use[0] = D.s0v; o_r[0] = D.s0r;
    o_use[1] = D.s1v; o_r[1] = D.s1r;
    o_use[2] = D.mst; o_r[2] = D.sdr;
    for (int s = 0; s < 3; s++) begin
      logic [60:0] bf;
      o_v[s] = 60'd0; o_w[s] = 1'b0; o_t[s] = 5'd0;
      bf = 61'd0;
      if (o_use[s]) begin
        if (rsv_v[o_r[s]]) begin
          bf = busf(rsv_t[o_r[s]]);
          o_t[s] = rsv_t[o_r[s]];
          if (bf[60]) o_v[s] = bf[59:0]; else o_w[s] = 1'b1;
        end else o_v[s] = regval(o_r[s]);
      end
    end
  end

  // free tag
  logic       tag_ok;
  logic [3:0] new_tag;
  always_comb begin
    tag_ok = 1'b0; new_tag = 4'd0;
    for (int t = 15; t >= 0; t--) if (tag_pend[t] == 2'b00) begin tag_ok = 1'b1; new_tag = t[3:0]; end
  end

  // memory queue
  logic [Q-1:0]  mq_v, mq_st, mq_av, mq_dv, mq_dw;
  logic [17:0]   mq_a[0:Q-1];
  logic [59:0]   mq_d[0:Q-1];
  logic [4:0]    mq_dt[0:Q-1];
  logic [4:0]    mq_rt[0:Q-1];
  logic [1:0]    mq_head, mq_tail;
  logic [2:0]    mq_n;
  logic          mq_pop;

  logic drained;
  logic hold;                                          // issue is held
  logic br_pend, halt_req;
  logic [6:0] halt_cnt;

  logic       unit_ok, dest_ok, need_tag, need_mq, mq_ok, sp_ok;
  logic [4:0] sel_u;
  always_comb begin
    unit_ok = 1'b0; sel_u = 5'd0;
    case (D.uk)
      UK_BR: begin unit_ok = accept[U_BR]; sel_u = U_BR; end
      UK_BOOL: begin unit_ok = accept[U_BOOL]; sel_u = U_BOOL; end
      UK_SHIFT: begin unit_ok = accept[U_SHIFT]; sel_u = U_SHIFT; end
      UK_LADD: begin unit_ok = accept[U_LADD]; sel_u = U_LADD; end
      UK_FADD: begin unit_ok = accept[U_FADD]; sel_u = U_FADD; end
      UK_MUL: begin unit_ok = accept[U_MUL]; sel_u = U_MUL; end
      UK_DIV: begin unit_ok = accept[U_DIV]; sel_u = U_DIV; end
      UK_INC: begin
        for (int n = NINC - 1; n >= 0; n--) if (accept[U_INC0 + n]) begin unit_ok = 1'b1; sel_u = U_INC0 + n; end
      end
      default: unit_ok = 1'b1;
    endcase
    dest_ok = (!D.d0v || !rsv_v[D.d0r]) && (!D.d1v || !rsv_v[D.d1r]) && (!D.mld || !rsv_v[rx_(dr_i)]);
    need_tag = D.d0v || D.d1v || D.mld;
    need_mq = D.mld || D.mst || D.rj;
    mq_ok = !need_mq || (mq_n < 3'd4);
    sp_ok = !D.special || drained;
  end

  assign hold = br_pend || halt_req || halted;
  assign issue_fire = dr_v && boot == 3'd4 && !hold && unit_ok && dest_ok && (!need_tag || tag_ok) && mq_ok && sp_ok;

  // jumps taken at issue: RJ, and JP with no register
  logic jump_now;
  logic [17:0] jump_t;
  always_comb begin
    jump_now = 1'b0; jump_t = 18'd0;
    if (issue_fire && D.rj) begin jump_now = 1'b1; jump_t = dr_K + 18'd1; end
    if (issue_fire && D.jimm) begin jump_now = 1'b1; jump_t = dr_K; end
  end

  // ------------------------------------------------------------------ units: execution
  // the operands the unit would use are in e_s0/e_s1; the results go to the delay line of the unit
  logic [59:0] c_d0[0:NU-1];
  logic [17:0] c_d1[0:NU-1];
  logic        c_p1[0:NU-1];
  logic        c_v[0:NU-1];

  // floating point: shared decode of the operands per unit
  fl_t         fa_x, fa_y, fm_x, fm_y;
  logic [95:0] fm_p;
  nrm_t        fa_n, fm_n;
  logic [59:0] fa_res, fm_res, fm_dres;
  always_comb begin
    fa_n = add_prep(e_s0[U_FADD], e_s1[U_FADD], e_f[U_FADD][0]);
    fa_res = fnorm(fa_n, e_f[U_FADD][2]);
    fm_x = funpack(e_s0[U_MUL]); fm_y = funpack(e_s1[U_MUL]);
    fm_p = {48'd0, fm_x.c} * {48'd0, fm_y.c};
    fm_n = mul_prep(e_s0[U_MUL], e_s1[U_MUL], fm_p);
    fm_res = fnorm(fm_n, e_f[U_MUL][0]);
    fm_dres = dmul_from(e_s0[U_MUL], e_s1[U_MUL], fm_p);
  end

  // divide: restoring, one quotient bit a cycle
  fl_t         dv_x, dv_y;
  logic [47:0] dv_xc, dv_yc;
  logic        dv_neg, dv_round, dv_run;
  int          dv_e;
  logic [49:0] dv_r;
  logic [50:0] dv_q;
  logic [5:0]  dv_cnt;
  nrm_t        dv_n;
  logic        dv_special_v, dv_fin;
  logic [59:0] dv_special_d;
  always_comb begin
    dv_x = funpack(e_s0[U_DIV]); dv_y = funpack(e_s1[U_DIV]);
    dv_special_v = 1'b0; dv_special_d = 60'd0;
    if (dv_y.c == 0) begin dv_special_v = 1'b1; dv_special_d = {12'h3ff, 48'd0}; end
    else if (dv_x.c == 0) begin dv_special_v = 1'b1; dv_special_d = 60'd0; end
    dv_n = '0;
    dv_n.zero = 1'b0; dv_n.neg = dv_neg; dv_n.m = NW'(dv_q); dv_n.e = 16'(dv_e);
  end
  logic [49:0] dv_rr;
  logic        dv_qb;
  always_comb begin
    dv_rr = (dv_cnt == 6'd0) ? {2'b00, dv_xc} : {dv_r[48:0], 1'b0};
    dv_qb = (dv_rr >= {2'b00, dv_yc});
  end

  // increment: the sum of two operands and a constant
  logic [17:0] inc_s[0:NU-1];
  always_comb begin
    for (int u = 0; u < NU; u++) begin
      logic [17:0] k18, s0_18, s1_18;
      k18 = e_K[u]; s0_18 = e_s0[u][17:0]; s1_18 = e_s1[u][17:0];
      case (e_f[u][2:0])
        3'd0, 3'd1, 3'd2: inc_s[u] = add18(k18, s0_18);
        3'd3, 3'd4: inc_s[u] = add18(s1_18, s0_18);
        3'd5: inc_s[u] = add18(neg18(s1_18), s0_18);
        3'd6: inc_s[u] = add18(s1_18, s0_18);
        default: inc_s[u] = add18(neg18(s1_18), s0_18);
      endcase
    end
  end

  // branch unit
  logic        br_go, br_taken;
  logic [17:0] br_tgt;
  logic signed [18:0] bva, bvb;
  always_comb begin
    bva = sb18(e_s0[U_BR][17:0]); bvb = sb18(e_s1[U_BR][17:0]);
    br_taken = 1'b0; br_tgt = e_K[U_BR];
    case (e_f[U_BR])
      6'o02: begin br_taken = 1'b1; br_tgt = e_K[U_BR] + e_s0[U_BR][17:0]; end
      6'o03: br_taken = cond(e_ijk[U_BR][8:6], e_s0[U_BR]);
      6'o04: br_taken = (bva == bvb);
      6'o05: br_taken = (bva != bvb);
      6'o06: br_taken = (bva >= bvb);
      6'o07: br_taken = (bva < bvb);
      default: ;
    endcase
  end

  always_comb begin
    for (int u = 0; u < NU; u++) begin
      logic [2:0] fi_, fj_, fk_;
      logic [5:0] f_;
      logic [17:0] bj18;
      logic [5:0] shn_;
      logic shl_;
      fi_ = e_ijk[u][8:6]; fj_ = e_ijk[u][5:3]; fk_ = e_ijk[u][2:0]; f_ = e_f[u];
      c_d0[u] = 60'd0; c_d1[u] = 18'd0; c_p1[u] = 1'b0; c_v[u] = 1'b0;
      bj18 = e_s0[u][17:0];
      shn_ = shcount(bj18); shl_ = !bj18[17];
      if (u == U_BOOL) begin
        case (f_)
          6'o10: c_d0[u] = e_s0[u];
          6'o11: c_d0[u] = e_s0[u] & e_s1[u];
          6'o12: c_d0[u] = e_s0[u] | e_s1[u];
          6'o13: c_d0[u] = e_s0[u] ^ e_s1[u];
          6'o14: c_d0[u] = ~e_s0[u];
          6'o15: c_d0[u] = ~e_s1[u] & e_s0[u];
          6'o16: c_d0[u] = ~e_s1[u] | e_s0[u];
          6'o17: c_d0[u] = ~(e_s1[u] ^ e_s0[u]);
          default: c_d0[u] = mask43({fj_, fk_});
        endcase
      end else if (u == U_SHIFT) begin
        case (f_)
          6'o20: c_d0[u] = rotl60(e_s0[u], {fj_, fk_});
          6'o21: c_d0[u] = sar60(e_s0[u], {fj_, fk_});
          6'o22: c_d0[u] = shl_ ? rotl60(e_s1[u], shn_) : sar60(e_s1[u], shn_);
          6'o23: c_d0[u] = shl_ ? sar60(e_s1[u], shn_) : rotl60(e_s1[u], shn_);
          6'o24, 6'o25: begin c_d0[u] = nx_result(e_s0[u]); c_d1[u] = nx_count(e_s0[u]); c_p1[u] = (fj_ != 0); end
          6'o26: begin c_d0[u] = ux_coef(e_s0[u]); c_d1[u] = ux_exp(e_s0[u]); c_p1[u] = (fj_ != 0); end
          6'o27: c_d0[u] = px_result(e_s1[u], bj18);
          default: c_d0[u] = popcount60(e_s0[u]);
        endcase
      end else if (u == U_LADD) begin
        c_d0[u] = (f_ == 6'o36) ? add60(e_s0[u], e_s1[u]) : add60(e_s0[u], ~e_s1[u]);
      end else if (u == U_FADD) begin
        c_d0[u] = fa_res;
      end else if (u == U_MUL) begin
        c_d0[u] = (f_ == 6'o42) ? fm_dres : fm_res;
      end else if (u >= U_INC0) begin
        if (f_[5:3] == 3'd7) c_d0[u] = sx18(inc_s[u]); else c_d0[u] = {42'd0, inc_s[u]};
      end
    end
  end

  // entry launch: operands there; the divider and the branch unit have their own rules
  always_comb begin
    for (int u = 0; u < NU; u++) begin
      launch[u] = e_v[u] && !e_w0[u] && !e_w1[u];
      accept[u] = !e_v[u] || launch[u];
    end
    // the divider keeps its entry while it works
    launch[U_DIV] = e_v[U_DIV] && !e_w0[U_DIV] && !e_w1[U_DIV] && (dv_run ? dv_fin : dv_special_v);
    accept[U_DIV] = !e_v[U_DIV];
  end

  // result of the divider when it is done
  logic [59:0] dv_out;
  always_comb begin
    dv_fin = dv_run && (dv_cnt == 6'd51);
    dv_out = dv_run ? fnorm(dv_n, dv_round) : dv_special_d;
  end

  // ------------------------------------------------------------------ delay lines
  logic        u_o_v[0:NU-1];
  logic [3:0]  u_o_t[0:NU-1];
  logic [59:0] u_o_d0[0:NU-1];
  logic [17:0] u_o_d1[0:NU-1];
  logic        u_o_p1[0:NU-1];
  logic        u_o_mf[0:NU-1];
  logic [1:0]  u_o_mq[0:NU-1];
  logic [NU-1:0] u_o_busy;

  generate
    for (genvar g = 0; g < NU; g++) begin : g_unit
      logic in_v;
      logic [59:0] in_d0;
      assign in_v = launch[g] && (e_d0v[g] || e_d1v[g]) && (g != U_BR);
      assign in_d0 = (g == U_DIV) ? dv_out : c_d0[g];
      sb_pipe #(.LAT(lat_of(g))) pipe (
        .clk(clk), .reset(reset),
        .in_v(in_v), .in_t(e_tag[g]), .in_d0(in_d0), .in_d1(c_d1[g]), .in_p1(c_p1[g]), .in_mf(e_mf[g]), .in_mq(e_mq[g]),
        .out_v(u_o_v[g]), .out_t(u_o_t[g]), .out_d0(u_o_d0[g]), .out_d1(u_o_d1[g]), .out_p1(u_o_p1[g]),
        .out_mf(u_o_mf[g]), .out_mq(u_o_mq[g]), .busy(u_o_busy[g]));
    end
  endgenerate

  // memory read data on the bus
  logic        ld_v, ld_v2;
  logic [4:0]  ld_t, ld_t2;
  logic [59:0] ld_d2;
  logic        mb_v;                                   // the memory's bus, port 1
  logic [4:0]  mb_t;
  logic [59:0] mb_d;
  logic        io_res_v;                               // a result of a device or character load, on the bus this cycle
  logic [4:0]  io_res_t;
  logic [59:0] io_res_d;

  always_comb begin
    for (int b = 0; b < NB; b++) begin rb_v[b] = 1'b0; rb_t[b] = 5'd0; rb_d[b] = 60'd0; end
    for (int u = 0; u < NU; u++) begin
      rb_v[2*u] = u_o_v[u];
      rb_t[2*u] = {u_o_t[u], 1'b0};
      rb_d[2*u] = u_o_d0[u];
      rb_v[2*u+1] = u_o_v[u] && u_o_p1[u];
      rb_t[2*u+1] = {u_o_t[u], 1'b1};
      rb_d[2*u+1] = {42'd0, u_o_d1[u]};
    end
    // the memory: a read that came back, or a device/character load
    mb_v = 1'b0; mb_t = 5'd0; mb_d = 60'd0;
    if (MEMLAT == 1) begin
      if (ld_v) begin mb_v = 1'b1; mb_t = ld_t; mb_d = drdata; end
    end else begin
      if (ld_v2) begin mb_v = 1'b1; mb_t = ld_t2; mb_d = ld_d2; end
    end
    if (io_res_v) begin mb_v = 1'b1; mb_t = io_res_t; mb_d = io_res_d; end
    rb_v[2*NU+1] = mb_v; rb_t[2*NU+1] = mb_t; rb_d[2*NU+1] = mb_d;
  end

  // register writes: the bus that carries the tag of the reservation
  logic        rw_v[0:23];
  logic [59:0] rw_d[0:23];
  always_comb begin
    for (int r = 0; r < 24; r++) begin
      logic [60:0] bf;
      bf = 61'd0;
      if (rsv_v[r]) bf = busf(rsv_t[r]);
      rw_v[r] = bf[60]; rw_d[r] = bf[59:0];
    end
  end

  // ------------------------------------------------------------------ memory unit: the queue head
  logic        h_ok, h_pop, h_dev, h_io;
  logic [17:0] h_a;
  logic        h_st;
  logic [59:0] h_d;
  logic [4:0]  h_rt;
  logic        pipe_empty;                              // no BRAM load on its way back
  logic        mem_busy_boot;
  logic        halt_set;
  logic        dev_cool;                                // dev_req must fall for a cycle between two accesses
  logic        rx_take_c, tx_valid_c, fo_valid_c;
  logic [5:0]  tx_char_c;
  logic [59:0] fo_word_c;

  always_comb begin
    h_a = mq_a[mq_head]; h_st = mq_st[mq_head]; h_d = mq_d[mq_head]; h_rt = mq_rt[mq_head];
    h_ok = mq_v[mq_head] && mq_av[mq_head] && (!h_st || mq_dv[mq_head]) && (boot == 3'd4);
    h_dev = is_dev(h_a); h_io = is_io(h_a);
    pipe_empty = !ld_v && !ld_v2;
  end

  always_comb begin
    // defaults
    dwe = 1'b0; daddr = '0; dwdata = 60'd0;
    mq_pop = 1'b0;
    st_v = 1'b0; st_a = '0; st_d = 60'd0;
    dev_req = 1'b0; dev_we = 1'b0; dev_off = 13'd0; dev_wdata = 60'd0;
    rx_take_c = 1'b0; tx_valid_c = 1'b0; tx_char_c = 6'd0;
    fo_valid_c = 1'b0; fo_word_c = 60'd0;
    io_res_v = 1'b0; io_res_t = 5'd0; io_res_d = 60'd0;
    halt_set = 1'b0;
    case (boot)
      3'd0: daddr = MEM_AW'(1);
      3'd1: daddr = MEM_AW'(2);
      3'd2: daddr = MEM_AW'(3);
      default: ;
    endcase
    ld_v_next = 1'b0; ld_t_next = 5'd0;
    if (h_ok && !halt_req) begin
      if (h_dev) begin
        if (pipe_empty && !dev_cool) begin
          dev_req = 1'b1; dev_we = h_st; dev_off = h_a[12:0]; dev_wdata = h_d;
          if (dev_ack) begin
            mq_pop = 1'b1;
            if (!h_st) begin io_res_v = 1'b1; io_res_t = h_rt; io_res_d = dev_rdata; end
          end
        end
      end else if (h_io) begin
        if (h_st) begin
          if (h_a[2:0] == 3'd1) begin
            if (tx_ready) begin tx_valid_c = 1'b1; tx_char_c = h_d[5:0]; mq_pop = 1'b1; end
          end else if (h_a[2:0] == 3'd3) begin
            if (fo_ready) begin fo_valid_c = 1'b1; fo_word_c = h_d; mq_pop = 1'b1; end
          end else begin
            mq_pop = 1'b1;                              // IO_FILE, IO_HALT and the rest: handled with the pop
            if (h_a[2:0] == 3'd4) halt_set = 1'b1;
          end
        end else if (pipe_empty && rx_valid) begin
          rx_take_c = 1'b1; mq_pop = 1'b1;
          io_res_v = 1'b1; io_res_t = h_rt; io_res_d = rx_eof ? M60 : {54'd0, rx_char};
        end
      end else begin
        daddr = MEM_AW'(h_a);
        if (h_st) begin dwe = 1'b1; dwdata = h_d; st_v = 1'b1; st_a = MEM_AW'(h_a); st_d = h_d; end
        else begin ld_v_next = 1'b1; ld_t_next = h_rt; end
        mq_pop = 1'b1;
      end
    end
  end

  // ------------------------------------------------------------------ drained
  always_comb begin
    drained = (e_v == '0) && (u_o_busy == '0) && (mq_n == 3'd0) && !ld_v && !ld_v2 && !br_pend && !dv_run;
  end

  // ------------------------------------------------------------------ the clocked part
  always_ff @(posedge clk) begin
    if (reset) begin
      boot <= 3'd0; halted <= 1'b0; fault <= 1'b0; icount <= 64'd0;
      for (int q = 0; q < 8; q++) begin xr[q] <= 60'd0; ar[q] <= 18'd0; br[q] <= 18'd0; end
      rsv_v <= '0; e_v <= '0; sk_v <= '0; sk_wp <= '0;
      for (int t = 0; t < 16; t++) tag_pend[t] <= 2'b00;
      mq_v <= '0; mq_head <= 2'd0; mq_tail <= 2'd0; mq_n <= 3'd0;
      dp_w <= 18'd0; dp_s <= 2'd0; dp_nskip <= 5'd0; f_v <= 1'b0; f_drop <= 1'b0; f_a <= '0;
      dr_v <= 1'b0; br_pend <= 1'b0; halt_req <= 1'b0; halt_cnt <= 7'd0;
      ld_v <= 1'b0; ld_v2 <= 1'b0; ld_t <= 5'd0; ld_t2 <= 5'd0; ld_d2 <= 60'd0;
      io_file <= 60'd0; dv_run <= 1'b0; dv_cnt <= 6'd0; dev_cool <= 1'b0;
      rx_take <= 1'b0; tx_valid <= 1'b0; tx_char <= 6'd0; fo_valid <= 1'b0; fo_word <= 60'd0; fo_name <= 60'd0;
    end else begin
      // ---- boot block: entry point, B5 = B6, B4
      case (boot)
        3'd0: boot <= 3'd1;
        3'd1: begin dp_w <= drdata[17:0]; boot <= 3'd2; end
        3'd2: begin br[5] <= drdata[17:0]; br[6] <= drdata[17:0]; boot <= 3'd3; end
        3'd3: begin br[4] <= drdata[17:0]; xr[7] <= 60'o100; boot <= 3'd4; end
        default: ;
      endcase

      // ---- instruction fetch: the word that came, then a new request
      if (f_v && !f_drop) begin
        logic [$clog2(NSK)-1:0] vi;
        vi = sk_wp;
        if (sk_v[vi] && sk_a[vi] == dpa) vi = sk_wp + 1'b1;
        sk_d[vi] <= ret_data; sk_a[vi] <= f_a; sk_v[vi] <= 1'b1;
        sk_wp <= vi + 1'b1;
      end
      // a store into a word of the stack: write through
      if (st_v) for (int n = 0; n < NSK; n++) if (sk_v[n] && sk_a[n] == st_a) sk_d[n] <= st_d;
      f_v <= 1'b0; f_drop <= 1'b0;
      if (boot == 3'd4 && want_v) begin
        f_v <= 1'b1; f_a <= dpa + MEM_AW'(want_n);
        f_drop <= st_v && (st_a == dpa + MEM_AW'(want_n));
      end

      // ---- decode into the decode register
      if (redirect_v) begin
        dp_w <= redirect_t; dp_s <= 2'd0; dr_v <= 1'b0; dp_nskip <= 5'd0;
      end else if (!dr_v || issue_fire) begin
        dr_v <= 1'b0;
        if (boot == 3'd4 && dec_hit) begin
          if (found) begin
            dr_v <= 1'b1; dr_w <= dp_w; dr_f <= pf0; dr_i <= pc0[8:6]; dr_j <= pc0[5:3]; dr_k <= pc0[2:0];
            dr_K <= {pc0[2:0], pc1}; dr_ns <= dp_nskip + skipc; dr_bad <= pl && (spos == 3'd3);
            dp_nskip <= 5'd0;
          end else dp_nskip <= dp_nskip + skipc;
          if (nextw) begin dp_w <= dp_w + 18'd1; dp_s <= 2'd0; end
          else dp_s <= newpos[1:0];
        end
      end else if (issue_fire) dr_v <= 1'b0;

      // ---- units: operand capture, launch
      for (int u = 0; u < NU; u++) begin
        if (e_v[u]) begin
          if (e_w0[u]) begin
            logic [60:0] bf; bf = busf(e_t0[u]);
            if (bf[60]) begin e_s0[u] <= bf[59:0]; e_w0[u] <= 1'b0; end
          end
          if (e_w1[u]) begin
            logic [60:0] bf; bf = busf(e_t1[u]);
            if (bf[60]) begin e_s1[u] <= bf[59:0]; e_w1[u] <= 1'b0; end
          end
          if (launch[u]) e_v[u] <= 1'b0;
        end
      end
      // ---- issue
      if (issue_fire) begin
        icount <= icount + 64'd1 + 64'(dr_ns);
        // reservations and tags
        if (need_tag) begin
          tag_pend[new_tag] <= {D.d1v || D.mld, D.d0v};
          if (D.d0v) begin rsv_v[D.d0r] <= 1'b1; rsv_t[D.d0r] <= {new_tag, 1'b0}; end
          if (D.d1v) begin rsv_v[D.d1r] <= 1'b1; rsv_t[D.d1r] <= {new_tag, 1'b1}; end
          if (D.mld) begin rsv_v[rx_(dr_i)] <= 1'b1; rsv_t[rx_(dr_i)] <= {new_tag, 1'b1}; end
        end
        // into a unit
        if (D.uk != UK_NONE) begin
          e_v[sel_u] <= 1'b1;
          e_f[sel_u] <= dr_f; e_ijk[sel_u] <= {dr_i, dr_j, dr_k}; e_K[sel_u] <= dr_K; e_tag[sel_u] <= new_tag;
          e_d0v[sel_u] <= D.d0v; e_d1v[sel_u] <= D.d1v;
          e_s0[sel_u] <= o_v[0]; e_w0[sel_u] <= o_w[0]; e_t0[sel_u] <= o_t[0];
          e_s1[sel_u] <= o_v[1]; e_w1[sel_u] <= o_w[1]; e_t1[sel_u] <= o_t[1];
          e_mf[sel_u] <= D.mld || D.mst; e_mq[sel_u] <= mq_tail;
          e_wd[sel_u] <= dr_w;
          if (D.uk == UK_BR) br_pend <= 1'b1;
        end
        // the memory queue
        if (need_mq) begin
          mq_v[mq_tail] <= 1'b1; mq_st[mq_tail] <= D.mst || D.rj;
          mq_av[mq_tail] <= D.rj; mq_a[mq_tail] <= dr_K;
          mq_rt[mq_tail] <= {new_tag, 1'b1};
          if (D.rj) begin
            mq_dv[mq_tail] <= 1'b1; mq_dw[mq_tail] <= 1'b0;
            mq_d[mq_tail] <= {6'o04, 6'o00, 18'(dr_w + 18'd1), 30'd0};
          end else if (D.mst) begin
            mq_dv[mq_tail] <= !o_w[2]; mq_dw[mq_tail] <= o_w[2]; mq_dt[mq_tail] <= o_t[2]; mq_d[mq_tail] <= o_v[2];
          end else begin
            mq_dv[mq_tail] <= 1'b0; mq_dw[mq_tail] <= 1'b0;
          end
          mq_tail <= mq_tail + 2'd1;
        end
        if (D.special) begin halted <= 1'b1; fault <= 1'b1; end
      end
      mq_n <= mq_n + ((issue_fire && need_mq) ? 3'd1 : 3'd0) - (mq_pop ? 3'd1 : 3'd0);
      if (mq_pop) begin mq_v[mq_head] <= 1'b0; mq_head <= mq_head + 2'd1; end

      // an increment result that is a memory address
      for (int u = U_INC0; u < NU; u++) begin
        if (u_o_v[u] && u_o_mf[u]) begin
          mq_a[u_o_mq[u]] <= u_o_d0[u][17:0]; mq_av[u_o_mq[u]] <= 1'b1;
        end
      end
      // data of a store arrives
      for (int q = 0; q < Q; q++) if (mq_v[q] && mq_dw[q]) begin
        logic [60:0] bf; bf = busf(mq_dt[q]);
        if (bf[60]) begin mq_d[q] <= bf[59:0]; mq_dw[q] <= 1'b0; mq_dv[q] <= 1'b1; end
      end

      // ---- the divider
      if (e_v[U_DIV] && !e_w0[U_DIV] && !e_w1[U_DIV]) begin
        if (!dv_run && !dv_special_v) begin
          dv_run <= 1'b1; dv_cnt <= 6'd0; dv_q <= 51'd0; dv_r <= 50'd0;
          dv_neg <= dv_x.neg ^ dv_y.neg; dv_round <= e_f[U_DIV][0];
          dv_xc <= dv_x.c << clz48(dv_x.c); dv_yc <= dv_y.c << clz48(dv_y.c);
          dv_e <= int'(dv_x.e) - clz48(dv_x.c) - (int'(dv_y.e) - clz48(dv_y.c)) - 50;
        end else if (dv_run) begin
          if (dv_cnt != 6'd51) begin
            dv_q <= {dv_q[49:0], dv_qb}; dv_r <= dv_qb ? dv_rr - {2'b00, dv_yc} : dv_rr[49:0];
            dv_cnt <= dv_cnt + 6'd1;
          end else dv_run <= 1'b0;
        end
      end

      // ---- the branch unit
      if (launch[U_BR]) begin
        br_pend <= 1'b0;
      end

      // ---- registers and reservations
      for (int r = 0; r < 24; r++) if (rw_v[r]) begin
        rsv_v[r] <= 1'b0;
        if (r < 8) xr[r] <= rw_d[r];
        else if (r < 16) ar[r-8] <= rw_d[r][17:0];
        else if (r != 16) br[r-16] <= rw_d[r][17:0];
      end
      // tags are given back when their results are on the bus
      for (int b = 0; b < NB; b++) if (rb_v[b]) begin
        tag_pend[rb_t[b][4:1]][rb_t[b][0]] <= 1'b0;
      end

      // ---- loads coming back
      ld_v <= ld_v_next; ld_t <= ld_t_next;
      ld_v2 <= ld_v; ld_t2 <= ld_t; ld_d2 <= drdata;

      // ---- strobes to the outside
      rx_take <= rx_take_c; tx_valid <= tx_valid_c; tx_char <= tx_char_c;
      fo_valid <= fo_valid_c; fo_word <= fo_word_c; fo_name <= io_file;
      dev_cool <= dev_req && dev_ack;
      // ---- the stop
      if (halt_set) halt_req <= 1'b1;
      if (halt_req) begin
        halt_cnt <= halt_cnt + 7'd1;
        if (halt_cnt == HALT_WAIT[6:0]) halted <= 1'b1;
      end
      if (mq_pop && h_io && h_st && h_a[2:0] == 3'd2) io_file <= h_d;
    end
  end

  // redirect: the branch unit resolved a taken branch, or a jump at issue
  always_comb begin
    redirect_v = 1'b0; redirect_t = 18'd0;
    if (jump_now) begin redirect_v = 1'b1; redirect_t = jump_t; end
    if (launch[U_BR] && br_taken) begin redirect_v = 1'b1; redirect_t = br_tgt; end
  end

  assign dbg_pc = dp_w;
  always_comb begin
    for (int q = 0; q < 8; q++) begin
      dbg_x[60*q +: 60] = xr[q]; dbg_a[18*q +: 18] = ar[q]; dbg_b[18*q +: 18] = br[q];
    end
  end
endmodule


// a delay line for a result: LAT-1 register stages after the unit; LAT = 1 is a wire
module sb_pipe #(parameter int LAT = 1) (
  input  logic clk, input logic reset,
  input  logic in_v, input logic [3:0] in_t, input logic [59:0] in_d0, input logic [17:0] in_d1, input logic in_p1,
  input  logic in_mf, input logic [1:0] in_mq,
  output logic out_v, output logic [3:0] out_t, output logic [59:0] out_d0, output logic [17:0] out_d1, output logic out_p1,
  output logic out_mf, output logic [1:0] out_mq,
  output logic busy
);
  localparam int W = 1 + 4 + 60 + 18 + 1 + 1 + 2;
  logic [W-1:0] vin;
  assign vin = {in_v, in_t, in_d0, in_d1, in_p1, in_mf, in_mq};
  generate
    if (LAT <= 1) begin : g_wire
      assign {out_v, out_t, out_d0, out_d1, out_p1, out_mf, out_mq} = vin;
      assign busy = 1'b0;
    end else begin : g_regs
      logic [W-1:0] st[0:LAT-2];
      always_ff @(posedge clk) begin
        if (reset) for (int s = 0; s < LAT - 1; s++) st[s] <= '0;
        else begin
          st[0] <= vin;
          for (int s = 1; s < LAT - 1; s++) st[s] <= st[s-1];
        end
      end
      assign {out_v, out_t, out_d0, out_d1, out_p1, out_mf, out_mq} = st[LAT-2];
      always_comb begin
        busy = 1'b0;
        for (int s = 0; s < LAT - 1; s++) busy = busy | st[s][W-1];
      end
    end
  endgenerate
endmodule
