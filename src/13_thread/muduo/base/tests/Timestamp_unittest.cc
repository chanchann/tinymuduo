#include "muduo/base/Timestamp.h"
#include <vector>
#include <stdio.h>

using muduo::Timestamp;

void passByConstReference(const Timestamp& x) {
    printf("now time pass by reference : %s\n", x.toString().c_str());
}

void passByValue(Timestamp x) {
    printf("now time pass by value : %s\n", x.toString().c_str());
}

void benchmark() {
    const int kNumber = 1000*1000;  // const带k前缀命名

    std::vector<Timestamp> stamps;
    stamps.reserve(kNumber);  // 预留了100w个空间
    for (int i = 0; i < kNumber; ++i) {
        stamps.push_back(Timestamp::now());  // 可以查看gettimeofday消耗的时间
    }
    printf("The first time : %s\n", stamps.front().toString().c_str());
    printf("The last time : %s\n", stamps.back().toString().c_str());
    printf("Time : diff : %f\n", timeDifference(stamps.back(), stamps.front()));

    int increments[100] = { 0 };
    int64_t start = stamps.front().microSecondsSinceEpoch();
    for (int i = 1; i < kNumber; ++i) {
        int64_t next = stamps[i].microSecondsSinceEpoch();
        int64_t inc = next - start;
        start = next;
        if (inc < 0) {
            printf("reverse!\n"); // 基本不可能，可能机器出问题
        } else if (inc < 100) {
            ++increments[inc];
        } else {
            printf("big gap %d\n", static_cast<int>(inc));
        }
    }

    for (int i = 0; i < 100; ++i) {
        printf("%2d: %d\n", i, increments[i]);
    }
}

int main() {
    Timestamp now(Timestamp::now());
    // Timestamp now = Timestamp::now(); // 也可以，都是调用了拷贝构造函数
    printf("now time : %s\n", now.toString().c_str());
    passByValue(now);
    passByConstReference(now);
    benchmark();
}