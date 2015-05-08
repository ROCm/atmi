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
#ifndef __SNK_CPU_TASK_DEFS
#define __SNK_CPU_TASK_DEFS

#include <hsa.h>
#include <hsa_ext_finalize.h>

#include <pthread.h>

#define check(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed.\n", #msg); \
    exit(1); \
}
//#define DEBUG_SNK
//#define VERBOSE_SNK
#ifdef DEBUG_SNK
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

#ifdef VERBOSE_SNK
#define VERBOSE_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define VERBOSE_PRINT(...) do{ } while ( false )
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/* ---------------------------------------------------------------------------------
 * Simulated CPU Data Structures
 * --------------------------------------------------------------------------------- */
enum {
    PROCESS_PKT = 0,
    FINISH,
    IDLE
};

typedef struct cpu_kernel_table_s {
    const char *name; 
    union {
        void (*function0) (void);
        void (*function1) (uint64_t);
        void (*function2) (uint64_t,uint64_t);
        void (*function3) (uint64_t,uint64_t,uint64_t);
        void (*function4) (uint64_t,uint64_t,uint64_t,uint64_t);
    } function;
    uint64_t ptrs[4];
} cpu_kernel_table_t;



typedef struct agent_t
{
  int num_queues;
  int id;
  hsa_queue_t *queue;
  //hsa_agent_t cpu_agent;
  //hsa_region_t cpu_region;
} agent_t;

/* ---------------------------------------------------------------------------------
 * Simulated CPU Management API
 * --------------------------------------------------------------------------------- */
void set_cpu_kernel_table(const cpu_kernel_table_t *kernel_table); 
agent_t get_cpu_q_agent(int id);
void cpu_agent_init(hsa_agent_t cpu_agent, hsa_region_t cpu_region, 
                const size_t num_queues, const size_t capacity
                );
void agent_fini();
hsa_queue_t* get_cpu_queue(int id);
void signal_worker(int id, int signal);
void *agent_worker(void *agent_args);
int process_packet(hsa_queue_t *queue, int id);

#ifdef __cplusplus
} //end extern "C" block
#endif

#endif // __SNK_CPU_TASK_DEFS
