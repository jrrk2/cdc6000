# Software-only change: new memory contents into the routed design, no synthesis, placement or routing.
#   ./mkimage.sh <dir>   (writes <dir>/cdc_ram_bram.init.tcl)   then from <dir>:  vivado -mode batch -source <repo>/fpga/vc707-eth/swap.tcl
# needs out/routed.dcp from a full build (build.tcl).  Writes out/vc707_cdc_eth_sw.bit.
open_checkpoint out/routed.dcp
set n [llength [get_cells -hier -filter {REF_NAME == RAMB36E1}]]
if {$n != 120} { error "expected 120 RAMB36E1, found $n" }
foreach c [get_cells -hier -filter {REF_NAME == RAMB36E1}] { set cell([lindex [split $c /] end]) $c }
proc I {name ln val} { global cell; set_property INIT_$ln $val $cell($name) }
source cdc_ram_bram.init.tcl
write_bitstream -force out/vc707_cdc_eth_sw.bit
puts "CDC_ETH_SWAP_DONE"
