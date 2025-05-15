#pragma once

#include<iostream>
#include<string>

// 定义时间类
class Timestamp{
private:
    int64_t microSecondsSinceEpoch_;
public:
    explicit Timestamp(); //禁止隐式转换(timestamp = 5)
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;
};