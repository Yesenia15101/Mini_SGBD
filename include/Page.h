#ifndef PAGE_H
#define PAGE_H

#include <cstdint>

const int PAGE_SIZE = 4096;

struct RID{
    int page_id;
    uint16_t slot_id;
};

struct Page{
    int page_id;
    int next_page;
    char buffer[PAGE_SIZE - (sizeof(int) * 2)];
};

static_assert(sizeof(Page) == PAGE_SIZE, "Page debe medir exactamente 4096 bytes");

#endif