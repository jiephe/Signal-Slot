cmake_minimum_required(VERSION 3.10)

project(CSTEST)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

file(GLOB SRC_FILES
    "*.cpp"
    "signal/*.cpp"
)

# Add include directories
include_directories(
    .
    signal
)

# Libraries to link
set(LIBS
    pthread
    dl
    z
)

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} ${LIBS})
