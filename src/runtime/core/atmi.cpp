/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#include <rt.h>

/*
 * Initialize/Finalize
 */
atmi_status_t atmi_init(atmi_devtype_t devtype) {
  return core::Runtime::getInstance().Initialize(devtype);
}

atmi_status_t atmi_finalize() {
  return core::Runtime::getInstance().Finalize();
}

/*
 * Machine Info
 */
atmi_machine_t* atmi_machine_get_info() {
  return core::Runtime::getInstance().GetMachineInfo();
}

/*
 * Modules
 */
atmi_status_t atmi_module_register_from_memory(void **modules, size_t *module_sizes,
                                               atmi_platform_type_t *types, const int num_modules) {
  return core::Runtime::getInstance().RegisterModuleFromMemory(modules, module_sizes,
                                                               types, num_modules);
}

atmi_status_t atmi_module_register(const char **filenames, atmi_platform_type_t *types,
                                   const int num_modules) {
  return core::Runtime::getInstance().RegisterModule(filenames, types, num_modules);
}

/*
 * Kernels
 */
atmi_status_t atmi_kernel_create(atmi_kernel_t *atmi_kernel, const int num_args,
                                 const size_t *arg_sizes, const int num_impls, ...) {
  va_list arguments;
  va_start(arguments, num_impls);
  return core::Runtime::getInstance().CreateKernel(atmi_kernel, num_args, arg_sizes, num_impls,
                                                   arguments);
  va_end(arguments);
}

atmi_status_t atmi_kernel_release(atmi_kernel_t atmi_kernel) {
  return core::Runtime::getInstance().ReleaseKernel(atmi_kernel);
}

atmi_status_t atmi_kernel_create_empty(atmi_kernel_t *atmi_kernel, const int num_args,
                                       const size_t *arg_sizes) {
  return core::Runtime::getInstance().CreateEmptyKernel(atmi_kernel, num_args, arg_sizes);
}

atmi_status_t atmi_kernel_add_gpu_impl(atmi_kernel_t atmi_kernel, const char *impl,
                                       const unsigned int ID) {
  return core::Runtime::getInstance().AddGPUKernelImpl(atmi_kernel, impl, ID);
}

atmi_status_t atmi_kernel_add_cpu_impl(atmi_kernel_t atmi_kernel, atmi_generic_fp impl,
                                       const unsigned int ID) {
  return core::Runtime::getInstance().AddCPUKernelImpl(atmi_kernel, impl, ID);
}

/*
 * Synchronize
 */
atmi_status_t atmi_taskgroup_wait(atmi_taskgroup_handle_t group_handle) {
  return core::Runtime::getInstance().TaskGroupSync(group_handle);
}

atmi_status_t atmi_task_wait(atmi_task_handle_t task) {
  return core::Runtime::getInstance().TaskWait(task);
}

/*
 * [Experimental] print buffers/pipes
 */
atmi_status_t atmi_register_task_init_buffer(task_process_init_buffer_t fp) {
  return core::Runtime::getInstance().RegisterTaskInitBuffer(fp);
}

atmi_status_t atmi_register_task_fini_buffer(task_process_fini_buffer_t fp) {
  return core::Runtime::getInstance().RegisterTaskFiniBuffer(fp);
}

/*
 * Tasks
 */
atmi_task_handle_t atmi_task_template_create(atmi_kernel_t atmi_kernel) {
  return core::Runtime::getInstance().CreateTaskTemplate(atmi_kernel);
}

atmi_task_handle_t atmi_task_template_activate(atmi_task_handle_t task, atmi_lparm_t *lparm,
                                               void **args) {
  return core::Runtime::getInstance().ActivateTaskTemplate(task, lparm, args);
}

atmi_task_handle_t atmi_task_create(atmi_lparm_t *lparm,
                                    atmi_kernel_t atmi_kernel,
                                    void **args) {
  return core::Runtime::getInstance().CreateTask(lparm, atmi_kernel, args);
}

atmi_task_handle_t atmi_task_activate(atmi_task_handle_t task) {
  return core::Runtime::getInstance().ActivateTask(task);
}

atmi_task_handle_t atmi_task_launch(atmi_lparm_t *lparm, atmi_kernel_t atmi_kernel,
                                    void **args/*, more params for place info? */) {
  return core::Runtime::getInstance().LaunchTask(lparm, atmi_kernel, args);
}

/*
 * Task groups
 */
atmi_status_t atmi_taskgroup_create(atmi_taskgroup_handle_t *group_handle,
                                     bool ordered,
                                     atmi_place_t place) {
  return core::Runtime::getInstance().TaskGroupCreate(group_handle, ordered, place);
}

atmi_status_t atmi_taskgroup_release(atmi_taskgroup_handle_t group_handle) {
  return core::Runtime::getInstance().TaskGroupRelease(group_handle);
}

/*
 * Data
 */
atmi_status_t atmi_memcpy(void *dest, const void *src, size_t size) {
  return core::Runtime::getInstance().Memcpy(dest, src, size);
}

atmi_task_handle_t atmi_memcpy_async(atmi_cparm_t *lparm, void *dest, const void *src, size_t size) {
  return core::Runtime::getInstance().MemcpyAsync(lparm, dest, src, size);
}

atmi_status_t atmi_free(void *ptr) {
  return core::Runtime::getInstance().Memfree(ptr);
}

atmi_status_t atmi_malloc(void **ptr, size_t size, atmi_mem_place_t place) {
  return core::Runtime::getInstance().Malloc(ptr, size, place);
}
