module full_adder (
    input wire x_,
    input wire y,
    input wire d,
    output wire c,
    output wire s
);

    assign c = (x_ & y) | (x_ & d) | (y & d);
    assign s = x_ ^ y ^ d;
 
    
endmodule

module bit_serial_mul #(parameter W = 16) (
    input wire clk,
    input wire rst_n,
    input wire [W-1:0] a,
    input wire x_bit,
    output wire y
);

    reg [W-1:0] s_reg;
    reg [W-1:0] c_reg;
    wire [W-1:0] a_reversed;

    genvar i;
    generate
        for(i = 0; i < W; i = i+1) begin
            assign a_reversed[i] = a[W-1-i];
        end
    endgenerate

    wire [W-1:0] x_in = a_reversed & ({W{x_bit}});
    wire [W-1:0] y_in = {s_reg[W-2:0], s_reg[0]}; 
    wire [W-1:0] d_in = c_reg;
    wire [W-1:0] c_out, s_out;

    assign y = s_out[W-1];

    always @(negedge rst_n or posedge clk) begin
        if(!rst_n) begin
            s_reg <= 0;
            c_reg <= 0;
        end
        else begin
            s_reg <= s_out;
            c_reg <= c_out;
        end
    end

    full_adder fa[W-1:0] (
        .x_(x_in),
        .y(y_in),
        .d(d_in),
        .c(c_out),
        .s(s_out)
    );

endmodule