#include "../include/SlottedPage.h"
#include <cstring>

SlottedPage::SlotHeader* SlottedPage::header(Page& page){
    return reinterpret_cast<SlotHeader*>(page.buffer);
}

const SlottedPage::SlotHeader* SlottedPage::header(const Page& page){
    return reinterpret_cast<const SlotHeader*>(page.buffer);
}

SlottedPage::SlotEntry* SlottedPage::slot(Page& page, uint16_t slot_id){
    return reinterpret_cast<SlotEntry*>(
        page.buffer + sizeof(SlotHeader) + slot_id * sizeof(SlotEntry)
    );
}

const SlottedPage::SlotEntry* SlottedPage::slot(const Page& page, uint16_t slot_id){
    return reinterpret_cast<const SlotEntry*>(
        page.buffer + sizeof(SlotHeader) + slot_id * sizeof(SlotEntry)
    );
}

void SlottedPage::update_free_space(Page& page){
    SlotHeader* h = header(page);

    if(h->free_end >= h->free_start)
        h->free_space = h->free_end - h->free_start;
    else
        h->free_space = 0;
}

int SlottedPage::find_deleted_slot(Page& page){
    SlotHeader* h = header(page);

    for(uint16_t i = 0; i < h->slot_count; i++){
        SlotEntry* s = slot(page, i);

        if(s->flags == SLOT_DELETED)
            return i;
    }

    return -1;
}

void SlottedPage::init(Page& page, int page_id, int next_page){
    std::memset(&page, 0, sizeof(Page));

    page.page_id = page_id;
    page.next_page = next_page;

    SlotHeader* h = header(page);

    h->magic = MAGIC;
    h->slot_count = 0;
    h->free_start = sizeof(SlotHeader);
    h->free_end = DATA_SIZE;
    h->free_space = DATA_SIZE - sizeof(SlotHeader);
    h->deleted_count = 0;
}

bool SlottedPage::is_initialized(const Page& page){
    const SlotHeader* h = header(page);

    return h->magic == MAGIC;
}

bool SlottedPage::insert_record(Page& page, const char* record, uint16_t size, RID& rid){
    if(record == nullptr || size == 0)
        return false;

    if(!is_initialized(page))
        init(page, page.page_id, page.next_page);

    SlotHeader* h = header(page);

    int deleted_slot = find_deleted_slot(page);

    uint16_t required = size;

    if(deleted_slot == -1)
        required += sizeof(SlotEntry);

    if(h->free_space < required)
        return false;

    h->free_end -= size;

    std::memcpy(page.buffer + h->free_end, record, size);

    uint16_t slot_id;

    if(deleted_slot != -1){
        slot_id = static_cast<uint16_t>(deleted_slot);

        if(h->deleted_count > 0)
            h->deleted_count--;
    }else{
        slot_id = h->slot_count;
        h->slot_count++;
        h->free_start = sizeof(SlotHeader) + h->slot_count * sizeof(SlotEntry);
    }

    SlotEntry* s = slot(page, slot_id);

    s->offset = h->free_end;
    s->size = size;
    s->flags = SLOT_USED;

    update_free_space(page);

    rid.page_id = page.page_id;
    rid.slot_id = slot_id;

    return true;
}

bool SlottedPage::get_record(const Page& page, uint16_t slot_id, char* output, uint16_t& size){
    if(output == nullptr)
        return false;

    if(!is_initialized(page))
        return false;

    const SlotHeader* h = header(page);

    if(slot_id >= h->slot_count)
        return false;

    const SlotEntry* s = slot(page, slot_id);

    if(s->flags != SLOT_USED)
        return false;

    if(s->offset + s->size > DATA_SIZE)
        return false;

    std::memcpy(output, page.buffer + s->offset, s->size);

    size = s->size;

    return true;
}

bool SlottedPage::delete_record(Page& page, uint16_t slot_id){
    if(!is_initialized(page))
        return false;

    SlotHeader* h = header(page);

    if(slot_id >= h->slot_count)
        return false;

    SlotEntry* s = slot(page, slot_id);

    if(s->flags != SLOT_USED)
        return false;

    s->flags = SLOT_DELETED;
    h->deleted_count++;

    return true;
}

uint16_t SlottedPage::get_free_space(const Page& page){
    if(!is_initialized(page))
        return 0;

    const SlotHeader* h = header(page);

    return h->free_space;
}

uint16_t SlottedPage::get_slot_count(const Page& page){
    if(!is_initialized(page))
        return 0;

    const SlotHeader* h = header(page);

    return h->slot_count;
}

bool SlottedPage::validate(const Page& page){
    if(!is_initialized(page))
        return false;

    const SlotHeader* h = header(page);

    if(h->free_start < sizeof(SlotHeader))
        return false;

    if(h->free_end > DATA_SIZE)
        return false;

    if(h->free_start > h->free_end)
        return false;

    for(uint16_t i = 0; i < h->slot_count; i++){
        const SlotEntry* s = slot(page, i);

        if(s->flags == SLOT_USED){
            if(s->size == 0)
                return false;

            if(s->offset + s->size > DATA_SIZE)
                return false;

            if(s->offset < h->free_end)
                return false;
        }
    }

    return true;
}