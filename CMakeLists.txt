cmake_minimum_required(VERSION 3.10..4.1)

project(xcoroutine LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/install_test_deps.cmake)
include(cmake/formate.cmake)
include(GNUInstallDirs)
add_subdirectory(xc/coroutine)
add_subdirectory(examples)
add_subdirectory(tests)
