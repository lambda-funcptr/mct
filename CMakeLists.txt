cmake_minimum_required(VERSION 3.22)
project(mct C)

set(CC musl-cc)
set(CMAKE_C_STANDARD 11)

file(GLOB C_FILES "${PROJECT_SOURCE_DIR}/src/*.c")
file(GLOB HEADERS "${PROJECT_SOURCE_DIR}/src/*.h")

add_executable(mct ${C_FILES} ${HEADERS})
target_link_libraries(mct cap)