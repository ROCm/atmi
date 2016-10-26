#MIT License
#
#Copyright © 2016 Advanced Micro Devices, Inc.
#
#Permission is hereby granted, free of charge, to any person obtaining a copy of
#this software and associated documentation files (the "Software"), to deal in
#the Software
#without restriction, including without limitation the rights to use, copy,
#modify, merge, publish, distribute, sublicense, and/or sell copies of the
#Software, and to permit
#persons to whom the Software is furnished to do so, subject to the following
#conditions:
#
#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR
#PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
#BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
#CONTRACT, TORT OR
#OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
#OR OTHER DEALINGS IN THE SOFTWARE.

function write_kernel_dispatch_template(){
/bin/cat <<"EOF"
/*
MIT License

Copyright © 2016 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software
without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit
persons to whom the Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "hsa_kl.h"
#include "atmi.h"

#define INIT_KLPARM_1D(X,Y) atmi_klparm_t *X ; atmi_klparm_t  _ ## X ={.ndim=1,.gdims={Y},.ldims={Y > 64 ? 64 : Y},.stream=-1,.barrier=0,.acquire_fence_scope=2,.release_fence_scope=2,.klist=thisTask->klist} ; X = &_ ## X ;

void kernel_dispatch(const atmi_klparm_t *klparm, const int k_id) {

    atmi_kernel_dispatch_packet_t *kernel_packet = klparm->klist->kernel_packets + k_id;

    hsa_queue_t* this_Q = (hsa_queue_t *)klparm->klist->queues[k_id];

    /* Find the queue index address to write the packet info into.  */
    const uint32_t queueMask = this_Q->size - 1;
    uint64_t index = hsa_queue_load_write_index_relaxed(this_Q);
    hsa_kernel_dispatch_packet_t* this_aql = &(((hsa_kernel_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]);

    /*  Process lparm values */
    this_aql->setup  |= (uint16_t) klparm->ndim << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
    this_aql->grid_size_x=klparm->gdims[0];
    this_aql->workgroup_size_x=klparm->ldims[0];
    if (klparm->ndim>1) {
        this_aql->grid_size_y=klparm->gdims[1];
        this_aql->workgroup_size_y=klparm->ldims[1];
    } else {
        this_aql->grid_size_y=1;
        this_aql->workgroup_size_y=1;
    }

    if (klparm->ndim>2) {
        this_aql->grid_size_z=klparm->gdims[2];
        this_aql->workgroup_size_z=klparm->ldims[2];
    }
    else
    {
        this_aql->grid_size_z=1;
        this_aql->workgroup_size_z=1;
    }

    /* thisKernargAddress has already been set up in the beginning of this routine */
    /*  Bind kernel argument buffer to the aql packet.  */
    this_aql->kernarg_address = kernel_packet->kernarg_address;
    this_aql->kernel_object = kernel_packet->kernel_object;
    this_aql->private_segment_size = kernel_packet->private_segment_size;
    this_aql->group_segment_size = kernel_packet->group_segment_size;
    this_aql->completion_signal = kernel_packet->completion_signal;

    hsa_signal_add_relaxed(this_aql->completion_signal, 1);

    /*  Prepare and set the packet header */ 
    /* Only set barrier bit if asynchrnous execution */
    int stream_num = klparm->stream;
    if ( stream_num >= 0 )  
        this_aql->header |= klparm->barrier << HSA_PACKET_HEADER_BARRIER; 
    this_aql->header |= klparm->acquire_fence_scope << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
    this_aql->header |= klparm->release_fence_scope << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;

    ((uint8_t*)(&this_aql->header))[0] = (uint8_t)HSA_PACKET_TYPE_KERNEL_DISPATCH;

    /* Increment write index and ring doorbell to dispatch the kernel.  */
    hsa_queue_store_write_index_relaxed(this_Q, index + 1);

    //FIXME ring doorbell not work on GPU
    hsa_signal_store_relaxed(this_Q->doorbell_signal, index);
}
EOF
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
__CLF=$1
ATMI_RUNTIME_PATH=$2

#  Work space
__TMPD=$3
__UPDATED_CL=$4

#  Outputs: cwrapper, header file, and updated CL 
__STRUCT_CL=${__TMPD}/kernel_struct.cl
__SPAWN_CL=${__TMPD}/kernel_spawn.cl

# Intermediate files.
__EXTRACL=extra.cl
__KARGLIST=klist
__ARGL=""

__WRAPPRE="_"
__SEDCMD=" "

#   if [ $GENW_ADD_DUMMY ] ; then 
#      echo
#      echo "WARNING:  DUMMY ARGS ARE ADDED FOR STABLE COMPILER "
#      echo
#   fi

#  Read the CLF and build a list of kernels and args, one kernel and set of args per line of KARGLIST file
   cpp -I$ATMI_RUNTIME_PATH/include $__CLF | sed -e '/__kernel/,/)/!d' |  sed -e ':a;$!N;s/\n/ /;ta;P;D' | sed -e 's/__kernel/\n__kernel/g'  | grep "__kernel" | \
   sed -e "s/__kernel//;s/__global//g;s/{//g;s/ \*/\*/g"  | cut -d\) -f1 | sed -e "s/\*/\* /g;s/__restrict__//g" >$__KARGLIST

