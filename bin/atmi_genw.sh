#!/bin/bash
#
#  atmi_genw.sh: Part of snack that generates the user callable wrapper functions.
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
#include "atmi.h"
EOF
}

function write_global_functions_template(){
/bin/cat  <<"EOF"

EOF
} # end of bash function write_global_functions_template() 

function write_context_template(){
/bin/cat  <<"EOF"

/* Context(cl file) specific globals */
hsa_agent_t                      __CN__CPU_Agent;
hsa_region_t                     __CN__KernargRegion;
hsa_region_t                     __CN__CPU_KernargRegion;
int                              __CN__FC = 0; 

#include "__CN__brig.h" 

status_t __CN__InitContext(){
    return snk_init_context(
                            __CN__HSA_BrigMem,
                            &__CN__KernargRegion,
                            &__CN__CPU_Agent,
                            &__CN__CPU_KernargRegion
                            );
} /* end of ___CN__InitContext */

EOF
}

function write_KernelStatics_template(){
/bin/cat <<"EOF"

/* Kernel specific globals, one set for each kernel  */
hsa_executable_symbol_t          _KN__Symbol;
int                              _KN__GPU_FK = 0 ; 
status_t                         _KN__init();
status_t                         _KN__gpu_init();
status_t                         _KN__stop();
uint64_t                         _KN__Kernel_Object;
uint32_t                         _KN__Kernarg_Segment_Size; /* May not need to be global */
uint32_t                         _KN__Group_Segment_Size;
uint32_t                         _KN__Private_Segment_Size;

EOF
}

function write_InitKernel_template(){
/bin/cat <<"EOF"
extern status_t _KN__init(){

    if (__CN__FC == 0 ) {
       status_t status = __CN__InitContext();
       if ( status  != STATUS_SUCCESS ) return; 
       __CN__FC = 1;
    }
    //snk_init_cpu_kernel();
    return snk_init_gpu_kernel(&_KN__Symbol, 
                      "&__OpenCL__KN__kernel",
                      &_KN__Kernel_Object,
                      &_KN__Kernarg_Segment_Size,
                      &_KN__Group_Segment_Size,
                      &_KN__Private_Segment_Size
                      ); 
} /* end of _KN__init */


extern status_t _KN__gpu_init(){

    if (__CN__FC == 0 ) {
       status_t status = __CN__InitContext();
       if ( status  != STATUS_SUCCESS ) return; 
       __CN__FC = 1;
    }
    return snk_init_gpu_kernel(&_KN__Symbol, 
                      "&__OpenCL__KN__kernel",
                      &_KN__Kernel_Object,
                      &_KN__Kernarg_Segment_Size,
                      &_KN__Group_Segment_Size,
                      &_KN__Private_Segment_Size
                      ); 
} /* end of _KN__init */


extern status_t _KN__stop(){
    status_t err;
    if (__CN__FC == 0 ) {
       /* weird, but we cannot stop unless we initialized the context */
       err = __CN__InitContext();
       if ( err != STATUS_SUCCESS ) return err; 
       __CN__FC = 1;
    }
    if ( _KN__GPU_FK == 1 ) {
        /*  Currently nothing kernel specific must be recovered */
       _KN__GPU_FK = 0;
    }
    return STATUS_SUCCESS;

} /* end of _KN__stop */


EOF
}

