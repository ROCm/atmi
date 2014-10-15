/* Copyright 2014 HSA Foundation Inc.  All Rights Reserved.
 *
 * HSAF is granting you permission to use this software and documentation (if
 * any) (collectively, the "Materials") pursuant to the terms and conditions
 * of the Software License Agreement included with the Materials.  If you do
 * not have a copy of the Software License Agreement, contact the  HSA Foundation for a copy.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hsa.h"
#include "hsa_ext_finalize.h"
#include "../common/elf_utils.h"

#define check(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed.\n", #msg); \
    exit(1); \
} else { \
   printf("%s succeeded.\n", #msg); \
}

#define GRID_SIZE_X 1024*1024
#define GROUP_SIZE_X 256

/*
 * Define required BRIG data structures.
 */

typedef uint32_t BrigCodeOffset32_t;

typedef uint32_t BrigDataOffset32_t;

typedef uint16_t BrigKinds16_t;

typedef uint8_t BrigLinkage8_t;

typedef uint8_t BrigExecutableModifier8_t;

typedef BrigDataOffset32_t BrigDataOffsetString32_t;

enum BrigKinds {
    BRIG_KIND_NONE = 0x0000,
    BRIG_KIND_DIRECTIVE_BEGIN = 0x1000,
    BRIG_KIND_DIRECTIVE_KERNEL = 0x1008,
};

typedef struct BrigBase BrigBase;
struct BrigBase {
    uint16_t byteCount;
    BrigKinds16_t kind;
};

typedef struct BrigExecutableModifier BrigExecutableModifier;
struct BrigExecutableModifier {
    BrigExecutableModifier8_t allBits;
};

typedef struct BrigDirectiveExecutable BrigDirectiveExecutable;
struct BrigDirectiveExecutable {
    uint16_t byteCount;
    BrigKinds16_t kind;
    BrigDataOffsetString32_t name;
    uint16_t outArgCount;
    uint16_t inArgCount;
    BrigCodeOffset32_t firstInArg;
    BrigCodeOffset32_t firstCodeBlockEntry;
    BrigCodeOffset32_t nextModuleEntry;
    uint32_t codeBlockEntryCount;
    BrigExecutableModifier modifier;
    BrigLinkage8_t linkage;
    uint16_t reserved;
};

typedef struct BrigData BrigData;
struct BrigData {
    uint32_t byteCount;
    uint8_t bytes[1];
};

/*
 * Determines if the given agent is of type HSA_DEVICE_TYPE_GPU
 * and sets the value of data to the agent handle if it is.
 */
static hsa_status_t find_gpu(hsa_agent_t agent, void *data) {
    if (data == NULL) {
        return HSA_STATUS_ERROR_INVALID_ARGUMENT;
    }
    hsa_device_type_t device_type;
    hsa_status_t stat =
    hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
    if (stat != HSA_STATUS_SUCCESS) {
        return stat;
    }
    if (device_type == HSA_DEVICE_TYPE_GPU) {
        *((hsa_agent_t *)data) = agent;
    }
    return HSA_STATUS_SUCCESS;
}

/*
 * Determines if a memory region can be used for kernarg
 * allocations.
 */
static hsa_status_t get_kernarg(hsa_region_t region, void* data) {
    hsa_region_flag_t flags;
    hsa_region_get_info(region, HSA_REGION_INFO_FLAGS, &flags);
    if (flags & HSA_REGION_FLAG_KERNARG) {
        hsa_region_t* ret = (hsa_region_t*) data;
        *ret = region;
        return HSA_STATUS_SUCCESS;
    }
    return HSA_STATUS_SUCCESS;
}

/*
 * Finds the specified symbols offset in the specified brig_module.
 * If the symbol is found the function returns HSA_STATUS_SUCCESS, 
 * otherwise it returns HSA_STATUS_ERROR.
 */
