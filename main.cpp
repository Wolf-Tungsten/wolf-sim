//
// Created by gaoruihao on 6/23/23.
//


#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "async_simple/async_simple/coro/Lazy.h"
#include "async_simple/async_simple/coro/SyncAwait.h"
#include <coroutine>

using namespace async_simple::coro;

using Texts = std::vector<std::string>;

Lazy<Texts> ReadFile(const std::string &filename) {
    Texts Result;
    std::ifstream infile(filename);
    std::string line;
    while (std::getline(infile, line))
        Result.push_back(line);
    co_return Result;
}

Lazy<int> CountLineChar(const std::string &line, char c) {
    co_return std::count(line.begin(), line.end(), c);
}

Lazy<int> CountTextChar(const Texts &Content, char c) {
    int Result = 0;
    for (const auto &line : Content)
        Result += co_await CountLineChar(line, c);
    co_return Result;
}

Lazy<int> CountFileCharNum(const std::string &filename, char c) {
    Texts Contents = co_await ReadFile(filename);
    co_return co_await CountTextChar(Contents, c);
}

int main() {
    int Num = syncAwait(CountFileCharNum("/home/gaoruihao/hdd0/project/wolf-sim/async_simple/demo_example/Input/file.txt", 'x'));
    std::cout << "The number of 'x' in file.txt is " << Num << "\n";
    return 0;
}