#ifndef PAGE_H
#define PAGE_H

#include <iostream>

const int PAGE_SIZE = 4096;

struct Page{
    int page_id;
    int next_page;
    int data_count;

    char buffer[PAGE_SIZE - (sizeof(int)*3)];
    Page() {
        page_id = -1;
        next_page = -1;
        data_count = 0;
        for(int i = 0; i < (PAGE_SIZE - 12); i++) buffer[i] = 0;
    }


};

#endif