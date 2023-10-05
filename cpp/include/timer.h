#pragma once
#include <chrono>
#include <iostream>
#include <string>

class Timer {
public:
    Timer(const std::string& name);
    ~Timer();

private:
    std::string name;
    std::chrono::high_resolution_clock::time_point start_time;
};