
import { Member, SyllabusItem, Resource, STLComponent } from './types';

export const MEMBERS: Member[] = [
  {
    name: "Hao Luo (TA)",
    background: "Professional (Jump Trading SWE)",
    techStack: "Expert C++",
    motivation: "Technical guidance, code review, and HFT industry standards"
  },
  {
    name: "Junshi Wang (Leader)",
    background: "PhD (CS)",
    techStack: "Intermediate C++",
    motivation: "Group leadership, performance optimization and distributed systems"
  },
  {
    name: "Zhijie (Hugo) Gao",
    background: "Professional (Quant Dev)",
    techStack: "Advanced C++",
    motivation: "Modern C++ practices and low-latency system design"
  },
  {
    name: "Estelle (Zhouchen) Xu",
    background: "Professional (Quant Analyst)",
    techStack: "Advanced C++",
    motivation: "Deep dive into STL internals and memory management"
  },
  {
    name: "Qiang Liu",
    background: "Professional (Senior SWE)",
    techStack: "Advanced C++",
    motivation: "Rebuilding C++ mastery for finance career pivot"
  },
  {
    name: "Flynn Yuying Guo",
    background: "PhD (Physics)",
    techStack: "Beginner C++",
    motivation: "Migrating HPC/GPU research experience to quant systems"
  },
  {
    name: "Zaichen Hao",
    background: "Undergraduate (CS)",
    techStack: "Beginner C++",
    motivation: "Understanding core HFT mechanisms and industrial engineering"
  },
  {
    name: "Zhuo Zhao",
    background: "PhD (Math Finance)",
    techStack: "Intermediate C++",
    motivation: "Performance-driven programming and container optimization"
  },
  {
    name: "Han Lewei (David)",
    background: "Undergraduate (CS)",
    techStack: "Beginner C++",
    motivation: "Applying C++ fundamentals to quantitative finance"
  }
];

export const SYLLABUS: SyllabusItem[] = [
  {
    week: 1,
    title: "C++ Low-latency Overview",
    speaker: "Junshi Wang",
    description: "Performance goals and why C++ is the king of low latency.",
    details: ["Common designs: Streaming, Trading, AI Infra", "Best practices: What to use/avoid", "Compiler optimizations (GCC flags)"]
  },
  {
    week: 2,
    title: "Core Building Blocks",
    speaker: "Flynn Guo",
    description: "Threads and concurrency model.",
    details: ["Memory pool design", "Lock-free queue", "Low-latency logging", "TCP/UDP utilities"]
  },
  {
    week: 3,
    title: "Trading Ecosystem Design",
    speaker: "Zhijie Gao",
    description: "Architecture of an exchange and strategy engine.",
    details: ["System constraints", "Exchange-Client architecture", "Latency budgets"]
  },
  {
    week: 4,
    title: "Matching Engine",
    speaker: "Zhouchen Xu",
    description: "Limit Order Book (LOB) and order matching.",
    details: ["Order book design", "Matching algorithm implementation", "O(1) operations"]
  },
  {
    week: 5,
    title: "Exchange Connectivity",
    speaker: "David Han",
    description: "Market data and order management.",
    details: ["Market data publisher", "Order server", "Assembling the binary"]
  },
  {
    week: 6,
    title: "Client Connectivity",
    speaker: "Qiang Liu",
    description: "Consuming market data and placing orders.",
    details: ["Market data consumer", "Client-side LOB", "Order gateway"]
  },
  {
    week: 7,
    title: "Strategy Framework",
    speaker: "Zhuo Zhao",
    description: "Generating signals and managing risk.",
    details: ["PnL tracking", "Signal generation", "Risk controls", "Market making logic"]
  },
  {
    week: 8,
    title: "Instrumentation & Optimization",
    speaker: "Zaichen Hao",
    description: "Measuring and tuning performance.",
    details: ["Timestamping (High-granularity)", "Benchmarking", "System tuning"]
  }
];

