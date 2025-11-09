cmake_minimum_required(VERSION 3.20)
project(SystemMonitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# App sources
add_executable(system_monitor
    src/main.cpp
    src/process_utils.cpp
    src/system_stats.cpp
    src/renderer.cpp
)

# Define UNICODE and prevent Windows macros from breaking std::min/std::max
target_compile_definitions(system_monitor PRIVATE UNICODE _UNICODE NOMINMAX)

# Link PSAPI for process memory info
target_link_libraries(system_monitor PRIVATE Psapi)
