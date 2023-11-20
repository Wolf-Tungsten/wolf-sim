module bus_arbiter_tb;


    initial begin
   		$fsdbDumpfile("bus_arbiter_tb.fsdb");
        $fsdbDumpvars(0, bus_arbiter_tb, "+mda");
        $fsdbDumpvars(0, bus_arbiter_tb);
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

    wire done;

    bus_arbiter dut (
        .clk(clk),
        .rst_n(rst_n),
        .done(done)
    );

    always @(posedge clk) begin
        if (done) begin
            $display("Simulation finished");
            $finish;
        end
    end
endmodule
