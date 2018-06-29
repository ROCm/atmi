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
#include "atl_internal.h"

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

atmi_status_t atmi_interop_hsa_get_symbol_info(atmi_mem_place_t place, 
        const char *symbol, void **var_addr, unsigned int *var_size) {
    /*
       // Typical usage:
       void *var_addr;
       size_t var_size;
       atmi_interop_hsa_get_symbol_addr(gpu_place, "symbol_name", &var_addr, &var_size);
       atmi_memcpy(host_add, var_addr, var_size);
    */

    if(!atl_is_atmi_initialized()) return ATMI_STATUS_ERROR;
    atmi_machine_t *machine = atmi_machine_get_info();
    if(!symbol || !var_addr || !var_size || !machine) return ATMI_STATUS_ERROR;
    if(place.dev_id < 0 || 
        place.dev_id >= machine->device_count_by_type[place.dev_type])
        return ATMI_STATUS_ERROR;

    // get the symbol info
    std::string symbolStr = std::string(symbol);
    if(SymbolInfoTable[place.dev_id].find(symbolStr) != SymbolInfoTable[place.dev_id].end()) {
        atl_symbol_info_t info = SymbolInfoTable[place.dev_id][symbolStr];
        *var_addr = (void *)info.addr;
        *var_size = info.size;
        return ATMI_STATUS_SUCCESS;
    }
    else {
        *var_addr = NULL;
        *var_size = 0;
        return ATMI_STATUS_ERROR;
    }
}

atmi_status_t atmi_interop_hsa_get_kernel_info(atmi_mem_place_t place, 
        const char *kernel_name, hsa_executable_symbol_info_t kernel_info, uint32_t *value) {
    /*
       // Typical usage:
       uint32_t value;
       atmi_interop_hsa_get_kernel_addr(gpu_place, "kernel_name", 
                                    HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE,
                                    &val);
    */

    if(!atl_is_atmi_initialized()) return ATMI_STATUS_ERROR;
    atmi_machine_t *machine = atmi_machine_get_info();
    if(!kernel_name || !value || !machine) return ATMI_STATUS_ERROR;
    if(place.dev_id < 0 || 
        place.dev_id >= machine->device_count_by_type[place.dev_type])
        return ATMI_STATUS_ERROR;

    atmi_status_t status = ATMI_STATUS_SUCCESS;
    // get the kernel info
    std::string kernelStr = std::string(kernel_name);
    if(KernelInfoTable[place.dev_id].find(kernelStr) != KernelInfoTable[place.dev_id].end()) {
        atl_kernel_info_t info = KernelInfoTable[place.dev_id][kernelStr];
        switch(kernel_info) {
            case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE:
                *value = info.group_segment_size;
                break;
            case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE:
                *value = info.private_segment_size;
                break;
            case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE:
                *value = info.kernel_segment_size;
                break;
            default:
                *value = 0;
                status = ATMI_STATUS_ERROR;
                break;
        }
    }
    else {
        *value = 0;
        status = ATMI_STATUS_ERROR;
    }
    
    return status;
}
