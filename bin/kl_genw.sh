function write_hw_init_template(){
/bin/cat << "EOF"
    if (klist_initalized == 0) {
        atmi_klist = (atmi_klist_t *)malloc(sizeof(atmi_klist_t));
        atmi_klist->qlist = NULL;
        atmi_klist->slist = NULL;
        atmi_klist->plist = NULL;
        atmi_klist->num_signal = 0;
        atmi_klist->num_queue = 0;
        atmi_klist->num_kernel = 0;
        klist_initalized = 1;
    }

    if(gpu_initalized == 0) {
        snk_init_context(atmi_klist);
        snk_init_gpu_context();
        snk_gpu_create_program();
        snk_gpu_add_brig_module(hw_HSA_BrigMem); 
        snk_gpu_build_executable(&g_executable);
        gpu_initalized = 1;
    }
EOF
}

function write_kernel_init_template(){
/bin/cat <<"EOF"

    hsa_status_t err;

    err = hsa_init();
    ErrorCheck(Initializing the hsa device, err);

    hsa_agent_t kernel_dispatch_Agent;
    err = hsa_iterate_agents(get_gpu_agent, &kernel_dispatch_Agent);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    ErrorCheck(Getting a gpu agent, err);

    uint32_t queue_size = 0;
    err = hsa_agent_get_info(kernel_dispatch_Agent, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size);
    ErrorCheck(Querying the agent maximum queue size, err);

    hsa_queue_t *queue;
    err = hsa_queue_create(kernel_dispatch_Agent, queue_size, HSA_QUEUE_TYPE_SINGLE, NULL, NULL, UINT32_MAX, UINT32_MAX, &queue);
    ErrorCheck(Creating the queue, err);

    atmi_klist->num_queue++;
    atmi_klist->qlist = (uint64_t *)realloc(atmi_klist->qlist, sizeof(uint64_t) * atmi_klist->num_queue);
    atmi_klist->qlist[atmi_klist->num_queue - 1] = (uint64_t)queue;

    hsa_signal_t signal = 0;
    if(lparm->synchronous == ATMI_FALSE)
    {
        err = hsa_signal_create(0, 0, NULL, &signal);
        ErrorCheck(Creating a HSA signal, err);
    }

    atmi_klist->num_signal++;
    atmi_klist->slist = (hsa_signal_t *)realloc(atmi_klist->slist, sizeof(hsa_signal_t) * atmi_klist->num_signal);
    atmi_klist->slist[atmi_klist->num_signal - 1] = signal;

    atmi_klist->num_kernel++;
    atmi_klist->plist = (hsa_kernel_dispatch_packet_t *)realloc(atmi_klist->plist, sizeof(hsa_kernel_dispatch_packet_t) * atmi_klist->num_kernel);
    hsa_kernel_dispatch_packet_t *this_aql = &atmi_klist->plist[atmi_klist->num_kernel - 1];

    uint64_t _KN__Kernel_Object;
    uint32_t _KN__Group_Segment_Size;
    uint32_t _KN__Private_Segment_Size;
    snk_get_gpu_kernel_info(g_executable, kernel_name, &_KN__Kernel_Object, 
    &_KN__Group_Segment_Size, &_KN__Private_Segment_Size);

    /* thisKernargAddress has already been set up in the beginning of this routine */
    /*  Bind kernel argument buffer to the aql packet.  */
    this_aql->kernarg_address = (void*) thisKernargAddress;
    this_aql->kernel_object = _KN__Kernel_Object;
    this_aql->private_segment_size = _KN__Private_Segment_Size;
    this_aql->group_segment_size = _KN__Group_Segment_Size;

    //printf("queue: %d signal: %d aql:%d %d %d %d\n", queue, signal, thisKernargAddress, _KN__Kernel_Object, _KN__Private_Segment_Size, _KN__Group_Segment_Size);

EOF
}

function write_utils_template(){
/bin/cat <<"EOF"

#define ErrorCheck(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed.\n", #msg); \
    exit(1); \
} else { \
 /*  printf("%s succeeded.\n", #msg);*/ \
}

