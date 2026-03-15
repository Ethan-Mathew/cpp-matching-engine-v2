cmake_minimum_required(VERSION 4.2)

project(CppMatchingEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Main ./main.cpp)

add_subdirectory(src)

target_link_libraries(Main PRIVATE MemoryPool)
