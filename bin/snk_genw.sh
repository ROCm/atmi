#!/bin/bash
#
#  snk_genw.sh: Part of snack that generates the user callable wrapper functions.
#
#  Written by Greg Rodgers  Gregory.Rodgers@amd.com
#  Maintained by Shreyas Ramalingam Shreyas.Ramalingam@amd.com
#
# Copyright (c) 2015 ADVANCED MICRO DEVICES, INC.  
# 
# AMD is granting you permission to use this software and documentation (if any) (collectively, the 
# Materials) pursuant to the terms and conditions of the Software License Agreement included with the 
# Materials.  If you do not have a copy of the Software License Agreement, contact your AMD 
# representative for a copy.
# 
# You agree that you will not reverse engineer or decompile the Materials, in whole or in part, except for 
# example code which is provided in source code form and as allowed by applicable law.
# 
# WARRANTY DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
# KIND.  AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT 
# LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
# PURPOSE, TITLE, NON-INFRINGEMENT, THAT THE SOFTWARE WILL RUN UNINTERRUPTED OR ERROR-
# FREE OR WARRANTIES ARISING FROM CUSTOM OF TRADE OR COURSE OF USAGE.  THE ENTIRE RISK 
# ASSOCIATED WITH THE USE OF THE SOFTWARE IS ASSUMED BY YOU.  Some jurisdictions do not 
# allow the exclusion of implied warranties, so the above exclusion may not apply to You. 
# 
# LIMITATION OF LIABILITY AND INDEMNIFICATION:  AMD AND ITS LICENSORS WILL NOT, 
# UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT, INCIDENTAL, 
# INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF THE SOFTWARE OR THIS 
# AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH 
# DAMAGES.  In no event shall AMD's total liability to You for all damages, losses, and 
# causes of action (whether in contract, tort (including negligence) or otherwise) 
# exceed the amount of $100 USD.  You agree to defend, indemnify and hold harmless 
# AMD and its licensors, and any of their directors, officers, employees, affiliates or 
# agents from and against any and all loss, damage, liability and other expenses 
# (including reasonable attorneys' fees), resulting from Your use of the Software or 
# violation of the terms and conditions of this Agreement.  
# 
# U.S. GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with "RESTRICTED RIGHTS." 
# Use, duplication, or disclosure by the Government is subject to the restrictions as set 
# forth in FAR 52.227-14 and DFAR252.227-7013, et seq., or its successor.  Use of the 
# Materials by the Government constitutes acknowledgement of AMD's proprietary rights in them.
# 
# EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as stated in the 
# Software License Agreement.
# 

function write_copyright_template(){
/bin/cat  <<"EOF"
/*

  Copyright (c) 2015 ADVANCED MICRO DEVICES, INC.  

  AMD is granting you permission to use this software and documentation (if any) (collectively, the 
  Materials) pursuant to the terms and conditions of the Software License Agreement included with the 
  Materials.  If you do not have a copy of the Software License Agreement, contact your AMD 
  representative for a copy.

  You agree that you will not reverse engineer or decompile the Materials, in whole or in part, except for 
  example code which is provided in source code form and as allowed by applicable law.

  WARRANTY DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
  KIND.  AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT 
  LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
  PURPOSE, TITLE, NON-INFRINGEMENT, THAT THE SOFTWARE WILL RUN UNINTERRUPTED OR ERROR-
  FREE OR WARRANTIES ARISING FROM CUSTOM OF TRADE OR COURSE OF USAGE.  THE ENTIRE RISK 
  ASSOCIATED WITH THE USE OF THE SOFTWARE IS ASSUMED BY YOU.  Some jurisdictions do not 
  allow the exclusion of implied warranties, so the above exclusion may not apply to You. 

  LIMITATION OF LIABILITY AND INDEMNIFICATION:  AMD AND ITS LICENSORS WILL NOT, 
  UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT, INCIDENTAL, 
  INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF THE SOFTWARE OR THIS 
  AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH 
  DAMAGES.  In no event shall AMD's total liability to You for all damages, losses, and 
  causes of action (whether in contract, tort (including negligence) or otherwise) 
  exceed the amount of $100 USD.  You agree to defend, indemnify and hold harmless 
  AMD and its licensors, and any of their directors, officers, employees, affiliates or 
  agents from and against any and all loss, damage, liability and other expenses 
  (including reasonable attorneys' fees), resulting from Your use of the Software or 
  violation of the terms and conditions of this Agreement.  

  U.S. GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with "RESTRICTED RIGHTS." 
  Use, duplication, or disclosure by the Government is subject to the restrictions as set 
  forth in FAR 52.227-14 and DFAR252.227-7013, et seq., or its successor.  Use of the 
  Materials by the Government constitutes acknowledgement of AMD's proprietary rights in them.

  EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as stated in the 
  Software License Agreement.

*/ 

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <libelf.h>
#include "hsa.h"
#include "hsa_ext_finalize.h"

/*  set NOTCOHERENT needs this include
#include "hsa_ext_amd.h"
*/

typedef enum status_t status_t;
enum status_t {
    STATUS_SUCCESS=0,
    STATUS_KERNEL_INVALID_SECTION_HEADER=1,
    STATUS_KERNEL_ELF_INITIALIZATION_FAILED=2,
    STATUS_KERNEL_INVALID_ELF_CONTAINER=3,
    STATUS_KERNEL_MISSING_DATA_SECTION=4,
    STATUS_KERNEL_MISSING_CODE_SECTION=5,
    STATUS_KERNEL_MISSING_OPERAND_SECTION=6,
    STATUS_UNKNOWN=7,
};
EOF
}

