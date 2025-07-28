

#ifndef BIOAUTH_TIMER_H
#define BIOAUTH_TIMER_H

#include <chrono>
#include <iostream>

namespace bioauth {

class Timer {
public:
    Timer() = default;

    void start();

    void stop();

    [[nodiscard]] long long elapsed() const;

    void printElapsed() const;

    template <typename Func, typename... Args>
    long long benchmark(const Func& f, Args&&... args);

private:
    std::chrono::steady_clock::time_point start_;
    std::chrono::steady_clock::time_point stop_;
};


template <typename Func, typename... Args>
long long Timer::benchmark(const Func& f, Args&&... args) {
    start();
    f(std::forward<Args>(args)...);
    stop();
    return elapsed();
}

}


#endif //BIOAUTH_TIMER_H
