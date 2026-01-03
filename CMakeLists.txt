cmake_minimum_required(VERSION 3.14)
project(MyProject)

if(BUILD_GATEWAY)
    include("${CMAKE_CURRENT_SOURCE_DIR}/pi_gateway.cmake")
else()
    include("${CMAKE_CURRENT_SOURCE_DIR}/pico_build.cmake")
endif()

