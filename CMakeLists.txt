cmake_minimum_required(VERSION 3.10)

project(qi-parser-example)

find_package(Boost REQUIRED)

add_executable(qi-parser qi-parser.cpp)

target_include_directories(qi-parser PRIVATE ${Boost_INCLUDE_DIRS})

target_compile_features(qi-parser PRIVATE cxx_std_11)
