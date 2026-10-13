# Vivado build of the CDC 6000 core on the VC707.   usage (from a work directory containing mem.hex):
#   vivado -mode batch -source <repo>/fpga/vc707/build.tcl
# Reports and vc707_cdc.bit go to ./out.   Env: CLK_MHZ (default 25), BAUD (default 921600).
set part xc7vx485tffg1761-2
set here [file dirname [file normalize [info script]]]
set repo [file normalize $here/../..]
set clk 25
if {[info exists ::env(CLK_MHZ)]} { set clk $::env(CLK_MHZ) }
set baud 921600
if {[info exists ::env(BAUD)]} { set baud $::env(BAUD) }
file mkdir out
read_verilog -sv $repo/rtl/cdc_pkg.sv $repo/rtl/cdc6000_core.sv
read_verilog [list $here/vc707_cdc_top.v $here/simpleuart.v]
read_xdc $here/vc707.xdc
synth_design -top vc707_cdc_top -part $part -verilog_define SYNTHESIS -generic CLK_MHZ=$clk -generic BAUD=$baud
report_utilization -file out/util_synth.rpt
opt_design
place_design
route_design
report_timing_summary -max_paths 10 -file out/timing.rpt
report_utilization -file out/util.rpt
report_methodology -file out/methodology.rpt
report_drc -file out/drc.rpt
write_bitstream -force out/vc707_cdc.bit
puts "CDC_VIVADO_BUILD_DONE"
