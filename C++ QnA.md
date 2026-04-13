# Week 1
### Q1 如何 trace 系统的 lag

一般就用 MacOS 的系统自带的 debugger

补充资料 
- https://mwhittaker.github.io/papers/html/sigelman2010dapper.html
- https://github.com/janestreet/magic-trace

# Week 2

### Q1 epoll 如何找到上万连接中 active 的部分

可以参考 [详解 epoll 原理【Redis，Netty，Nginx实现高性能IO的核心原理】](https://github.com/0voice/cpp_backend_awsome_blog/blob/main/%E3%80%90NO.104%E3%80%91%E8%AF%A6%E8%A7%A3%20epoll%20%E5%8E%9F%E7%90%86%E3%80%90Redis%EF%BC%8CNetty%EF%BC%8CNginx%E5%AE%9E%E7%8E%B0%E9%AB%98%E6%80%A7%E8%83%BDIO%E7%9A%84%E6%A0%B8%E5%BF%83%E5%8E%9F%E7%90%86%E3%80%91.md)，大概意思是只有 active 的 node 会 callback，然后储存在红黑树里

这本质上是个event driving mechanism。 epoll 创建的时候会给每一个socket 在kernel那里注册一个callback，而这个callback就是执行当有新事件进来的时候（比如新数据， 或者连接断开等等），kernel会自动把相应数据copy到epoll可读的memory（比如这里提到的链表）。所以epoll只需要处理它关心新事件，其他的比如不active的socket不会产生新的事件 则会被skip掉（这里inactive不是指disconnection，而是不产生新事件）

### Q2 套圈的解释，lock free queue 写入速度会不会快于读取速度?

写的时候会atomically check queue是不是full的。lock free应该是没有mutex，但会需要使用atomic variable

## Week 3
### Q1 每个exchange是否只维护自己的orderbook? user如何获得NBBO
每个exchange之间只建立并维护自己独立的orderbook,如果过user希望获得NBBO, 有几种渠道：
1） broker必须给customer提供最优价格
2）Security Information Processors (SIPs) play a role in calculating and disseminating NBBO values as part of the National Market System Plan

### Q2 exchange在维护orderbook时，如何降低latency
1) 没有具体的data strcuture,通常每个exchange不太一样，但一般来说会使用std::array去减少cache miss
2）memory pool：用内存池在启动时预分配所有 Order 节点，彻底消除 new/delete 的堆锁和碎片。加上 alignas(64) 和热字段前置，保证一次 cache line 加载就能读完撮合所需的全部字段。
3）线程间通过无锁队列传递数据，撮合引擎、行情发布、报单网关三个线程完全解耦并行。

## Week 4

### Q1: Array vs Linked List — 什么时候用哪个？
 
#### Array（std::array）适合的场景
 
- Lookup 是主要操作时
  - `OrdersAtPriceHashMap`: 给定 price，O(1) 找到对应的 `MEOrdersAtPrice*`
  - `ClientOrderHashMap`: 给定 clientId + orderId，O(1) 找到 `MEOrder*`
  - 这些 lookup 在每次 add、cancel、match 时都会发生，是 **最高频的操作**
 
- 数据量有上界、capacity 可以提前确定时
  - `ME_MAX_PRICE_LEVELS`、`ME_MAX_ORDER_IDS`、`ME_MAX_NUM_CLIENTS` 都是编译期常量
  - 既然 capacity 已知，就不需要 dynamic resizing，array 是最自然的选择
 
- 当需要 cache locality 时
  - std::array 是一块 contiguous memory，CPU 可以 prefetch
  - 相比 std::unordered_map 的 bucket + chain（node 分散在 heap 各处），array 的 cache behavior 好得多
 
- 插入/删除不频繁，或者主要发生在头部时
  - 比如 `OrdersAtPriceHashMap` 的 "插入" 就是 `array[index] = ptr`，O(1)
  - "删除" 就是 `array[index] = nullptr`，O(1)
  - 这里没有 shift 或 resize 的问题，因为 array 被当作 **direct-index hash map** 使用，不是 sequential container
 
