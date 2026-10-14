#!/bin/sh
# openflow.sh dir [seq|sb] [SYS_DIV CLK_HZ] : the open tool flow (sv2v, yosys, nextpnr, Project X-Ray) on the board design, no Vivado.
# dir holds mem.hex-derived cdc_ram_bram.v from mkimage_shell.sh; the result is dir/of_<core>.bit.  Tools come from ~/xc7-bitstream-tools
# (override with XT=...).  Run with `seq` (cdc6000_core, 50 MHz) first; `sb` is the scoreboarded core.
set -e
dir=$(cd "$1" && pwd); core=${2:-seq}; sd=${3:-20.000}; hz=${4:-50000000}
here=$(cd "$(dirname "$0")" && pwd); repo=$here/../..
XT=${XT:-$HOME/xc7-bitstream-tools}
YOSYS=$($XT/scripts/pinned_yosys.sh)
NEXTPNR=$XT/build/nextpnr-himbaechel
DB=$XT/.deps/prjxray-db
PART=xc7vx485tffg1761-2
eth=$repo/fpga/eth-rtl
cd "$dir"
case $core in
	sb)  SV="$repo/rtl/cdc_pkg.sv $repo/rtl/cdc6000_core.sv $repo/rtl/cdc6000_sb.sv"; DEF=CORE_SB=1 ;;   # yosys wants both modules of the generate
	seq) SV="$repo/rtl/cdc_pkg.sv $repo/rtl/cdc6000_core.sv"; DEF=CORE_SB=0 ;;
esac
# yosys does not read the packages and structs of the cores: sv2v turns them into plain Verilog
${SV2V:-$HOME/sv2v/bin/sv2v} -DSYNTHESIS $SV | sed -E 's/always @\([^)]*_sv2v_0\) begin/always @* begin/' > of_core.v   # sv2v lists unpacked arrays in sensitivity lists, which yosys rejects
SRCS="of_core.v cdc_ram_bram.v $here/cdc_eth_core.v $here/vc707_cdc_eth.v $eth/simpleuart.v $eth/liteeth_sgmii_phy.v \
 $eth/eth_mac_1g.sv $eth/axis_gmii_rx.sv $eth/axis_gmii_tx.sv $eth/rgmii_lfsr.sv $eth/eth_lutram_fifo.sv $eth/eth_stream_dma.sv \
 $eth/eth_gmii_retime256.sv $eth/eth_pkt_buf256.sv $eth/sgmii_soc_liteeth.sv $eth/clkgen_vc707.sv"
$YOSYS -q -l of_yosys.log -p "read_verilog -sv -DSYNTHESIS -D$DEF -DSYS_DIV=$sd -DCLK_HZ=$hz $SRCS; synth_xilinx -flatten -abc9 -arch xc7 -top vc707_cdc_eth; write_json of_$core.json"
$NEXTPNR --device $PART -o xdc=$here/vc707_cdc_nextpnr.xdc --json of_$core.json -o fasm=of_$core.fasm -o placement=of_${core}_placement.json \
	--router router2 --placer-heap-timingweight 60 --timing-allow-fail 2>&1 | tee of_nextpnr.log | grep -E "Max frequency|ERROR|Info: Program finished|slack" | tail -15
$XT/scripts/check_fasm_expressible.py $DB/virtex7 $PART of_$core.fasm
$XT/.venv/bin/python $XT/scripts/convert.py --arch xilinx --family xc7 --part $PART --db $DB --fasm of_$core.fasm --output of_$core.bit
ls -la of_$core.bit
