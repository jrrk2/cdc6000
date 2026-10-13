#!/bin/sh
# fpcmp.sh [core] [cases]: every integer/shift/float instruction on many operand pairs (fptest.py): cdcemu against a Verilated core, memory dumps compared.
cd "$(dirname "$0")/.."
core=${1:-core}; cases=${2:-300}
if [ -n "$RTL" ]; then :; elif [ $core = sb ]; then RTL=rtl/obj_dir_sb/Vcdc6000_sb; else RTL=rtl/obj_dir/Vcdc6000_core; fi
T=${TMPDIR:-/tmp}/fpcmp.$$; mkdir -p $T
python3 tests/fptest.py $T/fp.img $cases && python3 host/cdcasm.py $T/fp.img.asm $T/fp.img
host/cdcemu $T/fp.img $T/fp.img -m $T/e.mem > /dev/null 2> $T/e.err < /dev/null
$RTL $T/fp.img $T/fp.img -m $T/r.mem -n 200000000 > /dev/null 2> $T/r.err < /dev/null
if cmp -s $T/e.mem $T/r.mem; then echo "float/integer/shift test ($core, $cases cases): $(wc -l < $T/e.mem) words identical; $(tail -n 1 $T/r.err)"; else echo "DIFFERENT"; diff $T/e.mem $T/r.mem | head -5; fi
rm -rf $T