function write_kernel_template(){
/bin/cat <<"EOF"
    return snk_gpu_kernel(lparm, 
                _KN__Kernel_Object, 
                group_base,
                _KN__Private_Segment_Size, 
                thisKernargAddress
                );
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
      type, BIND(C) :: atmi_lparm_t
          integer (C_INT) :: ndim = 1
          integer (C_SIZE_T) :: gdims(3) = (/ 1 , 0, 0 /)
          integer (C_SIZE_T) :: ldims(3) = (/ 64, 0, 0 /)
          integer (C_INT) :: stream = -1 
          integer (C_INT) :: barrier = 1
          integer (C_INT) :: acquire_fence_scope = 2
          integer (C_INT) :: release_fence_scope = 2
      end type atmi_lparm_t
      type (atmi_lparm_t) lparm
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
    scalartypes="int,float,char,double,void,size_t,image3d_t,long,long long"
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

#  atmi_genw starts here
   
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

# If snack call atmi_genw with -fort option
__IS_FORTRAN=$8

# If snack was called with -noglobs
__NO_GLOB_FUNS=$9

__SNACK_RUNTIME_PATH=${10}
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
   cpp -I$__SNACK_RUNTIME_PATH/include $__CLF | sed -e '/__kernel/,/)/!d' |  sed -e ':a;$!N;s/\n/ /;ta;P;D' | sed -e 's/__kernel/\n__kernel/g'  | grep "__kernel" | \
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
   __KN_NUM=0
   while read line ; do 
   ((__KN_NUM++))
#     parse the kernel name __KN and the native argument list __ARGL
      TYPE_NAME=`echo ${line%(*}`
      __KN=`echo $TYPE_NAME | awk '{print $2}'`
      __KT=`echo $TYPE_NAME | awk '{print $1}'`
      __ARGL=${line#*(}
#     force it to return pointer to atmi_task_t
      __KT="atmi_task_t*" 
         

#     Add the kernel initialization routine to the c wrapper
      write_KernelStatics_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g" >>$__CWRAP
      
#     Build a corrected argument list , change CL types to c types as necessary, see parse_arg
      __CFN_ARGL=""
      __PROTO_ARGL=""
      sepchar=""
      IFS=","
      NEXTI=0
      for _val in $__ARGL ; do 
         if ((NEXTI > 0)) ; then 
         parse_arg $_val
         if [ $is_local == 1 ] ; then 
            arg_type="size_t"
            simple_arg_type="size_t"
            arg_name="${arg_name}_size"
            last_char=" " 
         fi
         __CFN_ARGL="${__CFN_ARGL}${sepchar}${simple_arg_type}${last_char} ${arg_name}"
         __PROTO_ARGL="${__PROTO_ARGL}${sepchar}${arg_type} ${arg_name}"
         sepchar=","
         fi
         NEXTI=$(( NEXTI + 1 ))
      done

#     Write start of the SNACK function
      echo "/* ------  Start of ATMI's PIF function ${__KN}_pif ------ */ " >> $__CWRAP 
      if [ "$__IS_FORTRAN" == "1" ] ; then 
#        Add underscore to kernel name and resolve lparm pointer 
         echo "extern ${__KT} ${__KN}_pif_(const atmi_lparm_t *lparm, $__CFN_ARGL) {" >>$__CWRAP
      else  
         if [ "$__CFN_ARGL" == "" ] ; then 
            echo "extern ${__KT} ${__KN}_pif(const atmi_lparm_t * lparm) {" >>$__CWRAP
         else
            echo "extern ${__KT} ${__KN}_pif(const atmi_lparm_t *lparm, $__CFN_ARGL) {" >>$__CWRAP
         fi
      fi
      echo "  if(lparm->devtype == ATMI_DEVTYPE_GPU) { " >>$__CWRAP
      #echo "   printf(\"In SNACK Kernel GPU Function ${__KN}\n\"); " >> $__CWRAP
	  echo "    /* Kernel initialization has to be done before kernel arguments are set/inspected */ " >> $__CWRAP
      echo "    if (${__KN}_GPU_FK == 0 ) { " >> $__CWRAP
      echo "      status_t status = ${__KN}_gpu_init(); " >> $__CWRAP
      echo "      if ( status  != STATUS_SUCCESS ) return; " >> $__CWRAP
      echo "      ${__KN}_GPU_FK = 1; " >> $__CWRAP
      echo "    } " >> $__CWRAP
#     Write the structure definition for the kernel arguments.
#     Consider eliminating global _KN__args and memcopy and write directly to thisKernargAddress.
#     by writing these statements here:
      echo "    /* Allocate the kernel argument buffer from the correct region. */ " >> $__CWRAP
      echo "    void* thisKernargAddress; " >> $__CWRAP
      echo "    /* FIXME: HSA 1.0F may have a bug that serializes all queue operations when hsa_memory_allocate is used. " >> $__CWRAP
	  echo " 	   Investigate more and revert back to hsa_memory_allocate once bug is fixed. */ " >> $__CWRAP
	  echo "    thisKernargAddress = malloc(${__KN}_Kernarg_Segment_Size); " >> $__CWRAP
      #echo "   int ret = posix_memalign(&thisKernargAddress, 4096, ${__KN}_Kernarg_Segment_Size); " >> $__CWRAP
	  #echo "   hsa_memory_allocate(${__SN}_KernargRegion, ${__KN}_Kernarg_Segment_Size, &thisKernargAddress); " >> $__CWRAP
#     How to map a structure into an malloced memory area?
      echo "   size_t group_base ; " >>$__CWRAP 
      echo "   group_base  = (size_t) ${__KN}_Group_Segment_Size;" >>$__CWRAP 
      echo "   struct ${__KN}_args_struct {" >> $__CWRAP
      NEXTI=0
      if [ $GENW_ADD_DUMMY ] ; then 
         echo "       uint64_t arg0;"  >> $__CWRAP
         echo "       uint64_t arg1;"  >> $__CWRAP
         echo "       uint64_t arg2;"  >> $__CWRAP
         echo "       uint64_t arg3;"  >> $__CWRAP
         echo "       uint64_t arg4;"  >> $__CWRAP
         echo "       uint64_t arg5;"  >> $__CWRAP
         echo "       atmi_task_t* arg6;"  >> $__CWRAP
         NEXTI=6
      fi
      IFS=","
      for _val in $__ARGL ; do 
         if ((NEXTI != 6)) ; then 
         parse_arg $_val
         if [ "$last_char" == "*" ] ; then 
            if [ $is_local == 1 ] ; then 
               echo "       uint64_t arg${NEXTI};" >> $__CWRAP
            else
               echo "       ${simple_arg_type}* arg${NEXTI};"  >> $__CWRAP
            fi
         else
            is_scalar $simple_arg_type
            if [ $? == 1 ] ; then 
               echo "       ${simple_arg_type} arg${NEXTI};"  >> $__CWRAP
            else
               echo "       ${simple_arg_type}* arg${NEXTI};"  >> $__CWRAP
            fi
         fi
         fi
         NEXTI=$(( NEXTI + 1 ))
      done
      echo "    } __attribute__ ((aligned (16))) ; "  >> $__CWRAP
      echo "    struct ${__KN}_args_struct* ${__KN}_args ; "  >> $__CWRAP
	  echo "    /* Setup kernel args */ " >> $__CWRAP
	  echo "    ${__KN}_args = (struct ${__KN}_args_struct*) thisKernargAddress; " >> $__CWRAP

#     Write statements to fill in the argument structure and 
#     keep track of updated CL arg list and new call list 
#     in case we have to create a wrapper CL function.
#     to call the real kernel CL function. 
      NEXTI=0
      NEXT_ARGI=0
      if [ $GENW_ADD_DUMMY ] ; then 
         echo "    ${__KN}_args->arg0=0 ; "  >> $__CWRAP
         echo "    ${__KN}_args->arg1=0 ; "  >> $__CWRAP
         echo "    ${__KN}_args->arg2=0 ; "  >> $__CWRAP
         echo "    ${__KN}_args->arg3=0 ; "  >> $__CWRAP
         echo "    ${__KN}_args->arg4=0 ; "  >> $__CWRAP
         echo "    ${__KN}_args->arg5=0 ; "  >> $__CWRAP
         echo "    ${__KN}_args->arg6=NULL ; "  >> $__CWRAP
         NEXTI=6
      fi
      KERN_NEEDS_CL_WRAPPER="FALSE"
      arglistw=""
      calllist=""
      sepchar=""
      IFS=","
      for _val in $__ARGL ; do 
         if ((NEXTI != 6)) ; then 
         parse_arg $_val
#        These echo statments help debug a lot
#        echo "simple_arg_type=|${simple_arg_type}|" 
#        echo "arg_type=|${arg_type}|" 
         if [ "$last_char" == "*" ] ; then 
            arglistw="${arglistw}${sepchar}${arg_type} ${arg_name}"
            calllist="${calllist}${sepchar}${arg_name}"
            if [ $is_local == 1 ] ; then 
               echo "    ${__KN}_args->arg${NEXTI} = (unit64_t) group_base ; "  >> $__CWRAP
               echo "    group_base += ( sizeof($simple_arg_type) * ${arg_name}_size ) ; " >> $__CWRAP
            else
               echo "    ${__KN}_args->arg${NEXTI} = $arg_name ; "  >> $__CWRAP
            fi
         else
            is_scalar $simple_arg_type
            if [ $? == 1 ] ; then 
               arglistw="$arglistw${sepchar}${arg_type} $arg_name"
               calllist="${calllist}${sepchar}${arg_name}"
               echo "    ${__KN}_args->arg${NEXTI} = $arg_name ; "  >> $__CWRAP
            else
               KERN_NEEDS_CL_WRAPPER="TRUE"
               arglistw="$arglistw${sepchar}${arg_type}* $arg_name"
               calllist="${calllist}${sepchar}${arg_name}[0]"
               echo "    ${__KN}_args->arg${NEXTI} = &$arg_name ; "  >> $__CWRAP
            fi
         fi 
         sepchar=","
         fi
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
         echo "extern _CPPSTRING_ $__KT ${__KN}_pif_(const atmi_lparm_t *lparm, $__PROTO_ARGL);" >>$__HDRF
      else
         if [ "$__PROTO_ARGL" == "" ] ; then 
            echo "extern _CPPSTRING_ $__KT ${__KN}_pif(const atmi_lparm_t * lparm);" >>$__HDRF
         else
            echo "extern _CPPSTRING_ $__KT ${__KN}_pif(const atmi_lparm_t *lparm, $__PROTO_ARGL);" >>$__HDRF
         fi
      fi

#     Now add the kernel template to wrapper and change all three strings
#     1) Context Name _CN_ 2) Kerneel name _KN_ and 3) Funtion name _FN_
      write_kernel_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g;s/_FN_/${__FN}/g" >>$__CWRAP

      echo "  } " >>$__CWRAP
      echo "  else if(lparm->devtype == ATMI_DEVTYPE_CPU) { " >>$__CWRAP
      echo "  } " >>$__CWRAP
      echo "} " >> $__CWRAP 
      echo "/* ------  End of ATMI's PIF function ${__KN}_pif ------ */ " >> $__CWRAP 

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
      cat $__EXTRACL | sed -e "s/ atmi_task_t/ void/g" >> $__UPDATED_CL
   else 
#  No changes to the CL file are needed, so just make a copy
      cat $__CLF | sed -e "s/ atmi_task_t/ void/g" > $__UPDATED_CL
   fi

   rm $__KARGLIST
   rm $__EXTRACL 
