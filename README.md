# Qishi 3rd C++ Study Group

## 8-Week Curriculum

1. C++ Low-latency systems overview
    
    Performance goals, why C++ for low latency, common designs across streaming, gaming, IoT, electronic trading, AI Infra, Crypto;
    
    C++ best practices for low latency, what to use/avoid, compiler optimizations (GCC flags).
    
2. Core building blocks
    
    Threads and concurrency model, memory pool design, lock-free queue, low-latency logging, TCP/UDP utilities.
    
3. Trading ecosystem design
    
    Trading system requirements and constraints; overall architecture (exchange, clients, strategy engine).
    
4. Matching engine
    
    Limit order book design and order matching implementation.
    
5. Exchange connectivity
    
    Market data publisher, order server, and assembling the exchange binary.
    
6. Client connectivity
    
    Market data consumer, client-side order book, and order gateway.
    
7. Strategy framework and Trading strategies
    
    Positions and PnL tracking, signal generation, order management, and risk controls. Market making and liquidity taking strategies; integrate full client and run end-to-end.
    
8. Instrumentation and optimization
    
    Timestamping and high-granularity performance measurement; analyze and visualize metrics, apply tuning, benchmark gains, and propose extensions.

## Presentation Guidelines

Weekly sessions run 1.5–2 hours with two parts:

Part 1: Theoretical Foundation  
Review book content, address concepts from previous weeks, and introduce new knowledge points.

Part 2: Code & Demos  
Analyze GitHub code, share personal projects, and deep dive into STL underlying mechanisms.

Slides must be in English and uploaded to OneDrive 3–7 days before each session. Presentations may be delivered in Chinese or English. Content should blend book material, GitHub code analysis, and STL data structure internals.

The TA provides syllabus design and complex Q&A support, with weekly check-ins on code review and learning plans. Participants may reach out via WeChat for unresolved technical blockers.

There are no mandatory problem sets. Instead, we practice C++ Enrichment: participants share code snippets or deep-dive into internal data structure logic during presentations. Code is the homework.

## Learning Resources

**Video Resources**  
[CppCon: "When a Microsecond Is an Eternity" – Carl Cook (Optiver) explaining HFT systems in C++](https://www.youtube.com/watch?v=NH1Tta7purM)

[The Cherno C++ Series – Visual guide to C++ internals](https://www.youtube.com/playlist?list=PLlrATfBNZ98dudnM48yfGUldqGD0S4FFb)

**Reference**  
[cppreference.com – Authoritative online reference for ISO C++](https://zh.cppreference.com/)

[C++ STL Template](https://www.amazon.com/C-Standard-Template-Library/dp/0134376331)

[Bjarne Stroustrup on C++](https://www.stroustrup.com/)

**Books**  
Effective C++ (3rd Ed) by Scott Meyers – Intermediate patterns and best practices

C++ Primer (5th Ed) – Comprehensive introduction for beginners

**Courses**

[Effective Programming in C and C++](https://ocw.mit.edu/courses/6-s096-effective-programming-in-c-and-c-january-iap-2014/) (MIT OpenCourse)

[Advanced C++ & Modern Design (Quantnet/Baruch)](https://quantnet.com/advancedcpp/)

---

"Tomorrow's code is only as fast as today's design."
