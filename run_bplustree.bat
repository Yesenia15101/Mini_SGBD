@echo off
g++ -std=c++17 -I include tests\test_bplustree.cpp src\PageManager.cpp src\BufferManager.cpp src\BPlusTree.cpp src\SlottedPage.cpp -o build\test_bplustree.exe
if errorlevel 1 (
    echo Error al compilar la demostracion del B+ Tree.
    exit /b 1
)

.\build\test_bplustree.exe %1
