cmake_minimum_required(VERSION 3.10)
project(StackVector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(example example.cpp)
add_executable(debug debug.cpp)
add_executable(debug2 debug2.cpp)

# Optional: Add compiler warnings
if(MSVC)
    target_compile_options(example PRIVATE /W4)
    target_compile_options(debug PRIVATE /W4)
    target_compile_options(debug2 PRIVATE /W4)
else()
    target_compile_options(example PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(debug PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(debug2 PRIVATE -Wall -Wextra -Wpedantic)
endif()
