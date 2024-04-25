#include "memory.h"

pool_s* Mem_Pool::create_pool(size_t size) {
    pool_s* p;

    p = (pool_s*) malloc(size) ;
    if (p == nullptr) {
        return nullptr;
    }

    p->d.last = (u_char*)p + sizeof(pool_s);
    p->d.end = (u_char*)p + size;
    p->d.next = nullptr;
    p->d.failed = 0;

    size = size - sizeof(pool_s);
    p->max = (size < MAX_ALLOC_FROM_POOL) ? size : MAX_ALLOC_FROM_POOL;

    p->current = p;
    p->large = nullptr;
    p->cleanup = nullptr;

    pool = p;
    return p;
}

void* Mem_Pool::palloc(size_t size) {
    if (size <= pool->max) {
        return palloc_small(size, 1);
    }

    return palloc_large(size);
}

void* Mem_Pool::palloc_small(size_t size, uint_t align) {
    u_char* m;
    pool_s* p;

    p = pool->current;

    do {
        m = p->d.last;

        if (align) {
            m = align_ptr(m, ALIGNMENT);
        }

        if ((size_t)(p->d.end - m) >= size) {
            p->d.last = m + size; // 剩余够用，直接偏移

            return m;
        }

        p = p->d.next;

    } while (p);

    return palloc_block(size);
}

void* Mem_Pool::palloc_block(size_t size) {
    u_char* m;
    size_t       psize;
    pool_s* p, * newpool;

    psize = (size_t)(pool->d.end - (u_char*)pool); // 整个小块容量大小应该相等

    m = (u_char*)malloc(psize);
    if (m == nullptr) {
        return nullptr;
    }

    newpool = (pool_s*)m;

    newpool->d.end = m + psize;
    newpool->d.next = nullptr;
    newpool->d.failed = 0;

    m += sizeof(pool_data_t); // 除去头所用内存量
    m = align_ptr(m, ALIGNMENT);
    newpool->d.last = m + size; // 直接偏移使用

    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) { // 小块分配失败超过四次之后，则不进行分配
            pool->current = p->d.next;
        }
    }
    p->d.next = newpool;

    return m;
}

void* Mem_Pool::palloc_large(size_t size) {
    void* p;
    uint_t n;
    pool_large_s* large;

    p = malloc(size);
    if (p == nullptr) 
        return nullptr;

    n = 0;

    for (large = pool->large; large; large = large->next) {
        if (large->alloc == nullptr) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) { // 查找可用大块头部，如果超过3次，直接再新建一个新的
            break;
        }
    }

    large = (pool_large_s*)palloc_small(sizeof(pool_large_s), 1);
    if (large == nullptr) {
        free(p);
        return nullptr;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}

void* Mem_Pool::pnalloc(size_t size) {
    if (size <= pool->max) {
        return palloc_small(size, 0);
    }

    return palloc_large(size);
}

void* Mem_Pool::pcalloc(size_t size) {
    void* p;

    p = palloc(size);
    if (p) {
        memzero(p, size);
    }

    return p;
}

void Mem_Pool::pfree(void* p) {
    pool_large_s* l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            free(l->alloc);
            l->alloc = nullptr;

        }
    }

}

void Mem_Pool::reset_pool() {
    pool_s* p;
    pool_large_s* l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }

    p = pool;
    p->d.last = (u_char*)p + sizeof(pool_s);
    p->d.failed = 0;
    for (p = p->d.next; p; p = p->d.next) {
        p->d.last = (u_char*)p + sizeof(pool_data_t);
    }

    pool->current = pool;
    pool->large = nullptr;
}

void Mem_Pool::destroy_pool() {
    pool_s* p, * n;
    pool_large_s* l;
    pool_cleanup_s* c;

    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            c->handler(c->data);
        }
    }



    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        free(p);

        if (n == nullptr) {
            break;
        }
    }
}

pool_cleanup_s* Mem_Pool::pool_cleanup_add(size_t size) {
    pool_cleanup_s* c;

    c = (pool_cleanup_s*)palloc(sizeof(pool_cleanup_s));
    if (c == nullptr) {
        return nullptr;
    }

    if (size) {
        c->data = palloc(size);
        if (c->data == nullptr) {
            return nullptr;
        }

    }
    else {
        c->data = nullptr;
    }

    c->handler = nullptr;
    c->next = pool->cleanup;
    pool->cleanup = c;
    return c;
}   




