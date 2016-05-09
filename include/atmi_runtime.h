#ifndef __ATMI_RUNTIME_H__
#define __ATMI_RUNTIME_H__

#include "atmi.h"
#include <inttypes.h>
#include <stdlib.h>
/* Structs and enums */
typedef enum atmi_status_t {
    ATMI_STATUS_SUCCESS=0,
    ATMI_STATUS_UNKNOWN=1,
    ATMI_STATUS_ERROR=2
} atmi_status_t;

typedef enum {
    BRIG = 0,
    AMDGCN, // offline finalized BRIG 
    /* -- support in the future? -- 
    HSAIL,
    CL,
    x86, 
    PTX
    */
} atmi_platform_type_t;

typedef struct atmi_kernel_s {
    uint64_t handle;
} atmi_kernel_t;

typedef void (*atmi_generic_fp)(void);
#ifdef __cplusplus
extern "C" {
#endif

/* Context */
atmi_status_t atmi_init(int type);
atmi_status_t atmi_finalize();

/* Module */
atmi_status_t atmi_module_register(const char **filename, atmi_platform_type_t *type, const int num_modules);

/* machine */
atmi_status_t atmi_machine_get_info(atmi_machine_t **machine);
//atmi_status_t atmi_machine_get_memory_info(atmi_machine_memory_t *m);
//atmi_status_t atmi_machine_get_compute_info(atmi_machine_compute_t *c);

/* Kernel */
atmi_status_t atmi_kernel_create_empty(atmi_kernel_t *kernel, const int num_args, 
                                    const size_t *arg_sizes);
atmi_status_t atmi_kernel_add_gpu_impl(atmi_kernel_t atmi_kernel, const char *impl, const unsigned int ID);
atmi_status_t atmi_kernel_add_cpu_impl(atmi_kernel_t atmi_kernel, atmi_generic_fp impl, const unsigned int ID);
atmi_status_t atmi_kernel_release(atmi_kernel_t kernel);

/* Task (kernel invocation) */
atmi_task_handle_t atmi_task_launch(atmi_kernel_t kernel, atmi_lparm_t *lparm, void **args);
atmi_status_t atmi_task_wait(atmi_task_handle_t task);

/* memory/data */
atmi_status_t atmi_data_map(void *ptr, size_t size, atmi_mem_place_t place, atmi_data_t *data);
atmi_status_t atmi_data_unmap(void *ptr, atmi_data_t *data);
atmi_status_t atmi_data_copy(atmi_data_t *dest, const atmi_data_t *src);
atmi_status_t atmi_data_create(atmi_data_t *data, size_t size, atmi_mem_place_t place);
atmi_status_t atmi_data_destroy(atmi_data_t *data);

#ifdef __cplusplus
}
#endif

#endif // __ATMI_RUNTIME_H__
