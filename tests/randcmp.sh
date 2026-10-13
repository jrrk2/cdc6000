#!/bin/sh
# randcmp.sh first last [size] [core] : random programs (randprog.py) on cdcemu and on a Verilated core; registers and memory must agree.
#   core: core (the sequential core, default) or sb (the scoreboarded core)
cd "$(dirname "$0")/.."
first=$1; last=$2; size=${3:-60}; core=${4:-core}
if [ $core = sb ]; then RTL=rtl/obj_dir_sb/Vcdc6000_sb; else RTL=rtl/obj_dir/Vcdc6000_core; fi
T=${TMPDIR:-/tmp}/randcmp.$$; mkdir -p $T
bad=0; n=0
seed=$first
while [ $seed -le $last ]; do
  python3 tests/randprog.py $seed $T/p.img $size || { echo "generator failed"; exit 2; }
  host/cdcemu host/runtime.img $T/p.img -R -m $T/e.mem -n 2000000 > $T/e.out 2> $T/e.err
  $RTL host/runtime.img $T/p.img -R -m $T/r.mem -n 20000000 > $T/r.out 2> $T/r.err
  if ! cmp -s $T/e.out $T/r.out || ! cmp -s $T/e.mem $T/r.mem; then
    echo "seed $seed DIFFERS"; bad=$((bad+1))
    [ $bad -le 3 ] && { diff $T/e.out $T/r.out | head -6; diff $T/e.mem $T/r.mem | head -4; tail -n 2 $T/e.err; tail -n 2 $T/r.err; }
  fi
  n=$((n+1)); seed=$((seed+1))
done
echo "$n programs, $bad differ ($core)"
rm -rf $T
