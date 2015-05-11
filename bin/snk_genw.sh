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

#include "snk.h"
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
enum status_t {
    STATUS_SUCCESS=0,
    STATUS_UNKNOWN=1
};
typedef enum status_t status_t;

#define SNK_MAX_STREAMS 8 
#define SNK_MAX_TASKS 100001
#define SNK_MAX_CPU_STREAMS 4
#define SNK_MAX_CPU_FUNCTIONS   100
extern _CPPSTRING_ void stream_sync(const int stream_num);

#define SNK_ORDERED 1
#define SNK_UNORDERED 0

#include <stdint.h>
#ifndef HSA_RUNTIME_INC_HSA_H_
typedef struct hsa_signal_s { uint64_t handle; } hsa_signal_t;
#endif

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
    // , SNK_ISPARENT  = 4 
    // I am not convinced about a task state being a parent
    // How is it different from being dispatched? What additional capabilities
    // will it give the user if they have the knowledge that this task is a
    // parent?
} snk_state_t;
/*
typedef struct snk_task_profile_s {
    double dispatch_time;
    double start_time;
    double end_time;
} snk_task_profile_t;
*/
typedef hsa_signal_t snk_handle_t;
typedef struct snk_task_s { 
    snk_handle_t handle; 
    snk_state_t state;
    // snk_task_profile_t profile;
} snk_task_t;

typedef struct snk_lparm_s snk_lparm_t;
struct snk_lparm_s { 
   int ndim;                  /* default = 1 */
   size_t gdims[3];           /* NUMBER OF THREADS TO EXECUTE MUST BE SPECIFIED */ 
   size_t ldims[3];           /* Default = {64} , e.g. 1 of 8 CU on Kaveri */
   int stream;                /* default = -1 , synchrnous */
   int barrier;               /* default = SNK_UNORDERED */
   int acquire_fence_scope;   /* default = 2 */
   int release_fence_scope;   /* default = 2 */
   int num_required;          /* Number of required parent tasks, default = 0 */
   snk_task_t** requires;     /* Array of required parent tasks, default = NULL */
   int num_needs_any;         /* Number of parent tasks where only one must complete, default = 0 */
   snk_task_t** needs_any;    /* Array of parent tasks where only one must complete, default = NULL */
   snk_device_type_t device_type; /* default = SNK_DEVICE_TYPE_GPU */
} ;

/* This string macro is used to declare launch parameters set default values  */
#define SNK_INIT_LPARM(X,Y) snk_lparm_t * X ; snk_lparm_t  _ ## X ={.ndim=1,.gdims={Y},.ldims={64},.stream=0,.barrier=SNK_UNORDERED,.acquire_fence_scope=2,.release_fence_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.device_type=SNK_DEVICE_TYPE_GPU} ; X = &_ ## X ;
 
#define SNK_INIT_CPU_LPARM(X) snk_lparm_t * X ; snk_lparm_t  _ ## X ={.ndim=1,.gdims={1},.ldims={1},.stream=0,.barrier=SNK_UNORDERED,.acquire_fence_scope=2,.release_fence_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.device_type=SNK_DEVICE_TYPE_CPU} ; X = &_ ## X ;
 
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

extern _CPPSTRING_ void snk_task_wait(snk_task_t *task);

#define __SNK_DEFS
#endif
EOF
}
function write_global_functions_template(){
/bin/cat  <<"EOF"

EOF
} # end of bash function write_global_functions_template() 

function write_context_template(){
/bin/cat  <<"EOF"

/* Context(cl file) specific globals */
hsa_ext_module_t*                _CN__BrigModule;
hsa_agent_t                      _CN__Agent;
hsa_agent_t                      _CN__CPU_Agent;
hsa_ext_program_t                _CN__HsaProgram;
hsa_executable_t                 _CN__Executable;
hsa_region_t                     _CN__KernargRegion;
hsa_region_t                     _CN__CPU_KernargRegion;
int                              _CN__FC = 0; 

cpu_kernel_table_t _CN__CPU_kernels[SNK_MAX_CPU_FUNCTIONS];

#include "_CN__brig.h" 

status_t _CN__InitContext(){

    /* FIXME: Move loading the BRIG binary to the libsnk.
       How to get rid of the warning "note: expected ‘char **’ but argument is of type ‘char (*)[]’"?
    */
    /* Load the BRIG binary.  */
    _CN__BrigModule = (hsa_ext_module_t*) &_CN__HSA_BrigMem;
    return snk_init_context(&_CN__Agent, 
                            &_CN__BrigModule, 
                            &_CN__HsaProgram, 
                            &_CN__Executable, 
                            &_CN__KernargRegion,
                            &_CN__CPU_Agent,
                            &_CN__CPU_KernargRegion
                            );
} /* end of __CN__InitContext */

EOF
}

