cmake_minimum_required(VERSION 3.0)

project(LowLatencyApp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -Wpedantic")
set(CMAKE_VERBOSE_MAKEFILE on)

add_subdirectory(common)
add_subdirectory(exchange)
add_subdirectory(trading)

list(APPEND LIBS libexchange)
list(APPEND LIBS libtrading)
list(APPEND LIBS libcommon)
list(APPEND LIBS pthread)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/exchange)
include_directories(${PROJECT_SOURCE_DIR}/trading)

add_executable(exchange_main exchange/exchange_main.cpp)
target_link_libraries(exchange_main PUBLIC ${LIBS})

add_executable(trading_main trading/trading_main.cpp)
target_link_libraries(trading_main PUBLIC ${LIBS})

# add_executable(demo1_pnl demos/demo1_pnl.cpp)
# target_link_libraries(demo1_pnl PUBLIC ${LIBS})

# add_executable(demo2_orders demos/demo2_orders.cpp)
# target_link_libraries(demo2_orders PUBLIC ${LIBS})

# add_executable(demo3_strategies demos/demo3_strategies.cpp)
# target_link_libraries(demo3_strategies PUBLIC ${LIBS})

add_executable(demo_interactive demos/demo_interactive.cpp)
target_link_libraries(demo_interactive PUBLIC ${LIBS})

add_executable(demo_full_market demos/demo_full_market.cpp)
target_link_libraries(demo_full_market PUBLIC ${LIBS})
