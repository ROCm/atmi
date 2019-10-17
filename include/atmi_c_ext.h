/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef INCLUDE_ATMI_C_EXT_H_
#define INCLUDE_ATMI_C_EXT_H_

#include <atmi.h>
/** \defgroup Helper macros when using ATMI C Extension feature.
 * @{
 */
#ifdef __cplusplus
#define _CPPSTRING_ "C"
#endif
#ifndef __cplusplus
#define _CPPSTRING_
#endif
/**
 * @brief \deprecated Predefined function calling a null CPU task. 
 */
extern _CPPSTRING_ atmi_task_handle_t __sync_kernel_pif(atmi_lparm_t* lparm);

/**
 * @brief \deprecated Helper macros calling a 
 * null CPU task under specific conditions.
 */
#define SYNC_STREAM(s)                            \
  {                                               \
    ATMI_LPARM(__lparm_sync_kernel);              \
    __lparm_sync_kernel->synchronous = ATMI_TRUE; \
    __lparm_sync_kernel->groupable = ATMI_TRUE;   \
    __lparm_sync_kernel->group = s;               \
    __sync_kernel_pif(__lparm_sync_kernel);       \
  }

#define SYNC_TASK(t)                              \
  {                                               \
    ATMI_LPARM(__lparm_sync_kernel);              \
    __lparm_sync_kernel->synchronous = ATMI_TRUE; \
    __lparm_sync_kernel->num_required = 1;        \
    __lparm_sync_kernel->requires = &t;           \
    __sync_kernel_pif(__lparm_sync_kernel);       \
  }
/**
 * @}
 */

#endif  // INCLUDE_ATMI_C_EXT_H_
