// ============================================================================
// Exercise 3: SOA vs AOS — Memory Layout Matters
// ============================================================================
// 两种内存布局，两种访问模式：
//   Operation 1 (SOA-friendly): 只读 bid + ask，计算平均 spread
//   Operation 2 (AOS-friendly): 找到 bid 最高的 quote，读取所有字段
// ============================================================================

#include <iostream>
#include <vector>
#include <array>
#include <chrono>
#include <iomanip>
#include <random>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <numeric>

// ── 防止编译器优化 ──
volatile double vol_sink = 0;
volatile uint64_t vol_sink_u = 0;

struct Timer {
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point start;
    Timer() : start(Clock::now()) {}
    double elapsed_ms() const {
        return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    }
};

// ============================================================================
// AOS: Array of Structures — C++默认方式
// ============================================================================
struct QuoteAOS {
    uint64_t timestamp;
    double   bid;
    double   ask;
    uint32_t bid_size;
    uint32_t ask_size;
    char     symbol[8];
    // sizeof = 48 bytes (with padding)
};

// ============================================================================
// SOA: Structure of Arrays — 分字段存储
// ============================================================================
struct QuoteSOA {
    std::vector<uint64_t> timestamps;
    std::vector<double>   bids;
    std::vector<double>   asks;
    std::vector<uint32_t> bid_sizes;
    std::vector<uint32_t> ask_sizes;
    std::vector<std::array<char, 8>> symbols;

    void resize(size_t n) {
        timestamps.resize(n);
        bids.resize(n);
        asks.resize(n);
        bid_sizes.resize(n);
        ask_sizes.resize(n);
        symbols.resize(n);
    }
};

// ── 统计 ──
struct Stats {
    double mean, stddev;
};

