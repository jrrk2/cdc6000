// Simulation stand-in for cdc_disk_bram (the generated RAMB36E1 ramdisk): DISK_KB kilobytes of bytes, loaded from disk.hex
// in the current directory (gendisk.py writes it, mkimage_shell.sh runs gendisk.py).  The byte at addr is on dout two
// clocks after addr changes, as in the real one.
module cdc_disk_bram #(parameter integer DISK_KB = 1024) (
	input  wire        clk,
	input  wire        we,
	input  wire [19:0] addr,
	input  wire [7:0]  din,
	output reg  [7:0]  dout,
	output wire [15:0] size_kb
);
	assign size_kb = DISK_KB;
	reg [7:0] mem [0:DISK_KB*1024-1];
	reg [7:0] q1;
	initial if (DISK_KB > 0) $readmemh("disk.hex", mem);
	always @(posedge clk) begin
		q1 <= mem[addr];
		dout <= q1;
		if (we) mem[addr] <= din;
	end
endmodule
