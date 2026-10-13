// Arithmetic of the CDC 6000 series: 60 bit and 18 bit ones-complement integers, and the 60 bit floating point
// format (1 sign, 11 exponent, 48 coefficient bits; the negative of a number is the complement of the whole word).
// Every function is a transcription of the same function in host/cdcemu.c, which is the reference model.
package cdc_pkg;

  localparam logic [59:0] M60 = 60'hFFFFFFFFFFFFFFF;

  // ---------------- 18 bit (B and A registers, addresses) ----------------
  // x + (-x) is +0 on this machine; only -0 + -0 stays -0
  function automatic logic [17:0] add18(input logic [17:0] a, input logic [17:0] b);
    logic [18:0] s;
    s = {1'b0, a} + {1'b0, b};
    if (s[18]) s = {1'b0, s[17:0]} + 19'd1;
    if (s[17:0] == 18'h3FFFF && !(a == 18'h3FFFF && b == 18'h3FFFF)) s[17:0] = 18'd0;
    add18 = s[17:0];
  endfunction
  function automatic logic [17:0] neg18(input logic [17:0] a);
    neg18 = ~a;
  endfunction
  // value of a ones-complement number: -0 is 0
  function automatic logic signed [18:0] sb18(input logic [17:0] a);
    sb18 = a[17] ? -$signed({1'b0, ~a}) : $signed({1'b0, a});
  endfunction
  function automatic logic [59:0] sx18(input logic [17:0] a);
    sx18 = {{42{a[17]}}, a};
  endfunction

  // ---------------- 60 bit ----------------
  function automatic logic [59:0] add60(input logic [59:0] a, input logic [59:0] b);
    logic [60:0] s;
    s = {1'b0, a} + {1'b0, b};
    if (s[60]) s = {1'b0, s[59:0]} + 61'd1;
    if (s[59:0] == M60 && !(a == M60 && b == M60)) s[59:0] = 60'd0;
    add60 = s[59:0];
  endfunction

  function automatic logic [59:0] rotl60(input logic [59:0] x, input logic [5:0] n);
    logic [5:0] r;
    logic [119:0] t;
    r = (n >= 6'd60) ? n - 6'd60 : n;
    t = ({60'd0, x} << r);
    rotl60 = t[59:0] | t[119:60];
  endfunction

  function automatic logic [59:0] sar60(input logic [59:0] x, input logic [5:0] n);
    logic signed [59:0] s;
    s = x;
    if (n >= 6'd60) sar60 = {60{x[59]}};
    else sar60 = 60'(s >>> n);
  endfunction

  function automatic logic [59:0] mask43(input logic [5:0] n);
    logic [59:0] t;
    if (n == 0) mask43 = 60'd0;
    else if (n >= 6'd60) mask43 = M60;
    else begin
      t = M60 << (6'd60 - n);
      mask43 = t;
    end
  endfunction

  function automatic logic [59:0] popcount60(input logic [59:0] x);
    logic [6:0] c;
    c = 0;
    for (int i = 0; i < 60; i++) c = c + {6'd0, x[i]};
    popcount60 = {53'd0, c};
  endfunction

  // 22/23: the shift count of a B register: low 6 bits, direction from the sign bit
  function automatic logic [5:0] shcount(input logic [17:0] b);
    shcount = b[17] ? ~b[5:0] : b[5:0];
  endfunction

  // ---------------- floating point ----------------
  typedef struct packed {
    logic              neg;
    logic signed [15:0] e;     // value = c * 2**e
    logic [47:0]       c;
  } fl_t;

  function automatic fl_t funpack(input logic [59:0] w);
    fl_t f;
    logic [59:0] v;
    f.neg = w[59];
    v = f.neg ? ~w : w;
    f.e = $signed({4'b0, v[59:48]}) - 16'sd1024;
    f.c = v[47:0];
    funpack = f;
  endfunction

  function automatic logic [59:0] fpack(input logic neg, input logic signed [15:0] e, input logic [47:0] c);
    logic [11:0] ef;
    logic [59:0] w;
    ef = 12'(e + 16'sd1024);
    w = {ef, c};
    fpack = neg ? ~w : w;
  endfunction

  // ---- floating results are produced in two steps: a *prep* function turns the operands into an unnormalised magnitude
  // (sign, m * 2**e, or a ready result), and one shared fnorm normalises, rounds and packs it.
  localparam int NW = 112;
  typedef struct packed {
    logic              zero;      // the result is +0
    logic              neg;
    logic [NW-1:0]     m;
    logic signed [15:0] e;
  } nrm_t;

  function automatic int msbn(input logic [NW-1:0] m);
    int p;
    p = 0;
    for (int i = 0; i < NW; i++) if (m[i]) p = i;
    msbn = p;
  endfunction

  // normalise a magnitude m * 2**e to 48 bits (truncating or rounding) and pack it
  function automatic logic [59:0] fnorm(input nrm_t n, input logic round);
    logic [NW-1:0] m;
    int e, p, sh;
    m = n.m; e = int'(n.e);
    if (n.zero || m == 0) fnorm = 60'd0;
    else begin
      p = msbn(m);
      sh = p - 47;
      if (sh > 0) begin
        if (round) m = m + (112'd1 << (sh - 1));
        m = m >> sh;
        e = e + sh;
        if (m[48]) begin m = m >> 1; e = e + 1; end
      end else if (sh < 0) begin
        m = m << (-sh);
        e = e + sh;
      end
      if (e + 1024 <= 0) fnorm = 60'd0;                                  // underflow
      else if (e + 1024 >= 'h7ff) fnorm = n.neg ? ~{12'h7ff, 48'd0} : {12'h7ff, 48'd0};   // overflow: infinite
      else fnorm = fpack(n.neg, 16'(e), m[47:0]);
    end
  endfunction

  function automatic nrm_t add_prep(input logic [59:0] a, input logic [59:0] b, input logic sub);
    fl_t x, y, t;
    int d;
    logic signed [NW:0] sx, sy, s;
    nrm_t r;
    x = funpack(a); y = funpack(b);
    if (sub) y.neg = !y.neg;
    r.zero = 1'b0; r.neg = 1'b0; r.m = '0; r.e = 16'sd0;
    if (x.c == 0) begin
      if (y.c == 0) r.zero = 1'b1;
      else begin r.neg = y.neg; r.m = NW'(y.c); r.e = y.e; end
    end else if (y.c == 0) begin
      r.neg = x.neg; r.m = NW'(x.c); r.e = x.e;
    end else begin
      if (x.e < y.e) begin t = x; x = y; y = t; end
      d = int'(x.e) - int'(y.e);
      if (d > 60) begin r.neg = x.neg; r.m = NW'(x.c); r.e = x.e; end
      else begin
        sx = $signed({{(NW+1-48){1'b0}}, x.c}) <<< d;
        if (x.neg) sx = -sx;
        sy = $signed({{(NW+1-48){1'b0}}, y.c});
        if (y.neg) sy = -sy;
        s = sx + sy;
        r.e = y.e;
        if (s == 0) r.zero = 1'b1;
        else if (s < 0) begin r.neg = 1'b1; r.m = NW'(-s); end
        else r.m = NW'(s);
      end
    end
    add_prep = r;
  endfunction

  function automatic nrm_t mul_prep(input logic [59:0] a, input logic [59:0] b, input logic [95:0] p);
    fl_t x, y;
    nrm_t r;
    x = funpack(a); y = funpack(b);
    r.zero = 1'b0; r.neg = x.neg ^ y.neg; r.m = NW'(p); r.e = 16'(int'(x.e) + int'(y.e));
    mul_prep = r;
  endfunction

  // DX*: the low half of the product; with both exponents zero (two integers) this is the integer product
  function automatic logic [59:0] dmul_from(input logic [59:0] a, input logic [59:0] b, input logic [95:0] p);
    fl_t x, y;
    logic [95:0] lo;
    logic neg;
    int pp, sh;
    x = funpack(a); y = funpack(b);
    neg = x.neg ^ y.neg;
    if (x.e == -16'sd1024 && y.e == -16'sd1024)
      dmul_from = neg ? ~{12'd0, p[47:0]} : {12'd0, p[47:0]};
    else if (p == 0) dmul_from = 60'd0;
    else begin
      pp = msbn(NW'(p));
      sh = pp - 47; if (sh < 0) sh = 0;
      lo = p & ((96'd1 << sh) - 96'd1);
      dmul_from = fpack(neg, 16'(int'(x.e) + int'(y.e)), lo[47:0]);
    end
  endfunction

  // the same operations in one piece, for tests
  function automatic logic [59:0] fadd(input logic [59:0] a, input logic [59:0] b, input logic sub, input logic round);
    fadd = fnorm(add_prep(a, b, sub), round);
  endfunction
  function automatic logic [59:0] fmul(input logic [59:0] a, input logic [59:0] b, input logic round);
    logic [95:0] p;
    fl_t x, y;
    x = funpack(a); y = funpack(b);
    p = {48'd0, x.c} * {48'd0, y.c};
    fmul = fnorm(mul_prep(a, b, p), round);
  endfunction
  function automatic logic [59:0] dmul(input logic [59:0] a, input logic [59:0] b);
    logic [95:0] p;
    fl_t x, y;
    x = funpack(a); y = funpack(b);
    p = {48'd0, x.c} * {48'd0, y.c};
    dmul = dmul_from(a, b, p);
  endfunction

  // number of leading zeros of a 48 bit coefficient
  function automatic int clz48(input logic [47:0] c);
    int n;
    n = 48;
    for (int i = 0; i < 48; i++) if (c[i]) n = 47 - i;
    clz48 = n;
  endfunction

  // NX / ZX / UX / PX
  function automatic logic [59:0] nx_result(input logic [59:0] w);
    fl_t f; int n; logic [47:0] c; logic signed [15:0] e;
    f = funpack(w);
    if (f.c == 0) nx_result = 60'd0;
    else begin
      n = clz48(f.c);
      c = f.c << n;
      e = f.e - 16'(n);
      nx_result = fpack(f.neg, e, c);
    end
  endfunction
  function automatic logic [17:0] nx_count(input logic [59:0] w);
    fl_t f;
    f = funpack(w);
    nx_count = (f.c == 0) ? 18'd0 : 18'(clz48(f.c));
  endfunction

  function automatic logic [59:0] ux_coef(input logic [59:0] w);
    fl_t f;
    f = funpack(w);
    ux_coef = f.neg ? ~{12'd0, f.c} : {12'd0, f.c};
  endfunction
  function automatic logic [17:0] ux_exp(input logic [59:0] w);
    fl_t f; logic signed [18:0] e;
    f = funpack(w);
    e = 19'(f.e);
    ux_exp = (e < 0) ? ~18'(-e) : 18'(e);
  endfunction

  function automatic logic [59:0] px_result(input logic [59:0] w, input logic [17:0] bj);
    logic neg; logic [59:0] v; logic [47:0] c; logic signed [15:0] e; logic [59:0] r;
    neg = w[59];
    v = neg ? ~w : w;
    c = v[47:0];
    e = 16'(sb18(bj));
    px_result = fpack(neg, e, c);
  endfunction

endpackage
