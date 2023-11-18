#include "wolf_sim/Environment.h"
#include "wolf_sim/AlwaysBlock.h"
#include "wolf_sim/Register.h"
#include <iostream>
#include <memory>

class WeightStationaryPE : public wolf_sim::AlwaysBlock {
    private:
        int rowIdx;
        int colIdx;
        int mSize;
        wolf_sim::RegRef<int, 1> featureIn;
        wolf_sim::RegRef<int, 1> featureOut;
        wolf_sim::RegRef<int, 1> resultIn;
        wolf_sim::RegRef<int, 1> resultOut;
        int weight;

    public:
        WeightStationaryPE(int rowIdx_, int colIdx_, int mSize_, 
        std::shared_ptr<wolf_sim::Register<int, 1>> featureIn_, 
        std::shared_ptr<wolf_sim::Register<int, 1>> featureOut_, 
        std::shared_ptr<wolf_sim::Register<int, 1>> resultIn_, 
        std::shared_ptr<wolf_sim::Register<int, 1>> resultOut_, int weight_) 
        : rowIdx(rowIdx_), colIdx(colIdx_), mSize(mSize_), weight(weight_) {
            featureIn.asInput(this, featureIn_);
            featureOut.asOutput(this, featureOut_);
            resultIn.asInput(this, resultIn_);
            resultOut.asOutput(this, resultOut_);
            blockIdentifier = std::string("PE[") + std::to_string(rowIdx) + std::string(",") + std::to_string(colIdx) + std::string("]");
        }

        wolf_sim::ReturnNothing always(){
            blockTimestamp += mSize;
            int fin=0;
            int rin=0;
            while(1){
                blockTimestamp += 1; 
                
                co_await resultOut.put(rin + fin * weight);
                if(colIdx != mSize-1) {
                    co_await featureOut.put(fin);
                }

                fin = co_await featureIn.get();
                if(rowIdx == 0){
                    rin = 0;
                } else {
                    rin = co_await resultIn.get();
                }
            }
        }
};

class SystolicArray {
    private:
    wolf_sim::Environment& env;
    int mSize;
    const std::vector<int>& transposedWeight;
    std::vector<std::vector<std::shared_ptr<WeightStationaryPE>>> peVec;
    std::vector<std::vector<std::shared_ptr<wolf_sim::Register<int, 1>>>> featureRegVec;
    std::vector<std::vector<std::shared_ptr<wolf_sim::Register<int, 1>>>> resultRegVec;
    std::shared_ptr<wolf_sim::Register<int, 1>> nullReg;

    public:
    std::vector<std::shared_ptr<wolf_sim::Register<int, 1>>> featureInRegVec;
    std::vector<std::shared_ptr<wolf_sim::Register<int, 1>>> resultOutRegVec;
    SystolicArray(wolf_sim::Environment& env_, int mSize_, const std::vector<int>& transposedWeight_) : env(env_), mSize(mSize_), transposedWeight(transposedWeight_) {

        nullReg = std::make_shared<wolf_sim::Register<int, 1>>();

        // 创建 featureReg 和 resultReg
        for(int row = 0; row < mSize; row++){
            featureRegVec.emplace_back();
            resultRegVec.emplace_back();
            for(int col = 0; col < mSize; col++){
                featureRegVec[row].push_back(std::make_shared<wolf_sim::Register<int, 1>>());
                resultRegVec[row].push_back(std::make_shared<wolf_sim::Register<int, 1>>());
            }
        }

        // 创建并连接 PE
        for(int row = 0; row < mSize; row++){
            peVec.emplace_back();
            for(int col = 0; col < mSize; col++){
                std::shared_ptr<wolf_sim::Register<int, 1>> featureInReg = featureRegVec[row][col];
                std::shared_ptr<wolf_sim::Register<int, 1>> featureOutReg = (col == mSize-1 ? nullReg : featureRegVec[row][col+1]);
                std::shared_ptr<wolf_sim::Register<int, 1>> resultInReg = (row == 0 ? nullReg : resultRegVec[row-1][col]);
                std::shared_ptr<wolf_sim::Register<int, 1>> resultOutReg = resultRegVec[row][col];
                peVec[row].push_back(std::make_shared<WeightStationaryPE>(row, col, mSize, featureInReg, featureOutReg, resultInReg, resultOutReg, transposedWeight[row * mSize + col]));
                env.addAlwaysBlock(peVec[row][col]);
            }
        }

        // featureReg 和 resultReg 的输入输出部分
        for(int idx = 0; idx < mSize; idx++){
            featureInRegVec.push_back(featureRegVec[idx][0]); // 第一列
            resultOutRegVec.push_back(resultRegVec[mSize-1][idx]); // 最后一行
        }
    }
};

class SystolicArrayDriver : public wolf_sim::AlwaysBlock {
    private:
    int featureCount;
    int mSize;
    const std::vector<int>& shiftedFeature;
    std::vector<wolf_sim::RegRef<int, 1>> featureInRegRefVec;

    public:
    SystolicArrayDriver(std::vector<std::shared_ptr<wolf_sim::Register<int, 1>>> featureInRegVec, int featureCount_, int mSize_,  const std::vector<int>& shiftedFeature_) :
     featureCount(featureCount_) , mSize(mSize_), shiftedFeature(shiftedFeature_) {
        for(auto featureInReg: featureInRegVec){
            featureInRegRefVec.emplace_back();
            featureInRegRefVec.back().asOutput(this, featureInReg);
        }
    }

