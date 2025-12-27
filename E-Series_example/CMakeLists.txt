cmake_minimum_required(VERSION 3.21)
project(record_replay)

set(CMAKE_CXX_STANDARD 17)

find_library(libiio_LIBRARY iio)
if(libiio_FOUND)
    message(STATUS "Find libiio library")
endif ()

add_executable(record_replay main.cpp antsdrDevice.h antsdrDevice.cpp)

target_link_libraries(record_replay iio ad9361)