hsa_status_t find_symbol_offset(hsa_ext_brig_module_t* brig_module, 
    char* symbol_name,
    hsa_ext_brig_code_section_offset32_t* offset) {
    
    /* 
     * Get the data section 
     */
    hsa_ext_brig_section_header_t* data_section_header = 
                brig_module->section[HSA_EXT_BRIG_SECTION_DATA];
    /* 
     * Get the code section
     */
    hsa_ext_brig_section_header_t* code_section_header =
             brig_module->section[HSA_EXT_BRIG_SECTION_CODE];

    /* 
     * First entry into the BRIG code section
     */
    BrigCodeOffset32_t code_offset = code_section_header->header_byte_count;
    BrigBase* code_entry = (BrigBase*) ((char*)code_section_header + code_offset);
    while (code_offset != code_section_header->byte_count) {
        if (code_entry->kind == BRIG_KIND_DIRECTIVE_KERNEL) {
            /* 
             * Now find the data in the data section
             */
            BrigDirectiveExecutable* directive_kernel = (BrigDirectiveExecutable*) (code_entry);
            BrigDataOffsetString32_t data_name_offset = directive_kernel->name;
            BrigData* data_entry = (BrigData*)((char*) data_section_header + data_name_offset);
            if (!strncmp(symbol_name, (char*)data_entry->bytes, strlen(symbol_name))){
                *offset = code_offset;
                return HSA_STATUS_SUCCESS;
            }
        }
        code_offset += code_entry->byteCount;
        code_entry = (BrigBase*) ((char*)code_section_header + code_offset);
    }
    return HSA_STATUS_ERROR;
}