function write_header_template(){
/bin/cat  <<"EOF"
#ifdef __cplusplus
#define _CPPSTRING_ "C" 
#endif
#ifndef __cplusplus
#define _CPPSTRING_ 
#endif
#ifndef __SNK_DEFS
#define SNK_MAX_STREAMS 8 
extern _CPPSTRING_ void stream_sync(const int stream_num);

#define SNK_MAXEDGESIN 10
#define SNK_MAXEDGESOUT 10
#define SNK_ORDERED 1
#define SNK_UNORDERED 0
#define SNK_GPU 0
#define SNK_SIM 1
#define SNK_CPU 2

typedef struct snk_lparm_s snk_lparm_t;
struct snk_lparm_s { 
   int ndim;                         /* default = 1 */
   size_t gdims[3];                  /* NUMBER OF THREADS TO EXECUTE MUST BE SPECIFIED */ 
   size_t ldims[3];                  /* Default = {64} , e.g. 1 of 8 CU on Kaveri */
   int stream;                       /* default = -1 , synchrnous */
   int barrier;                      /* default = SNK_ORDERED */
   int acquire_fence_scope;          /* default = 2 */
   int release_fence_scope;          /* default = 2 */
   int num_edges_in;                 /*  not yet implemented */
   int num_edges_out;                /*  not yet implemented */
   int * edges_in;                   /*  not yet implemented */
   int * edges_out;                  /*  not yet implemented */
   int devtype;                      /*  not yet implemented-default=SNK_GPU */
   int rank;                         /*  not yet implemented-used for MPI work sharing */
} ;

/* This string macro is used to declare launch parameters set default values  */
#define SNK_INIT_LPARM(X,Y) snk_lparm_t * X ; snk_lparm_t  _ ## X ={.ndim=1,.gdims={Y},.ldims={64},.stream=-1,.barrier=SNK_ORDERED,.acquire_fence_scope=2,.release_fence_scope=2,.num_edges_in=0,.num_edges_out=0,.edges_in=NULL,.edges_out=NULL,.devtype=SNK_GPU,.rank=0} ; X = &_ ## X ;
 
/* Equivalent host data types for kernel data types */
typedef struct snk_image3d_s snk_image3d_t;
struct snk_image3d_s { 
   unsigned int channel_order; 
   unsigned int channel_data_type; 
   size_t width, height, depth;
   size_t row_pitch, slice_pitch;
   size_t element_size;
   void *data;
};

#define __SNK_DEFS
#endif
EOF
}
function write_global_functions_template(){
/bin/cat  <<"EOF"

extern void stream_sync(int stream_num) {

    hsa_queue_t *queue = Stream_CommandQ[stream_num];
    hsa_signal_t signal = Stream_Signal[stream_num];

    hsa_barrier_packet_t barrier;
    memset (&barrier, 0, sizeof(hsa_barrier_packet_t));
    barrier.header.type=HSA_PACKET_TYPE_BARRIER;
    barrier.header.acquire_fence_scope=2;
    barrier.header.release_fence_scope=2;
    barrier.header.barrier=1;
    barrier.completion_signal = signal;

    uint64_t index = hsa_queue_load_write_index_relaxed(queue);
    const uint32_t queue_mask = queue->size - 1;
    ((hsa_barrier_packet_t*)(queue->base_address))[index&queue_mask]=barrier; 
    hsa_queue_store_write_index_relaxed(queue,index+1);
    //Ring the doorbell.
    hsa_signal_store_relaxed(queue->doorbell_signal, index);

    //Wait for completion signal
    /* printf("DEBUG STREAM_SYNC:Call #%d for stream %d \n",(int) index,stream_num);  */
    hsa_signal_wait_acquire(signal, HSA_LT, 1, (uint64_t) -1, HSA_WAIT_EXPECTANCY_UNKNOWN);
}


EOF
}

