#include "../include/JournalManager.h"
#include <cstring>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

struct JournalHeader{
    char magic[4];
    int page_count;
    int active;
};

JournalManager::JournalManager(const std::string& name):journal(nullptr),journal_name(name),page_count(0){}

JournalManager::~JournalManager(){
    if(journal!=nullptr)
        std::fclose(journal);
}

bool JournalManager::exists() const{
    FILE* f=std::fopen(journal_name.c_str(),"rb");
    if(f==nullptr)
        return false;
    std::fclose(f);
    return true;
}

bool JournalManager::write_header(){
    if(journal==nullptr)
        return false;

    JournalHeader h;
    h.magic[0]='M';
    h.magic[1]='J';
    h.magic[2]='R';
    h.magic[3]='N';
    h.page_count=page_count;
    h.active=1;

    std::fseek(journal,0,SEEK_SET);

    size_t written=std::fwrite(&h,1,sizeof(JournalHeader),journal);

    return written==sizeof(JournalHeader);
}

bool JournalManager::read_header(int& count){
    FILE* f=std::fopen(journal_name.c_str(),"rb");
    if(f==nullptr)
        return false;

    JournalHeader h;
    size_t readed=std::fread(&h,1,sizeof(JournalHeader),f);
    std::fclose(f);

    if(readed!=sizeof(JournalHeader))
        return false;

    if(h.magic[0]!='M' || h.magic[1]!='J' || h.magic[2]!='R' || h.magic[3]!='N')
        return false;

    if(h.active!=1)
        return false;

    count=h.page_count;
    return true;
}

bool JournalManager::sync_journal(){
    if(journal==nullptr)
        return false;

    if(std::fflush(journal)!=0)
        return false;

#ifdef _WIN32
    HANDLE h=CreateFileA(
        journal_name.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if(h==INVALID_HANDLE_VALUE)
        return false;

    BOOL ok=FlushFileBuffers(h);
    CloseHandle(h);

    return ok!=0;
#else
    int fd=fileno(journal);
    if(fd==-1)
        return false;

    return fsync(fd)==0;
#endif
}

bool JournalManager::begin(){
    if(journal!=nullptr)
        std::fclose(journal);

    journal=std::fopen(journal_name.c_str(),"w+b");
    if(journal==nullptr)
        return false;

    page_count=0;

    if(!write_header())
        return false;

    return sync_journal();
}

bool JournalManager::save_page_before_change(int page_id,const Page& original){
    if(journal==nullptr)
        return false;

    std::fseek(journal,0,SEEK_END);

    size_t w1=std::fwrite(&page_id,1,sizeof(int),journal);
    size_t w2=std::fwrite(&original,1,PAGE_SIZE,journal);

    if(w1!=sizeof(int) || w2!=PAGE_SIZE)
        return false;

    page_count++;

    if(!write_header())
        return false;

    return sync_journal();
}

bool JournalManager::rollback(PageManager& db){
    if(journal!=nullptr){
    std::fclose(journal);
    journal=nullptr;
    }
    int count=0;

    if(!read_header(count))
        return false;

    FILE* f=std::fopen(journal_name.c_str(),"rb");
    if(f==nullptr)
        return false;

    std::fseek(f,sizeof(JournalHeader),SEEK_SET);

    for(int i=0;i<count;i++){
        int page_id;
        Page original;

        size_t r1=std::fread(&page_id,1,sizeof(int),f);
        size_t r2=std::fread(&original,1,PAGE_SIZE,f);

        if(r1!=sizeof(int) || r2!=PAGE_SIZE){
            std::fclose(f);
            return false;
        }

        if(!db.write_page(page_id,original)){
            std::fclose(f);
            return false;
        }
    }

    std::fclose(f);

    if(!db.sync())
        return false;

    std::remove(journal_name.c_str());

    return true;
}

bool JournalManager::commit(){
    if(journal!=nullptr){
        std::fclose(journal);
        journal=nullptr;
    }

    std::remove(journal_name.c_str());

    return true;
}