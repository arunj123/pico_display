cmake_minimum_required(VERSION 3.10)
project(ble_daemon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
# Fallback if compiler doesn't support 23 fully yet, though GCC 14 should.
# If 23 fails, we might need 20, but let's try 23 as requested.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add source directories
include_directories(src)
include_directories(src/core/compat)

# Collect sources (globing for simplicity in this assistant context, explicit list preferred in prod)
file(GLOB_RECURSE SRC_FILES 
    "src/*.cpp"
)

add_executable(ble_daemon ${SRC_FILES})

# Link threads if necessary (usually needed for C++ std::thread)
find_package(Threads REQUIRED)
target_link_libraries(ble_daemon PRIVATE Threads::Threads)
