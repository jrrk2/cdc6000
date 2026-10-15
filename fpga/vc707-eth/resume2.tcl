# More post-route physical optimisation on a routed checkpoint, until it stops gaining.
#   vivado -mode batch -source resume2.tcl -tclargs <in dir> <out dir>
lassign $argv id od
file mkdir $od
open_checkpoint $id/routed.dcp
set wns [get_property SLACK [get_timing_paths -max_paths 1 -nworst 1 -setup]]
puts "R2_WNS start: $wns"
for {set round 0} {$round < 4 && $wns < 0} {incr round} {
    set before $wns
    foreach dir {AggressiveFanoutOpt AlternateFlowWithRetiming AddRetime AggressiveExplore Explore} {
        if {$wns >= 0} break
        phys_opt_design -directive $dir
        set wns [get_property SLACK [get_timing_paths -max_paths 1 -nworst 1 -setup]]
        puts "R2_WNS round $round after $dir: $wns"
    }
    if {$wns < 0} {
        route_design -tns_cleanup
        set wns [get_property SLACK [get_timing_paths -max_paths 1 -nworst 1 -setup]]
        puts "R2_WNS round $round after tns_cleanup: $wns"
    }
    if {$wns <= $before + 0.005} break
}
report_timing_summary -max_paths 10 -file $od/timing.rpt
file copy -force $id/cdc_ram.mmi $od/cdc_ram.mmi
write_checkpoint -force $od/routed.dcp
if {$wns >= 0} { write_bitstream -force $od/vc707_cdc_eth.bit }
puts "R2_DONE WNS $wns"