export const STL_COMPONENTS: STLComponent[] = [
  {
    name: "vector",
    description: "内存连续、支持随机访问的动态增长数组，是 C++ 中首选的通用序列容器。",
    applications: {
      ai: "存储神经网络权重或特征向量。",
      crypto: "存储交易池中的原始交易字节流。",
      hft: "存储行情快照，利用缓存友好性加速回测。"
    }
  },
  {
    name: "unordered_map",
    description: "基于哈希表实现，提供平均 O(1) 的键值查找。",
    applications: {
      ai: "词嵌入中单词到 ID 的映射。",
      crypto: "快速查询地址余额或交易是否存在。",
      hft: "维护证券代码到订单簿对象的内存索引。"
    }
  },
  {
    name: "deque",
    description: "分段连续存储，支持首尾 O(1) 插入/删除。",
    applications: {
      ai: "深度强化学习中的经验回放池。",
      crypto: "存储近期区块头用于轻客户端同步。",
      hft: "实现滑动窗口均值，快速更新行情窗口。"
    }
  },
  {
    name: "priority_queue",
    description: "基于堆实现，自动维护最大/最小元素在顶部，支持高效优先级调度。",
    applications: {
      ai: "A* 算法中管理待探索节点。",
      crypto: "按手续费率排序待打包交易。",
      hft: "事件引擎中按时间戳调度任务。"
    }
  },
  {
    name: "list",
    description: "双向链表，任意位置 O(1) 插入/删除，但不支持随机访问。",
    applications: {
      ai: "动态构建计算图节点。",
      crypto: "P2P 网络中管理活跃节点列表。",
      hft: "结合哈希表实现 LRU 缓存，管理高频变动的订单项。"
    }
  },
  {
    name: "map",
    description: "基于红黑树实现的有序关联容器，支持按键 O(log n) 查找、插入、删除及范围遍历 。",
    applications: {
      ai: "存储按时间戳排序的日志事件流。",
      crypto: "维护按 nonce 有序的账户交易队列。",
      hft: "实现限价订单簿（LOB）——按价格档位升序/降序遍历买卖盘口。"
    }
  },
  {
    name: "Iterator",
    description: "提供统一接口遍历容器元素，支持算法与数据结构解耦；在低延迟场景中可避免中间拷贝和临时对象 。",
    applications: {
      ai: "使用 transform + 迭代器直接处理特征向量流，避免生成中间 buffer。",
      crypto: "遍历交易输入/输出时用 const_iterator 零拷贝解析。",
      hft: "用 span<T>::iterator 或自定义迭代器高效扫描行情缓冲区，消除冗余复制。"
    }
  }
];

export const RESOURCES: Resource[] = [
  {
    title: "CppCon: When a Microsecond Is an Eternity",
    url: "https://www.youtube.com/watch?v=NH1Tta7purM",
    description: "Carl Cook (Optiver) explaining HFT systems in C++.",
    level: "Video"
  },
  {
    title: "The Cherno C++ Series",
    url: "https://www.youtube.com/playlist?list=PLlrATfBNZ98dudnM48yfGUldqGD0S4FFb",
    description: "Legendary visual guide to C++ internals.",
    level: "Video"
  },
  {
    title: "cppreference.com",
    url: "https://zh.cppreference.com/",
    description: "The most authoritative online reference for ISO C++.",
    level: "Reference"
  },
  {
    title: "Effective C++ (3rd Ed)",
    description: "By Scott Meyers. Crucial for intermediate patterns.",
    level: "Intermediate"
  },
  {
    title: "C++ Primer (5th Ed)",
    description: "Comprehensive introduction for beginners.",
    level: "Beginner"
  },
  {
    title: "Advanced C++ & Modern Design",
    url: "https://quantnet.com/advancedcpp/",
    description: "Quantnet/Baruch specialized course.",
    level: "Advanced"
  }
];
