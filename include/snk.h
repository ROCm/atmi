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
/* This file defines the Asynchronous Task Management Interface (ATMI)
 */
#ifdef __cplusplus
#define _CPPSTRING_ "C" 
#endif
#ifndef __cplusplus
#define _CPPSTRING_ 
#endif

#ifndef __SNK_DEFS

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define SNK_MAX_STREAMS 8 
#define SNK_MAX_TASKS 4000

#define SNK_ORDERED 0
#define SNK_UNORDERED 1

#define SNK_TRUE    1
#define SNK_FALSE   0
typedef enum snk_device_type_s {
    SNK_DEVICE_TYPE_CPU = 0,
    SNK_DEVICE_TYPE_GPU = 1,
    SNK_DEVICE_TYPE_DSP = 2
} snk_device_type_t;

typedef enum snk_state_s {
    SNK_INITIALIZED = 0,
    SNK_DISPATCHED  = 1,
    SNK_COMPLETED   = 2,
    SNK_FAILED      = 3
} snk_state_t;

typedef struct snk_task_profile_s {
    double dispatch_time;
    double start_time;
    double end_time;
} snk_task_profile_t;

#define snk_handle_t hsa_signal_t
typedef struct snk_task_s { 
    snk_handle_t handle;
    snk_state_t state;
    //snk_task_profile_t* profile;
} snk_task_t;

typedef char boolean;
typedef struct snk_stream_s {
    //char id;
    boolean ordered;
} snk_stream_t;

typedef struct snk_lparm_s snk_lparm_t;
struct snk_lparm_s { 
   int ndim;                  /* default = 1 */
   size_t gdims[3];           /* NUMBER OF THREADS TO EXECUTE MUST BE SPECIFIED */ 
   size_t ldims[3];           /* Default = {64} , e.g. 1 of 8 CU on Kaveri */
   snk_stream_t *stream;      /* default = NULL */
   boolean synchronous;          /* default = SNK_FALSE; */
   int acquire_fence_scope;   /* default = 2 */
   int release_fence_scope;   /* default = 2 */
   int num_required;          /* Number of required parent tasks, default = 0 */
   snk_task_t** requires;     /* Array of required parent tasks, default = NULL */
   int num_needs_any;         /* Number of parent tasks where only one must complete, default = 0 */
   snk_task_t** needs_any;    /* Array of parent tasks where only one must complete, default = NULL */
   snk_device_type_t device_type; /* default = SNK_DEVICE_TYPE_GPU */
} ;

/* This string macro is used to declare launch parameters set default values  */
#define SNK_INIT_LPARM(X,Y) snk_lparm_t * X ; snk_lparm_t  _ ## X ={.ndim=1,.gdims={Y},.ldims={64},.stream=NULL,.synchronous=SNK_FALSE,.acquire_fence_scope=2,.release_fence_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.device_type=SNK_DEVICE_TYPE_GPU} ; X = &_ ## X ;
 
#define SNK_INIT_CPU_LPARM(X) snk_lparm_t * X ; snk_lparm_t  _ ## X ={.ndim=1,.gdims={1},.ldims={1},.stream=NULL,.synchronous=SNK_FALSE,.acquire_fence_scope=2,.release_fence_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.device_type=SNK_DEVICE_TYPE_CPU} ; X = &_ ## X ;
 
extern _CPPSTRING_ void snk_task_wait(snk_task_t *task);
extern _CPPSTRING_ void snk_stream_sync(snk_stream_t *stream);

#ifdef __cplusplus
} //end extern "C" block
#endif

#define __SNK_DEFS
#endif //__SNK_DEFS
