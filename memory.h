#pragma once

/*
    整体仿照Nginx内存池，小块通过指针偏移进行复用内存
    大块是有释放内存操作函数的，小块没有。
*/
#include <stdlib.h>
#include <memory.h>
#include <string.h>

using u_char = unsigned char;
using uint_t = unsigned int;
using uintptr_t = unsigned long;
struct pool_s;
struct pool_large_s;

#define memzero(buf, n)      memset(buf, 0, n)
#define memset(buf, c, n)    memset(buf, c, n)
#define align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))



const int ALIGNMENT = sizeof(unsigned long);

const int pagesize = 4096;

const int MAX_ALLOC_FROM_POOL = pagesize - 1;

const int DEFAULT_POOL_SIZE = 16 * 1024;

const int POOL_ALIGNMENT = 16;



typedef void (*pool_cleanup_pt)(void* data);

struct pool_cleanup_s {
    pool_cleanup_pt handler; 
    void* data;
    pool_cleanup_s* next; 
};

struct pool_large_s {
    pool_large_s* next;
    void* alloc;
};

struct pool_data_t {
    u_char* last;
    u_char* end;
    pool_s* next;
    uint_t  failed;
};


struct pool_s {
    pool_data_t d;
    size_t max;
    pool_s* current;
    pool_large_s* large;
    pool_cleanup_s* cleanup;
};

const int MIN_POOL_SIZE =
    align((sizeof(pool_s) + 2 * sizeof(pool_large_s)),
        POOL_ALIGNMENT);

class Mem_Pool {
public:
    pool_s* create_pool(size_t size);
    void* palloc(size_t size);
    void* pnalloc(size_t size);
    void* pcalloc(size_t size);
    void pfree(void *p);
    void reset_pool();
    void destroy_pool();
    pool_cleanup_s* pool_cleanup_add(size_t size);

private:
    void* palloc_small(size_t size, uint_t align);
    void* palloc_large(size_t size);
    void* palloc_block(size_t size);

    pool_s *pool;

};