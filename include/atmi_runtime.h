#ifndef __ATMI_RUNTIME_H__
#define __ATMI_RUNTIME_H__

#include "atmi.h"

/* Structs and enums */
typedef enum {
    ATMI_SUCCESS = 0,
    ATMI_ERROR
} atmi_status_t;

typedef enum {
    BRIG = 0,
    HSAIL,
    CL,
    x86, 
    PTX
} atmi_platform_type_t;

typedef atmi_kernel_s {
    uint64_t handle;
} ati_kernel_t;

/* Context */
atmi_status_t atmi_init(atmi_devtype_t type);
atmi_status_t atmi_finalize();

/* Module */
atmi_status_t atmi_module_register(const char *filename, atmi_platform_type_t type);

/* Kernel */
atmi_status_t atmi_kernel_create_empty(atmi_kernel_t *kernel, const unsigned int num_args);
//atmi_status_t atmi_kernel_create(const char *impl, atmi_devtype_t type, const unsigned int num_args);
atmi_status_t atmi_kernel_add_impl(atmi_kernel_t kernel, const char *impl, atmi_devtype_t type);

/* Task (kernel invocation) */
atmi_task_t *atmi_try_launch(atmi_kernel_t kernel, atmi_lparm_t *launch_params, void **args, size_t *arg_sizes);
atmi_status_t atmi_wait(atmi_task_t *task);

#endif // __ATMI_RUNTIME_H__
