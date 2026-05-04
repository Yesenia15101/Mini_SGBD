
#include "../include/PageManager.h"
#include <iostream>
#include <fstream>

PageManager::PageManager(std::string name) : file_name(name){
    file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);

    if (!file.is_open()) {
        file.open(file_name, std::ios::out | std::ios::binary);
        file.close();
        file.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
    }
}
PageManager::~PageManager(){
    if (file.is_open()) {
        file.close();
    }
}
bool PageManager::write_page(int id, const Page& p) {
    long offset = (long)id * PAGE_SIZE;
    file.seekp(offset, std::ios::beg);
    file.write(reinterpret_cast<const char*>(&p), PAGE_SIZE);
    file.flush();
    return file.good();
}

Page PageManager::read_page(int id) {
    Page p;
    long offset = (long)id * PAGE_SIZE;
    file.seekg(offset, std::ios::beg);
    file.read(reinterpret_cast<char*>(&p), PAGE_SIZE);
    return p;
}
