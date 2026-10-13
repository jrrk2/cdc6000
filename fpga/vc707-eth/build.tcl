# Vivado build of vc707_cdc_eth.  First make cdc_ram_bram.v (the memory holding runtime + program + NFS driver) with ./mkimage.sh <dir>, then from <dir> as the work
# directory:   vivado -mode batch -source <repo>/fpga/vc707-eth/build.tcl      (reports and vc707_cdc_eth.bit go to ./out)
# The routed design is saved as out/routed.dcp: a change of the software alone then needs only swap.tcl (seconds, no P&R).
# Env: VM_DEFS="CORE_SB=1" selects the scoreboarded core;  VM_DEFS="SYS_DIV=20.000 CLK_HZ=50000000" to change the core clock (the Ethernet side does not follow it).
set part xc7vx485tffg1761-2
set here [file dirname [file normalize [info script]]]
set repo [file normalize $here/../..]
set eth $repo/fpga/eth-rtl
file mkdir out
read_verilog -sv [list $repo/rtl/cdc_pkg.sv $repo/rtl/cdc6000_core.sv $repo/rtl/cdc6000_sb.sv]
read_verilog [list [file normalize cdc_ram_bram.v] $here/cdc_eth_core.v $here/vc707_cdc_eth.v $eth/simpleuart.v $eth/liteeth_sgmii_phy.v]
read_verilog -sv [list $eth/eth_mac_1g.sv $eth/axis_gmii_rx.sv $eth/axis_gmii_tx.sv $eth/rgmii_lfsr.sv \
    $eth/eth_lutram_fifo.sv $eth/eth_stream_dma.sv $eth/eth_gmii_retime256.sv $eth/eth_pkt_buf256.sv \
    $eth/sgmii_soc_liteeth.sv $eth/clkgen_vc707.sv]
read_xdc $here/vc707_cdc_eth.xdc
set defs [list SYNTHESIS]
if {[info exists env(VM_DEFS)]} { lappend defs {*}$env(VM_DEFS) }
synth_design -top vc707_cdc_eth -part $part -verilog_define $defs
report_utilization -file out/util_synth.rpt
opt_design
place_design -directive Explore
phys_opt_design -directive AggressiveExplore
route_design -directive Explore
phys_opt_design -directive AggressiveExplore
report_timing_summary -max_paths 10 -file out/timing.rpt
report_utilization -file out/util.rpt
report_methodology -file out/methodology.rpt
report_drc -file out/drc.rpt
report_cdc -file out/cdc.rpt
# memory map of the 60 x 64K RAM for updatemem (a software change then needs no Vivado: see swap.sh)
set fh [open out/cdc_ram.mmi w]
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
write_checkpoint -force out/routed.dcp
write_bitstream -force out/vc707_cdc_eth.bit
puts "CDC_ETH_BUILD_DONE"
