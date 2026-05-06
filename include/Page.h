#ifndef PAGE_H
#define PAGE_H

#include <cstring>

const int PAGE_SIZE = 4096;

struct Header {
    int numSlots;
    int freeSpaceOffset;
};

struct Slot {
    int offset;
    int size;
};

struct Page {
    int page_id;
    int next_page;

    char buffer[PAGE_SIZE - (sizeof(int) * 2)];

    Page() {
        page_id = -1;
        next_page = -1;
        Header* h = reinterpret_cast<Header*>(buffer);
        h->numSlots = 0;
        h->freeSpaceOffset = sizeof(buffer);

        memset(buffer + sizeof(Header), 0, sizeof(buffer) - sizeof(Header));
    }
};

#endif