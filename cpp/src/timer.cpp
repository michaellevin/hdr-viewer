#include "timer.h"

Timer::Timer(const std::string& name) : name(name), start_time(std::chrono::high_resolution_clock::now()) {}

Timer::~Timer() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << name << " took " << duration.count() << "ms" << std::endl;
}