Stats calc_stats(std::vector<double>& times) {
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
    double var = (sum_sq / count) - (mean * mean);
    return {mean, std::sqrt(std::max(0.0, var))};
}

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << " Exercise 3: SOA vs AOS Benchmark" << std::endl;
    std::cout << "============================================================" << std::endl;

    constexpr size_t N = 10'000'000;
    constexpr int RUNS = 20;

    std::cout << " N = " << N << " quotes, " << RUNS << " runs each" << std::endl;
    std::cout << " sizeof(QuoteAOS) = " << sizeof(QuoteAOS) << " bytes" << std::endl;
    std::cout << std::endl;

    // ── 生成随机数据 ──
    std::cout << "Generating data..." << std::flush;
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> price_dist(90.0, 110.0);
    std::uniform_int_distribution<uint32_t> size_dist(100, 10000);

    // AOS 数据
    std::vector<QuoteAOS> aos(N);
    for (size_t i = 0; i < N; ++i) {
        aos[i].timestamp = i;
        aos[i].bid = price_dist(rng);
        aos[i].ask = aos[i].bid + 0.01 + (rng() % 10) * 0.001;
        aos[i].bid_size = size_dist(rng);
        aos[i].ask_size = size_dist(rng);
        std::memcpy(aos[i].symbol, "AAPL    ", 8);
    }

    // SOA 数据（相同的值）
    QuoteSOA soa;
    soa.resize(N);
    for (size_t i = 0; i < N; ++i) {
        soa.timestamps[i] = aos[i].timestamp;
        soa.bids[i] = aos[i].bid;
        soa.asks[i] = aos[i].ask;
        soa.bid_sizes[i] = aos[i].bid_size;
        soa.ask_sizes[i] = aos[i].ask_size;
        std::memcpy(soa.symbols[i].data(), aos[i].symbol, 8);
    }
    std::cout << " done." << std::endl << std::endl;

    // ====================================================================
    // Operation 1: 计算平均 bid-ask spread（只访问 bid 和 ask）
    // 这个操作只接触 2 个字段 → SOA 应该更快
    // ====================================================================
    std::cout << "── Operation 1: Average Spread (bid + ask only) ──" << std::endl;
    std::cout << "   SOA-friendly: only touches 2 of 6 fields" << std::endl;
    std::cout << std::endl;

    // AOS - Op1
    std::vector<double> aos_op1_times;
    for (int r = 0; r < RUNS; ++r) {
        Timer t;
        double total_spread = 0;
        for (size_t i = 0; i < N; ++i) {
            total_spread += aos[i].ask - aos[i].bid;
        }
        double elapsed = t.elapsed_ms();
        vol_sink = total_spread / N;
        aos_op1_times.push_back(elapsed);
    }

    // SOA - Op1
    std::vector<double> soa_op1_times;
    for (int r = 0; r < RUNS; ++r) {
        Timer t;
        double total_spread = 0;
        const double* bids = soa.bids.data();
        const double* asks = soa.asks.data();
        for (size_t i = 0; i < N; ++i) {
            total_spread += asks[i] - bids[i];
        }
        double elapsed = t.elapsed_ms();
        vol_sink = total_spread / N;
        soa_op1_times.push_back(elapsed);
    }

    auto aos1 = calc_stats(aos_op1_times);
    auto soa1 = calc_stats(soa_op1_times);

    std::cout << "   AOS: " << std::fixed << std::setprecision(2)
              << aos1.mean << " ms  (stddev " << aos1.stddev << ")" << std::endl;
    std::cout << "   SOA: " << soa1.mean << " ms  (stddev " << soa1.stddev << ")" << std::endl;
    std::cout << "   Ratio: AOS/SOA = " << std::setprecision(1)
              << aos1.mean / soa1.mean << "x" << std::endl;
    std::cout << std::endl;

    std::cout << "   Why? AOS loads " << sizeof(QuoteAOS) << " bytes/quote but uses only 16."
              << std::endl;
    std::cout << "         Cache utilization: AOS="
              << std::setprecision(0) << (16.0 / sizeof(QuoteAOS) * 100) << "%"
              << ", SOA=100%" << std::endl;
    std::cout << std::endl;

    // ====================================================================
    // Operation 2: 找 bid 最高的 quote，然后读取所有字段
    // 这个操作访问所有字段 → AOS 应该持平或更快
    // ====================================================================
    std::cout << "── Operation 2: Find Max Bid + Read All Fields ──" << std::endl;
    std::cout << "   AOS-friendly: touches all fields" << std::endl;
    std::cout << std::endl;

    // AOS - Op2
    std::vector<double> aos_op2_times;
    for (int r = 0; r < RUNS; ++r) {
        Timer t;
        size_t max_idx = 0;
        double max_bid = aos[0].bid;
        for (size_t i = 1; i < N; ++i) {
            if (aos[i].bid > max_bid) {
                max_bid = aos[i].bid;
                max_idx = i;
            }
        }
        // 访问所有字段
        const auto& q = aos[max_idx];
        vol_sink_u = q.timestamp;
        vol_sink = q.bid + q.ask;
        vol_sink_u = q.bid_size + q.ask_size;
        double elapsed = t.elapsed_ms();
        aos_op2_times.push_back(elapsed);
    }

    // SOA - Op2
    std::vector<double> soa_op2_times;
    for (int r = 0; r < RUNS; ++r) {
        Timer t;
        size_t max_idx = 0;
        double max_bid = soa.bids[0];
        for (size_t i = 1; i < N; ++i) {
            if (soa.bids[i] > max_bid) {
                max_bid = soa.bids[i];
                max_idx = i;
            }
        }
        // 访问所有字段（在不同数组中）
        vol_sink_u = soa.timestamps[max_idx];
        vol_sink = soa.bids[max_idx] + soa.asks[max_idx];
        vol_sink_u = soa.bid_sizes[max_idx] + soa.ask_sizes[max_idx];
        double elapsed = t.elapsed_ms();
        soa_op2_times.push_back(elapsed);
    }

    auto aos2 = calc_stats(aos_op2_times);
    auto soa2 = calc_stats(soa_op2_times);

    std::cout << "   AOS: " << std::fixed << std::setprecision(2)
              << aos2.mean << " ms  (stddev " << aos2.stddev << ")" << std::endl;
    std::cout << "   SOA: " << soa2.mean << " ms  (stddev " << soa2.stddev << ")" << std::endl;
    std::cout << "   Ratio: AOS/SOA = " << std::setprecision(2)
              << aos2.mean / soa2.mean << "x" << std::endl;
    std::cout << "   (Close to 1.0 — both mainly scan one field, all-field access is O(1))"
              << std::endl;
    std::cout << std::endl;

    // ====================================================================
    // 总结
    // ====================================================================
    std::cout << "============================================================" << std::endl;
    std::cout << " Summary:" << std::endl;
    std::cout << " Op1 (few fields): SOA wins by ~"
              << std::setprecision(1) << aos1.mean / soa1.mean << "x" << std::endl;
    std::cout << " Op2 (all fields): Roughly equal" << std::endl;
    std::cout << std::endl;
    std::cout << " Practical advice for HFT:" << std::endl;
    std::cout << " - If hot loop touches few fields -> SOA or Hot/Cold split" << std::endl;
    std::cout << " - If hot loop touches all fields -> AOS is fine" << std::endl;
    std::cout << " - Most real systems use Hot/Cold split as compromise" << std::endl;
    std::cout << "============================================================" << std::endl;

    return 0;
}
