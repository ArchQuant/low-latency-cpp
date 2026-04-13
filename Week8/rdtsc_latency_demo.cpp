/*
 * RDTSC Latency Measurement Demo
 *
 * Compile:
 *   g++ -O2 -std=c++17 -o rdtsc_demo rdtsc_latency_demo.cpp
 *
 * Run:
 *   ./rdtsc_demo
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

// ============================================================================
// SECTION 1: TIME MEASUREMENT UTILITIES
// ============================================================================

namespace Common {

/*
 * Read CPU Time Stamp Counter (TSC) with light serialization.
 * x86_64 only.
 */
inline uint64_t rdtsc() noexcept {
  unsigned int lo = 0;
  unsigned int hi = 0;
  __asm__ __volatile__("lfence\n\t"
                       "rdtsc\n\t"
                       : "=a"(lo), "=d"(hi)
                       :
                       : "memory");
  return (static_cast<uint64_t>(hi) << 32) | lo;
}

/*
 * Monotonic nanosecond timestamp.
 */
inline uint64_t getCurrentNanos() noexcept {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

/*
 * Human-readable wall clock timestamp for logs.
 */
inline std::string getCurrentTimeStr() {
  const auto now = std::chrono::system_clock::now();
  const auto time_t_now = std::chrono::system_clock::to_time_t(now);
  const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         now.time_since_epoch())
                         .count() %
                     1000000000LL;

  char buffer[32];
  std::tm tm_now{};
  localtime_r(&time_t_now, &tm_now);

  std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%09lld", tm_now.tm_hour,
                tm_now.tm_min, tm_now.tm_sec, static_cast<long long>(nanos));

  return std::string(buffer);
}

} // namespace Common

// ============================================================================
// SECTION 2: SIMPLE LOGGER CLASS
// ============================================================================

class Logger {
public:
  struct LogEntry {
    std::string timestamp;
    std::string tag;
    uint64_t value;
  };

  void logRDTSC(const std::string &tag, uint64_t cycles) {
    entries_.push_back({Common::getCurrentTimeStr(), tag, cycles});
    if (verbose_) {
      const auto &e = entries_.back();
      std::cout << e.timestamp << " RDTSC " << e.tag << " " << e.value << '\n';
    }
  }

  void logTTT(const std::string &tag, uint64_t nanos) {
    if (verbose_) {
      std::cout << Common::getCurrentTimeStr() << " TTT " << tag << " " << nanos
                << '\n';
    }
  }

  const std::vector<LogEntry> &getEntries() const { return entries_; }
  void clear() { entries_.clear(); }
  void setVerbose(bool v) { verbose_ = v; }

private:
  std::vector<LogEntry> entries_;
  bool verbose_ = false;
};

// Global logger instance
Logger g_logger;

// ============================================================================
// SECTION 3: MEASUREMENT MACROS
// ============================================================================

#define START_MEASURE(TAG) const uint64_t TAG = Common::rdtsc()

