// ============================================================================
// Exercise 4: Pre-allocated Order Buffer
// ============================================================================
// 实现一个 OrderBuffer 类：
//   - 构造时一次性分配内存
//   - 之后的 add_order() 和 clear_for_next_tick() 零分配
//   - 模拟 10,000 个 tick，每个 tick 1~100 笔订单
//   - 验证：构造之后再无任何内存分配
// ============================================================================

#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <cassert>
#include <cstring>

// ── Order 结构体 ──
struct Order {
    uint64_t order_id;
    uint64_t timestamp;
    double   price;
    uint32_t quantity;
    char     symbol[8];
    bool     is_buy;

    // padding 使其凑齐 48 bytes
};

// ============================================================================
// OrderBuffer: 预分配的订单缓冲区
// ============================================================================
class OrderBuffer {
public:
    // 构造时一次性分配
    explicit OrderBuffer(size_t max_orders)
        : max_capacity_(max_orders)
    {
        orders_.reserve(max_orders);
        std::cout << "  [OrderBuffer] Constructed: reserved " << max_orders
                  << " orders (" << max_orders * sizeof(Order)
                  << " bytes)" << std::endl;
    }

    // 添加订单 —— 零分配（前提是不超过 max_capacity）
    bool add_order(const Order& order) {
        if (orders_.size() >= max_capacity_) {
            return false;  // 满了，拒绝（不触发 reallocation）
        }
        orders_.push_back(order);
        return true;
    }

    // emplace 版本 —— 直接在内存中构造，避免拷贝
    template<typename... Args>
    bool emplace_order(Args&&... args) {
        if (orders_.size() >= max_capacity_) {
            return false;
        }
        orders_.emplace_back(std::forward<Args>(args)...);
        return true;
    }

    // 清空当前 tick 的数据，但保留已分配的内存
    void clear_for_next_tick() {
        orders_.clear();
        // clear() 只重置 size 为 0，capacity 不变
        // 这意味着下一个 tick 可以直接复用这块内存
    }

    // ── 访问器 ──
    size_t size() const { return orders_.size(); }
    size_t capacity() const { return orders_.capacity(); }
    const Order& operator[](size_t i) const { return orders_[i]; }
    const std::vector<Order>& orders() const { return orders_; }

    // 检查是否还有剩余空间
    bool has_space() const { return orders_.size() < max_capacity_; }

private:
    std::vector<Order> orders_;
    size_t max_capacity_;
};

// ============================================================================
// 自定义 allocator 用于追踪分配次数（教学用）
// ============================================================================
static size_t g_alloc_count = 0;
static size_t g_alloc_bytes = 0;

template<typename T>
struct TrackingAllocator {
    using value_type = T;

    TrackingAllocator() = default;
    template<typename U>
    TrackingAllocator(const TrackingAllocator<U>&) noexcept {}

    T* allocate(size_t n) {
        g_alloc_count++;
        g_alloc_bytes += n * sizeof(T);
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, size_t n) noexcept {
        ::operator delete(p);
    }
};

template<typename T, typename U>
bool operator==(const TrackingAllocator<T>&, const TrackingAllocator<U>&) { return true; }
template<typename T, typename U>
bool operator!=(const TrackingAllocator<T>&, const TrackingAllocator<U>&) { return false; }

// 带追踪功能的 OrderBuffer
class TrackedOrderBuffer {
public:
    explicit TrackedOrderBuffer(size_t max_orders) : max_capacity_(max_orders) {
        orders_.reserve(max_orders);
    }

    bool add_order(const Order& order) {
        if (orders_.size() >= max_capacity_) return false;
        orders_.push_back(order);
        return true;
    }

    void clear_for_next_tick() { orders_.clear(); }
    size_t size() const { return orders_.size(); }
    size_t capacity() const { return orders_.capacity(); }

private:
    std::vector<Order, TrackingAllocator<Order>> orders_;
    size_t max_capacity_;
};

