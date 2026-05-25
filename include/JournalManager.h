#ifndef JOURNALMANAGER_H
#define JOURNALMANAGER_H

#include <cstdio>
#include <string>
#include "Page.h"
#include "PageManager.h"

class JournalManager{
private:
    FILE* journal;
    std::string journal_name;
    int page_count;

    bool write_header();
    bool read_header(int& count);
    bool sync_journal();

public:
    JournalManager(const std::string& name);
    ~JournalManager();

    bool begin();
    bool save_page_before_change(int page_id,const Page& original);
    bool rollback(PageManager& db);
    bool commit();
    bool exists() const;
};

#endif