#define END_MEASURE(TAG, LOGGER)                                               \
  do {                                                                         \
    const uint64_t end_##TAG = Common::rdtsc();                                \
    (LOGGER).logRDTSC(#TAG, end_##TAG - TAG);                                  \
  } while (false)

#define TTT_MEASURE(TAG, LOGGER)                                               \
  do {                                                                         \
    const uint64_t ts_##TAG = Common::getCurrentNanos();                       \
    (LOGGER).logTTT(#TAG, ts_##TAG);                                           \
  } while (false)

// ============================================================================
// SECTION 4: SIMULATED EXCHANGE COMPONENTS
// ============================================================================

enum class Side { BUY, SELL };

struct Order {
  uint64_t order_id;
  uint64_t client_id;
  Side side;
  double price;
  uint32_t quantity;

  Order(uint64_t oid, uint64_t cid, Side s, double p, uint32_t q)
      : order_id(oid), client_id(cid), side(s), price(p), quantity(q) {}
};

/*
 * MEOrderBook - simulated matching engine order book
 */
class MEOrderBook {
public:
  void addOrder(const Order &order) {
    if (order.side == Side::BUY) {
      bids_.insert_or_assign(order.order_id, order);
    } else {
      asks_.insert_or_assign(order.order_id, order);
    }
    id_map_.insert_or_assign(order.order_id, order);
  }

  /*
   * Remove order from order book.
   */
  bool removeOrder(uint64_t client_id, uint64_t order_id, Side side) {
    START_MEASURE(Exchange_MEOrderBook_removeOrder);

    auto it = id_map_.find(order_id);
    if (it == id_map_.end()) {
      END_MEASURE(Exchange_MEOrderBook_removeOrder, g_logger);
      return false;
    }

    if (it->second.client_id != client_id || it->second.side != side) {
      END_MEASURE(Exchange_MEOrderBook_removeOrder, g_logger);
      return false;
    }

    if (side == Side::BUY) {
      bids_.erase(order_id);
    } else {
      asks_.erase(order_id);
    }

    id_map_.erase(it);

    END_MEASURE(Exchange_MEOrderBook_removeOrder, g_logger);
    return true;
  }

  size_t getBidCount() const { return bids_.size(); }
  size_t getAskCount() const { return asks_.size(); }

private:
  std::map<uint64_t, Order> bids_;
  std::map<uint64_t, Order> asks_;
  std::map<uint64_t, Order> id_map_;
};

/*
 * FIFOSequencer - simulated FIFO message sequencer
 */
class FIFOSequencer {
public:
  void addRequest(uint64_t client_id, uint64_t order_id) {
    pending_.push({client_id, order_id, 0});
  }

  /*
   * Sequence pending requests and publish to matching engine.
   */
  void sequenceAndPublish() {
    START_MEASURE(Exchange_FIFOSequencer_seqPub);

    std::queue<std::tuple<uint64_t, uint64_t, uint64_t>> sequenced;
    while (!pending_.empty()) {
      auto [cid, oid, unused_seq] = pending_.front();
      (void)unused_seq;
      pending_.pop();
      sequenced.push({cid, oid, next_seq_num_++});
    }

    while (!sequenced.empty()) {
      auto [cid, oid, seq] = sequenced.front();
      sequenced.pop();
      published_.push_back({cid, oid, seq});
    }

    // Simulate periodic network flush (causes spikes)
    if (published_.size() > 100) {
      published_.clear();
    }

    END_MEASURE(Exchange_FIFOSequencer_seqPub, g_logger);
  }

private:
  std::queue<std::tuple<uint64_t, uint64_t, uint64_t>> pending_;
  std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> published_;
  uint64_t next_seq_num_ = 1;
};

// ============================================================================
// SECTION 5: LATENCY ANALYSIS UTILITIES
// ============================================================================

struct LatencyStats {
  uint64_t min = 0;
  uint64_t max = 0;
  double mean = 0.0;
  double median = 0.0;
  uint64_t p50 = 0;
  uint64_t p95 = 0;
  uint64_t p99 = 0;
  size_t count = 0;
};

static size_t percentileIndex(size_t n, double p) {
  if (n == 0) {
    return 0;
  }
  size_t idx = static_cast<size_t>(p * static_cast<double>(n - 1));
  return std::min(idx, n - 1);
}

LatencyStats computeStats(const std::vector<uint64_t> &latencies) {
  if (latencies.empty()) {
    return {};
  }

  std::vector<uint64_t> sorted = latencies;
  std::sort(sorted.begin(), sorted.end());

  const uint64_t sum =
      std::accumulate(sorted.begin(), sorted.end(), uint64_t{0});

  LatencyStats stats;
  stats.count = sorted.size();
  stats.min = sorted.front();
  stats.max = sorted.back();
  stats.mean = static_cast<double>(sum) / static_cast<double>(stats.count);

  const size_t mid = stats.count / 2;
  if (stats.count % 2 == 0) {
    stats.median = (static_cast<double>(sorted[mid - 1]) +
                    static_cast<double>(sorted[mid])) /
                   2.0;
  } else {
    stats.median = static_cast<double>(sorted[mid]);
  }

  stats.p50 = sorted[percentileIndex(stats.count, 0.50)];
  stats.p95 = sorted[percentileIndex(stats.count, 0.95)];
  stats.p99 = sorted[percentileIndex(stats.count, 0.99)];

  return stats;
}

void printHistogram(const std::vector<uint64_t> &latencies,
                    const std::string &title) {
  if (latencies.empty()) {
    return;
  }

  std::cout << '\n' << title << " - Latency Distribution Histogram\n";
  std::cout << std::string(70, '=') << '\n';

  const size_t num_bins = 20;
  const uint64_t min_val =
      *std::min_element(latencies.begin(), latencies.end());
  const uint64_t max_val =
      *std::max_element(latencies.begin(), latencies.end());

  if (min_val == max_val) {
    std::cout << std::setw(8) << min_val << "-" << std::setw(8) << max_val
              << " | " << std::string(50, '#') << " " << latencies.size()
              << '\n';
    std::cout << std::string(70, '=') << '\n';
    return;
  }

  const double range = static_cast<double>(max_val - min_val);
  const double bin_width = range / static_cast<double>(num_bins);

  std::vector<size_t> bins(num_bins, 0);
  for (uint64_t val : latencies) {
    size_t bin =
        static_cast<size_t>(static_cast<double>(val - min_val) / bin_width);
    if (bin >= num_bins) {
      bin = num_bins - 1;
    }
    ++bins[bin];
  }

  const size_t max_count = *std::max_element(bins.begin(), bins.end());
  const size_t bar_width = 50;

  for (size_t i = 0; i < num_bins; ++i) {
    const uint64_t range_start =
        min_val + static_cast<uint64_t>(static_cast<double>(i) * bin_width);
    const uint64_t range_end =
        (i == num_bins - 1)
            ? max_val
            : min_val +
                  static_cast<uint64_t>(static_cast<double>(i + 1) * bin_width);

    const size_t bar_len =
        (max_count == 0) ? 0 : (bins[i] * bar_width) / max_count;

    std::cout << std::setw(8) << range_start << "-" << std::setw(8) << range_end
              << " |" << std::string(bar_len, '#') << " " << bins[i] << '\n';
  }

  std::cout << std::string(70, '=') << '\n';
}

void printStats(const LatencyStats &stats, const std::string &component,
                double cpu_freq_ghz = 2.5) {
  // cpu_freq_ghz is numerically cycles/ns
  const double cycles_to_ns = 1.0 / cpu_freq_ghz;
  const double cycles_to_us = cycles_to_ns / 1000.0;

  std::cout << '\n' << component << " - Performance Statistics\n";
  std::cout << std::string(70, '-') << '\n';
  std::cout << "  Samples:     " << stats.count << '\n';
  std::cout << "  Min:         " << stats.min << " cycles  (~" << std::fixed
            << std::setprecision(2)
            << (static_cast<double>(stats.min) * cycles_to_us) << " us)\n";
  std::cout << "  Max:         " << stats.max << " cycles  (~"
            << (static_cast<double>(stats.max) * cycles_to_us) << " us)\n";
  std::cout << "  Mean:        " << static_cast<uint64_t>(stats.mean)
            << " cycles  (~" << (stats.mean * cycles_to_us) << " us)\n";
  std::cout << "  Median:      " << static_cast<uint64_t>(stats.median)
            << " cycles  (~" << (stats.median * cycles_to_us) << " us)\n";
  std::cout << "  P95:         " << stats.p95 << " cycles  (~"
            << (static_cast<double>(stats.p95) * cycles_to_us) << " us)\n";
  std::cout << "  P99:         " << stats.p99 << " cycles  (~"
            << (static_cast<double>(stats.p99) * cycles_to_us) << " us)\n";
  std::cout << std::string(70, '-') << '\n';
}

// ============================================================================
// SECTION 6: MAIN DEMONSTRATION
// ============================================================================

int main() {
  std::cout
      << "==============================================================\n";
  std::cout << "  RDTSC Latency Measurement Demo\n";
  std::cout << "  Demonstrating instrumentation concepts\n";
  std::cout
      << "==============================================================\n\n";

  // Estimate CPU frequency
  std::cout << "Estimating CPU frequency...\n";
  const auto start_time = std::chrono::steady_clock::now();
  const uint64_t start_tsc = Common::rdtsc();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  const uint64_t end_tsc = Common::rdtsc();
  const auto end_time = std::chrono::steady_clock::now();

  const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              end_time - start_time)
                              .count();
  const uint64_t elapsed_cycles = end_tsc - start_tsc;
  const double cpu_freq_ghz =
      static_cast<double>(elapsed_cycles) / static_cast<double>(elapsed_ns);

  std::cout << "Estimated CPU frequency: " << std::fixed << std::setprecision(2)
            << cpu_freq_ghz << " GHz\n\n";

  // ========================================================================
  // Demo 1: MEOrderBook::removeOrder() - tighter distribution
  // ========================================================================

  std::cout << std::string(70, '=') << '\n';
  std::cout << "DEMO 1: MEOrderBook::removeOrder()\n";
  std::cout << std::string(70, '=') << "\n\n";

  g_logger.clear();
  g_logger.setVerbose(true);

  MEOrderBook order_book;

  // Pre-populate order book
  for (uint64_t i = 0; i < 1000; ++i) {
    order_book.addOrder(Order(i, i % 100, (i % 2 == 0) ? Side::BUY : Side::SELL,
                              100.0 + static_cast<double>(i % 100), 100));
  }

  std::cout << "Running 200 removeOrder() measurements...\n\n";

  std::vector<uint64_t> remove_latencies;
  for (size_t i = 0; i < 200; ++i) {
    if (i == 5) {
      g_logger.setVerbose(false);
    }

    const uint64_t oid = static_cast<uint64_t>(i);
    const uint64_t cid = oid % 100;
    const Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;

    (void)order_book.removeOrder(cid, oid, side);
  }

  for (const auto &entry : g_logger.getEntries()) {
    if (entry.tag == "Exchange_MEOrderBook_removeOrder") {
      remove_latencies.push_back(entry.value);
    }
  }

  const auto remove_stats = computeStats(remove_latencies);
  printStats(remove_stats, "MEOrderBook::removeOrder()", cpu_freq_ghz);
  printHistogram(remove_latencies, "MEOrderBook::removeOrder()");

  std::cout << "\nAnalysis:\n";
  std::cout
      << "  - This path should usually look tighter and more predictable.\n";
  std::cout << "  - Tail behavior still depends on cache and tree state.\n\n";

  // ========================================================================
  // Demo 2: FIFOSequencer::sequenceAndPublish() - spikier distribution
  // ========================================================================

  std::cout << std::string(70, '=') << '\n';
  std::cout << "DEMO 2: FIFOSequencer::sequenceAndPublish()\n";
  std::cout << std::string(70, '=') << "\n\n";

  g_logger.clear();
  g_logger.setVerbose(true);

  FIFOSequencer sequencer;

  std::cout << "Running 200 sequenceAndPublish() measurements...\n\n";

  std::vector<uint64_t> seq_latencies;
  for (size_t i = 0; i < 200; ++i) {
    if (i == 5) {
      g_logger.setVerbose(false);
    }

    // Add varying number of requests to create periodic bursts
    const size_t num_requests = (i % 10 == 0) ? 50 : 5;
    for (size_t j = 0; j < num_requests; ++j) {
      sequencer.addRequest(static_cast<uint64_t>(j),
                           static_cast<uint64_t>(i * 100 + j));
    }

    sequencer.sequenceAndPublish();
  }

  for (const auto &entry : g_logger.getEntries()) {
    if (entry.tag == "Exchange_FIFOSequencer_seqPub") {
      seq_latencies.push_back(entry.value);
    }
  }

  const auto seq_stats = computeStats(seq_latencies);
  printStats(seq_stats, "FIFOSequencer::sequenceAndPublish()", cpu_freq_ghz);
  printHistogram(seq_latencies, "FIFOSequencer::sequenceAndPublish()");

  std::cout << "\nAnalysis:\n";
  std::cout
      << "  - This path should usually show wider spread and fatter tails.\n";
  std::cout << "  - Batch/flush behavior tends to create spikes.\n\n";

  // ========================================================================
  // Summary
  // ========================================================================

  std::cout << std::string(70, '=') << '\n';
  std::cout << "SUMMARY - Comparing Both Components\n";
  std::cout << std::string(70, '=') << "\n\n";

  std::cout << std::left << std::setw(34) << "Component"
            << "| " << std::right << std::setw(13) << "Mean (cycles)"
            << " | " << std::setw(12) << "P99 (cycles)"
            << " | Pattern\n";
  std::cout << std::string(70, '-') << '\n';

  std::cout << std::left << std::setw(34) << "MEOrderBook::removeOrder()"
            << "| " << std::right << std::setw(13)
            << static_cast<uint64_t>(remove_stats.mean) << " | "
            << std::setw(12) << remove_stats.p99 << " | Tight\n";

  std::cout << std::left << std::setw(34) << "FIFOSequencer::seqPub()"
            << "| " << std::right << std::setw(13)
            << static_cast<uint64_t>(seq_stats.mean) << " | " << std::setw(12)
            << seq_stats.p99 << " | Spiky\n";

  std::cout << std::string(70, '-') << "\n\n";

  std::cout << "Key Takeaways:\n";
  std::cout << "  1. RDTSC provides very fine-grained measurement.\n";
  std::cout
      << "  2. Tight distributions suggest more predictable code paths.\n";
  std::cout
      << "  3. Spiky distributions highlight bursty or tail-heavy behavior.\n";
  std::cout << "  4. Measure before optimizing.\n\n";

  std::cout << "Demo complete.\n";
  std::cout
      << "Compile: g++ -O2 -std=c++17 -o rdtsc_demo rdtsc_latency_demo.cpp\n";

  return 0;
}
