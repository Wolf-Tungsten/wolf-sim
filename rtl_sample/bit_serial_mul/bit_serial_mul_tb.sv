module bit_serial_mul_tb;


    initial begin
   		$fsdbDumpfile("bit_serial_mul_tb.fsdb");
        $fsdbDumpvars(0, bit_serial_mul_tb, "+mda");
        $fsdbDumpvars(0, bit_serial_mul_tb);
    end

    reg clk = 0;
    reg rst_n = 0;
 
    initial begin
        clk = 0;
        forever #5 clk = ~clk;
    end

    initial begin
        rst_n = 0;
        #10 rst_n = 1;
    end

    localparam W = 16;
    localparam TESTCOUNT = 1024;

    reg [W-1:0] a = 0;
    reg x_bit = 0;
    reg [W-1:0] x_value, x_value_ = 0;
    reg [W*2-1:0] y_value = 0;
    wire y;
    integer i;

    bit_serial_mul #(.W(W)) dut (
        .clk(clk),
        .rst_n(rst_n),
        .a(a),
        .x_bit(x_bit),
        .y(y)
    );
 
    initial begin
        @(posedge rst_n);
        @(posedge clk);
        repeat (TESTCOUNT) begin
            a = $urandom_range(0, 1024);
            x_value = $urandom_range(0, 1024);
            x_value_ = x_value;
            i = 0;
            y_value = 0;
            repeat(W*2-1) begin
                x_bit = x_value[0];
                x_value = x_value >> 1;
                @(posedge clk);
                y_value = y_value | ({{(W*2){1'b0}},y} << i);
                i = i+1;
            end
            if({{(W*2){1'b0}},a}*x_value_ != y_value) begin
                $display("Error: a = %d, x = %d, y = %d, a*x = %d", a, x_value_, y_value, {{(W*2){1'b0}},a}*x_value_);
                $finish;
            end
        end
        $display("Success");
        $finish;
    end
        
endmodule
