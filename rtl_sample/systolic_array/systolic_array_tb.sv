module systolic_array_tb;

    initial begin
   		// $fsdbDumpfile("systolic_array_tb.fsdb");
        // $fsdbDumpvars(0, systolic_array_tb, "+mda");
        // $fsdbDumpvars(0, systolic_array_tb);
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

    localparam WIDTH = 32;
    localparam M_SIZE = 16;
    localparam FEATURE_COUNT = 1024;

    reg [WIDTH-1:0] weight[M_SIZE][M_SIZE];
    reg [WIDTH-1:0] feature[FEATURE_COUNT][M_SIZE][M_SIZE];
    reg [WIDTH-1:0] feature_transpose[FEATURE_COUNT][M_SIZE][M_SIZE];
    reg [WIDTH-1:0] gemm_result[FEATURE_COUNT][M_SIZE][M_SIZE];
    reg [WIDTH-1:0] gemm_result_transpose[FEATURE_COUNT][M_SIZE][M_SIZE];
    reg [WIDTH-1:0] weight_in[M_SIZE][M_SIZE];
    reg [WIDTH-1:0] feature_in[M_SIZE*FEATURE_COUNT+M_SIZE-1][M_SIZE];
    reg [WIDTH-1:0] golden_result_out[M_SIZE*FEATURE_COUNT+M_SIZE-1][M_SIZE];

    integer row, col;
    integer fid, i;
    initial begin
        // 生成随机权重和特征数据
        for(row = 0; row < M_SIZE; row = row+1) begin
            for(col = 0; col < M_SIZE; col = col+1) begin
                weight[row][col] = $urandom_range(0, 1024);
            end
        end
        for(fid = 0; fid < FEATURE_COUNT; fid = fid + 1) begin
            for(row = 0; row < M_SIZE; row = row + 1) begin
                for(col = 0; col < M_SIZE; col = col+1) begin
                    feature[fid][row][col] = $urandom_range(0, 1024);
                end
            end
        end
        
        // 计算矩阵乘法的结果
        for(fid = 0; fid < FEATURE_COUNT; fid = fid+1) begin
            for(row = 0; row < M_SIZE; row = row + 1) begin
                for(col = 0; col < M_SIZE; col = col +1) begin
                    gemm_result[fid][row][col] = 0;
                    for(i = 0; i < M_SIZE; i = i + 1) begin
                        gemm_result[fid][row][col] = gemm_result[fid][row][col] + weight[row][i] * feature[fid][i][col];
                    end
                end
            end
        end
        // 按照脉动阵列数据摆放要求装填 weight_in 和 feature_in
        // weigth in 是 weight 颠倒行顺序
        for(row = 0; row < M_SIZE; row = row + 1) begin
            for(col = 0; col < M_SIZE; col = col + 1) begin
                weight_in[M_SIZE-row-1][col] = weight[row][col];
            end
        end
        // 将 feature 转置
        for(fid = 0; fid < FEATURE_COUNT; fid = fid + 1) begin
            for(row = 0; row < M_SIZE; row = row + 1) begin
                for(col = 0; col < M_SIZE; col = col + 1) begin
                    feature_transpose[fid][col][row] = feature[fid][row][col];
                end
            end
        end
        // 倾斜填充 feature_in
        for(row = 0; row < M_SIZE*FEATURE_COUNT+M_SIZE-1; row = row + 1) begin
            for(col = 0; col < M_SIZE; col = col + 1) begin
                if(row - col < 0 || row - col >= M_SIZE*FEATURE_COUNT) begin
                    feature_in[row][col] = 0;
                end else begin
                    feature_in[row][col] = feature_transpose[(row-col)/M_SIZE][(row-col)%M_SIZE][col];
                end
            end
        end
        // 将 gemm_result 转置
        for(fid = 0; fid < FEATURE_COUNT; fid = fid + 1) begin
            for(row = 0; row < M_SIZE; row = row + 1) begin
                for(col = 0; col < M_SIZE; col = col + 1) begin
                    gemm_result_transpose[fid][col][row] = gemm_result[fid][row][col];
                end
            end
        end
        // 倾斜填充 gold_result_out
        for(row = 0; row < M_SIZE*FEATURE_COUNT+M_SIZE-1; row = row + 1) begin
            for(col = 0; col < M_SIZE; col = col + 1) begin
                if(row - col < 0 || row - col >= M_SIZE*FEATURE_COUNT) begin
                    golden_result_out[row][col] = 0;
                end else begin
                    golden_result_out[row][col] = gemm_result_transpose[(row-col)/M_SIZE][(row-col)%M_SIZE][col];
                end
            end
        end
    end


    reg load_weight;
    reg [WIDTH*M_SIZE-1:0] dut_weight_in;
    reg [WIDTH*M_SIZE-1:0] dut_feature_in;
    wire [WIDTH*M_SIZE-1:0] dut_result_out;
    systolic_array #(
        .WIDTH(WIDTH),
        .M_SIZE(M_SIZE)
    ) dut (
        .clk(clk),
        .rst_n(rst_n),
        .load_weight(load_weight),
        .weight_in(dut_weight_in),
        .feature_in(dut_feature_in),
        .result_out(dut_result_out)
    );

    initial begin
        load_weight = 0;
        dut_feature_in = 0; 
        @(!rst_n);
        @(negedge clk);
        load_weight = 1;
        for(i = 0; i < M_SIZE; i = i + 1) begin
            for(col = 0; col < M_SIZE; col = col + 1) begin
                dut_weight_in[col*WIDTH +: WIDTH] = weight_in[i][col];
            end
            @(negedge clk);
        end
        load_weight = 0;
        for(i = 0; i < M_SIZE*FEATURE_COUNT+M_SIZE-1; i = i + 1) begin
            for(col = 0; col < M_SIZE; col = col + 1) begin
                dut_feature_in[col*WIDTH +: WIDTH] = feature_in[i][col];
            end
            @(negedge clk);
            // $display("i=%d, dut_result_out[0]=%d\n", i, dut_result_out[WIDTH-1:0]);
            if(i >= M_SIZE-1) begin
                for(col = 0; col < M_SIZE; col = col + 1) begin
                    if(dut_result_out[col*WIDTH +: WIDTH] !== golden_result_out[i-M_SIZE+1][col]) begin
                        $display("Error: dut_result_out[%d][%d] = %d, golden_result_out[%d][%d] = %d", i-M_SIZE+1, col, dut_result_out[col*WIDTH +: WIDTH], i-M_SIZE+1, col, golden_result_out[i-M_SIZE+1][col]);
                        $finish;
                    end
                end
            end
        end
        $display("Simulation finished successfully!");
        $finish;
    end



endmodule