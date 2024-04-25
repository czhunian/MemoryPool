#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Data stData;
struct Data
{
    char* ptr;
    FILE* pfile;
};

void func1(void* str)
{
    char *p = (char*) str;
    printf("free ptr mem!\n");
    free(p);
}

void func2(void *str)
{
    FILE* pf = (FILE *)str;
    printf("close file!\n");
    fclose(pf);
}

int main()
{
    Mem_Pool pool;
    if (pool.create_pool(512) == nullptr) {
        printf("create_pool fail...\n");
        return 0;
    }  
    

    void* p1 = pool.palloc(128); // 从小块内存池分配的
    if (p1 == nullptr) {
        printf("palloc 128 bytes fail...\n");
        return 0;
    }

    stData* p2 = (stData*)pool.palloc(512); // 从大块内存池分配的
    if (p2 == nullptr) {
        printf("palloc 512 bytes fail...\n");
        return 0;
    }
    p2->ptr = (char*)malloc(12);
    strcpy(p2->ptr, "hello world");
    p2->pfile = fopen("data.txt", "w");

    pool_cleanup_s* c1 = pool.pool_cleanup_add(sizeof(char*));
    c1->handler = func1;
    c1->data = p2->ptr;

    pool_cleanup_s* c2 = pool.pool_cleanup_add(sizeof(FILE*));
    c2->handler = func2;
    c2->data = p2->pfile;

    pool.destroy_pool(); // 1.调用所有的预置的清理函数 2.释放大块内存 3.释放小块内存池所有内存

    return 0;
}
