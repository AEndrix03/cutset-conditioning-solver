cmake_minimum_required(VERSION 3.16)

project(cutset_csp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cutset_csp
        src/main.cpp
        src/backtracking.cpp
        src/cutset.cpp
        src/csp.cpp
        src/experiment.cpp
        src/graph.cpp
        src/problems.cpp
        src/tree_solver.cpp
)

target_include_directories(cutset_csp PRIVATE include)