function write_context_template(){
/bin/cat  <<"EOF"
static Elf_Scn* snk_extract_elf_sect (Elf *elfP, Elf_Data *secHdr, char const *brigName, char const *bifName) {
    int cnt = 0;
    Elf_Scn* scn = NULL;
    Elf32_Shdr* shdr = NULL;
    char* sectionName = NULL;

    /* Iterate thru the elf sections */
    for (cnt = 1, scn = NULL; scn = elf_nextscn(elfP, scn); cnt++) {
        if (((shdr = elf32_getshdr(scn)) == NULL)) {
            return NULL;
        }
        sectionName = (char *)secHdr->d_buf + shdr->sh_name;
        if (sectionName &&
           ((strcmp(sectionName, brigName) == 0) ||
           (strcmp(sectionName, bifName) == 0))) {
            return scn;
        }
     }

     return NULL;
}

/* Extract section and copy into HsaBrig */
static status_t snk_CopyElfSectToModule (Elf *elfP, Elf_Data *secHdr, char const *brigName, char const *bifName, 
                                       hsa_ext_brig_module_t* brig_module,
                                       hsa_ext_brig_section_id_t section_id) {
    Elf_Scn* scn = NULL;
    Elf_Data* data = NULL;
    void* address_to_copy;
    size_t section_size=0;

    scn = snk_extract_elf_sect(elfP, secHdr, brigName, bifName);

    if (scn) {
        if ((data = elf_getdata(scn, NULL)) == NULL) {
            return STATUS_UNKNOWN;
        }
        section_size = data->d_size;
        if (section_size > 0) {
          address_to_copy = malloc(section_size);
          memcpy(address_to_copy, data->d_buf, section_size);
        }
    }

    if ((!scn ||  section_size == 0))  { return STATUS_UNKNOWN; }

    /* Create a section header */
    brig_module->section[section_id] = (hsa_ext_brig_section_header_t*) address_to_copy; 

    return STATUS_SUCCESS;
} 

/* Reads binary of BRIG and BIF format */
static status_t snk_ReadBinary(hsa_ext_brig_module_t **brig_module_t, char* binary, size_t binsz) {
    /* Create the brig_module */
    uint32_t number_of_sections = 3;
    hsa_ext_brig_module_t* brig_module;

    brig_module = (hsa_ext_brig_module_t*)
                  (malloc (sizeof(hsa_ext_brig_module_t) + sizeof(void*)*number_of_sections));
    brig_module->section_count = number_of_sections;

    status_t status;
    Elf* elfP = NULL;
    Elf32_Ehdr* ehdr = NULL;
    Elf_Data *secHdr = NULL;
    Elf_Scn* scn = NULL;

    if (elf_version ( EV_CURRENT ) == EV_NONE) { return STATUS_KERNEL_ELF_INITIALIZATION_FAILED; } 
    if ((elfP = elf_memory(binary,binsz)) == NULL) { return STATUS_KERNEL_INVALID_ELF_CONTAINER; }
    if (elf_kind (elfP) != ELF_K_ELF) { return STATUS_KERNEL_INVALID_ELF_CONTAINER; }
  
    if (((ehdr = elf32_getehdr(elfP)) == NULL) ||
       ((scn = elf_getscn(elfP, ehdr->e_shstrndx)) == NULL) ||
       ((secHdr = elf_getdata(scn, NULL)) == NULL)) {
        return STATUS_KERNEL_INVALID_SECTION_HEADER;
    }

    status = snk_CopyElfSectToModule(elfP, secHdr,"hsa_data",".brig_hsa_data",
                                   brig_module, HSA_EXT_BRIG_SECTION_DATA);
    if (status != STATUS_SUCCESS) { return STATUS_KERNEL_MISSING_DATA_SECTION; }

    status = snk_CopyElfSectToModule(elfP, secHdr, "hsa_code",".brig_hsa_code",
                                   brig_module, HSA_EXT_BRIG_SECTION_CODE);
    if (status != STATUS_SUCCESS) { return STATUS_KERNEL_MISSING_CODE_SECTION; }

    status = snk_CopyElfSectToModule(elfP, secHdr, "hsa_operand",".brig_hsa_operand",
                                   brig_module, HSA_EXT_BRIG_SECTION_OPERAND);
    if (status != STATUS_SUCCESS) { return STATUS_KERNEL_MISSING_OPERAND_SECTION; }

    elf_end(elfP);
    *brig_module_t = brig_module;

    return STATUS_SUCCESS;
}


#define ErrorCheck(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed.\n", #msg); \
    exit(1); \
} else { \
 /*  printf("%s succeeded.\n", #msg);*/ \
}