function write_KernelStatics_template(){
/bin/cat <<"EOF"

/* Kernel specific globals, one set for each kernel  */
hsa_executable_symbol_t          _KN__Symbol;
int                              _KN__FK = 0 ; 
status_t                         _KN__init();
status_t                         _KN__stop();
uint64_t                         _KN__Kernel_Object;
uint32_t                         _KN__Kernarg_Segment_Size; /* May not need to be global */
uint32_t                         _KN__Group_Segment_Size;
uint32_t                         _KN__Private_Segment_Size;
uint32_t                         _KN__cpu_task_num_args;

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
    return snk_init_kernel(&_KN__Symbol, 
                      "&__OpenCL__KN__kernel",
                      &_KN__Kernel_Object,
                      &_KN__Kernarg_Segment_Size,
                      &_KN__Group_Segment_Size,
                      &_KN__Private_Segment_Size,
                      _CN__Agent,
                      _CN__Executable); 
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

function write_cpu_kernel_template(){
/bin/cat <<"EOF"
   return snk_cpu_kernel(lparm, 
                _CN__CPU_kernels,
                "_KN_",
                _KN__cpu_task_num_args,
                cpu_kernel_arg_list);
   /*  *** END OF KERNEL LAUNCH TEMPLATE ***  */
EOF
}

function write_kernel_template(){
/bin/cat <<"EOF"
   return snk_kernel(lparm, 
                _KN__Kernel_Object, 
                _KN__Group_Segment_Size,
                _KN__Private_Segment_Size, 
                thisKernargAddress
                );
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
      end type snk_lparm_t
      type (snk_lparm_t) lparm
C  
C     Set default values
C     lparm%ndim=1 
C     lparm%gdims(1)=1
C     lparm%ldims(1)=64
C     lparm%stream=0 
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
      arg_type=${arg_type%%[ *]restrict*}
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
#   echo "arg_name:$arg_name arg_type:$arg_type  simple_arg_type:$simple_arg_type last_char:$last_char"
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
   sed -e "s/__kernel//;s/__global//g;s/{//g;s/ \*/\*/g"  | cut -d\) -f1 | sed -e "s/\*/\* /g;s/__restrict__//g" >$__KARGLIST

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
   # The header information to the C wrapper should already be available in snk.h
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
   __KN_NUM=0
   while read line ; do 
   ((__KN_NUM++))
#     parse the kernel name __KN and the native argument list __ARGL
      TYPE_NAME=`echo ${line%(*}`
      __KN=`echo $TYPE_NAME | awk '{print $2}'`
      __KT=`echo $TYPE_NAME | awk '{print $1}'`
      __ARGL=${line#*(}
#     force it to return pointer to snk_task_t
      __KT="snk_task_t*" 
         

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
         echo "extern ${__KT} ${__KN}_gpu_($__CFN_ARGL, const snk_lparm_t * lparm) {" >>$__CWRAP
      else  
         if [ "$__CFN_ARGL" == "" ] ; then 
            echo "extern ${__KT} ${__KN}_gpu(const snk_lparm_t * lparm) {" >>$__CWRAP
         else
            echo "extern ${__KT} ${__KN}_gpu($__CFN_ARGL, const snk_lparm_t * lparm) {" >>$__CWRAP
         fi
      fi
      echo "   printf(\"In SNACK Kernel GPU Function ${__KN}\n\"); " >> $__CWRAP
	  echo "   /* Kernel initialization has to be done before kernel arguments are set/inspected */ " >> $__CWRAP
      echo "   if (${__KN}_FK == 0 ) { " >> $__CWRAP
      echo "     status_t status = ${__KN}_init(); " >> $__CWRAP
      echo "     if ( status  != STATUS_SUCCESS ) return; " >> $__CWRAP
      echo "     ${__KN}_FK = 1; " >> $__CWRAP
      echo "   } " >> $__CWRAP
#     Write the structure definition for the kernel arguments.
#     Consider eliminating global _KN__args and memcopy and write directly to thisKernargAddress.
#     by writing these statements here:
      echo "   /* Allocate the kernel argument buffer from the correct region. */ " >> $__CWRAP
      echo "   void* thisKernargAddress; " >> $__CWRAP
      echo "   /* FIXME: HSA 1.0F may have a bug that serializes all queue operations when hsa_memory_allocate is used. " >> $__CWRAP
	  echo "	  Investigate more and revert back to hsa_memory_allocate once bug is fixed. */ " >> $__CWRAP
	  #echo "   thisKernargAddress = malloc(${__KN}_Kernarg_Segment_Size); " >> $__CWRAP
      #echo "   int ret = posix_memalign(&thisKernargAddress, 4096, ${__KN}_Kernarg_Segment_Size); " >> $__CWRAP
	  echo "   hsa_memory_allocate(${__SN}_KernargRegion, ${__KN}_Kernarg_Segment_Size, &thisKernargAddress); " >> $__CWRAP
#     How to map a structure into an malloced memory area?
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
      echo "   struct ${__KN}_args_struct* ${__KN}_args ; "  >> $__CWRAP
	  echo "   /* Setup kernel args */ " >> $__CWRAP
	  echo "   ${__KN}_args = (struct ${__KN}_args_struct*) thisKernargAddress; " >> $__CWRAP

#     Write statements to fill in the argument structure and 
#     keep track of updated CL arg list and new call list 
#     in case we have to create a wrapper CL function.
#     to call the real kernel CL function. 
      NEXTI=0
      NEXT_ARGI=0
      if [ $GENW_ADD_DUMMY ] ; then 
         echo "   ${__KN}_args->arg0=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args->arg1=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args->arg2=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args->arg3=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args->arg4=0 ; "  >> $__CWRAP
         echo "   ${__KN}_args->arg5=0 ; "  >> $__CWRAP
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
            echo "   ${__KN}_args->arg${NEXTI} = $arg_name ; "  >> $__CWRAP
         else
            is_scalar $simple_arg_type
            if [ $? == 1 ] ; then 
               arglistw="$arglistw${sepchar}${arg_type} $arg_name"
               calllist="${calllist}${sepchar}${arg_name}"
               echo "   ${__KN}_args->arg${NEXTI} = $arg_name ; "  >> $__CWRAP
            else
               KERN_NEEDS_CL_WRAPPER="TRUE"
               arglistw="$arglistw${sepchar}${arg_type}* $arg_name"
               calllist="${calllist}${sepchar}${arg_name}[0]"
               echo "   ${__KN}_args->arg${NEXTI} = &$arg_name ; "  >> $__CWRAP
            fi
         fi 
         sepchar=","
         NEXTI=$(( NEXTI + 1 ))
         NEXT_ARGI=$(( NEXT_ARGI + 1 ))
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
         echo "extern _CPPSTRING_ $__KT ${__KN}_gpu_($__PROTO_ARGL, const snk_lparm_t * lparm_p);" >>$__HDRF
      else
         if [ "$__PROTO_ARGL" == "" ] ; then 
            echo "extern _CPPSTRING_ $__KT ${__KN}_gpu(const snk_lparm_t * lparm);" >>$__HDRF
         else
            echo "extern _CPPSTRING_ $__KT ${__KN}_gpu($__PROTO_ARGL, const snk_lparm_t * lparm);" >>$__HDRF
         fi
      fi

#     Now add the kernel template to wrapper and change all three strings
#     1) Context Name _CN_ 2) Kerneel name _KN_ and 3) Funtion name _FN_
      write_kernel_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g;s/_FN_/${__FN}/g" >>$__CWRAP

      echo "} " >> $__CWRAP 
      echo "/* ------  End of SNACK function ${__KN} ------ */ " >> $__CWRAP 

#     Write start of the SNACK function
      echo >> $__CWRAP
      echo "/* ------  Start of SNACK function ${__KN}_cpu ------ */ " >> $__CWRAP 
      if [ "$__IS_FORTRAN" == "1" ] ; then 
#        Add underscore to kernel name and resolve lparm pointer 
         echo "extern ${__KT} ${__KN}_cpu_($__CFN_ARGL, const snk_lparm_t * lparm) {" >>$__CWRAP
      else  
         if [ "$__CFN_ARGL" == "" ] ; then 
            echo "extern ${__KT} ${__KN}_cpu(const snk_lparm_t * lparm) {" >>$__CWRAP
         else
            echo "extern ${__KT} ${__KN}_cpu($__CFN_ARGL, const snk_lparm_t * lparm) {" >>$__CWRAP
         fi
      fi
 
      echo "   printf(\"In SNACK Kernel CPU Function ${__KN}\n\"); " >> $__CWRAP
	  echo "   /* Kernel initialization has to be done before kernel arguments are set/inspected */ " >> $__CWRAP
      echo "   if (${__KN}_FK == 0 ) { " >> $__CWRAP
      echo "     status_t status = ${__KN}_init(); " >> $__CWRAP
      echo "     if ( status  != STATUS_SUCCESS ) return; " >> $__CWRAP
      echo "     ${__KN}_FK = 1; " >> $__CWRAP
      echo "   } " >> $__CWRAP
#     Write the structure definition for the kernel arguments.
      echo "   snk_kernel_args_t *cpu_kernel_arg_list = (snk_kernel_args_t *)malloc(sizeof(snk_kernel_args_t)); " >> $__CWRAP
#     Write statements to fill in the argument structure and 
#     keep track of updated CL arg list and new call list 
#     in case we have to create a wrapper CL function.
#     to call the real kernel CL function. 
      NEXT_ARGI=0
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
         if [ $NEXT_ARGI == 20 ] ; then 
            echo "ERROR! SNACK supports only up to 20 args for CPU tasks!"
         fi
         if [ "$last_char" == "*" ] ; then 
            arglistw="${arglistw}${sepchar}${arg_type} ${arg_name}"
            calllist="${calllist}${sepchar}${arg_name}"
            echo "   cpu_kernel_arg_list->args[${NEXT_ARGI}] = (uint64_t)$arg_name; " >>$__CWRAP
         else
            is_scalar $simple_arg_type
            if [ $? == 1 ] ; then 
               arglistw="$arglistw${sepchar}${arg_type} $arg_name"
               calllist="${calllist}${sepchar}${arg_name}"
               echo "   cpu_kernel_arg_list->args[${NEXT_ARGI}] = (uint64_t)$arg_name; " >>$__CWRAP
            else
               KERN_NEEDS_CL_WRAPPER="TRUE"
               arglistw="$arglistw${sepchar}${arg_type}* $arg_name"
               calllist="${calllist}${sepchar}${arg_name}[0]"
               echo "   cpu_kernel_arg_list->args[${NEXT_ARGI}] = (uint64_t)&$arg_name; " >>$__CWRAP
            fi
         fi 
         sepchar=","
         NEXT_ARGI=$(( NEXT_ARGI + 1 ))
      done
      
      echo "   ${__KN}_cpu_task_num_args = ${NEXT_ARGI}; " >> $__CWRAP;
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


#     Write the prototype CPU function to the header file
      if [ "$__IS_FORTRAN" == "1" ] ; then 
#        don't use headers for fortran but it is a good reference for how to call from fortran
         echo "extern _CPPSTRING_ $__KT ${__KN}_cpu_($__PROTO_ARGL, const snk_lparm_t * lparm_p);" >>$__HDRF
      else
         if [ "$__PROTO_ARGL" == "" ] ; then 
            echo "extern _CPPSTRING_ $__KT ${__KN}_cpu(const snk_lparm_t * lparm);" >>$__HDRF
         else
            echo "extern _CPPSTRING_ $__KT ${__KN}_cpu($__PROTO_ARGL, const snk_lparm_t * lparm);" >>$__HDRF
         fi
      fi

      echo "   ${__SN}_CPU_kernels[${__KN_NUM}].name = \"${__KN}\"; " >> $__CWRAP
      if (( ${NEXT_ARGI} == 0 )) ; then
        echo "   extern void ${__KN}(void);" >> $__CWRAP;
        echo "   ${__SN}_CPU_kernels[${__KN_NUM}].function.function0=${__KN}; " >>$__CWRAP
      elif (( ${NEXT_ARGI} <= 20 )) ; then
        printf "   extern void ${__KN}(uint64_t" >> $__CWRAP
        for(( arg_id=2 ; arg_id<=${NEXT_ARGI}; arg_id++ ))
        do 
            printf ', uint64_t' >> $__CWRAP; 
        done
        printf ');\n' >> $__CWRAP
        echo "   ${__SN}_CPU_kernels[${__KN_NUM}].function.function${NEXT_ARGI}=${__KN}; " >>$__CWRAP
      else
        echo "ERROR! SNACK supports only up to 20 args for CPU tasks!"
      fi
#     Now add the kernel template to wrapper and change all three strings
#     1) Context Name _CN_ 2) Kerneel name _KN_ and 3) Funtion name _FN_
      write_cpu_kernel_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g;s/_FN_/${__FN}/g" >>$__CWRAP

      echo "} " >> $__CWRAP 
      echo "/* ------  End of SNACK function ${__KN}_cpu ------ */ " >> $__CWRAP 

#     Add the kernel initialization routine to the c wrapper
      write_InitKernel_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g;s/_FN_/${__FN}/g" >>$__CWRAP

#  END OF WHILE LOOP TO PROCESS EACH KERNEL IN THE CL FILE
   done < $__KARGLIST


   if [ "$__IS_FORTRAN" == "1" ] ; then 
      write_fortran_lparm_t
   fi

#  Write the updated CL
   if [ "$__SEDCMD" != " " ] ; then 
#     Remove extra spaces, then change "__kernel void" to "void" if they have call-by-value structs
#     Still could fail if __kernel void _FN_ split across multple lines, FIX THIS
      awk '$1=$1'  $__CLF | sed -e "$__SEDCMD" > $__UPDATED_CL
      cat $__EXTRACL | sed -e "s/ snk_task_t/ void/g" >> $__UPDATED_CL
   else 
#  No changes to the CL file are needed, so just make a copy
      cat $__CLF | sed -e "s/ snk_task_t/ void/g" > $__UPDATED_CL
   fi

   rm $__KARGLIST
   rm $__EXTRACL 

