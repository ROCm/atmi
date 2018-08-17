/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef __ATMI_RUNTIME_H__
#define __ATMI_RUNTIME_H__

#include "atmi.h"
#include <inttypes.h>
#include <stdlib.h>


/** \defgroup kernel Kernel Handles
 * This module includes all kernel-related classes, structs and functions.
 * @{
 */
/**
 * @brief Opaque handle representing an ATMI Kernel.
 *
 * @details ATMI kernels are instantiated in two steps. First, an empty kernel is
 * created. Next, architecture specific implementations are added. Each kernel
 * can have several implementations, but should have at least one implementation
 *
 */
typedef struct atmi_kernel_s {
    /**
     * Opaque handle.
     */
    uint64_t handle;
} atmi_kernel_t;

/**
 * @brief A generic function pointer representing CPU tasks. 
 */
typedef void (*atmi_generic_fp)(void);

typedef void (*task_process_init_buffer_t) (void *, int);
/**
 * @brief Register a callback to init a buffer assoicated with a task
 */
atmi_status_t atmi_register_task_init_buffer(task_process_init_buffer_t fp);

typedef void (*task_process_fini_buffer_t) (void *, int);
/**
 * @brief Register a callback to fini a buffer assoicated with a task
 */
atmi_status_t atmi_register_task_fini_buffer(task_process_fini_buffer_t fp);
/** @} */
#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup context_functions ATMI Context Setup and Finalize
 *  @{
 */
/**
 * @brief Initialize the ATMI runtime environment.
 *
 * @detal All ATMI runtime functions will fail if this function is not called
 * at least once. The user may initialize difference device types at different
 * regions in the program in order for optimization purposes. 
 * 
 * @param[in] type The types of devices that will be used by the application.
 *
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 */
atmi_status_t atmi_init(atmi_devtype_t type);

/**
 * @brief Finalize the ATMI runtime environment.
 *
 * @detail ATMI runtime functions will fail if called after finalize. 
 * 
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 */
atmi_status_t atmi_finalize();
/** @} */

/** \defgroup module_functions ATMI Module
 * @{
 */
/**
 * @brief Register the ATMI code module from file. 
 *
 * @detail Currently, only GPU devices need explicit module registration because
 * of their specific ISAs that require a separate compilation phase. On the other
 * hand, CPU devices execute regular x86 functions that are compiled with the
 * host program. 
 * 
 * @param[in] filenames A collection of files that contain the GPU modules 
 * targeting either the ::BRIG or ::AMDGCN platform types. Value cannot be NULL.
 *
 * @param[in] types A collection of platform types corresponding to the files.
 * Value cannot be NULL.
 *
 * @param[in] num_modules Size of @p filenames and @p types. Value should be
 * greater than 0.
 *
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 *
 */
atmi_status_t atmi_module_register(
    const char **filenames, 
    atmi_platform_type_t *types, 
    const int num_modules);

/**
 * @brief Register the ATMI code module from memory. 
 *
 * @detail Currently, only GPU devices need explicit module registration because
 * of their specific ISAs that require a separate compilation phase. On the other
 * hand, CPU devices execute regular x86 functions that are compiled with the
 * host program. 
 * 
 * @param[in] modules A collection of memory regions that contain the GPU modules 
 * targeting either the ::BRIG or ::AMDGCN platform types. Value cannot be NULL.
 *
 * @param[in] module_sizes Sizes of each module region in @p modules. Value
 * cannot be NULL.
 *
 * @param[in] types A collection of platform types corresponding to the modules.
 * Value cannot be NULL.
 *
 * @param[in] num_modules Size of @p modules. @p module_sizes and @p types. 
 * Value should be greater than 0.
 *
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 *
 */
atmi_status_t atmi_module_register_from_memory(
    void **modules, 
    size_t *module_sizes, 
    atmi_platform_type_t *types, 
    const int num_modules);
/** @} */

/** \defgroup machine ATMI Machine
 * @{
 */
/**
 * @brief ATMI's device discovery function to get the current machine's topology. 
 *
 * @detail The @p atmi_machine_t structure is a tree-based representation of the
 * compute and memory elements in the current node. Once ATMI is initialized,
 * this function can be called to retrieve the pointer to this global structure.  
 * 
 * @return Returns a pointer to a global structure of tyoe @p atmi_machine_t.
 * Returns NULL if ATMI is not initialized.
 */
atmi_machine_t *atmi_machine_get_info();
/** @} */

/* Kernel */
/** \defgroup kernel ATMI Kernel and Implementation 
 * @{
 */
/**
 * @brief Create an kernel opaque structure with all its architecture 
 * specific implementations. 
 *
 * @detail An ATMI kernel object is created and its architecture specific implementations 
 * are added. Each kernel can have several implementations, but should have at least one 
 * implementation. The opaque kernel handle acts as a key to identify the set of kernel
 * implementations. An ATMI GPU kernel implementation is identified by a char string, 
 * whereas a CPU kernel implementation is identified by a function pointer. 
 * These implementations must have the same number of arguments as specified in
 * this function. Each kernel implementation is associated with an identifier, 
 * which is nothing but the order in which the implementations have been added to the 
 * kernel. The advanced user may want to run the specific implementation of the kernel 
 * by using the unique identifier in the launch parameter of task launch functions. 
 * 
 * @param[out] kernel The opaque kernel handle.
 *
 * @param[in] num_args Number of arguments of the kernel. All implementations
 * must have the same number of input arguments. May be 0.
 *
 * @param[in] arg_sizes Size of each argument. May be NULL only if @p num_args is 0.
 *
 * @param[in] num_impls Number of implementations for this kernel.
 *
 * @param[in] ... va_list Key-value pairs separated by commas of the format 
 * (@atmi_devtype_t, implementation), where implementation is either the char
 * string for GPU implementation or function pointer for CPU implementation.
 *
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 *
 */
atmi_status_t atmi_kernel_create(atmi_kernel_t *atmi_kernel, const int num_args,
                                 const size_t *arg_sizes, const int num_impls, ...);
/**
 * @brief Create an empty kernel opaque structure. 
 *
 * @detail ATMI kernels are instantiated in two steps. First, an empty kernel is
 * created. Next, architecture specific implementations are added. Each kernel
 * can have several implementations, but should have at least one implementation.
 * The opaque kernel handle acts as a key to identify the set of kernel
 * implementations.
 * 
 * @param[out] kernel The opaque kernel handle.
 *
 * @param[in] num_args Number of arguments of the kernel. All implementations
 * must have the same number of input arguments. May be 0.
 *
 * @param[in] arg_sizes Size of each argument. May be NULL only if @p num_args is 0.
 *
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 *
 */
atmi_status_t atmi_kernel_create_empty(
    atmi_kernel_t *kernel, 
    const int num_args, 
    const size_t *arg_sizes);

/**
 * @brief Add a GPU kernel implementation.
 *
 * @detail An ATMI GPU kernel implementation is identified by a char array. 
 * The implementation must have the same number of arguments as the kernel. 
 * A unique user-specified identifier is associated with each implementation. 
 * The advanced user may want to run the specific implementation of the kernel 
 * by using the unique identifier in the launch parameter of task launch functions. 
 * 
 * @param[in] kernel The opaque kernel handle.
 *
 * @param[in] impl The kernel implementation name. 
 *
 * @param[in] ID The user-specified unique kernel identifier. 
 *
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 *
 */
atmi_status_t atmi_kernel_add_gpu_impl(
    atmi_kernel_t atmi_kernel, 
    const char *impl, 
    const unsigned int ID);

/**
 * @brief Add a CPU kernel implementation.
 *
 * @detail An ATMI CPU kernel implementation is identified by a function pointer. 
 * The implementation must have the same number of arguments as the kernel. 
 * A unique user-specified identifier is associated with each implementation. 
 * The advanced user may want to run the specific implementation of the kernel 
 * by using the unique identifier in the launch parameter of task launch functions. 
 * 
 * @param[in] kernel The opaque kernel handle.
 *
 * @param[in] impl The kernel implementation function pointer. 
 *
 * @param[in] ID The user-specified unique kernel identifier. 
 *
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 *
 */
atmi_status_t atmi_kernel_add_cpu_impl(
    atmi_kernel_t atmi_kernel, 
    atmi_generic_fp impl, 
    const unsigned int ID);

/**
 * @brief Release the kernel and all of its implementations.
 *
 * @detail After the kernel is released, its implementations may not be used to
 * launch any ATMI tasks. 
 *
 * @param[in] kernel The opaque kernel handle.
 *
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 *
 */
atmi_status_t atmi_kernel_release(atmi_kernel_t kernel);

/** @} */

/** \defgroup task ATMI Task
 * @{
 */
/**
 * @brief The ATMI task launcher function.
 *
 * @detail This function is used to launch any ATMI task (CPU or GPU). The @p 
 * kernel parameter specifies what has to be launched.The @p
 * lparm structure defines the task's launch parameters, which will guide the
 * ATMI runtime how to launch and manage the task.
 *
 * @param[in] lparm The structure desribing how the task has to be managed.
 *
 * @param[in] kernel The opaque kernel handle, which denotes what has to be
 * launched. 
 *
 * @param[in] args The bag of arguments all passed by reference. Their sizes
 * should be consistent with the kernel's @p arg_sizes parameter. 
 *
 * @return A handle to the ATMI task. The task handle may be used to setup
 * dependencies with other copy and compute tasks or for explicit synchronization 
 * by the host. Returns @ATMI_NULL_TASK_HANDLE on an error.  
 */
atmi_task_handle_t atmi_task_launch(
    atmi_lparm_t *lparm, 
    atmi_kernel_t kernel, 
    void **args);

/**
 * @brief Creating an ATMI task template for a future launch.
 *
 * @detail This function is used to create a placeholder ATMI task (CPU or GPU). 
 * The @p kernel parameter specifies what has to be eventually launched. This
 * function is especially useful when the user wants a placeholder task to wait
 * on, but its predecessor task graph has not yet been determined and may be
 * generated dynamically at some point in the future. A typical use case would be
 * to represent a fork-join model as a directed acyclic graph (DAG).
 *
 * @param[in] kernel The opaque kernel handle that denotes what has to be
 * launched in the future. 
 *
 * @return A handle to the placeholder ATMI task. The task handle may be used to setup
 * dependencies with other copy and compute tasks or for explicit synchronization 
 * by the host. Returns @ATMI_NULL_TASK_HANDLE on an error. 
 */
atmi_task_handle_t atmi_task_template_create(atmi_kernel_t kernel);

/**
 * @brief The ATMI task template activator function.
 *
 * @detail This function is used to activate an ATMI task (CPU or GPU) that was
 * previously created using @p atmi_task_template_create. The @p 
 * task parameter specifies what has to be activated. The @p
 * lparm structure defines the task's launch parameters, which will guide the
 * ATMI runtime how to activate and manage the task. A task that is created using
 * @atmi_task_create can be activated only once. Activating an already active
 * task is an error. 
 *
 * @param[in] lparm The structure desribing how the task has to be managed.
 *
 * @param[in] task The task handle, which was created previously using
 * atmi_task_create.
 *
 * @param[in] args The bag of arguments all passed by reference. Their sizes
 * should be consistent with the kernel's @p arg_sizes parameter. 
 *
 * @return The handle to the activated ATMI task. It should be the same as the input 
 * @task handle, otherwise it is an error. The returned task handle may be used to setup
 * dependencies with other copy and compute tasks or for explicit synchronization 
 * by the host. Returns @ATMI_NULL_TASK_HANDLE on an error.  
 */
atmi_task_handle_t atmi_task_template_activate(
    atmi_task_handle_t task,
    atmi_lparm_t *lparm,
    void **args);

/**
 * @brief Creating an ATMI task for a future launch.
 *
 * @detail This function is used to create an ATMI task (CPU or GPU) without
 * actually launching it. 
 * The @p kernel parameter specifies what has to be eventually launched. This
 * function is especially useful when the user wants a placeholder task to wait
 * on, but its predecessor task graph has not yet been determined and may be
 * generated dynamically at some point in the future. A typical use case would be
 * to represent a fork-join model as a directed acyclic graph (DAG).
 *
 * @param[in] lparm The structure desribing how the task has to be managed.
 *
 * @param[in] kernel The opaque kernel handle that denotes what has to be
 * launched in the future. 
 *
 * @param[in] args The bag of arguments all passed by reference. Their sizes
 * should be consistent with the kernel's @p arg_sizes parameter. 
 *
 * @return A handle to the placeholder ATMI task. The task handle may be used to setup
 * dependencies with other copy and compute tasks or for explicit synchronization 
 * by the host. The initial state of the task will be ATMI_UNINITIALIZED.
 * Returns @ATMI_NULL_TASK_HANDLE on an error. 
 */
atmi_task_handle_t atmi_task_create(
    atmi_lparm_t *lparm,
    atmi_kernel_t kernel,
    void **args);

/**
 * @brief The ATMI task activator function.
 *
 * @detail This function is used to activate an ATMI task (CPU or GPU) that was
 * previously created using @p atmi_task_create. The @p 
 * task parameter specifies what has to be activated. The @p
 * lparm structure defines the task's launch parameters, which will guide the
 * ATMI runtime how to activate and manage the task. A task that is created using
 * @atmi_task_create can be activated only once. Activating an already active
 * task is an error. 
 *
 * @param[in] task The task handle, which was created previously using
 * atmi_task_create.
 *
 * @return The handle to the activated ATMI task. It should be the same as the input 
 * @task handle, otherwise it is an error. The returned task handle may be used to setup
 * dependencies with other copy and compute tasks or for explicit synchronization 
 * by the host. Returns @ATMI_NULL_TASK_HANDLE on an error.  
 */
atmi_task_handle_t atmi_task_activate(
    atmi_task_handle_t task);

/**
 * @brief Wait for a launched task or a data movement operation.  
 *
 * @param[in] task The handle to an already launched task or an in-flight data
 * movement operation.
 * 
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 */
 atmi_status_t atmi_task_wait(atmi_task_handle_t task);

/**
 * @brief Wait for the launched task group, which could be a group of compute
 * tasks and data movement tasks.  
 *
 * @param[in] group The task group of already launched tasks or an in-flight data
 * movement operations.
 * 
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 */
 atmi_status_t atmi_task_group_sync(atmi_task_group_t *group);

/** @} */

/** \defgroup memory_functions ATMI Data Management
 * @{
 */
/**
 * @brief Allocate memory from the specified memory place.
 *
 * @detail This function allocates memory from the specified memory place. If the memory 
 * place belongs primarily to the CPU, then the memory will be accessible by
 * other GPUs and CPUs in the system. If the memory place belongs primarily to a GPU, 
 * then it cannot be accessed by other devices in the system. 
 *
 * @param[in] ptr The pointer to the memory that will be allocated.
 *
 * @param[in] size The size of the allocation in bytes. 
 *
 * @param[in] place The memory place in the system to perform the allocation.
 *
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 *
 */
atmi_status_t atmi_malloc(void **ptr, size_t size, atmi_mem_place_t place);

/**
 * @brief Frees memory that was previously allocated.
 *
 * @detail This function frees memory that was previously allocated by calling 
 * @p atmi_malloc. It throws an error otherwise. It is illegal to access a
 * pointer after a call to this function. 
 *
 * @param[in] ptr The pointer to the memory that has to be freed.
 *
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 *
 */
atmi_status_t atmi_free(void *ptr);

/**
 * @brief Syncrhonously copy memory from the source to destination memory locations. 
 *
 * @detail This function assumes that the source and destination regions are 
 * non-overlapping. The runtime determines the memory place of the source and the
 * destination and executes the appropriate optimized data movement methodology.
 *
 * @param[in] dest The destination pointer previously allocated by a system
 * allocator or @p atmi_malloc.
 *
 * @param[in] src The source pointer previously allocated by a system
 * allocator or @p atmi_malloc.
 *
 * @param[in] size The size of the data to be copied in bytes. 
 *
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR The function encountered errors.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 *
 */
atmi_status_t atmi_memcpy(void *dest, const void *src, size_t size);

/**
 * @brief Asyncrhonously copy memory from the source to destination memory locations. 
 *
 * @detail This function assumes that the source and destination regions are 
 * non-overlapping. The runtime determines the memory place of the source and the
 * destination and executes the appropriate optimized data movement methodology.
 * This function is equivalent to an asynchronous task, which means that it can
 * be used to setup dependencies with other memory copy routines or compute
 * tasks. The @p cparm structure can be used to provide additional information
 * about the copy operation. 
 *
 * @param[in] cparm The structure desribing how the copy task has to be managed.
 *
 * @param[in] dest The destination pointer previously allocated by a system
 * allocator or @p atmi_malloc.
 *
 * @param[in] src The source pointer previously allocated by a system
 * allocator or @p atmi_malloc.
 *
 * @param[in] size The size of the data to be copied in bytes. 
 *
 * @return A handle to the ATMI task. The task handle may be used to setup
 * dependencies with other copy and compute tasks or for explicit synchronization 
 * by the host.  
 *
 */
atmi_task_handle_t atmi_memcpy_async(
    atmi_cparm_t *cparm, 
    void *dest, 
    const void *src, 
    size_t size);
/** @} */

/** \defgroup cpu_dev_runtime ATMI CPU Device Runtime
 * @{
 */
/**
 * @brief Retrieve the task handle of 
 * the currently running task. This function is valid 
 * only within the body of a CPU task. 
 *
 * @return A handle to the ATMI CPU task. 
 *
 */
atmi_task_handle_t get_atmi_task_handle(); 

/**
 * @brief Retrieve the pointer to the task group object 
 * of the currently running task. This function is valid 
 * only within the body of a CPU task. 
 *
 * @return A pointer to the task group of the ATMI CPU task. 
 *
 */
atmi_task_group_t *get_atmi_task_group();

/**
 * @brief Retrieve the global thread ID of 
 * the currently running task. This function is valid 
 * only within the body of a CPU task. 
 *
 * @param[in] dim The dimension of the CPU task. Valid
 * dimensions are 0, 1 and 2.
 *
 * @return The global thread ID of the ATMI CPU task. 
 *
 */
unsigned long get_global_id(unsigned int dim);

/**
 * @brief Retrieve the global thread count of 
 * the currently running task. This function is valid 
 * only within the body of a CPU task. 
 *
 * @param[in] dim The dimension of the CPU task. Valid
 * dimensions are 0, 1 and 2.
 *
 * @return The global thread count of the ATMI CPU task. 
 *
 */
unsigned long get_global_size(unsigned int dim);

/**
 * @brief Retrieve the local thread ID of 
 * the currently running task. This function is valid 
 * only within the body of a CPU task.
 *
 * @param[in] dim The dimension of the CPU task. Valid
 * dimensions are 0, 1 and 2.
 *
 * @return The local thread ID of the ATMI CPU task. The
 * current ATMI CPU task model assumes the workgroup size
 * of 1 at all times for all dimensions, so this call 
 * always returns 0.
 */
unsigned long get_local_id(unsigned int dim);

/**
 * @brief Retrieve the local thread count of 
 * the currently running task. This function is valid 
 * only within the body of a CPU task. 
 *
 * @param[in] dim The dimension of the CPU task. Valid
 * dimensions are 0, 1 and 2.
 *
 * @return The local thread count of the ATMI CPU task. The
 * current ATMI CPU task model assumes the workgroup size
 * of 1 at all times for all dimensions, so this call 
 * always returns 1.
 *
 */
unsigned long get_local_size(unsigned int dim);

/**
 * @brief Retrieve the thread workgroup ID of 
 * the currently running task. This function is valid 
 * only within the body of a CPU task. 
 *
 * @param[in] dim The dimension of the CPU task. Valid
 * dimensions are 0, 1 and 2.
 *
 * @return The thread workgroup ID of the ATMI CPU task. The
 * current ATMI CPU task model assumes the workgroup size
 * of 1 at all times for all dimensions, so this call 
 * is equivalent to calling @p get_global_id.
 */
unsigned long get_group_id(unsigned int dim);

/**
 * @brief Retrieve the thread workgroup count of 
 * the currently running task. This function is valid 
 * only within the body of a CPU task. 
 *
 * @param[in] dim The dimension of the CPU task. Valid
 * dimensions are 0, 1 and 2.
 *
 * @return The thread workgroup count of the ATMI CPU task. The
 * current ATMI CPU task model assumes the workgroup size
 * of 1 at all times for all dimensions, so this call 
 * is equivalent to calling @p get_global_size.
 */
unsigned long get_num_groups(unsigned int dim);
/** @} */

#ifdef __cplusplus
}
#endif

#endif // __ATMI_RUNTIME_H__