/*  Define required BRIG data structures.  */
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
static hsa_status_t snk_FindGPU(hsa_agent_t agent, void *data) {
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

/*  Determines if a memory region can be used for kernarg allocations.  */
static hsa_status_t snk_GetKernArrg(hsa_region_t region, void* data) {
    hsa_region_flag_t flags;
    hsa_region_get_info(region, HSA_REGION_INFO_FLAGS, &flags);
    if (flags & HSA_REGION_FLAG_KERNARG) {
        hsa_region_t* ret = (hsa_region_t*) data;
        *ret = region;
        return HSA_STATUS_SUCCESS;
    }
    return HSA_STATUS_SUCCESS;
}

/*  Determines if a memory region is device memory */
static hsa_status_t snk_GetDevRegion(hsa_region_t region, void* data) {
    hsa_segment_t segment;
    hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT , &segment);
    if (segment & HSA_SEGMENT_GROUP ) {
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
static hsa_status_t snk_FindSymbolOffset(hsa_ext_brig_module_t* brig_module, const char* symbol_name,
    hsa_ext_brig_code_section_offset32_t* offset) {
    
    /*  Get the data section */
    hsa_ext_brig_section_header_t* data_section_header = 
                brig_module->section[HSA_EXT_BRIG_SECTION_DATA];
    /*  Get the code section */
    hsa_ext_brig_section_header_t* code_section_header =
             brig_module->section[HSA_EXT_BRIG_SECTION_CODE];

    /*  First entry into the BRIG code section */
    BrigCodeOffset32_t code_offset = code_section_header->header_byte_count;
    BrigBase* code_entry = (BrigBase*) ((char*)code_section_header + code_offset);
    while (code_offset != code_section_header->byte_count) {
        if (code_entry->kind == BRIG_KIND_DIRECTIVE_KERNEL) {
            /*  Now find the data in the data section */
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

/* Stream specific globals */
hsa_signal_t   Stream_Signal[SNK_MAX_STREAMS];
hsa_queue_t*   Stream_CommandQ[SNK_MAX_STREAMS];


/* Context(cl file) specific globals */
hsa_ext_brig_module_t*           _CN__BrigModule;
hsa_agent_t                      _CN__Device;
hsa_ext_program_handle_t         _CN__HsaProgram;
hsa_ext_brig_module_handle_t     _CN__ModuleHandle;
int                              _CN__FC = 0; 

/* Global variables */
hsa_queue_t*                     Sync_CommandQ;
hsa_signal_t                     Sync_Signal; 
#include "_CN__brig.h" 

status_t _CN__InitContext(){

    hsa_status_t err;

    err = hsa_init();
    ErrorCheck(Initializing the hsa runtime, err);

    /*  Iterate over the agents and pick the gpu agent */
    _CN__Device = 0;
    err = hsa_iterate_agents(snk_FindGPU, &_CN__Device);
    ErrorCheck(Calling hsa_iterate_agents, err);

    err = (_CN__Device == 0) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    ErrorCheck(Checking if the GPU device is non-zero, err);
/*
    err = hsa_ext_set_memory_type(_CN__Device, HSA_EXT_MEMORY_TYPE_COHERENT );
    ErrorCheck(Calling hsa_ext_set_memory_type, err);
*/

    /*  Query the name of the device.  */
    char name[64] = { 0 };
    err = hsa_agent_get_info(_CN__Device, HSA_AGENT_INFO_NAME, name);
    ErrorCheck(Querying the device name, err);
/*
    printf("The device name is %s.\n", name);  
*/
    /*  Load BRIG, encapsulated in an ELF container, into a BRIG module.  */
    status_t status = snk_ReadBinary(&_CN__BrigModule,HSA_BrigMem,HSA_BrigMemSz);
    if (status != STATUS_SUCCESS) {
        printf("Could not create BRIG module: %d\n", status);
        if (status == STATUS_KERNEL_INVALID_SECTION_HEADER || 
            status == STATUS_KERNEL_ELF_INITIALIZATION_FAILED || 
            status == STATUS_KERNEL_INVALID_ELF_CONTAINER) {
            printf("The ELF file is invalid or possibley corrupted.\n");
        }
        if (status == STATUS_KERNEL_MISSING_DATA_SECTION ||
            status == STATUS_KERNEL_MISSING_CODE_SECTION ||
            status == STATUS_KERNEL_MISSING_OPERAND_SECTION) {
            printf("One or more ELF sections are missing. Use readelf command to \
            to check if hsa_data, hsa_code and hsa_operands exist.\n");
        }
    }

    /*  Create hsa program for this context */
    err = hsa_ext_program_create(&_CN__Device, 1, HSA_EXT_BRIG_MACHINE_LARGE, HSA_EXT_BRIG_PROFILE_FULL, &_CN__HsaProgram);
    ErrorCheck(Creating the hsa program, err);

    /*  Add the BRIG module to this hsa program.  */
    err = hsa_ext_add_module(_CN__HsaProgram, _CN__BrigModule, &_CN__ModuleHandle);
    ErrorCheck(Adding the brig module to the program, err);

    /*  Query the maximum size of the queue.  */
    uint32_t queue_size = 0;
    err = hsa_agent_get_info(_CN__Device, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size);
    ErrorCheck(Querying the device maximum queue size, err);

    /* printf("DEBUG: The maximum queue size is %u.\n", (unsigned int) queue_size);  */

    /*  Create a queue using the maximum size.  */
    err = hsa_queue_create(_CN__Device, queue_size, HSA_QUEUE_TYPE_MULTI, NULL, NULL, &Sync_CommandQ);
    ErrorCheck(Creating the queue, err);

    /*  Create signal to wait for the dispatch to finish. this Signal is only used for synchronous execution  */ 
    err=hsa_signal_create(1, 0, NULL, &Sync_Signal);
    ErrorCheck(Creating a HSA signal, err);

    /*  Create queues and signals for each stream */
    int stream_num;
    for ( stream_num = 0 ; stream_num < SNK_MAX_STREAMS ; stream_num++){

       /* printf("calling queue create for stream %d\n",stream_num); */
       err = hsa_queue_create(_CN__Device, queue_size, HSA_QUEUE_TYPE_MULTI, NULL, NULL, &Stream_CommandQ[stream_num]);
       ErrorCheck(Creating the Stream Command Q, err);

       /*  Create signal to wait for the dispatch to finish. this Signal is only used for synchronous execution  */ 
       err=hsa_signal_create(1, 0, NULL, &Stream_Signal[stream_num]);
       ErrorCheck(Creating the Stream Signal, err);
    }

    return STATUS_SUCCESS;
} /* end of __CN__InitContext */

EOF
}

function write_KernelStatics_template(){
/bin/cat <<"EOF"

/* Kernel specific globals, one set for each kernel  */
hsa_ext_code_descriptor_t*       _KN__HsaCodeDescriptor;
void*                            _KN__kernel_arg_buffer = NULL; /* Only for syncrhnous calls */  
size_t                           _KN__kernel_arg_buffer_size ;  
hsa_ext_finalization_request_t   _KN__FinalizationRequestList;
int                              _KN__FK = 0 ; 
status_t                         _KN__init();
status_t                         _KN__stop();

EOF
}

function write_InitKernel_template(){
/bin/cat <<"EOF"
extern status_t _KN__init(){

    if (_CN__FC == 0 ) {
       status_t status = _CN__InitContext();
       if ( status  != STATUS_SUCCESS ) return; 
       _CN__FC = 1;
    }
   
    hsa_status_t err;

    /*  Construct finalization request list for this kernel.  */
    _KN__FinalizationRequestList.module = _CN__ModuleHandle;
    _KN__FinalizationRequestList.program_call_convention = 0;

    err = snk_FindSymbolOffset(_CN__BrigModule, "_FN_" , &_KN__FinalizationRequestList.symbol);
    ErrorCheck(Finding the symbol offset for the kernel, err);

    /*  (RE) Finalize the hsa program with this kernel on the request list */
    err = hsa_ext_finalize_program(_CN__HsaProgram, _CN__Device, 1, &_KN__FinalizationRequestList, NULL, NULL, 0, NULL, 0);
    ErrorCheck(Finalizing the program, err);

    /*  Get the hsa code descriptor address.  */
    err = hsa_ext_query_kernel_descriptor_address(_CN__HsaProgram, _CN__ModuleHandle , _KN__FinalizationRequestList.symbol, &_KN__HsaCodeDescriptor);
    ErrorCheck(Querying the kernel descriptor address, err);

    /* Find a memory region that supports kernel arguments.  */
    hsa_region_t kernarg_region = 0;
    hsa_agent_iterate_regions(_CN__Device, snk_GetKernArrg, &kernarg_region);
    err = (kernarg_region == 0) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
    ErrorCheck(Finding a kernarg memory region, err);
   
    /*  Allocate the kernel argument buffer from the correct region.  */   
    _KN__kernel_arg_buffer_size = _KN__HsaCodeDescriptor->kernarg_segment_byte_size;
    err = hsa_memory_allocate(kernarg_region, _KN__kernel_arg_buffer_size, &_KN__kernel_arg_buffer);
    ErrorCheck(Allocating kernel argument memory buffer, err);

    return STATUS_SUCCESS;

} /* end of _KN__init */

extern status_t _KN__stop(){
    status_t err;
    if (_CN__FC == 0 ) {
       /* weird, but we cannot stop unless we initialized the context */
       err = _CN__InitContext();
       if ( err != STATUS_SUCCESS ) return err; 
       _CN__FC = 1;
    }
    if ( _KN__FK == 1 ) {
        /*  Currently nothing kernel specific must be recovered */
       _KN__FK = 0;
    }
    return STATUS_SUCCESS;

} /* end of _KN__stop */

EOF
}

function write_kernel_template(){
/bin/cat <<"EOF"

    hsa_status_t err;
    status_t status;

    /*  Get stream number from launch parameters.       */
    /*  This must be less than SNK_MAX_STREAMS.         */
    /*  If negative, then function call is synchrnous.  */
    int stream_num = lparm->stream;
    if ( stream_num >= SNK_MAX_STREAMS )  {
       printf(" ERROR Stream number %d specified, must be less than %d \n", stream_num, SNK_MAX_STREAMS);
       return; 
    }

    if (_KN__FK == 0 ) {
       status = _KN__init();
       if ( status  != STATUS_SUCCESS ) return; 
       _KN__FK = 1;
    }

    hsa_queue_t* this_Q ;
    hsa_signal_t this_sig ;

    /*  Setup this call to this kernel dispatch packet from scratch.  */
    hsa_dispatch_packet_t this_aql;
    memset(&this_aql, 0, sizeof(this_aql));

    if ( stream_num < 0 ) {
       /*  Sychronous execution */
       this_Q = Sync_CommandQ;
       this_sig = Sync_Signal;
       this_aql.completion_signal=this_sig;
    } else {
       /* Asynchrnous */
       this_Q = Stream_CommandQ[stream_num];
       this_sig = Stream_Signal[stream_num];
    }

    /*  Reset signal to original value. */
    /*  WARNING  atomic operation here. */
    hsa_signal_store_relaxed(this_sig,1);

    /*  Set the dimensions passed from the application */
    this_aql.dimensions=(uint16_t) lparm->ndim;
    this_aql.grid_size_x=lparm->gdims[0];
    this_aql.workgroup_size_x=lparm->ldims[0];
    if (lparm->ndim>1) {
       this_aql.grid_size_y=lparm->gdims[1];
       this_aql.workgroup_size_y=lparm->ldims[1];
    } else {
       this_aql.grid_size_y=1;
       this_aql.workgroup_size_y=1;
    }
    if (lparm->ndim>2) {
       this_aql.grid_size_z=lparm->gdims[2];
       this_aql.workgroup_size_z=lparm->ldims[2];
    } else {
       this_aql.grid_size_z=1;
       this_aql.workgroup_size_z=1;
    }

    this_aql.header.type=HSA_PACKET_TYPE_DISPATCH;
    this_aql.header.acquire_fence_scope=lparm->acquire_fence_scope;
    this_aql.header.release_fence_scope=lparm->release_fence_scope;

    /*  Set user defined barrier, default = 0 implies execution order not gauranteed */
    this_aql.header.barrier=lparm->barrier;
    this_aql.group_segment_size=_KN__HsaCodeDescriptor->workgroup_group_segment_byte_size;
    this_aql.private_segment_size=_KN__HsaCodeDescriptor->workitem_private_segment_byte_size;
    
    /*  copy args from the custom _KN__args structure */
    /*  FIXME We should align kernel_arg_buffer because _KN__args is aligned */
    memcpy(_KN__kernel_arg_buffer, &_KN__args, sizeof(_KN__args)); 

    /*  Bind kernelcode to the packet.  */
    this_aql.kernel_object_address=_KN__HsaCodeDescriptor->code.handle;

    /*  Bind kernel argument buffer to the aql packet.  */
    this_aql.kernarg_address=(uint64_t)_KN__kernel_arg_buffer;

    /*  Obtain the current queue write index. increases with each call to kernel  */
    uint64_t index = hsa_queue_load_write_index_relaxed(this_Q);
    /* printf("DEBUG:Call #%d to kernel \"%s\" \n",(int) index,"_KN_");  */

    /*  Write this_aql at the calculated queue index address.  */
    const uint32_t queueMask = this_Q->size - 1;
    ((hsa_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]=this_aql;

    /* Increment the write index and ring the doorbell to dispatch the kernel.  */
    hsa_queue_store_write_index_relaxed(this_Q, index+1);
    hsa_signal_store_relaxed(this_Q->doorbell_signal, index);

    /*  For synchronous execution, wait on the dispatch signal until the kernel is finished.  */
    if ( stream_num < 0 ) {
       err = hsa_signal_wait_acquire(this_sig, HSA_LT, 1, (uint64_t) -1, HSA_WAIT_EXPECTANCY_UNKNOWN);
       ErrorCheck(Waiting on the dispatch signal, err);
    }

    return; 

    /*  *** END OF KERNEL LAUNCH TEMPLATE ***  */
EOF
}

function write_fortran_lparm_t(){
if [ -f launch_params.f ] ; then 
   echo
   echo "WARNING: The file launch_params.f already exists.   "
   echo "         snack will not overwrite this file.  "
   echo
else
/bin/cat >launch_params.f <<"EOF"
C     INCLUDE launch_params.f in your FORTRAN source so you can set dimensions.
      use, intrinsic :: ISO_C_BINDING
      type, BIND(C) :: snk_lparm_t
          integer (C_INT) :: ndim = 1
          integer (C_SIZE_T) :: gdims(3) = (/ 1 , 0, 0 /)
          integer (C_SIZE_T) :: ldims(3) = (/ 64, 0, 0 /)
          integer (C_INT) :: stream = -1 
          integer (C_INT) :: barrier = 1
          integer (C_INT) :: acquire_fence_scope = 2
          integer (C_INT) :: release_fence_scope = 2
          integer (C_INT) :: num_edges_in = 0
          integer (C_INT) :: num_edges_out = 0
      end type snk_lparm_t
      type (snk_lparm_t) lparm
C  
C     Set default values
C     lparm%ndim=1 
C     lparm%gdims(1)=1
C     lparm%ldims(1)=64
C     lparm%stream=-1 
C     lparm%barrier=1
C  
C  
EOF
fi
}


function is_scalar() {
    scalartypes="int,float,char,double,void,size_t,image3d_t"
    local stype
    IFS=","
    for stype in $scalartypes ; do 
       if [ "$stype" == "$1" ] ; then 
          return 1
       fi
   done
   return 0
}

function parse_arg() {
   arg_name=`echo $1 | awk '{print $NF}'`
   arg_type=`echo $1 | awk '{$NF=""}1' | sed 's/ *$//'`
   if [ "${arg_type:0:7}" == "__local" ] ; then   
      is_local=1
#     arg_type=${arg_type:8}
      arg_type="size_t"
      arg_name="${arg_name}_size"
   else
      is_local=0
   fi
   if [ "${arg_type:0:4}" == "int3" ] ; then   
      arg_type="int*"
   fi
   simple_arg_type=`echo $arg_type | awk '{print $NF}' | sed 's/\*//'`
#  Drop keyword restrict from argument in host callable c function
   if [ "${simple_arg_type}" == "restrict" ] ; then 
      arg_type=${arg_type%%restrict*}
      simple_arg_type=`echo $arg_type | awk '{print $NF}' | sed 's/\*//'`
   fi
   last_char="${arg_type: $((${#arg_type}-1)):1}"
   if [ "$last_char" == "*" ] ; then 
      is_pointer=1
      local __lc='*'
   else
      is_pointer=0
      local __lc=""
      last_char=" " 
   fi
#  Convert CL types to c types.  A lot of work is needed here.
   if [ "$simple_arg_type" == "uint" ] ; then 
      simple_arg_type="int"
      arg_type="unsigned int${__lc}"
   elif [ "$simple_arg_type" == "uchar" ] ; then 
      simple_arg_type="char"
      arg_type="unsigned char${__lc}"
   elif [ "$simple_arg_type" == "uchar16" ] ; then 
      simple_arg_type="int"
      arg_type="unsigned short int${__lc}"
   fi
#   echo "arg_name:$arg_name arg_type:$arg_type  simple_arg_type:$simple_arg_type"
}

#  snk_genw starts here
   
#  Inputs 
__SN=$1
__CLF=$2
__PROGV=$3
#  Work space
__TMPD=$4

#  Outputs: cwrapper, header file, and updated CL 
__CWRAP=$5
__HDRF=$6
__UPDATED_CL=$7

# If snack call snk_genw with -fort option
__IS_FORTRAN=$8

# If snack was called with -noglobs
__NO_GLOB_FUNS=$9

# Intermediate files.
__EXTRACL=${__TMPD}/extra.cl
__KARGLIST=${__TMPD}/klist
__ARGL=""

__WRAPPRE="_"
__SEDCMD=" "

#   if [ $GENW_ADD_DUMMY ] ; then 
#      echo
#      echo "WARNING:  DUMMY ARGS ARE ADDED FOR STABLE COMPILER "
#      echo
#   fi

#  Read the CLF and build a list of kernels and args, one kernel and set of args per line of KARGLIST file
   cpp $__CLF | sed -e '/__kernel/,/)/!d' |  sed -e ':a;$!N;s/\n/ /;ta;P;D' | sed -e 's/__kernel/\n__kernel/g'  | grep "__kernel" | \
   sed -e "s/__kernel//;s/void//;s/__global//g;s/{//g;s/ \*/\*/g"  | cut -d\) -f1 | sed -e "s/\*/\* /g;s/__restrict__//g" >$__KARGLIST

#  The header and extra-cl files must start empty because lines are incrementally added to end of file
   if [ -f $__EXTRACL ] ; then rm -f $__EXTRACL ; fi
   touch $__EXTRACL

#  Create header file for c and c++ with extra lparm arg (global and local dimensions)
   echo "/* HEADER FILE GENERATED BY snack VERSION $__PROGV */" >$__HDRF
   echo "/* THIS FILE:  $__HDRF  */" >>$__HDRF
   echo "/* INPUT FILE: $__CLF  */" >>$__HDRF
   write_header_template >>$__HDRF

#  Write comments at the beginning of the c wrapper, include copyright notice
   echo "/* THIS TEMPORARY c SOURCE FILE WAS GENERATED BY snack version $__PROGV */" >$__CWRAP
   echo "/* THIS FILE : $__CWRAP  */" >>$__CWRAP
   echo "/* INPUT FILE: $__CLF  */" >>$__CWRAP
   echo "/* UPDATED CL: $__UPDATED_CL  */" >>$__CWRAP
   echo "/*                               */ " >>$__CWRAP
   echo "    " >>$__CWRAP

   write_copyright_template >>$__CWRAP
   write_header_template >>$__CWRAP
   write_context_template | sed -e "s/_CN_/${__SN}/g"  >>$__CWRAP

   if [ "$__NO_GLOB_FUNS" == "0" ] ; then 
      write_global_functions_template >>$__CWRAP
   fi

#  Add includes from CL to the generated C wrapper.
   grep "^#include " $__CLF >> $__CWRAP

#  Process each cl __kernel and its arguments stored as one line in the KARGLIST file
#  We need to process list of args 3 times in this loop.  
#      1) SNACK function declaration
#      2) Build structure for kernel arguments 
#      3) Write values to kernel argument structure

   sed_sepchar=""
   while read line ; do 

#     parse the kernel name __KN and the native argument list __ARGL
      __KN=`echo ${line%(*} | tr -d ' '`
      __ARGL=${line#*(}

#     Add the kernel initialization routine to the c wrapper
      write_KernelStatics_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g" >>$__CWRAP

#     Build a corrected argument list , change CL types to c types as necessary, see parse_arg
      __CFN_ARGL=""
      __PROTO_ARGL=""
      sepchar=""
      IFS=","
      for _val in $__ARGL ; do 
         parse_arg $_val
         __CFN_ARGL="${__CFN_ARGL}${sepchar}${simple_arg_type}${last_char} ${arg_name}"
         __PROTO_ARGL="${__PROTO_ARGL}${sepchar}${arg_type} ${arg_name}"
         sepchar=","
      done

#     Write start of the SNACK function
      echo "/* ------  Start of SNACK function ${__KN} ------ */ " >> $__CWRAP 
      if [ "$__IS_FORTRAN" == "1" ] ; then 
#        Add underscore to kernel name and resolve lparm pointer 
         echo "extern void ${__KN}_($__CFN_ARGL, const snk_lparm_t * lparm) {" >>$__CWRAP
      else  
         if [ "$__CFN_ARGL" == "" ] ; then 
            echo "extern void $__KN(const snk_lparm_t * lparm) {" >>$__CWRAP
         else
            echo "extern void $__KN($__CFN_ARGL, const snk_lparm_t * lparm) {" >>$__CWRAP
         fi
      fi

#     Write the structure definition for the kernel arguments
      echo "   struct ${__KN}_args_struct {" >> $__CWRAP
      NEXTI=0
      if [ $GENW_ADD_DUMMY ] ; then 
         echo "      uint64_t arg0;"  >> $__CWRAP
         echo "      uint64_t arg1;"  >> $__CWRAP
         echo "      uint64_t arg2;"  >> $__CWRAP
         echo "      uint64_t arg3;"  >> $__CWRAP
         echo "      uint64_t arg4;"  >> $__CWRAP
         echo "      uint64_t arg5;"  >> $__CWRAP
         NEXTI=6
      fi
      IFS=","
      for _val in $__ARGL ; do 
         parse_arg $_val
         if [ "$last_char" == "*" ] ; then 
            echo "      ${simple_arg_type}* arg${NEXTI};"  >> $__CWRAP
         else
            is_scalar $simple_arg_type
            if [ $? == 1 ] ; then 
               echo "      ${simple_arg_type} arg${NEXTI};"  >> $__CWRAP
            else
               echo "      ${simple_arg_type}* arg${NEXTI};"  >> $__CWRAP
            fi
         fi
         NEXTI=$(( NEXTI + 1 ))
      done
      echo "   } __attribute__ ((aligned (16))) ; "  >> $__CWRAP
      echo "   struct ${__KN}_args_struct ${__KN}_args ; "  >> $__CWRAP

#     Write statements to fill in the argument structure and 
#     keep track of updated CL arg list and new call list 
#     in case we have to create a wrapper CL function.
#     to call the real kernel CL function. 
      NEXTI=0
      if [ $GENW_ADD_DUMMY ] ; then 
         echo "   ${__KN}_args.arg0=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args.arg1=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args.arg2=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args.arg3=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args.arg4=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args.arg5=0 ; "  >> $__CWRAP
         NEXTI=6
      fi
      KERN_NEEDS_CL_WRAPPER="FALSE"
      arglistw=""
      calllist=""
      sepchar=""
      IFS=","
      for _val in $__ARGL ; do 
         parse_arg $_val
#        These echo statments help debug a lot
#        echo "simple_arg_type=|${simple_arg_type}|" 
#        echo "arg_type=|${arg_type}|" 
         if [ "$last_char" == "*" ] ; then 
            arglistw="${arglistw}${sepchar}${arg_type} ${arg_name}"
            calllist="${calllist}${sepchar}${arg_name}"
            echo "   ${__KN}_args.arg${NEXTI} = $arg_name ; "  >> $__CWRAP
         else
            is_scalar $simple_arg_type
            if [ $? == 1 ] ; then 
               arglistw="$arglistw${sepchar}${arg_type} $arg_name"
               calllist="${calllist}${sepchar}${arg_name}"
               echo "   ${__KN}_args.arg${NEXTI} = $arg_name ; "  >> $__CWRAP
            else
               KERN_NEEDS_CL_WRAPPER="TRUE"
               arglistw="$arglistw${sepchar}${arg_type}* $arg_name"
               calllist="${calllist}${sepchar}${arg_name}[0]"
               echo "   ${__KN}_args.arg${NEXTI} = &$arg_name ; "  >> $__CWRAP
            fi
         fi 
         sepchar=","
         NEXTI=$(( NEXTI + 1 ))
      done
      
#     Write the extra CL if we found call-by-value structs and write the extra CL needed
      if [ "$KERN_NEEDS_CL_WRAPPER" == "TRUE" ] ; then 
         echo "__kernel void ${__WRAPPRE}$__KN($arglistw){ $__KN($calllist) ; } " >> $__EXTRACL
         __FN="\&__OpenCL_${__WRAPPRE}${__KN}_kernel"
#        change the original __kernel (external callable) to internal callable
         __SEDCMD="${__SEDCMD}${sed_sepchar}s/__kernel void $__KN /void $__KN/;s/__kernel void $__KN(/void $__KN(/"
         sed_sepchar=";"
      else
         __FN="\&__OpenCL_${__KN}_kernel"
      fi

#     Write the prototype to the header file
      if [ "$__IS_FORTRAN" == "1" ] ; then 
#        don't use headers for fortran but it is a good reference for how to call from fortran
         echo "extern _CPPSTRING_ void ${__KN}_($__PROTO_ARGL, const snk_lparm_t * lparm_p);" >>$__HDRF
      else
         if [ "$__PROTO_ARGL" == "" ] ; then 
            echo "extern _CPPSTRING_ void ${__KN}(const snk_lparm_t * lparm);" >>$__HDRF
         else
            echo "extern _CPPSTRING_ void ${__KN}($__PROTO_ARGL, const snk_lparm_t * lparm);" >>$__HDRF
         fi
      fi

#     Now add the kernel template to wrapper and change all three strings
#     1) Context Name _CN_ 2) Kerneel name _KN_ and 3) Funtion name _FN_
      write_kernel_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g;s/_FN_/${__FN}/g" >>$__CWRAP

      echo "} " >> $__CWRAP 
      echo "/* ------  End of SNACK function ${__KN} ------ */ " >> $__CWRAP 

#     Add the kernel initialization routine to the c wrapper
      write_InitKernel_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g;s/_FN_/${__FN}/g" >>$__CWRAP

#  END OF WHILE LOOP TO PROCESS EACH KERNEL IN THE CL FILE
   done < $__KARGLIST


   if [ "$__IS_FORTRAN" == "1" ] ; then 
      write_fortran_lparm_t
   fi

#  Write the updated CL
   if [ "$__SEDCMD" != " " ] ; then 
#      Remove extra spaces, then change "__kernel void" to "void" if they have call-by-value structs
#      Still could fail if __kernel void _FN_ split across multple lines, FIX THIS
       awk '$1=$1'  $__CLF | sed -e "$__SEDCMD" > $__UPDATED_CL
       cat $__EXTRACL >> $__UPDATED_CL
   else 
#  No changes to the CL file is needed, so just make a copy
      cp -p $__CLF $__UPDATED_CL
   fi

   rm $__KARGLIST
   rm $__EXTRACL 

