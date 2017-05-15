/*
MIT License 

Copyright © 2016 Advanced Micro Devices, Inc.  

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef __ATMI_INTEROP_HSA_H__
#define __ATMI_INTEROP_HSA_H__

#include "atmi_runtime.h"
#include "hsa.h"
#include "hsa_ext_amd.h"

/** \defgroup interop_hsa_functions ATMI-HSA Interop 
 *  @{
 */
/**
 * @brief Get the HSA compute agent from the ATMI compute place.
 *
 * @detail Use this function to query more details about the underlying HSA agent.
 * 
 * @param[in] proc The ATMI compute place
 *
 * @param[in] agent Pointer to a non-NULL @p hsa_agent_t structure that will hold the
 * return value. 
 *
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR If @p proc is an invalid location in the current node, or
 * if ATMI is not initialized.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 */
atmi_status_t atmi_interop_hsa_get_agent(atmi_place_t proc, hsa_agent_t *agent);

/**
 * @brief Get the HSA memory pool handle from the ATMI memory place.
 *
 * @detail Use this function to query more details about the underlying HSA memory
 * pool handle.
 * 
 * @param[in] memory The ATMI memory place
 *
 * @param[in] pool Pointer to a non-NULL @p hsa_amd_memory_pool_t structure that will 
 * hold the return value. 
 *
 * @retval ::ATMI_STATUS_SUCCESS The function has executed successfully.
 *
 * @retval ::ATMI_STATUS_ERROR If @p memory is an invalid location in the current node, or
 * if ATMI is not initialized.
 * 
 * @retval ::ATMI_STATUS_UNKNOWN The function encountered errors.
 */
atmi_status_t atmi_interop_hsa_get_memory_pool(atmi_mem_place_t memory,
                                               hsa_amd_memory_pool_t *pool);
/** @} */

#endif // __ATMI_INTEROP_HSA_H__
