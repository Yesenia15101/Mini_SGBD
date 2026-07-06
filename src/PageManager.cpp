#include "../include/PageManager.h"
#include <cstring>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static_assert(sizeof(Page)==PAGE_SIZE,"Page debe medir exactamente PAGE_SIZE bytes");

PageManager::PageManager(const std::string& name):file(nullptr),file_name(name){
    file=std::fopen(file_name.c_str(),"r+b");
    if(file==nullptr)
        file=std::fopen(file_name.c_str(),"w+b");
}

PageManager::~PageManager(){
    if(file!=nullptr)
        std::fclose(file);
}

bool PageManager::is_open() const{
    return file!=nullptr;
}

bool PageManager::seek_page(int id){
    if(file==nullptr || id<0)
        return false;

    long offset=static_cast<long>(id)*PAGE_SIZE;

    return std::fseek(file,offset,SEEK_SET)==0;
}

bool PageManager::write_page(int id,const Page& p){
    if(!seek_page(id))
        return false;

    std::clearerr(file);

    size_t written=std::fwrite(reinterpret_cast<const char*>(&p),1,PAGE_SIZE,file);

    if(written!=PAGE_SIZE)
        return false;

    return std::fflush(file)==0;
}

bool PageManager::read_page(int id,Page& p){
    if(!seek_page(id))
        return false;

    std::clearerr(file);

    size_t readed=std::fread(reinterpret_cast<char*>(&p),1,PAGE_SIZE,file);

    return readed==PAGE_SIZE;
}

Page PageManager::read_page(int id){
    Page p;
    std::memset(&p,0,sizeof(Page));
    read_page(id,p);
    return p;
}

bool PageManager::sync(){
    if(file==nullptr)
        return false;

    if(std::fflush(file)!=0)
        return false;

#ifdef _WIN32
    HANDLE h=CreateFileA(
        file_name.c_str(),
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
    int fd=fileno(file);
    if(fd==-1)
        return false;

    return fsync(fd)==0;
#endif
}
int PageManager::allocate_page(){

    if(file == nullptr)
        return -1;

    std::fseek(file, 0, SEEK_END);

    long size = std::ftell(file);

    int new_page_id = size / PAGE_SIZE;

    Page empty;
    std::memset(&empty, 0, sizeof(Page));

    write_page(new_page_id, empty);

    sync();

    return new_page_id;
}