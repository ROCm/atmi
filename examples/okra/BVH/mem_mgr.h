#ifndef MEM_MGR_CLASS_H
#define MEM_MGR_CLASS_H

typedef struct mem_mgr_class_base *mem_mgr;
struct mem_mgr_class_base {
    void (*mem_init)(mem_mgr m, size_t size);
    void *(*mem_malloc)(mem_mgr m, size_t req_size);
    void (*mem_destroy)(mem_mgr m);
    size_t (*mem_usage)(mem_mgr m);
};

void create_mem_mgr(mem_mgr *m, void *(*cb_malloc)(size_t), void (*cb_free)(void*));

#endif