    wolf_sim::ReturnNothing always() override {
        for(int row = 0; row < featureCount * mSize + mSize; row++){
            for(int col = 0; col < mSize; col++){
                co_await featureInRegRefVec[col].put(shiftedFeature[row * mSize + col]);
            }
            blockTimestamp += 1;
        }
    }
};

class SystolicArrayMonitor : public wolf_sim::AlwaysBlock {
    private:
    std::vector<wolf_sim::RegRef<int, 1>> resultOutRegRefVec;
    int featureCount;
    int mSize;
    const std::vector<int>& shiftedResult;

    public:
    SystolicArrayMonitor(std::vector<std::shared_ptr<wolf_sim::Register<int, 1>>> resultOutRegVec, int featureCount_, int mSize_, const std::vector<int>& shiftedResult_) 
    : featureCount(featureCount_), mSize(mSize_), shiftedResult(shiftedResult_) {
        for(auto resultOutReg: resultOutRegVec){
            resultOutRegRefVec.emplace_back();
            resultOutRegRefVec.back().asInput(this, resultOutReg);
        }
    }

    wolf_sim::ReturnNothing always(){
        for(int row = 0; row < featureCount * mSize + mSize-1; row++){
            for(int col = 0; col < mSize; col++){
                int result = co_await resultOutRegRefVec[col].get();
                if(row >= mSize && result != shiftedResult[(row - mSize)*mSize + col]){
                    std::cout << "Error: result mismatch, row=" << row << " ,col=" << col << " expect=" << shiftedResult[(row - mSize + 1)*mSize+col] << ", actual=" << result << std::endl;
                    exit(1);
                }
            }
            blockTimestamp += 1;
        }
        std::cout << "Simulation finished successfully @ " << blockTimestamp << std::endl;
        exit(0);
    }
};

int main() {
    int featureCount = 1024;
    int mSize = 16;
    
    std::vector<int> weight(mSize * mSize);
    std::vector<int> feature(featureCount * mSize * mSize);
    std::vector<int> result(featureCount * mSize * mSize);

    // 随机初始化 weight 和 feature，每个数字都在[0, 1024]范围内
    for(int i = 0; i < mSize * mSize; i++){
        weight[i] = rand() % 1024;
    }
    for(int i = 0; i < featureCount * mSize * mSize; i++){
        feature[i] = rand() % 1024;
    }
    // 计算矩阵乘法结果 result = weight * feature
    for(int fid = 0; fid < featureCount; fid++){
        for(int row = 0; row < mSize; row++){
            for(int col = 0; col < mSize; col++){
                int sum = 0;
                for(int k = 0; k < mSize; k++){
                    sum += weight[row * mSize + k] * feature[fid * mSize * mSize + k * mSize + col];
                }
                result[fid * mSize * mSize + row * mSize + col] = sum;
            }
        }
    }
    
    std::vector<int> transposedWeight(mSize * mSize);
    for(int row = 0; row < mSize; row++){
        for(int col = 0; col < mSize; col++){
            transposedWeight[row * mSize + col] = weight[col * mSize + row];
        }
    }

    std::vector<int> transposedFeature(featureCount * mSize * mSize);
    std::vector<int> transposedResult(featureCount * mSize * mSize);
    for(int fid = 0; fid < featureCount; fid++){
        for(int row = 0; row < mSize; row++){
            for(int col = 0; col < mSize; col++){
                transposedFeature[fid * mSize * mSize + row * mSize + col] = feature[fid * mSize * mSize + col * mSize + row];
                transposedResult[fid * mSize * mSize + row * mSize + col] = result[fid * mSize * mSize + col * mSize + row];
            }
        }
    }

    std::vector<int> shiftedFeature((featureCount * mSize + mSize) * mSize);//((featureCount * mSize + mSize) * mSize));
    std::vector<int> shiftedResult((featureCount * mSize + mSize) * mSize);
    for(int row = 0; row < featureCount * mSize + mSize - 1; row++) {
        for(int col = 0; col < mSize; col++){
            int srcCol = col;
            int srcRow = row - col;
            if(srcRow < 0 || srcRow >= featureCount * mSize){
                shiftedFeature[row * mSize + col] = 0;
                shiftedResult[row * mSize + col] = 0;
            } else {
                //std::cout << "array size="  << (featureCount * mSize + mSize) * mSize << " ,idx=" <<  row * mSize + col << "row=" << row << ", col=" << col << ", srcRow=" << srcRow << ", srcCol=" << srcCol << ", srcIdx=" << srcRow * mSize + srcCol << std::endl;
                shiftedFeature[row * mSize + col] = transposedFeature[srcRow * mSize + srcCol];
                shiftedResult[row * mSize + col] = transposedResult[srcRow * mSize + srcCol];
            }
        }
    }
    for(int col = 0;  col < mSize; col++){
        shiftedFeature[(featureCount * mSize + mSize - 1) * mSize + col] = -1;
    }

    // 创建仿真环境
    wolf_sim::Environment env(72);
    SystolicArray dut(env, mSize, transposedWeight);
    std::shared_ptr<SystolicArrayDriver> driver = std::make_shared<SystolicArrayDriver>(dut.featureInRegVec, featureCount, mSize, shiftedFeature);
    std::shared_ptr<SystolicArrayMonitor> monitor = std::make_shared<SystolicArrayMonitor>(dut.resultOutRegVec, featureCount, mSize, shiftedResult);
    env.addAlwaysBlock(driver);
    env.addAlwaysBlock(monitor);
    env.run();

}