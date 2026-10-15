#!/bin/sh
# openflow.sh dir [seq|sb] [SYS_DIV CLK_HZ] : the open tool flow (sv2v, yosys, nextpnr, Project X-Ray) on the board design, no Vivado.
# dir holds cdc_ram_bram.v and cdc_disk_bram.v from mkimage_shell.sh; the result is dir/of_<core>.bit.  Tools come from ~/xc7-bitstream-tools
# (override with XT=...).  Run with `seq` (cdc6000_core, 50 MHz) first; `sb` is the scoreboarded core.
# NEXTPNR_FLAGS overrides the placer/router options.  The default is what measured best on the dual-issue core (2026-10-14): timing
# weight 60, congestion spreading, timing-driven rip-up and the fork's hold fixer (the packet buffer's RAM inputs fail hold without it).
# The toolkit's measured delay matrix (-o delay-matrix=$XT/examples/vc707-litex-ddr-ethmin/delay-matrix-xc7vx485t.txt) with weight
# 100 did worse on both cores here (59.9 vs 65.4 MHz, 32.7 vs 38.8 MHz).  Bitstreams must be assembled against a database in which
# the IOB STEPDOWN feature is "always" (openXC7 prjxray-db b12466df or later), else an INT_L LH12 pip can collide with it.
set -e
dir=$(cd "$1" && pwd); core=${2:-seq}; sd=${3:-20.000}; hz=${4:-50000000}
here=$(cd "$(dirname "$0")" && pwd); repo=$here/../..
XT=${XT:-$HOME/xc7-bitstream-tools}
YOSYS=$($XT/scripts/pinned_yosys.sh)
NEXTPNR=$XT/build/nextpnr-himbaechel
DB=$XT/.deps/prjxray-db
NEXTPNR_FLAGS=${NEXTPNR_FLAGS:-"--placer-heap-timingweight 60 --placer-heap-congestion-spread --tmg-ripup -o hold-fix"}
PART=xc7vx485tffg1761-2
eth=$repo/fpga/eth-rtl
cd "$dir"
case $core in
	sb)  SV="$repo/rtl/cdc_pkg.sv $repo/rtl/cdc6000_core.sv $repo/rtl/cdc6000_sb.sv"; DEF=CORE_SB=1 ;;   # yosys wants both modules of the generate
	seq) SV="$repo/rtl/cdc_pkg.sv $repo/rtl/cdc6000_core.sv"; DEF=CORE_SB=0 ;;
esac
# yosys does not read the packages and structs of the cores: sv2v turns them into plain Verilog
${SV2V:-$HOME/sv2v/bin/sv2v} -DSYNTHESIS $SV | perl -pe 's/always @\(.*_sv2v_0\) begin/always @* begin/' > of_core.v   # sv2v lists unpacked arrays in sensitivity lists, which yosys rejects
SRCS="of_core.v cdc_ram_bram.v cdc_disk_bram.v $here/cdc_eth_core.v $here/vc707_cdc_eth.v $eth/simpleuart.v $eth/liteeth_sgmii_phy.v \
 $eth/eth_mac_1g.sv $eth/axis_gmii_rx.sv $eth/axis_gmii_tx.sv $eth/rgmii_lfsr.sv $eth/eth_lutram_fifo.sv $eth/eth_stream_dma.sv \
 $eth/eth_gmii_retime256.sv $eth/eth_pkt_buf256.sv $eth/sgmii_soc_liteeth.sv $eth/clkgen_vc707.sv"
$YOSYS -q -l of_yosys.log -p "read_verilog -sv -DSYNTHESIS -D$DEF -DSYS_DIV=$sd -DCLK_HZ=$hz $SRCS; synth_xilinx -flatten -abc9 -arch xc7 -top vc707_cdc_eth; write_json of_$core.json"
$NEXTPNR --device $PART -o xdc=$here/vc707_cdc_nextpnr.xdc --json of_$core.json -o fasm=of_$core.fasm -o placement=of_${core}_placement.json \
	--router router2 $NEXTPNR_FLAGS --timing-allow-fail 2>&1 | tee of_nextpnr.log | grep -E "Max frequency|ERROR|Info: Program finished|slack" | tail -15
$XT/scripts/check_fasm_expressible.py $DB/virtex7 $PART of_$core.fasm
$XT/.venv/bin/python $XT/scripts/convert.py --arch xilinx --family xc7 --part $PART --db $DB --fasm of_$core.fasm --output of_$core.bit
ls -la of_$core.bit
