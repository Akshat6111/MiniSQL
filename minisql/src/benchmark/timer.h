#pragma once

#include <chrono>

namespace minisql {

class Stopwatch {
public:
    Stopwatch() : start_(std::chrono::high_resolution_clock::now()) {}

    void Reset() {
        start_ = std::chrono::high_resolution_clock::now();
    }

    double ElapsedMilliseconds() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }

    double ElapsedMicroseconds() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::micro>(end - start_).count();
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
};

}  // namespace minisql
