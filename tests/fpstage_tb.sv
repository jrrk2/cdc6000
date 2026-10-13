// Checks that the staged floating add / normalise functions of cdc_pkg give the results of the one-cycle ones.
//   build: run Verilator with --binary --timing on rtl/cdc_pkg.sv and this file, top module fpstage_tb
module fpstage_tb;
  import cdc_pkg::*;
  function automatic logic [59:0] rnd();
    logic [59:0] w;
    int k;
    k = $urandom_range(0, 9);
    w = {$urandom, $urandom};
    case (k)
      0: w = 60'd0;
      1: w = M60;
      2: w = {12'(1024 + $urandom_range(0, 100) - 50), 48'($urandom) | 48'h800000000000};
      3: w = ~{12'(1024 + $urandom_range(0, 100) - 50), 48'($urandom) | 48'h800000000000};
      4: w = {12'($urandom), 48'd0};
      5: w = {$urandom} & 60'hffffffffff;
      default: ;
    endcase
    rnd = w;
  endfunction
  initial begin
    int bad;
    bad = 0;
    for (int i = 0; i < 2000000; i++) begin
      logic [59:0] a, b, r0, r1;
      logic sub, round;
      a = rnd(); b = rnd(); sub = $urandom; round = $urandom;
      r0 = fnorm(add_prep(a, b, sub), round);
      r1 = fn_s3(fn_s2(fn_s1(fa_s2(fa_s1(a, b, sub)), round)));
      if (r0 !== r1) begin
        bad++;
        if (bad < 5) $display("DIFFER a=%o b=%o sub=%b round=%b  one cycle %o  staged %o", a, b, sub, round, r0, r1);
      end
    end
    $display("%0d differences in 2000000 float adds", bad);
    bad = 0;
    for (int i = 0; i < 2000000; i++) begin
      logic [59:0] a, b;
      a = rnd(); b = rnd();
      if (add60(a, b) !== add_s2(add_s1(a, b))) begin bad++; if (bad < 5) $display("add60 DIFFER %o %o", a, b); end
      if (add60(a, ~b) !== add_s2(add_s1(a, ~b))) begin bad++; if (bad < 5) $display("add60 DIFFER %o ~%o", a, b); end
      if (popcount60(a) !== pop_s2(pop_s1(a))) begin bad++; if (bad < 5) $display("popcount DIFFER %o", a); end
      if (nx_result(a) !== nx_s2(nx_s1(a))) begin bad++; if (bad < 5) $display("nx DIFFER %o", a); end
    end
    $display("%0d differences in 2000000 integer adds, counts and normalises", bad);
    $finish;
  end
endmodule
