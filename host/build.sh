#!/bin/sh
# build the FPC host compiler and run it on the compiler's own source (stage 1)
set -e
cd "$(dirname "$0")"
python3 mkfpc.py
fpc -Miso pascal_host.pas
python3 tocdc.py ../ocr/pascal.pas > pascal.cdc
mkdir -p run && cd run && ../pascal_host < ../pascal.cdc > listing.txt
grep -c '^ ****' listing.txt || true
