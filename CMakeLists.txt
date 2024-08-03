cmake_minimum_required(VERSION 3.14)

project(hello)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# disable logging
add_definitions(-DLOG_DISABLE_LOGS)
add_definitions(-DNDEBUG)

add_subdirectory(llama.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/ggml/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/ggml/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/common)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE llama ggml common)
