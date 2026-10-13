#!/bin/sh
# hostpc.sh src.pas ico outdir : compile a Pascal source with the host-built 1972 compiler at the given ICO.
# Leaves listing.txt, pasclgo.out, parmlist.out, pmd.out in outdir.
set -e
src=$(realpath "$1"); ico=$2; out=$3
H=$(dirname "$(realpath "$0")")/../host
mkdir -p "$out"
python3 $H/tocdc.py "$src" > "$out/src.cdc"
cd "$out"
$H/pascal_host $ico < src.cdc > listing.txt
grep -n '^ \*\*\*\*' listing.txt | head -20 || true
