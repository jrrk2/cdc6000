#!/bin/sh
# sbregress.sh : everything the scoreboarded core must pass, one after the other.
#   1. random programs, default configuration           2. random programs, other latencies / unit counts / stack sizes
#   3. float, integer and shift test                    4. the compiler compiles itself twice (fixed point), UART run time
#   5. the compiler compiles itself over NFS (the real file path of the board)           6. the shell, DHCP, compile and run (board-level simulation)
cd "$(dirname "$0")/.."
n=${1:-300}
echo "== 1 random programs"; tests/randcmp.sh 1 $n 120 sb | tail -n 3
echo "== 2 variants";        tests/sbvariants.sh $n 2>&1 | grep -v "^\[halt\|cycle limit"
echo "== 3 float test";      tests/fpcmp.sh sb 300 | cut -c1-120
echo "== 4 fixed point";     (cd rtl && make -s sb && CORE=./obj_dir_sb/Vcdc6000_sb sh ../tests/selfhost_sb.sh) 2>&1 | tail -n 2
echo "== 5 NFS self-compile"; (cd nfs && CORE=../rtl/obj_dir_sb/Vcdc6000_sb sh selfhost_nfs.sh rtl 2>&1 | tail -n 2)
echo "== done"
