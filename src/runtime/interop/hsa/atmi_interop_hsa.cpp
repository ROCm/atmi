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
#include "atmi_interop_hsa.h"

extern hsa_agent_t get_compute_agent(atmi_place_t place);
extern hsa_amd_memory_pool_t get_memory_pool_by_mem_place(atmi_mem_place_t place);
extern bool atl_is_atmi_initialized();

atmi_status_t atmi_interop_hsa_get_agent(atmi_place_t proc, hsa_agent_t *agent) {
    if(!atl_is_atmi_initialized()) return ATMI_STATUS_ERROR;
    if(!agent) return ATMI_STATUS_ERROR;

    *agent = get_compute_agent(proc);
    return ATMI_STATUS_SUCCESS;
}

atmi_status_t atmi_interop_hsa_get_memory_pool(atmi_mem_place_t memory,
                                               hsa_amd_memory_pool_t *pool) {
    if(!atl_is_atmi_initialized()) return ATMI_STATUS_ERROR;
    if(!pool) return ATMI_STATUS_ERROR;

    *pool = get_memory_pool_by_mem_place(memory);
    return ATMI_STATUS_SUCCESS;
}
