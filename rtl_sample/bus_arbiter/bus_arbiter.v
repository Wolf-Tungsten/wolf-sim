module master #(parameter INTERLEAVE=1) (
    input wire clk,
    input wire rst_n,
    output reg request,
    input wire grant
);

    reg [31:0] interleave_reg;

    always @(posedge clk or negedge rst_n) begin
        if (~rst_n) begin
            interleave_reg <= 0;
            request <= 0;
        end
        else begin
            if (interleave_reg <= INTERLEAVE) begin
                interleave_reg <= interleave_reg + 1;
            end else begin
                request <= 1'b1;
                if(grant) begin
                    request <= 1'b0;
                    interleave_reg <= 0;
                end
            end
        end
    end


endmodule

module arbiter8to1 (
    input wire clk,
    input wire rst_n,
    input wire [7:0] request,
    output reg [7:0] grant,
    output reg [31:0] request_count
);

    always @(negedge rst_n or posedge clk) begin
        if(!rst_n) begin
            request_count <= 0;
        end else begin
            casex (request)
                8'bxxxxxxx1: begin
                    $display("grant master 0 @ %t", $time);
                    request_count <= request_count + 1;
                    grant <= 8'b00000001;
                end
                8'bxxxxxx10: begin
                    $display("grant master 1 @ %t", $time);
                    request_count <= request_count + 1;
                    grant <= 8'b00000010;
                end
                8'bxxxxx100: begin
                    $display("grant master 2 @ %t", $time);
                    request_count <= request_count + 1;
                    grant <= 8'b00000100;
                end
                8'bxxxx1000: begin
                    $display("grant master 3 @ %t", $time);
                    request_count <= request_count + 1;
                    grant <= 8'b00001000;
                end
                8'bxxx10000: begin
                    $display("grant master 4 @ %t", $time);
                    request_count <= request_count + 1;
                    grant <= 8'b00010000;
                end
                8'bxx100000: begin
                    $display("grant master 5 @ %t", $time);
                    request_count <= request_count + 1;
                    grant <= 8'b00100000;
                end
                8'bx1000000: begin
                    $display("grant master 6 @ %t", $time);
                    request_count <= request_count + 1;
                    grant <= 8'b01000000;
                end
                8'b10000000: begin
                    $display("grant master 7 @ %t", $time);
                    request_count <= request_count + 1;
                    grant <= 8'b10000000;
                end
                default: grant <= 8'b00000000;
            endcase
        end
    end
endmodule

module bus_arbiter #(parameter MAX_REQUEST_COUNT = 1024) (
    input wire clk,
    input wire rst_n,
    output wire done
);

    wire [7:0] request;
    wire [7:0] grant;
    wire [31:0] request_count;

    assign done = (request_count >= MAX_REQUEST_COUNT);

    arbiter8to1 arbiter8to1_inst (
        .clk(clk),
        .rst_n(rst_n),
        .request(request),
        .grant(grant),
        .request_count(request_count)
    );

    genvar i;
    generate
        for (i = 0; i < 8; i = i + 1) begin
            master #(.INTERLEAVE(8 - i)) master_inst (
                .clk(clk),
                .rst_n(rst_n),
                .request(request[i]),
                .grant(grant[i])
            );
        end
    endgenerate

endmodule