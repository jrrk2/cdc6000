# Verilator build of the CDC 6000 core and its harness (same flags as ../template/Makefile)
VFLAGS = -CFLAGS -O2 --exe --Wno-fatal --Wno-widthtrunc --Wno-widthexpand --Wno-multidriven --Wno-BLKANDNBLK -Wno-UNOPTFLAT -Wno-lint --public-flat-rw
all: obj_dir/Vcdc6000_core

obj_dir/Vcdc6000_core: cdc_pkg.sv cdc6000_core.sv main_cdc.cpp ../host/cdcio.h ../nfs/netmodel.h
	verilator $(VFLAGS) --cc cdc_pkg.sv cdc6000_core.sv --top-module cdc6000_core main_cdc.cpp
	$(MAKE) -C obj_dir -f Vcdc6000_core.mk

clean:
	rm -rf obj_dir
