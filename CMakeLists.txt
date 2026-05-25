cmake_minimum_required(VERSION 3.11)

project(PageSystem)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(minisgbd
    src/main.cpp
    src/PageManager.cpp
    src/JournalManager.cpp
)