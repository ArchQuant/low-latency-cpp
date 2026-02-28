// ============================================================================
// Exercise 1: Measure the Cost of Reallocation
// ============================================================================
// 对比 push_back 不用 reserve() vs 用 reserve() 的性能差异
// 同时追踪 reallocation 的次数和每次的 capacity 变化
// ============================================================================

#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>

// 简单的计时器封装
struct Timer {
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point start;

    Timer() : start(Clock::now()) {}

    double elapsed_us() const {
        auto end = Clock::now();
        return std::chrono::duration<double, std::micro>(end - start).count();
    }

    double elapsed_ms() const {
        return elapsed_us() / 1000.0;
    }
};

// ── 工具函数：格式化数字（加千位分隔符）──
std::string format_number(size_t n) {
    std::string s = std::to_string(n);
    int insert_pos = static_cast<int>(s.length()) - 3;
    while (insert_pos > 0) {
        s.insert(insert_pos, ",");
        insert_pos -= 3;
    }
    return s;
}

// ── 方法 A：不用 reserve ──
void approach_a(size_t n) {
    std::cout << "=== Approach A: WITHOUT reserve() ===" << std::endl;
    std::cout << "Pushing " << format_number(n) << " elements..." << std::endl;
    std::cout << std::endl;

    std::vector<int> v;
    int realloc_count = 0;
    size_t prev_capacity = 0;
    size_t total_copies = 0;  // 累计被拷贝的元素数

    Timer timer;

    for (size_t i = 0; i < n; ++i) {
        v.push_back(static_cast<int>(i));

        if (v.capacity() != prev_capacity) {
            if (prev_capacity > 0) {
                realloc_count++;
                total_copies += prev_capacity;  // 旧元素全部要拷贝

                // 前几次和后几次打印详细信息
                if (realloc_count <= 5 || v.capacity() >= n / 4) {
                    std::cout << "  Realloc #" << std::setw(2) << realloc_count
                              << ": capacity " << std::setw(12) << format_number(prev_capacity)
                              << " -> " << std::setw(12) << format_number(v.capacity())
                              << "  (copied " << format_number(prev_capacity) << " elements)"
                              << std::endl;
                } else if (realloc_count == 6) {
                    std::cout << "  ... (more reallocations) ..." << std::endl;
                }
            }
            prev_capacity = v.capacity();
        }
    }

    double elapsed = timer.elapsed_ms();

    std::cout << std::endl;
    std::cout << "  Total reallocations:   " << realloc_count << std::endl;
    std::cout << "  Total elements copied: " << format_number(total_copies) << std::endl;
    std::cout << "  Final capacity:        " << format_number(v.capacity()) << std::endl;
    std::cout << "  Time:                  " << std::fixed << std::setprecision(2)
              << elapsed << " ms" << std::endl;
    std::cout << std::endl;
}

// ── 方法 B：用 reserve ──
void approach_b(size_t n) {
    std::cout << "=== Approach B: WITH reserve(" << format_number(n) << ") ===" << std::endl;

    std::vector<int> v;
    v.reserve(n);  // ← 一次性预分配

    size_t initial_capacity = v.capacity();
    int realloc_count = 0;
    size_t prev_capacity = initial_capacity;

    Timer timer;

    for (size_t i = 0; i < n; ++i) {
        v.push_back(static_cast<int>(i));

        if (v.capacity() != prev_capacity) {
            realloc_count++;
            prev_capacity = v.capacity();
        }
    }

    double elapsed = timer.elapsed_ms();

    std::cout << "  Reallocations:   " << realloc_count << " (should be 0!)" << std::endl;
    std::cout << "  Final capacity:  " << format_number(v.capacity()) << std::endl;
    std::cout << "  Time:            " << std::fixed << std::setprecision(2)
              << elapsed << " ms" << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << " Exercise 1: The Cost of Reallocation" << std::endl;
    std::cout << " Compiler: " <<
#if defined(__clang__)
        "Clang " __clang_version__
#elif defined(__GNUC__)
        "GCC " __VERSION__
#elif defined(_MSC_VER)
        "MSVC"
#else
        "Unknown"
#endif
    << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << std::endl;

    constexpr size_t N = 1'000'000;

    // 先跑一次 warmup（让 OS 准备好内存页等）
    {
        std::vector<int> warmup;
        warmup.reserve(N);
        for (size_t i = 0; i < N; ++i) warmup.push_back(static_cast<int>(i));
    }

    approach_a(N);
    approach_b(N);

    std::cout << "============================================================" << std::endl;
    std::cout << " Key Takeaway:" << std::endl;
    std::cout << " - Without reserve: ~20-30 reallocations, millions of copies" << std::endl;
    std::cout << " - With reserve: 0 reallocations, 0 extra copies" << std::endl;
    std::cout << " - The time difference is the cost of heap alloc + memcpy" << std::endl;
    std::cout << "============================================================" << std::endl;

    return 0;
}
