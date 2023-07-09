cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(axxonsoft_test)
add_executable(axxonsoft_test main.cpp)
target_link_libraries(axxonsoft_test pthread stdc++)