// ── 计时器 ──
struct Timer {
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point start;
    Timer() : start(Clock::now()) {}
    double elapsed_us() const {
        return std::chrono::duration<double, std::micro>(Clock::now() - start).count();
    }
};

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << " Exercise 4: Pre-allocated Order Buffer" << std::endl;
    std::cout << " sizeof(Order) = " << sizeof(Order) << " bytes" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << std::endl;

    // ── Part A: 基本功能演示 ──
    std::cout << "── Part A: Basic Functionality ──" << std::endl;
    {
        OrderBuffer buffer(1000);

        // 模拟添加几笔订单
        for (int i = 0; i < 5; ++i) {
            Order o{};
            o.order_id = i + 1;
            o.price = 100.0 + i * 0.25;
            o.quantity = (i + 1) * 100;
            o.is_buy = (i % 2 == 0);
            std::memcpy(o.symbol, "AAPL    ", 8);
            buffer.add_order(o);
        }

        std::cout << "  After adding 5 orders:" << std::endl;
        std::cout << "    size     = " << buffer.size() << std::endl;
        std::cout << "    capacity = " << buffer.capacity() << std::endl;

        buffer.clear_for_next_tick();

        std::cout << "  After clear_for_next_tick():" << std::endl;
        std::cout << "    size     = " << buffer.size() << " (reset!)" << std::endl;
        std::cout << "    capacity = " << buffer.capacity() << " (unchanged!)" << std::endl;
    }
    std::cout << std::endl;

    // ── Part B: 零分配验证 ──
    std::cout << "── Part B: Zero-Allocation Verification ──" << std::endl;
    {
        constexpr size_t MAX_PER_TICK = 100;
        constexpr size_t NUM_TICKS = 10'000;

        std::mt19937 rng(42);
        std::uniform_int_distribution<size_t> count_dist(1, MAX_PER_TICK);

        // 重置追踪器
        g_alloc_count = 0;
        g_alloc_bytes = 0;

        // 构造（这里会产生 1 次分配）
        TrackedOrderBuffer buffer(MAX_PER_TICK);
        size_t allocs_after_init = g_alloc_count;
        size_t bytes_after_init = g_alloc_bytes;

        std::cout << "  Construction phase:" << std::endl;
        std::cout << "    Allocations: " << allocs_after_init << std::endl;
        std::cout << "    Bytes: " << bytes_after_init << std::endl;
        std::cout << std::endl;

        // 重置，开始追踪热路径上的分配
        g_alloc_count = 0;
        g_alloc_bytes = 0;

        // 模拟交易日
        size_t total_orders = 0;
        Timer timer;

        for (size_t tick = 0; tick < NUM_TICKS; ++tick) {
            size_t n_orders = count_dist(rng);
            for (size_t i = 0; i < n_orders; ++i) {
                Order o{};
                o.order_id = total_orders++;
                o.price = 100.0 + (rng() % 1000) * 0.01;
                o.quantity = 100 + rng() % 9900;
                o.is_buy = rng() % 2;
                buffer.add_order(o);
            }
            buffer.clear_for_next_tick();
        }

        double elapsed = timer.elapsed_us();

        std::cout << "  Hot path (" << NUM_TICKS << " ticks, "
                  << total_orders << " total orders):" << std::endl;
        std::cout << "    Allocations: " << g_alloc_count;
        if (g_alloc_count == 0) {
            std::cout << " ✓ ZERO allocations!";
        } else {
            std::cout << " ✗ UNEXPECTED allocations!";
        }
        std::cout << std::endl;
        std::cout << "    Extra bytes: " << g_alloc_bytes << std::endl;
        std::cout << "    Total time: " << std::fixed << std::setprecision(1)
                  << elapsed / 1000.0 << " ms" << std::endl;
        std::cout << "    Per tick: " << std::setprecision(2)
                  << elapsed / NUM_TICKS << " us" << std::endl;
        std::cout << "    Per order: " << std::setprecision(0)
                  << elapsed / total_orders * 1000.0 << " ns" << std::endl;
    }
    std::cout << std::endl;

    // ── Part C: 对比不预分配的版本 ──
    std::cout << "── Part C: Comparison — Without Pre-allocation ──" << std::endl;
    {
        constexpr size_t NUM_TICKS = 10'000;
        constexpr size_t MAX_PER_TICK = 100;

        std::mt19937 rng(42);  // 同样的种子，保证公平
        std::uniform_int_distribution<size_t> count_dist(1, MAX_PER_TICK);

        g_alloc_count = 0;
        g_alloc_bytes = 0;

        size_t total_orders = 0;
        Timer timer;

        for (size_t tick = 0; tick < NUM_TICKS; ++tick) {
            // 每个 tick 创建一个新的 vector（没有预分配！）
            std::vector<Order, TrackingAllocator<Order>> orders;
            size_t n_orders = count_dist(rng);
            for (size_t i = 0; i < n_orders; ++i) {
                Order o{};
                o.order_id = total_orders++;
                o.price = 100.0 + (rng() % 1000) * 0.01;
                o.quantity = 100 + rng() % 9900;
                o.is_buy = rng() % 2;
                orders.push_back(o);
            }
            // vector 在作用域结束时被销毁
        }

        double elapsed = timer.elapsed_us();

        std::cout << "  Without pre-allocation (" << NUM_TICKS << " ticks):" << std::endl;
        std::cout << "    Allocations: " << g_alloc_count << " (!)" << std::endl;
        std::cout << "    Bytes allocated: " << g_alloc_bytes << std::endl;
        std::cout << "    Total time: " << std::fixed << std::setprecision(1)
                  << elapsed / 1000.0 << " ms" << std::endl;
        std::cout << "    Per tick: " << std::setprecision(2)
                  << elapsed / NUM_TICKS << " us" << std::endl;
        std::cout << "    Per order: " << std::setprecision(0)
                  << elapsed / total_orders * 1000.0 << " ns" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << " Key Takeaway:" << std::endl;
    std::cout << " - reserve() at construction → 0 allocs on hot path" << std::endl;
    std::cout << " - clear() resets size but keeps capacity for reuse" << std::endl;
    std::cout << " - Creating new vectors each tick = thousands of allocs" << std::endl;
    std::cout << " - This pattern is fundamental in HFT order management" << std::endl;
    std::cout << "============================================================" << std::endl;

    return 0;
}
