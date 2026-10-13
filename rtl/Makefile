# Verilator build of the CDC 6000 cores and their harness (same flags as ../template/Makefile)
#   make        the sequential core:        obj_dir/Vcdc6000_core
#   make sb     the scoreboarded core:      obj_dir_sb/Vcdc6000_sb
VFLAGS = -CFLAGS -O2 --exe --Wno-fatal --Wno-widthtrunc --Wno-widthexpand --Wno-multidriven --Wno-BLKANDNBLK -Wno-UNOPTFLAT -Wno-lint --public-flat-rw
all: obj_dir/Vcdc6000_core
sb: obj_dir_sb/Vcdc6000_sb

obj_dir/Vcdc6000_core: cdc_pkg.sv cdc6000_core.sv main_cdc.cpp ../host/cdcio.h ../nfs/netmodel.h
	verilator $(VFLAGS) --cc cdc_pkg.sv cdc6000_core.sv --top-module cdc6000_core main_cdc.cpp
	$(MAKE) -C obj_dir -f Vcdc6000_core.mk

obj_dir_sb/Vcdc6000_sb: cdc_pkg.sv cdc6000_sb.sv main_cdc.cpp ../host/cdcio.h ../nfs/netmodel.h
	verilator $(VFLAGS) -CFLAGS -DCORE_SB -Mdir obj_dir_sb --cc cdc_pkg.sv cdc6000_sb.sv --top-module cdc6000_sb main_cdc.cpp
	$(MAKE) -C obj_dir_sb -f Vcdc6000_sb.mk

clean:
	rm -rf obj_dir obj_dir_sb
