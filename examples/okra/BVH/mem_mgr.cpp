#include <iostream>
#include "mem_mgr.h"

struct mem_mgr_class_ext {
    void (*mem_init)(mem_mgr m, size_t size);
    void *(*mem_malloc)(mem_mgr m, size_t req_size);
    void (*mem_destroy)(mem_mgr m);
    size_t (*mem_usage)(mem_mgr m);
    void *memory;	
    void *end;
    size_t offset;
    void *(*cb_malloc)(size_t size);
    void (*cb_free)(void* ptr);
};

static void mem_init(mem_mgr m, size_t size) {
    mem_mgr_class_ext *mgr = (mem_mgr_class_ext*)m;
    if ((mgr->memory = mgr->cb_malloc(size)) == NULL) {
        printf("abc Error allocating memory for BVH object.\n");
        exit(1);
    }
    mgr->end = (void*)((unsigned char*)mgr->memory+size);
    mgr->offset = 0;
}

static void* mem_malloc(mem_mgr m, size_t req_size) {
    mem_mgr_class_ext *mgr = (mem_mgr_class_ext*)m;
    void *ptr;
    ptr = (void*)(((unsigned char*)mgr->memory)+mgr->offset);

    if((void*)((unsigned char*)ptr+req_size) < mgr->end) {
        mgr->offset += req_size;
        return ptr;				
    }
    return NULL;
}

static size_t mem_usage(mem_mgr m) {
    mem_mgr_class_ext *mgr = (mem_mgr_class_ext*)m;
    return mgr->offset;
}

static void destroy_mem_mgr(mem_mgr m) {
    mem_mgr_class_ext *mgr = (mem_mgr_class_ext*)m;
    if(mgr->memory) {
        mgr->cb_free(mgr->memory);
    }
    free(m);
}

void create_mem_mgr(mem_mgr *m, void *(*cb_malloc)(size_t), void (*cb_free)(void*)) {
    mem_mgr_class_ext *lmem = (mem_mgr_class_ext*) malloc(sizeof(mem_mgr_class_ext));
    lmem->mem_init = mem_init;
    lmem->mem_malloc = mem_malloc;
    lmem->mem_usage = mem_usage;
    lmem->mem_destroy = destroy_mem_mgr;
    lmem->cb_malloc = cb_malloc;
    lmem->cb_free = cb_free;
    *m = (mem_mgr)lmem;
}