int main(int argc, char **argv)
{
    hsa_status_t err;

    err = hsa_init();
    check(Initializing the hsa runtime, err);

    /* 
     * Iterate over the agents and pick the gpu agent using 
     * the find_gpu callback.
     */
    hsa_agent_t device = 0;
    err = hsa_iterate_agents(find_gpu, &device);
    check(Calling hsa_iterate_agents, err);

    err = (device == 0) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    check(Checking if the GPU device is non-zero, err);

    /*
     * Query the name of the device.
     */
    char name[64] = { 0 };
    err = hsa_agent_get_info(device, HSA_AGENT_INFO_NAME, name);
    check(Querying the device name, err);
    printf("The device name is %s.\n", name);

    /*
     * Query the maximum size of the queue.
     */
    uint32_t queue_size = 0;
    err = hsa_agent_get_info(device, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size);
    check(Querying the device maximum queue size, err);
    printf("The maximum queue size is %u.\n", (unsigned int) queue_size);

    /*
     * Create a queue using the maximum size.
     */
    hsa_queue_t* commandQueue;
    err = hsa_queue_create(device, queue_size, HSA_QUEUE_TYPE_MULTI, NULL, NULL, &commandQueue);
    check(Creating the queue, err);

    /*
     * Load BRIG, encapsulated in an ELF container, into a BRIG module.
     */
    hsa_ext_brig_module_t* brigModule;
    char file_name[128] = "vector_copy.brig";
    err = create_brig_module_from_brig_file(file_name, &brigModule);
    check(Creating the brig module from vector_copy.brig, err);

    /*
     * Create hsa program.
     */
    hsa_ext_program_handle_t hsaProgram;
    err = hsa_ext_program_create(&device, 1, HSA_EXT_BRIG_MACHINE_LARGE, HSA_EXT_BRIG_PROFILE_FULL, &hsaProgram);
    check(Creating the hsa program, err);

    /*
     * Add the BRIG module to hsa program.
     */
    hsa_ext_brig_module_handle_t module;
    err = hsa_ext_add_module(hsaProgram, brigModule, &module);
    check(Adding the brig module to the program, err);

    /* 
     * Construct finalization request list.
     */
    hsa_ext_finalization_request_t finalization_request_list;
    finalization_request_list.module = module;
    finalization_request_list.program_call_convention = 0;
    char kernel_name[128] = "&__OpenCL_vector_copy_kernel";
    err = find_symbol_offset(brigModule, kernel_name, &finalization_request_list.symbol);
    check(Finding the symbol offset for the kernel, err);

    /*
     * Finalize the hsa program.
     */
    err = hsa_ext_finalize_program(hsaProgram, device, 1, &finalization_request_list, NULL, NULL, 0, NULL, 0);
    check(Finalizing the program, err);

    /*
     * Get the hsa code descriptor address.
     */
    hsa_ext_code_descriptor_t *hsaCodeDescriptor;
    err = hsa_ext_query_kernel_descriptor_address(hsaProgram, module, finalization_request_list.symbol, &hsaCodeDescriptor);
    check(Querying the kernel descriptor address, err);

    /*
     * Create a signal to wait for the dispatch to finish.
     */ 
    hsa_signal_t signal;
    err=hsa_signal_create(1, 0, NULL, &signal);
    check(Creating a HSA signal, err);

    /*
     * Initialize the dispatch packet.
     */
    hsa_dispatch_packet_t aql;
    memset(&aql, 0, sizeof(aql));

    /*
     * Setup the dispatch information.
     */
    aql.completion_signal=signal;
    aql.dimensions=1;
    aql.workgroup_size_x=GROUP_SIZE_X;
    aql.workgroup_size_y=1;
    aql.workgroup_size_z=1;
    aql.grid_size_x=GRID_SIZE_X;
    aql.grid_size_y=1;
    aql.grid_size_z=1;
    aql.header.type=HSA_PACKET_TYPE_DISPATCH;
    aql.header.acquire_fence_scope=2;
    aql.header.release_fence_scope=2;
    aql.header.barrier=1;
    aql.group_segment_size=0;
    aql.private_segment_size=0;
    
    /*
     * Allocate and initialize the kernel arguments.
     */
    uint64_t total_buffer_size = GRID_SIZE_X * sizeof(int);
    int* in=(int*)malloc(total_buffer_size);
    memset(in, 1, total_buffer_size); 
    err=hsa_memory_register(in, total_buffer_size);
    check(Registering argument memory for input parameter, err);
    int* out=(int*)malloc(total_buffer_size);
    memset(out, 0, total_buffer_size);
    err=hsa_memory_register(out, total_buffer_size);
    check(Registering argument memory for output parameter, err);
    /*
     * Find a memory region that supports kernel arguments.
     */
    hsa_region_t kernarg_region = 0;
    hsa_agent_iterate_regions(device, get_kernarg, &kernarg_region);
    err = (kernarg_region == 0) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    check(Finding a kernarg memory region, err);
    void* kernel_arg_buffer = NULL;
   
    size_t kernel_arg_buffer_size = hsaCodeDescriptor->kernarg_segment_byte_size;
    /*
     * Allocate the kernel argument buffer from the correct region.
     */   
    err = hsa_memory_allocate(kernarg_region, kernel_arg_buffer_size, 
                        &kernel_arg_buffer);
    check(Allocating kernel argument memory buffer, err);
    uint64_t kernel_arg_start_offset = 0;
#ifdef DUMMY_ARGS
    //This flags should be set if HSA_HLC_Stable is used
    // This is because the high level compiler generates 6 extra args
    kernel_arg_start_offset += sizeof(uint64_t) * 6;
    printf("Using dummy args \n");
#endif
    memset(kernel_arg_buffer, 0, kernel_arg_buffer_size);
    void *kernel_arg_buffer_start = 
        (char*)kernel_arg_buffer + kernel_arg_start_offset;
    memcpy(kernel_arg_buffer_start, &in, sizeof(void*));
    memcpy(kernel_arg_buffer_start + sizeof(void*), &out, sizeof(void*));
 
    /*
     * Bind kernel code and the kernel argument buffer to the
     * aql packet.
     */
    aql.kernel_object_address=hsaCodeDescriptor->code.handle;
    aql.kernarg_address=(uint64_t)kernel_arg_buffer;

    /*
     * Obtain the current queue write index.
     */
    uint64_t index = hsa_queue_load_write_index_relaxed(commandQueue);

    /*
     * Write the aql packet at the calculated queue index address.
     */
    const uint32_t queueMask = commandQueue->size - 1;
    ((hsa_dispatch_packet_t*)(commandQueue->base_address))[index&queueMask]=aql;

    /*
     * Increment the write index and ring the doorbell to dispatch the kernel.
     */
    hsa_queue_store_write_index_relaxed(commandQueue, index+1);
    hsa_signal_store_relaxed(commandQueue->doorbell_signal, index);
    check(Dispatching the kernel, err);

    /*
     * Wait on the dispatch signal until the kernel is finished.
     */
    err = hsa_signal_wait_acquire(signal, HSA_LT, 1, (uint64_t) -1, HSA_WAIT_EXPECTANCY_UNKNOWN);
    check(Wating on the dispatch signal, err);

    /*
     * Validate the data in the output buffer.
     */
    int valid = memcmp(in, out, total_buffer_size);
    if(!valid) {
        printf("Passed validation.\n");
    } else {
        printf("Failed Validation %d!\n", valid);
    }

    /*
     * Cleanup all allocated resources.
     */
    destroy_brig_module(brigModule);
    err=hsa_signal_destroy(signal);
    check(Destroying the signal, err);

    err=hsa_ext_program_destroy(hsaProgram);
    check(Destroying the program, err);

    err=hsa_queue_destroy(commandQueue);
    check(Destroying the queue, err);
    
    err=hsa_shut_down();
    check(Shutting down the runtime, err);

    free(in);
    free(out);

    return 0;
}
