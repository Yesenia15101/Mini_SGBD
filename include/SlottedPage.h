#ifndef SLOTTEDPAGE_H
#define SLOTTEDPAGE_H

#include "Page.h"
#include <cstdint>

class SlottedPage{
private:
    static const uint16_t SLOT_FREE = 0;
    static const uint16_t SLOT_USED = 1;
    static const uint16_t SLOT_DELETED = 2;
    static const uint16_t MAGIC = 0xBEEF;
    static const uint16_t DATA_SIZE = PAGE_SIZE - (sizeof(int) * 2);

#pragma pack(push, 1)
    struct SlotHeader{
        uint16_t magic;
        uint16_t slot_count;
        uint16_t free_start;
        uint16_t free_end;
        uint16_t free_space;
        uint16_t deleted_count;
    };

    struct SlotEntry{
        uint16_t offset;
        uint16_t size;
        uint16_t flags;
    };
#pragma pack(pop)

    static SlotHeader* header(Page& page);
    static const SlotHeader* header(const Page& page);

    static SlotEntry* slot(Page& page, uint16_t slot_id);
    static const SlotEntry* slot(const Page& page, uint16_t slot_id);

    static void update_free_space(Page& page);
    static int find_deleted_slot(Page& page);

public:
    static void init(Page& page, int page_id, int next_page = -1);

    static bool is_initialized(const Page& page);

    static bool insert_record(Page& page, const char* record, uint16_t size, RID& rid);

    static bool get_record(const Page& page, uint16_t slot_id, char* output, uint16_t& size);

    static bool delete_record(Page& page, uint16_t slot_id);

    static uint16_t get_free_space(const Page& page);

    static uint16_t get_slot_count(const Page& page);

    static bool validate(const Page& page);
};

#endif