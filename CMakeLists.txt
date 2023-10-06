cmake_minimum_required(VERSION 3.5)

project(zmeika_for_pr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(zmeika_for_pr main.cpp)

install(TARGETS zmeika_for_pr
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
