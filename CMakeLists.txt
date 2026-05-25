cmake_minimum_required(VERSION 3.11)

project(PageSystem)

add_executable(minisgbd
    src/main.cpp
    src/PageManager.cpp
    src/JournalManager.cpp
    src/SlottedPage.cpp
    src/BufferManager.cpp
)