module weight_stationary_pe #(parameter WIDTH = 32) (
        input wire clk,
        input wire rst_n,
        input wire  load_weight,
        input wire [WIDTH-1:0] weight_in,
        output wire [WIDTH-1:0] weight_out,
        input wire [WIDTH-1:0] feature_in,
        output wire [WIDTH-1:0] feature_out,
        input wire [WIDTH-1:0] result_in,
        output wire [WIDTH-1:0] result_out
    );

    reg [WIDTH-1:0] weight_reg;
    reg [WIDTH-1:0] feature_reg;
    reg [WIDTH-1:0] result_reg;

    wire [WIDTH-1:0] next_result = (weight_reg * feature_in) + result_in;

    always @(posedge clk or negedge rst_n) begin
        if (~rst_n) begin
            weight_reg <= 0;
            feature_reg <= 0;
            result_reg <= 0;
        end
        else begin
            if (load_weight) begin
                weight_reg <= weight_in;
                feature_reg <= 0;
                result_reg <= 0;
            end else begin
                feature_reg <= feature_in;
                result_reg <= next_result;
            end
            
        end
    end

    assign weight_out = weight_reg;
    assign feature_out = feature_reg;
    assign result_out = result_reg;

endmodule

module systolic_array #(parameter WIDTH = 32, parameter M_SIZE = 16) (
    input wire clk,
    input wire rst_n,

    input wire load_weight,
    input wire [WIDTH*M_SIZE-1:0] weight_in,
    
    input wire [WIDTH*M_SIZE-1:0] feature_in,

    output wire [WIDTH*M_SIZE-1:0] result_out
);

    localparam PE_NUM = M_SIZE*M_SIZE;

    wire [WIDTH*PE_NUM-1:0] pe_feature_in;
    wire [WIDTH*PE_NUM-1:0] pe_feature_out;
    wire [WIDTH*PE_NUM-1:0] pe_weight_in;
    wire [WIDTH*PE_NUM-1:0] pe_weight_out;
    wire [WIDTH*PE_NUM-1:0] pe_result_in;
    wire [WIDTH*PE_NUM-1:0] pe_result_out;

    weight_stationary_pe pe_inst[PE_NUM-1:0] (
        .clk(clk),
        .rst_n(rst_n),
        .load_weight(load_weight),
        .weight_in(pe_weight_in),
        .weight_out(pe_weight_out),
        .feature_in(pe_feature_in),
        .feature_out(pe_feature_out),
        .result_in(pe_result_in),
        .result_out(pe_result_out)
    );
    
    genvar row, col;
    generate
        for(row = 0; row < M_SIZE; row = row+1) begin
            for(col = 0; col < M_SIZE; col = col+1) begin
                if(row == 0) begin
                    assign pe_result_in[(row*M_SIZE+col)*WIDTH +: WIDTH] = 0;
                end else begin
                    assign pe_result_in[(row*M_SIZE+col)*WIDTH +: WIDTH] = pe_result_out[((row-1)*M_SIZE+col)*WIDTH +: WIDTH];
                end
                if(col == 0) begin
                    assign pe_feature_in[(row*M_SIZE+col)*WIDTH +: WIDTH] = feature_in[row*WIDTH +: WIDTH];
                    assign pe_weight_in[(row*M_SIZE+col)*WIDTH +: WIDTH] = weight_in[row*WIDTH +: WIDTH];
                end else begin
                    assign pe_feature_in[(row*M_SIZE+col)*WIDTH +: WIDTH] = pe_feature_out[(row*M_SIZE+col-1)*WIDTH +: WIDTH];
                    assign pe_weight_in[(row*M_SIZE+col)*WIDTH +: WIDTH] = pe_weight_out[(row*M_SIZE+col-1)*WIDTH +: WIDTH];
                end
                if(row == M_SIZE-1) begin
                    assign result_out[col*WIDTH +: WIDTH] = pe_result_out[(row*M_SIZE+col)*WIDTH +: WIDTH];
                end
            end
        end
    endgenerate
endmodule