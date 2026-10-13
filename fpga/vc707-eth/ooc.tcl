# Out-of-context synthesis, placement and routing of the core alone, to see how fast it can go.
#   vivado -mode batch -source ooc.tcl -tclargs <period ns> <parameters for the core, e.g. "LAT_FADD=4 NINC=3">     (work in the current directory)
set part xc7vx485tffg1761-2
set here [file dirname [file normalize [info script]]]
set repo [file normalize $here/../..]
set period [lindex $argv 0]
set params [lrange $argv 1 end]
read_verilog -sv [list $repo/rtl/cdc_pkg.sv $repo/rtl/cdc6000_sb.sv]
set gen {}
lappend gen -generic MEM_AW=16 -generic EXT_MEM=1'b1
foreach p $params { lappend gen -generic $p }
synth_design -top cdc6000_sb -part $part -mode out_of_context {*}$gen -verilog_define SYNTHESIS
create_clock -name clk -period $period [get_ports clk]
set_false_path -from [get_ports reset]
write_checkpoint -force synth.dcp
report_utilization -file util_synth.rpt
opt_design
place_design
phys_opt_design
route_design
phys_opt_design
report_timing_summary -max_paths 20 -file timing.rpt
report_timing -max_paths 30 -nworst 1 -unique_pins -file paths.rpt
report_utilization -file util.rpt
write_checkpoint -force routed.dcp
puts "OOC_DONE"
