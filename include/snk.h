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
/* This file is the SNACK header. Idea is to move as much of code as possible
 * from the snk_genw.sh script to a library
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "hsa.h"
#include "hsa_ext_finalize.h"
#include <inttypes.h>
/*  set NOTCOHERENT needs this include
#include "hsa_ext_amd.h"
*/

#ifdef __cplusplus
#define _CPPSTRING_ "C" 
#endif
#ifndef __cplusplus
#define _CPPSTRING_ 
#endif

#ifndef __SNK_DEFS
#include "snk_cpu_task.h"
enum status_t {
    STATUS_SUCCESS=0,
    STATUS_UNKNOWN=1
};
typedef enum status_t status_t;

#define SNK_MAX_STREAMS 8 
#define SNK_MAX_TASKS 100001
#define SNK_MAX_CPU_STREAMS 2 
#define SNK_MAX_CPU_FUNCTIONS   100
extern _CPPSTRING_ void stream_sync(const int stream_num);

#define SNK_ORDERED 1
#define SNK_UNORDERED 0

#include <stdint.h>
#ifndef HSA_RUNTIME_INC_HSA_H_
typedef struct hsa_signal_s { uint64_t handle; } hsa_signal_t;
#endif

enum _snk_device_type_t {
    SNK_DEVICE_TYPE_CPU = 0,
    SNK_DEVICE_TYPE_GPU = 1,
    SNK_DEVICE_TYPE_DSP = 2
};
typedef enum _snk_device_type_t snk_device_type_t;

typedef struct snk_task_s snk_task_t;
struct snk_task_s { 
   hsa_signal_t signal ; 
   snk_task_t* next;
};

typedef struct snk_lparm_s snk_lparm_t;
struct snk_lparm_s { 
   int ndim;                  /* default = 1 */
   size_t gdims[3];           /* NUMBER OF THREADS TO EXECUTE MUST BE SPECIFIED */ 
   size_t ldims[3];           /* Default = {64} , e.g. 1 of 8 CU on Kaveri */
   int stream;                /* default = -1 , synchrnous */
   int barrier;               /* default = SNK_UNORDERED */
   int acquire_fence_scope;   /* default = 2 */
   int release_fence_scope;   /* default = 2 */
   snk_task_t *requires ;     /* Linked list of required parent tasks, default = NULL  */
   snk_task_t *needs ;        /* Linked list of parent tasks where only one must complete, default=NULL */
   snk_device_type_t device_type; /* default = SNK_DEVICE_TYPE_GPU */
} ;

/* This string macro is used to declare launch parameters set default values  */
#define SNK_INIT_LPARM(X,Y) snk_lparm_t * X ; snk_lparm_t  _ ## X ={.ndim=1,.gdims={Y},.ldims={64},.stream=0,.barrier=SNK_UNORDERED,.acquire_fence_scope=2,.release_fence_scope=2,.requires=NULL,.needs=NULL,.device_type=SNK_DEVICE_TYPE_GPU} ; X = &_ ## X ;
 
#define SNK_INIT_CPU_LPARM(X) snk_lparm_t * X ; snk_lparm_t  _ ## X ={.ndim=1,.gdims={1},.ldims={1},.stream=0,.barrier=SNK_UNORDERED,.acquire_fence_scope=2,.release_fence_scope=2,.requires=NULL,.needs=NULL,.device_type=SNK_DEVICE_TYPE_CPU} ; X = &_ ## X ;
 
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

extern snk_task_t   SNK_Tasks[SNK_MAX_TASKS];
extern int          SNK_NextTaskId;

extern _CPPSTRING_ void snk_task_wait(const snk_task_t *task);

status_t snk_init_context(
                        hsa_agent_t *_CN__Agent, 
                        hsa_ext_module_t **_CN__BrigModule,
                        hsa_ext_program_t *_CN__HsaProgram,
                        hsa_executable_t *_CN__Executable,
                        hsa_region_t *_CN__KernargRegion,
                        hsa_agent_t *_CN__CPU_Agent,
                        hsa_region_t *_CN__CPU_KernargRegion
                        );

status_t snk_init_kernel(hsa_executable_symbol_t          *_KN__Symbol,
                            const char *kernel_symbol_name,
                            uint64_t                         *_KN__Kernel_Object,
                            uint32_t                         *_KN__Kernarg_Segment_Size, /* May not need to be global */
                            uint32_t                         *_KN__Group_Segment_Size,
                            uint32_t                         *_KN__Private_Segment_Size,
                            hsa_agent_t _CN__Agent, 
                            hsa_executable_t _CN__Executable
                            );

snk_task_t *snk_kernel(const snk_lparm_t *lparm, 
                 uint64_t                         _KN__Kernel_Object,
                 uint32_t                         _KN__Group_Segment_Size,
                 uint32_t                         _KN__Private_Segment_Size,
                 void *thisKernargAddress);

snk_task_t *snk_cpu_kernel(const snk_lparm_t *lparm, 
                 const cpu_kernel_table_t *_CN__CPU_kernels,
                 const char *kernel_name,
                 const uint32_t _KN__cpu_task_num_args);

uint16_t create_header(hsa_packet_type_t type, int barrier);
void cpu_agent_init(hsa_agent_t cpu_agent, hsa_region_t cpu_region, 
                const size_t num_queues, const size_t capacity,
                const size_t num_workers);
#define __SNK_DEFS
#endif //__SNK_DEFS