/* Determines if the given agent is of type HSA_DEVICE_TYPE_GPU
   and sets the value of data to the agent handle if it is.
*/
static hsa_status_t get_gpu_agent(hsa_agent_t agent, void *data) {
    hsa_status_t status;
    hsa_device_type_t device_type;
    status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
    if (HSA_STATUS_SUCCESS == status && HSA_DEVICE_TYPE_GPU == device_type) {
        hsa_agent_t* ret = (hsa_agent_t*)data;
        *ret = agent;
        return HSA_STATUS_INFO_BREAK;
    }
    return HSA_STATUS_SUCCESS;
}

EOF
}
function write_kernel_dispatch_template(){
/bin/cat <<"EOF"

#include "hsa_kl.h"
#include "atmi.h"

#define INIT_KLPARM_1D(X,Y) atmi_klparm_t *X ; atmi_klparm_t  _ ## X ={.ndim=1,.gdims={Y},.ldims={Y > 64 ? 64 : Y},.stream=-1,.barrier=0,.acquire_fence_scope=2,.release_fence_scope=2,.klist=thisTask->klist} ; X = &_ ## X ;

void kernel_dispatch(const atmi_klparm_t *lparm_d, const int k_id) {

    hsa_queue_t* this_Q = (hsa_queue_t *)lparm_d->klist->qlist[k_id];

    /* Find the queue index address to write the packet info into.  */
    const uint32_t queueMask = this_Q->size - 1;
    uint64_t index = hsa_queue_load_write_index_relaxed(this_Q);
    hsa_kernel_dispatch_packet_t* this_aql = &(((hsa_kernel_dispatch_packet_t*)(this_Q->base_address))[index&queueMask]);

    /*  Process lparm values */
    this_aql->setup  |= (uint16_t) lparm_d->ndim << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
    this_aql->grid_size_x=lparm_d->gdims[0];
    this_aql->workgroup_size_x=lparm_d->ldims[0];
    if (lparm_d->ndim>1) {
        this_aql->grid_size_y=lparm_d->gdims[1];
        this_aql->workgroup_size_y=lparm_d->ldims[1];
    } else {
        this_aql->grid_size_y=1;
        this_aql->workgroup_size_y=1;
    }

    if (lparm_d->ndim>2) {
        this_aql->grid_size_z=lparm_d->gdims[2];
        this_aql->workgroup_size_z=lparm_d->ldims[2];
    }
    else
    {
        this_aql->grid_size_z=1;
        this_aql->workgroup_size_z=1;
    }

    /* thisKernargAddress has already been set up in the beginning of this routine */
    /*  Bind kernel argument buffer to the aql packet.  */
    hsa_kernel_dispatch_packet_t *aql = lparm_d->klist->plist + k_id;
    this_aql->kernarg_address = aql->kernarg_address;
    this_aql->kernel_object = aql->kernel_object;
    this_aql->private_segment_size = aql->private_segment_size;
    this_aql->group_segment_size = aql->group_segment_size;

    this_aql->completion_signal = lparm_d->klist->slist[k_id];
    hsa_signal_add_relaxed(this_aql->completion_signal, 1);

    /*  Prepare and set the packet header */ 
    /* Only set barrier bit if asynchrnous execution */
    int stream_num = lparm_d->stream;
    if ( stream_num >= 0 )  
        this_aql->header |= lparm_d->barrier << HSA_PACKET_HEADER_BARRIER; 
    this_aql->header |= lparm_d->acquire_fence_scope << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;
    this_aql->header |= lparm_d->release_fence_scope << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;

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
__CWRAP=kernel_wrapper.c

# If snack call snk_genw with -fort option
__IS_FORTRAN=0

# If snack was called with -noglobs
__NO_GLOB_FUNS=0

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
   sed -e "s/__kernel//;s/__global//g;s/{//g;s/ \*/\*/g"  | cut -d\) -f1 | sed -e "s/\*/\* /g;s/__restrict__//g;s/_gpu//g" >$__KARGLIST

#  The header and extra-cl files must start empty because lines are incrementally added to end of file
   if [ -f $__EXTRACL ] ; then rm -f $__EXTRACL ; fi
   touch $__EXTRACL

#  Create header file for c and c++ with extra lparm arg (global and local dimensions)

   echo "/* STRUCT OF KERNEL ARGS */" > $__STRUCT_CL

   echo "/* SPAWN KERNELS */" > $__SPAWN_CL

   write_utils_template > $__CWRAP

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
         

#     Add the kernel initialization routine to the c wrapper
      #write_KernelStatics_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g" >>$__CWRAP

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
      echo "/* ------  Start of dynamic dispatch function of ${__KN} ------ */ " >> $__CWRAP 
      echo "extern _CPPSTRING_ void ${__KN}_kl_init(atmi_lparm_t *lparm) {" >> $__CWRAP 
      echo "    if (${__KN}_FK == 0 ){" >> $__CWRAP
      echo "        snk_pif_init(${__KN}_pif_fn_table, sizeof(${__KN}_pif_fn_table)/sizeof(${__KN}_pif_fn_table[0]));" >> $__CWRAP
      echo "        ${__KN}_FK = 1;" >> $__CWRAP
      echo "    }" >> $__CWRAP
      write_hw_init_template >> $__CWRAP
      echo "    /* Allocate the kernel argument buffer from the correct region. */ " >> $__CWRAP
      echo "    void* thisKernargAddress; " >> $__CWRAP
      echo "    snk_gpu_memory_allocate(lparm, g_executable, \"${__KN}\", &thisKernargAddress);" >> $__CWRAP
      echo " " >> $__CWRAP 
      echo "    const char kernel_name[] = \"&__OpenCL_${__KN}_gpu_kernel\";" >> $__CWRAP

     
      write_kernel_init_template >> $__CWRAP

     
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

#     Now add the kernel template to wrapper and change all three strings
#     1) Context Name _CN_ 2) Kerneel name _KN_ and 3) Funtion name _FN_
      #write_kernel_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g;s/_FN_/${__FN}/g" >>$__CWRAP

      echo "    return;" >> $__CWRAP 
      echo "} " >> $__CWRAP 
      echo "/* ------  End of SNACK function ${__KN} ------ */ " >> $__CWRAP 

#     Add the kernel initialization routine to the c wrapper
      #write_InitKernel_template | sed -e "s/_CN_/${__SN}/g;s/_KN_/${__KN}/g;s/_FN_/${__FN}/g" >>$__CWRAP

      echo "void spawn_${__KN}(atmi_klparm_t *lparm_d, $__CFN_ARGL){" >> $__SPAWN_CL
      echo "   int k_id = $KERNEL_NUM;" >> $__SPAWN_CL
      KERNEL_NUM=$((KERNEL_NUM + 1))
      echo "   hsa_kernel_dispatch_packet_t *aql = lparm_d->klist->plist + k_id;" >> $__SPAWN_CL
      echo "   struct ${__KN}_args_struct * ${__KN}_args = aql->kernarg_address;" >> $__SPAWN_CL
      echo "   lparm_d->klist->slist[k_id] = lparm_d->klist->slist[k_id] == 0 ? *((hsa_signal_t *)(thisTask->handle)) : lparm_d->klist->slist[k_id];" >> $__SPAWN_CL

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
      #echo "   lparm_d->slist = thisTask->klist->slist;" >> $__SPAWN_CL
      #echo "   lparm_d->qlist = thisTask->klist->qlist;" >> $__SPAWN_CL
      #echo "   lparm_d->plist = thisTask->klist->plist;" >> $__SPAWN_CL
      echo "   kernel_dispatch(lparm_d, k_id);" >> $__SPAWN_CL 
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

   KERNEL_NUM=0
   while read line ; do 
       #     parse the kernel name __KN and the native argument list __ARGL
       TYPE_NAME=`echo ${line%(*}`
       __KN=`echo $TYPE_NAME | awk '{print $2}'`
       echo "extern _CPPSTRING_ void ${__KN}_kl_sync(){" >> $__CWRAP
       echo "    hsa_signal_wait_acquire(atmi_klist->slist[$KERNEL_NUM], HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, HSA_WAIT_STATE_BLOCKED);" >> $__CWRAP
       KERNEL_NUM=$((KERNEL_NUM + 1))
       echo "}" >> $__CWRAP
       #echo "extern _CPPSTRING_ void ${__KN}_kl_sync();" >> $__HDRF
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

   if [ "$__IS_FORTRAN" == "1" ] ; then 
      write_fortran_lparm_t
   fi

   rm $__STRUCT_CL
   rm $__SPAWN_CL
   rm $__KARGLIST
   rm $__EXTRACL 

