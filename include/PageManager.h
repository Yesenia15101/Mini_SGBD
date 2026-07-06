#ifndef PAGEMANAGER_H
#define PAGEMANAGER_H

#include <cstdio>
#include <string>
#include "Page.h"

class PageManager{
private:
    FILE* file;
    std::string file_name;
    bool seek_page(int id);

public:
    PageManager(const std::string& name);
    ~PageManager();

    bool write_page(int id,const Page& p);
    bool read_page(int id,Page& p);
    Page read_page(int id);

    bool sync();
    bool is_open() const;
    int allocate_page();
};

#endif