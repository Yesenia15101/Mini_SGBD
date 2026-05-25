#ifndef FRAME_H
#define FRAME_H

#include "Page.h"

struct Frame{
    Page page;
    int page_id;
    bool dirty;
    int pin_count;
    uint64_t last_used;
    bool occupied;
    Frame() {
        page_id = -1;
        dirty = false;
        pin_count = 0;
        last_used = 0;
        occupied = false;
    }
};


#endif
