// Simulation stand-in for cdc_ram_bram (the generated RAMB36E1 memory): 64K x 60, read first, loaded from mem.hex in the
// current directory (mkimage.sh writes it).
module cdc_ram_bram (
	input  wire        clk,
	input  wire        en,
	input  wire        we,
	input  wire [15:0] addr,
	input  wire [59:0] din,
	output reg  [59:0] dout,
	input  wire [15:0] iaddr,
	output reg  [59:0] idout
);
	reg [59:0] mem [0:65535];
	initial $readmemh("mem.hex", mem);
	always @(posedge clk) begin
		idout <= mem[iaddr];
		if (en) begin
			dout <= mem[addr];
			if (we) mem[addr] <= din;
		end
	end
endmodule