#  The header and extra-cl files must start empty because lines are incrementally added to end of file
   if [ -f $__EXTRACL ] ; then rm -f $__EXTRACL ; fi
   touch $__EXTRACL

#  Create header file for c and c++ with extra lparm arg (global and local dimensions)

   echo "/* STRUCT OF KERNEL ARGS */" > $__STRUCT_CL

   echo "/* SPAWN KERNELS */" > $__SPAWN_CL


   sed_sepchar=""
   KERNEL_NUM=0
   while read line ; do 

#     parse the kernel name __KN and the native argument list __ARGL
      TYPE_NAME=`echo ${line%(*}`
      __KN=`echo $TYPE_NAME | awk '{print $2}'`
      __KT=`echo $TYPE_NAME | awk '{print $1}'`
      __ARGL=${line#*(}
#     force it to return pointer to snk_task_t
      if [ "$__KT" == "snk_task_t" ] ; then  
         __KT="snk_task_t*" 
      fi
         

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

      echo "void spawn_${__KN}(atmi_klparm_t *klparm, $__CFN_ARGL){" >> $__SPAWN_CL
      echo "   int k_id = $KERNEL_NUM;" >> $__SPAWN_CL
      KERNEL_NUM=$((KERNEL_NUM + 1))
      echo "   atmi_kernel_dispatch_packet_t *packet = klparm->klist->kernel_packets + k_id;" >> $__SPAWN_CL
      echo "   struct ${__KN}_args_struct * ${__KN}_args = packet->kernarg_address;" >> $__SPAWN_CL
      echo "   packet->completion_signal = *((hsa_signal_t *)(thisTask->handle));" >> $__SPAWN_CL

      #     Write statements to fill in the argument structure and 
      #     keep track of updated CL arg list and new call list 
      #     in case we have to create a wrapper CL function.
      #     to call the real kernel CL function. 
      NEXTI=0
      #if [ $GENW_ADD_DUMMY ] ; then 
      echo "   ${__KN}_args->arg0=0 ; "  >> $__SPAWN_CL
      echo "   ${__KN}_args->arg1=0 ; "  >> $__SPAWN_CL
      echo "   ${__KN}_args->arg2=0 ; "  >> $__SPAWN_CL
      echo "   ${__KN}_args->arg3=0 ; "  >> $__SPAWN_CL
      echo "   ${__KN}_args->arg4=0 ; "  >> $__SPAWN_CL
      echo "   ${__KN}_args->arg5=0 ; "  >> $__SPAWN_CL
      NEXTI=6
      #fi
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
              echo "   ${__KN}_args->arg${NEXTI} = $arg_name ; "  >> $__SPAWN_CL
          else
              is_scalar $simple_arg_type
              if [ $? == 1 ] ; then 
                  arglistw="$arglistw${sepchar}${arg_type} $arg_name"
                  calllist="${calllist}${sepchar}${arg_name}"
                  echo "   ${__KN}_args->arg${NEXTI} = $arg_name ; "  >> $__SPAWN_CL
              else
                  KERN_NEEDS_CL_WRAPPER="TRUE"
                  arglistw="$arglistw${sepchar}${arg_type}* $arg_name"
                  calllist="${calllist}${sepchar}${arg_name}[0]"
                  echo "   ${__KN}_args->arg${NEXTI} = &$arg_name ; "  >> $__SPAWN_CL
              fi
          fi 
          sepchar=","
          NEXTI=$(( NEXTI + 1 ))
      done 
      #echo "   klparm->slist = thisTask->klist->slist;" >> $__SPAWN_CL
      #echo "   klparm->qlist = thisTask->klist->qlist;" >> $__SPAWN_CL
      #echo "   klparm->plist = thisTask->klist->plist;" >> $__SPAWN_CL
      echo "   kernel_dispatch(klparm, k_id);" >> $__SPAWN_CL 
      echo "}" >> $__SPAWN_CL

      echo "struct ${__KN}_args_struct {" >> $__STRUCT_CL
      NEXTI=0
      #if [ $GENW_ADD_DUMMY ] ; then 
      echo "  uint64_t arg0;"  >> $__STRUCT_CL
      echo "  uint64_t arg1;"  >> $__STRUCT_CL
      echo "  uint64_t arg2;"  >> $__STRUCT_CL
      echo "  uint64_t arg3;"  >> $__STRUCT_CL
      echo "  uint64_t arg4;"  >> $__STRUCT_CL
      echo "  uint64_t arg5;"  >> $__STRUCT_CL
      NEXTI=6
      #fi
      IFS=","
      for _val in $__ARGL ; do 
         parse_arg $_val
         if [ "$last_char" == "*" ] ; then 
            echo "  ${simple_arg_type}* arg${NEXTI};"  >> $__STRUCT_CL
         else
            is_scalar $simple_arg_type
            if [ $? == 1 ] ; then 
               echo "  ${simple_arg_type} arg${NEXTI};"  >> $__STRUCT_CL
            else
               echo "  ${simple_arg_type}* arg${NEXTI};"  >> $__STRUCT_CL
            fi
         fi
         NEXTI=$(( NEXTI + 1 ))
      done
      echo "} __attribute__ ((aligned (16))) ; "  >> $__STRUCT_CL


#  END OF WHILE LOOP TO PROCESS EACH KERNEL IN THE CL FILE
   done < $__KARGLIST

   
   #  Write the updated CL
   if [ "$__SEDCMD" != " " ] ; then 
       #     Remove extra spaces, then change "__kernel void" to "void" if they have call-by-value structs
       #     Still could fail if __kernel void _FN_ split across multple lines, FIX THIS
       write_kernel_dispatch_template > $__UPDATED_CL
       cat $__STRUCT_CL >> $__UPDATED_CL
       cat $__SPAWN_CL >> $__UPDATED_CL
       awk '$1=$1'  $__CLF | sed -e "$__SEDCMD" >> $__UPDATED_CL
       cat $__EXTRACL | sed -e "s/ snk_task_t/ void/g" >> $__UPDATED_CL
   else 
       #  No changes to the CL file are needed, so just make a copy
       write_kernel_dispatch_template > $__UPDATED_CL
       cat $__STRUCT_CL >> $__UPDATED_CL
       cat $__SPAWN_CL >> $__UPDATED_CL
       cat $__CLF | sed -e "s/ snk_task_t/ void/g" >> $__UPDATED_CL
   fi

   rm $__STRUCT_CL
   rm $__SPAWN_CL
   rm $__KARGLIST
   rm $__EXTRACL 

