@echo off
g++ -std=c++17 -I include tests\demo_hospital_indices.cpp src\PageManager.cpp src\BufferManager.cpp src\BPlusTree.cpp src\SlottedPage.cpp -o build\demo_hospital_indices.exe
if errorlevel 1 (
    echo Error al compilar la demostracion hospitalaria.
    exit /b 1
)

.\build\demo_hospital_indices.exe %1
