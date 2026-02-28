// ============================================================================
// Exercise 2: Vector vs List — Cache Locality in Action
// ============================================================================
// 同样的数据、同样的操作（遍历求和），不同的容器
// 展示 cache locality 对性能的巨大影响
// ============================================================================

#include <iostream>
#include <vector>
#include <list>
#include <deque>
#include <numeric>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cmath>

struct Timer {
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point start;
    Timer() : start(Clock::now()) {}
    double elapsed_us() const {
        return std::chrono::duration<double, std::micro>(Clock::now() - start).count();
    }
};

// ── 统计辅助 ──
struct BenchResult {
    std::string name;
    double mean_us;
    double stddev_us;
    double min_us;
    double max_us;
};

BenchResult compute_stats(const std::string& name, std::vector<double>& times) {
    // 去掉最快和最慢的 10% 来减少噪声
    std::sort(times.begin(), times.end());
    size_t trim = times.size() / 10;
    double sum = 0, sum_sq = 0;
    size_t count = 0;
    for (size_t i = trim; i < times.size() - trim; ++i) {
        sum += times[i];
        sum_sq += times[i] * times[i];
        count++;
    }
    double mean = sum / count;
    double variance = (sum_sq / count) - (mean * mean);
    return {name, mean, std::sqrt(std::max(0.0, variance)), times.front(), times.back()};
}

void print_result(const BenchResult& r) {
    std::cout << "  " << std::left << std::setw(16) << r.name
              << "mean = " << std::right << std::setw(10) << std::fixed << std::setprecision(1)
              << r.mean_us << " us"
              << "   stddev = " << std::setw(8) << r.stddev_us << " us"
              << "   [min=" << std::setw(8) << r.min_us
              << ", max=" << std::setw(8) << r.max_us << "]"
              << std::endl;
}

// ── 防止编译器优化掉结果 ──
volatile long long sink = 0;

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << " Exercise 2: Vector vs List vs Deque — Cache Locality" << std::endl;
    std::cout << "============================================================" << std::endl;

    // 测试多个规模
    const std::vector<size_t> sizes = {1'000, 10'000, 100'000, 1'000'000, 10'000'000};
    constexpr int RUNS = 50;

    for (size_t N : sizes) {
        std::cout << std::endl;
        std::cout << "── N = " << N << " ──" << std::endl;

        // ── 构建数据 ──
        std::vector<int> vec(N);
        std::iota(vec.begin(), vec.end(), 1);  // 填充 1, 2, 3, ...

        std::list<int> lst(vec.begin(), vec.end());

        std::deque<int> deq(vec.begin(), vec.end());

        // ── Benchmark: vector ──
        std::vector<double> vec_times;
        for (int r = 0; r < RUNS; ++r) {
            Timer t;
            long long sum = 0;
            for (const auto& x : vec) sum += x;
            double elapsed = t.elapsed_us();
            sink = sum;
            vec_times.push_back(elapsed);
        }

        // ── Benchmark: list ──
        std::vector<double> list_times;
        for (int r = 0; r < RUNS; ++r) {
            Timer t;
            long long sum = 0;
            for (const auto& x : lst) sum += x;
            double elapsed = t.elapsed_us();
            sink = sum;
            list_times.push_back(elapsed);
        }

        // ── Benchmark: deque ──
        std::vector<double> deq_times;
        for (int r = 0; r < RUNS; ++r) {
            Timer t;
            long long sum = 0;
            for (const auto& x : deq) sum += x;
            double elapsed = t.elapsed_us();
            sink = sum;
            deq_times.push_back(elapsed);
        }

        // ── 输出结果 ──
        auto vec_r = compute_stats("std::vector", vec_times);
        auto lst_r = compute_stats("std::list", list_times);
        auto deq_r = compute_stats("std::deque", deq_times);

        print_result(vec_r);
        print_result(lst_r);
        print_result(deq_r);

        double list_ratio = lst_r.mean_us / vec_r.mean_us;
        double deq_ratio = deq_r.mean_us / vec_r.mean_us;

        std::cout << std::endl;
        std::cout << "  list / vector = " << std::fixed << std::setprecision(1)
                  << list_ratio << "x slower" << std::endl;
        std::cout << "  deque / vector = " << std::fixed << std::setprecision(1)
                  << deq_ratio << "x slower" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << " Key Takeaway:" << std::endl;
    std::cout << " - vector is the fastest due to contiguous memory" << std::endl;
    std::cout << " - list is 10-40x slower due to pointer chasing" << std::endl;
    std::cout << " - deque is in between (chunked contiguous memory)" << std::endl;
    std::cout << " - The gap GROWS with N (larger N = more cache misses)" << std::endl;
    std::cout << "============================================================" << std::endl;

    return 0;
}
