#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "hsa.h"
#include "hsa_ext_finalize.h"
#include "Brig_new.hpp"
#include "../common/elf_utils.hpp"
#include <iostream>

#define MULTILINE(...) # __VA_ARGS__

#if defined(_MSC_VER)
#define ALIGNED_(x) __declspec(align(x))
#else
#if defined(__GNUC__)
#define ALIGNED_(x) __attribute__ ((aligned(x)))
#endif
#endif

#define CommonLog(msg) printf( "[ ERROR ] In %s() in line %d ", __FUNCTION__ , __LINE__);

#define ErrorCheck(status) if (status != HSA_STATUS_SUCCESS) { \
  CommonLog(msg) \
}
static hsa_status_t IterateAgent(hsa_agent_t agent, void *data) {
	// Find GPU device and use it.
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

size_t roundUp(size_t size, size_t round_value) {
  size_t times = size / round_value;
  size_t rem = size % round_value;
  if (rem != 0) ++times;
  return times * round_value;
}
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
bool FindSymbolOffset(hsa_ext_brig_module_t* brig_module, 
    char* symbol_name,
    hsa_ext_brig_code_section_offset32_t& offset) {
    
    //Get the data section
     hsa_ext_brig_section_header_t* data_section_header = 
                brig_module->section[HSA_EXT_BRIG_SECTION_DATA];
    //Get the code section
     hsa_ext_brig_section_header_t* code_section_header =
             brig_module->section[HSA_EXT_BRIG_SECTION_CODE];

    //First entry into the BRIG code section
    BrigCodeOffset32_t code_offset = code_section_header->header_byte_count;
    BrigBase* code_entry = (BrigBase*) ((char*)code_section_header + code_offset);
    while (code_offset != code_section_header->byte_count) {
        if (code_entry->kind == BRIG_KIND_DIRECTIVE_KERNEL) {
            //Now find the data in the data section
            BrigDirectiveKernel* directive_kernel = (BrigDirectiveKernel*) (code_entry);
            BrigDataOffsetString32_t data_name_offset = directive_kernel->name;
            BrigData* data_entry = (BrigData*)((char*) data_section_header + data_name_offset);
            if (!strcmp(symbol_name, (char*)data_entry->bytes)){
                offset = code_offset;
                return true;
            }

        }
        code_offset += code_entry->byteCount;
        code_entry = (BrigBase*) ((char*)code_section_header + code_offset);
    }
    return false;
}


int main(int argc, char **argv)
{
	
	hsa_status_t err;

	err=hsa_init();
	ErrorCheck(err);

	//Get the device
	hsa_agent_t device = 0;
    //Iterate over the agents and pick the agent using IterateAgent
	err = hsa_iterate_agents(IterateAgent, &device);
	ErrorCheck(err);

	if(device == 0)
	{
		printf("No HSA devices found!\n");
		return 1;
	}

	//Print out name of the device
	char name[64] = { 0 };
	err = hsa_agent_get_info(device, HSA_AGENT_INFO_NAME, name);
	ErrorCheck(err);
	printf("Using: %s\n", name);

	//Find the maximum size of the queue
	uint32_t queue_size = 0;
	err = hsa_agent_get_info(device, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size);
	ErrorCheck(err);

    //Create a queue
	hsa_queue_t* commandQueue;
	err =
		hsa_queue_create(
		device, queue_size, HSA_QUEUE_TYPE_MULTI, NULL, NULL, &commandQueue);
	ErrorCheck(err);

	//Convert hsail kernel text to BRIG.
	hsa_ext_brig_module_t* brigModule;
    char file_name[128] = "vector_copy.brig";
	if (!CreateBrigModuleFromBrigFile(file_name, &brigModule)){
		ErrorCheck(HSA_STATUS_ERROR);
	}

	//Create hsa program.
	hsa_ext_program_handle_t hsaProgram;
	err = hsa_ext_program_create(&device, 1, HSA_EXT_BRIG_MACHINE_LARGE, HSA_EXT_BRIG_PROFILE_FULL, &hsaProgram);
	ErrorCheck(err);

	//Add BRIG module to hsa program.
	hsa_ext_brig_module_handle_t module;
	err = hsa_ext_add_module(hsaProgram, brigModule, &module);
	ErrorCheck(err);

	// Construct finalization request list.
	hsa_ext_finalization_request_t finalization_request_list;
	finalization_request_list.module = module;              // module handle.
	finalization_request_list.program_call_convention = 0;  // program call convention. not supported.
    char kernel_name[128] = "&__OpenCL_test_kernel";
	if (!FindSymbolOffset(brigModule, kernel_name, finalization_request_list.symbol)){
		ErrorCheck(HSA_STATUS_ERROR);
	}

	//Finalize hsa program.
	err = hsa_ext_finalize_program(hsaProgram, device, 1, &finalization_request_list, NULL, NULL, 0, NULL, 0);
	ErrorCheck(err);

	//Get hsa code descriptor address.
	hsa_ext_code_descriptor_t *hsaCodeDescriptor;
	err = hsa_ext_query_kernel_descriptor_address(hsaProgram, module, finalization_request_list.symbol, &hsaCodeDescriptor);
	ErrorCheck(err);

	//Get a signal
	hsa_signal_t signal;
	err=hsa_signal_create(1, 0, NULL, &signal);
	ErrorCheck(err);

	//setup dispatch packet
	hsa_dispatch_packet_t aql;
	memset(&aql, 0, sizeof(aql));

	//Setup dispatch size and fences
	aql.completion_signal=signal;
	aql.dimensions=1;
	aql.workgroup_size_x=256;
	aql.workgroup_size_y=1;
	aql.workgroup_size_z=1;
	aql.grid_size_x=1024*1024;
	aql.grid_size_y=1;
	aql.grid_size_z=1;
	aql.header.type=HSA_PACKET_TYPE_DISPATCH;
	aql.header.acquire_fence_scope=2;
	aql.header.release_fence_scope=2;
	aql.header.barrier=1;
	aql.group_segment_size=0;
	aql.private_segment_size=0;
	
	//Setup kernel arguments
	char* in=(char*)malloc(1024*1024*4);
	char* out=(char*)malloc(1024*1024*4);

	memset(out, 0, 1024*1024*4);
	memset(in, 1, 1024*1024*4);

	err=hsa_memory_register(in, 1024*1024*4);
	ErrorCheck(err);
	err=hsa_memory_register(out, 1024*1024*4);
	ErrorCheck(err);

	struct ALIGNED_(HSA_ARGUMENT_ALIGN_BYTES) args_t
	{
		uint64_t arg0;
		uint64_t arg1;
		uint64_t arg2;
        uint64_t arg3;
        uint64_t arg4;
        uint64_t arg5;
		void* arg6;
		void* arg7;
	} args;
	args.arg0=0;
	args.arg1=0;
	args.arg2=0;
    args.arg3=0;
    args.arg4=0;
    args.arg5=0;
	args.arg6=out;
	args.arg7=in;

    hsa_region_t kernarg_region = 0;
    hsa_agent_iterate_regions(device, get_kernarg, &kernarg_region);
    if (!kernarg_region) {
        printf ("Could not find region for kernel arguments \n");
    }
    void* kernel_arg_buffer = NULL;
   
   
   /* Start Temporary work around since hsa_memory_allocate does not round up - Not required in alpha*/
    size_t granule;
    hsa_region_get_info(kernarg_region, HSA_REGION_INFO_ALLOC_GRANULE,  &granule);
    size_t kernel_arg_buffer_size = roundUp(hsaCodeDescriptor->kernarg_segment_byte_size, granule);
    /* End Temporary work around */
   
   
   err = hsa_memory_allocate(kernarg_region, kernel_arg_buffer_size, 
                        &kernel_arg_buffer);
    ErrorCheck(err);
	memcpy (kernel_arg_buffer, &args, sizeof(args));
	//Bind kernel arguments and kernel code
	aql.kernel_object_address=hsaCodeDescriptor->code.handle;
	aql.kernarg_address=(uint64_t)kernel_arg_buffer;

	//Register argument buffer
	hsa_memory_register(&args, sizeof(args_t));

	const uint32_t queueSize=commandQueue->size;
	const uint32_t queueMask=queueSize-1;

	uint64_t index=hsa_queue_load_write_index_relaxed(commandQueue);
	((hsa_dispatch_packet_t*)(commandQueue->base_address))[index&queueMask]=aql;
	hsa_queue_store_write_index_relaxed(commandQueue, index+1);

	//Ringdoor bell - Dispatch the kernel
	hsa_signal_store_relaxed(commandQueue->doorbell_signal, index+1);

    if (hsa_signal_wait_acquire(signal, HSA_LT, 1, uint64_t(-1), HSA_WAIT_EXPECTANCY_UNKNOWN)!=0)
	{
		printf("Signal wait returned unexpected value\n");
		exit(0);
	}

		hsa_signal_store_relaxed(signal, 1);

	//Validate
	bool valid=true;
	int failIndex=0;
	for(int i=0; i<1024*1024; i++)
	{
		if(out[i]!=in[i])
		{
			failIndex=i;
			valid=false;
			break;
		}
	}
	if(valid)
		printf("passed validation\n");
	else
	{
		printf("VALIDATION FAILED!\nBad index: %d\n", failIndex);
	}

	//Cleanup
	err=hsa_signal_destroy(signal);
	ErrorCheck(err);

	err=hsa_ext_program_destroy(hsaProgram);
	ErrorCheck(err);

	err=hsa_queue_destroy(commandQueue);
	ErrorCheck(err);
	
	err=hsa_shut_down();
	ErrorCheck(err);

	free(in);
	free(out);

    return 0;

}