#### Linked List（doubly-linked list）适合的场景
 
- 需要维护 sorted order 并支持 O(1) 删除时
  - Price level list（`MEOrdersAtPrice` 的 prev_entry_ / next_entry_）：按价格从 aggressive 到 passive 排序
  - FIFO order chain（`MEOrder` 的 prev_order_ / next_order_）：按到达时间排序
  - 这两个场景都需要 **ordered traversal**（matching 时从 best price 往差的方向走），array 做不到高效的 ordered traversal with dynamic membership
 
- 当删除是高频操作时
  - cancel() 是高频操作，需要 O(1) 删除任意位置的 order
  - Intrusive doubly-linked list 的删除只需要两次 pointer write：`prev->next = next; next->prev = prev`
  - 如果用 array 做顺序存储，删除中间元素需要 shift，O(n)
 
- 当节点数量动态变化时
  - Active price level 数量随 market 变化，不适合 fixed-size sequential array
  - 但注意：这里用的是 **intrusive** linked list + **memory pool**，不是 std::list，所以没有 heap allocation

### Q2: 为什么 low-latency 场景下不用 mutex？
 
#### 1. 等待时间不确定（Contention Latency）
 
- 当 thread A 持有 mutex、thread B 尝试获取时，B 必须等待 A 释放
- 等多久完全取决于 A 在做什么，这是不可预测的
- 如果 A 正好在处理一个复杂的 order（比如触发了多次 partial fill），B 可能等待几个甚至几十个 microseconds
- 对 exchange 来说，worst-case latency 和 average latency 一样重要
- 一个偶尔很慢的系统，参与者会用脚投票离开
 
#### 2. Context Switching 开销
 
- 当 mutex 被占用时，等待的 thread 通常会被 OS 挂起（sleep）
- 挂起和唤醒涉及 context switch，典型开销是 1–10 microseconds
- Context switch 意味着：
  - 保存当前 thread 的 register state
  - 切换 page table / TLB（如果跨 process）
  - 恢复目标 thread 的 register state
  - 最致命的：cache 和 TLB 可能被污染，唤醒后需要重新加载 working set
- 在 matching engine 这种要求 sub-microsecond 响应的场景，1 次 context switch 可能就超过了整个 operation 的 budget
 
#### 3. System Call 开销
 
- Mutex 的 lock/unlock 在 contention 时需要通过 system call 与 kernel 交互
  - Linux 上通常是 `futex()` system call
  - Uncontended 的 fast path 可以在 user space 完成（只是一个 atomic CAS）
  - 但一旦有 contention，就要进 kernel
- System call 的开销：
  - User space → kernel space 的切换：约 100–1000 nanoseconds
  - 涉及 privilege level change、stack switch、security check
  - 回来后可能 cache 已经被 kernel code 污染了
 
#### 4. Priority Inversion 风险
 
- 如果 low-priority thread 持有 mutex，high-priority thread 必须等它释放
- 在 matching engine 中，如果 logging thread 意外持有了某个 lock，matching thread 就被 block 了
- 这种 priority inversion 在 real-time system 中是经典问题
 
#### 5. 难以推理和调试
 
- Mutex 引入了 non-local reasoning：一个 thread 的性能取决于另一个 thread 的行为
- 死锁、活锁、priority inversion 都是 mutex 带来的额外复杂性
- 在 low-latency 系统中，简单性本身就是性能：代码越简单，行为越可预测

# Week 5

### Q1: order gateway server 会不会修改 client seq number? 有conflict 怎么处理?

textbook 中提及 outgoing 和 incoming seq num。server 发回的 filled 信息只会改 outgoing，incoming 不会变。（cid_next_outgoing_seq_num_ and cid_next_exp_seq_num_ variables）
