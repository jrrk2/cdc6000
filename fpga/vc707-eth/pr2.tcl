# Place and route again from the synthesised checkpoint, with the system clock overconstrained during placement.
#   vivado -mode batch -source pr2.tcl -tclargs <opt directive> <placer directive> <setup uncertainty ns> <route directive> <out dir>
# The uncertainty is applied to clk_sys_unbuf for opt/place/post-place phys_opt only and removed before routing.
lassign $argv odir pdir unc rdir od
file mkdir $od
open_checkpoint out/synth.dcp   ;# from build.tcl (or its first 31 lines: synthesis only)
set_clock_uncertainty -setup $unc [get_clocks clk_sys_unbuf]
opt_design -directive $odir
place_design -directive $pdir
phys_opt_design -directive AggressiveExplore
set_clock_uncertainty -setup 0 [get_clocks clk_sys_unbuf]
puts "PR2_WNS after place: [get_property SLACK [get_timing_paths -max_paths 1 -nworst 1 -setup]]"
write_checkpoint -force $od/placed.dcp
route_design -directive $rdir
write_checkpoint -force $od/routed_raw.dcp
set wns [get_property SLACK [get_timing_paths -max_paths 1 -nworst 1 -setup]]
puts "PR2_WNS after route: $wns"
foreach dir {Explore AggressiveExplore AggressiveFanoutOpt} {
    if {$wns >= 0} break
    phys_opt_design -directive $dir
    set wns [get_property SLACK [get_timing_paths -max_paths 1 -nworst 1 -setup]]
    puts "PR2_WNS after $dir: $wns"
}
report_timing_summary -max_paths 10 -file $od/timing.rpt
report_utilization -file $od/util.rpt
set fh [open $od/cdc_ram.mmi w]
puts $fh "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
puts $fh "<MemInfo Version=\"1\" Minor=\"0\">"
puts $fh "  <Processor Endianness=\"Little\" InstPath=\"cdc_ram\">"
puts $fh "    <AddressSpace Name=\"cdc_ram\" Begin=\"0\" End=\"524287\">"
foreach bank {0 1} {
    puts $fh "      <BusBlock>"
    for {set bit 59} {$bit >= 0} {incr bit -1} {
        set c [lindex [get_cells -hier -regexp ".*/r${bank}_${bit}"] 0]
        set loc [get_property LOC $c]
        regexp {_(X\d+Y\d+)$} $loc -> pl
        puts $fh "        <BitLane MemType=\"RAMB32\" Placement=\"$pl\">"
        puts $fh "          <DataWidth MSB=\"$bit\" LSB=\"$bit\"/>"
        puts $fh "          <AddressRange Begin=\"[expr {$bank * 32768}]\" End=\"[expr {$bank * 32768 + 32767}]\"/>"
        puts $fh "          <Parity ON=\"false\" NumBits=\"0\"/>"
        puts $fh "        </BitLane>"
    }
    puts $fh "      </BusBlock>"
}
puts $fh "    </AddressSpace>"
puts $fh "  </Processor>"
puts $fh "  <Config><Option Name=\"Part\" Val=\"xc7vx485tffg1761-2\"/></Config>"
puts $fh "</MemInfo>"
close $fh
write_checkpoint -force $od/routed.dcp
write_bitstream -force $od/vc707_cdc_eth.bit
puts "PR2_DONE $odir $pdir $unc $rdir WNS $wns"
