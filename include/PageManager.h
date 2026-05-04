#ifndef PAGEMANAGER_H
#define PAGEMANAGER_H

#include <string>
#include <fstream>
#include "Page.h"

class PageManager{
    private:
        std::fstream file;
        std::string file_name;
    public:
    PageManager(std::string name);
    ~PageManager();
    bool write_page(int id, const Page& p);
    Page read_page(int id);


};

#endif