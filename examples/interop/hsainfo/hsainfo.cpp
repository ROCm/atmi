/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "atmi_interop_hsa.h"
#include <iostream>
#include <vector>
#include <string>
using namespace std;

int print_memory_pool_info(hsa_amd_memory_pool_t memory_pool);
int print_agent_info(hsa_agent_t agent);

#define ATMIErrorCheck(err) { \
    if(err != ATMI_STATUS_SUCCESS) { \
        cerr << "[" << __FILE__ << ", " << __LINE__ << "] ATMI Interop Failed" << endl; \
        return -1; \
    } \
}

#define HSAErrorCheck(err) { \
    if(err != HSA_STATUS_SUCCESS) { \
        cerr << "[" << __FILE__ << ", " << __LINE__ << "] HSA Query Failed" << endl; \
        return -1; \
    } \
}

int main(int argc, char **argv) {
    atmi_status_t err = atmi_init(ATMI_DEVTYPE_ALL);
    ATMIErrorCheck(err);
    
    /* enumerate some ATMI compute and memory places */
    const int gpu_id = 0;
    const int cpu_id = 0;
    atmi_place_t gpu = (atmi_place_t)ATMI_PLACE_GPU(0, gpu_id);
    atmi_place_t cpu = (atmi_place_t)ATMI_PLACE_CPU(0, cpu_id);

    atmi_mem_place_t gpu_mem = ATMI_MEM_PLACE(ATMI_DEVTYPE_GPU, gpu_id, 0);
    atmi_mem_place_t cpu_mem = ATMI_MEM_PLACE(ATMI_DEVTYPE_CPU, cpu_id, 0);
    
    /* get the corresponding HSA handles */
    hsa_agent_t cpu_agent;
    hsa_agent_t gpu_agent;
    hsa_amd_memory_pool_t cpu_mem_pool;
    hsa_amd_memory_pool_t gpu_mem_pool;

    err = atmi_interop_hsa_get_agent(cpu, &cpu_agent);
    ATMIErrorCheck(err);
    err = atmi_interop_hsa_get_agent(gpu, &gpu_agent);
    ATMIErrorCheck(err);
    err = atmi_interop_hsa_get_memory_pool(cpu_mem, &cpu_mem_pool);
    ATMIErrorCheck(err);
    err = atmi_interop_hsa_get_memory_pool(gpu_mem, &gpu_mem_pool);
    ATMIErrorCheck(err);
    /* query and print */

    int ret = 0;
    ret = print_agent_info(cpu_agent);
    if(ret != 0) {
        cout << "HSA Agent Interop Failed\n" << endl;
        return ret;
    }
    ret = print_agent_info(gpu_agent);
    if(ret != 0) {
        cout << "HSA Agent Interop Failed\n" << endl;
        return ret;
    }

    ret = print_memory_pool_info(cpu_mem_pool);
    if(ret != 0) {
        cout << "HSA Agent Interop Failed\n" << endl;
        return ret;
    }
    ret = print_memory_pool_info(gpu_mem_pool);
    if(ret != 0) {
        cout << "HSA Agent Interop Failed\n" << endl;
        return ret;
    }
    /* finalize */
    atmi_finalize();
    return 0;
}

int print_agent_info(hsa_agent_t agent) {
    static int agent_number = 0;
	int region_number = 0;
	int memory_pool_number = 0;
	hsa_status_t err;
	{
		// Increase the number of agent 
		agent_number++;

		// Get agent name and vendor
		char name[64];
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_NAME, name);
		HSAErrorCheck(err);
		char vendor_name[64];
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_VENDOR_NAME, &vendor_name);
		HSAErrorCheck(err);
		
		// Get agent feature
		hsa_agent_feature_t agent_feature;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_FEATURE, &agent_feature);
		HSAErrorCheck(err);

		// Get profile supported by the agent
		hsa_profile_t agent_profile;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_PROFILE, &agent_profile);
		HSAErrorCheck(err);

		// Get floating-point rounding mode
  		hsa_default_float_rounding_mode_t float_rounding_mode;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEFAULT_FLOAT_ROUNDING_MODE, &float_rounding_mode);
		HSAErrorCheck(err);

		// Get max number of queue
		uint32_t max_queue = 0;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUES_MAX, &max_queue);
		HSAErrorCheck(err);

		// Get queue min size
		uint32_t queue_min_size = 0;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUE_MIN_SIZE, &queue_min_size);
		HSAErrorCheck(err);
		
		// Get queue max size
		uint32_t queue_max_size = 0;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_max_size);
		HSAErrorCheck(err);
		
		// Get queue type
		hsa_queue_type_t queue_type;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUE_TYPE, &queue_type);
		HSAErrorCheck(err);

		// Get agent node
		uint32_t node;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_NODE, &node);
		HSAErrorCheck(err);

		// Get device type
		hsa_device_type_t device_type;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
		HSAErrorCheck(err);
		
		// Get cache size
		uint32_t cache_size[4];
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_CACHE_SIZE, cache_size);
		HSAErrorCheck(err);
	
		// Get chip id
		uint32_t chip_id = 0; 
		err = hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_CHIP_ID, &chip_id);
		HSAErrorCheck(err);

		// Get cacheline size
		uint32_t cacheline_size = 0;
		err = hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_CACHELINE_SIZE, &cacheline_size);
		HSAErrorCheck(err);

		// Get Max clock frequency
		uint32_t max_clock_freq = 0;
		err = hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_MAX_CLOCK_FREQUENCY, &max_clock_freq);
		HSAErrorCheck(err);

		// Get number of Compute Unit
		uint32_t compute_unit = 0;
		err = hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_COMPUTE_UNIT_COUNT, &compute_unit);
		HSAErrorCheck(err);

		// Print out the common results
		cout<<endl;
		cout<<"Agent #"<<agent_number<<":"<<endl;
		cout<<"Agent Name:					"<<name<<endl;
		cout<<"Agent Vendor Name:				"<<vendor_name<<endl;
		if(agent_feature & HSA_AGENT_FEATURE_KERNEL_DISPATCH && agent_feature & HSA_AGENT_FEATURE_AGENT_DISPATCH)
			cout<<"Agent Feature:					KERNEL_DISPATCH & AGENT_DISPATCH"<<endl;
		else if(agent_feature & HSA_AGENT_FEATURE_KERNEL_DISPATCH)
			cout<<"Agent Feature:					KERNEL_DISPATCH"<<endl;
		else if(agent_feature & HSA_AGENT_FEATURE_AGENT_DISPATCH)
			cout<<"Agent Feature:					AGENT_DISPATCH"<<endl;
		else 
			cout<<"Agent Feature:					Not Supported"<<endl;
		if(HSA_PROFILE_BASE == agent_profile)
			cout<<"Agent Profile:					BASE_PROFILE"<<endl;
		else if(HSA_PROFILE_FULL == agent_profile)
			cout<<"Agent Profile:					FULL_PROFILE"<<endl;
		else 
			cout<<"Agent Profile:					Not Supported"<<endl;
		if(HSA_DEFAULT_FLOAT_ROUNDING_MODE_ZERO == float_rounding_mode)
			cout<<"Agent Floating Rounding Mode:			ZERO"<<endl;
		else if(HSA_DEFAULT_FLOAT_ROUNDING_MODE_NEAR == float_rounding_mode)
			cout<<"Agent Floating Rounding Mode:			NEAR"<<endl;
		else
			cout<<"Agent Floating Rounding Mode:			Not Supported"<<endl;
		cout<<"Agent Max Queue Number:				"<<max_queue<<endl;
		cout<<"Agent Queue Min Size:				"<<queue_min_size<<endl;
		cout<<"Agent Queue Max Size:				"<<queue_max_size<<endl;
		if(HSA_QUEUE_TYPE_MULTI == queue_type)
			cout<<"Agent Queue Type:				MULTI"<<endl;
		else if(HSA_QUEUE_TYPE_SINGLE == queue_type)
			cout<<"Agent Queue Type:				SINGLE"<<endl;
		else
			cout<<"Agent Queue Type:				Not Supported"<<endl;
		cout<<"Agent Node:					"<<node<<endl;
		if(HSA_DEVICE_TYPE_CPU == device_type)
			cout<<"Agent Device Type:				CPU"<<endl;
		else if(HSA_DEVICE_TYPE_GPU == device_type)
			{
				cout<<"Agent Device Type:				GPU"<<endl;
                #if 0
				// Get ISA info
				hsa_isa_t agent_isa;
				err = hsa_agent_get_info(agent, HSA_AGENT_INFO_ISA, &agent_isa);
				HSAErrorCheck(err);
				
                uint32_t full_name_len;
                err = hsa_isa_get_info(agent_isa, HSA_ISA_INFO_NAME_LENGTH, 0, &full_name_len);
                HSAErrorCheck(err);

                char *full_name = (char *)malloc(full_name_len);
                err = hsa_isa_get_info(agent_isa, HSA_ISA_INFO_NAME, 0, full_name);
                HSAErrorCheck(err);
                cout<<"Agent Isa:"<<endl;
				cout<<"  Name:						"<< full_name<<endl;
                free(full_name);
                // Convert isa handle to isa object
				::core::Isa* isa_ptr = reinterpret_cast<core::Isa*>(agent_isa.handle);
				std::string full_name = isa_ptr->full_name();
				std::string vendor = isa_ptr->vendor();
				cout<<"Agent Isa:"<<endl;
				cout<<"  Name:						"<<full_name.c_str()<<endl;
				cout<<"  Vendor:					"<<vendor.c_str()<<endl;
                #endif
			}
		else
			cout<<"Agent Device Type:				DSP"<<endl;
		
		cout<<"Agent Cache Info:"<<endl;
		for(int i = 0; i < 4; i++)
		{
			if(cache_size[i])
			{
				cout<<"  $L"<<i+1<<":						"<<cache_size[i]/1024<<"KB"<<endl;
			}
		}
		cout<<"Agent Chip ID:					"<<chip_id<<endl;
		cout<<"Agent Cacheline Size:				"<<cacheline_size<<endl;
		cout<<"Agent Max Clock Frequency:			"<<max_clock_freq<<"MHz"<<endl;
		cout<<"Agent Compute Unit:				"<<compute_unit<<endl;

		// Check if the agent is kernel agent
		if(agent_feature & HSA_AGENT_FEATURE_KERNEL_DISPATCH)
		{

			// Get flaf of fast_f16 operation
			bool fast_f16;
			err = hsa_agent_get_info(agent, HSA_AGENT_INFO_FAST_F16_OPERATION, &fast_f16);
			HSAErrorCheck(err);

			// Get wavefront size
			uint32_t wavefront_size = 0;
			err = hsa_agent_get_info(agent, HSA_AGENT_INFO_WAVEFRONT_SIZE, &wavefront_size);
			HSAErrorCheck(err);

			// Get max total number of work-items in a workgroup
			uint32_t workgroup_max_size = 0;
			err = hsa_agent_get_info(agent, HSA_AGENT_INFO_WORKGROUP_MAX_SIZE, &workgroup_max_size);
			HSAErrorCheck(err);

			// Get max number of work-items of each dimension of a work-group
			uint16_t workgroup_max_dim[3];
			err = hsa_agent_get_info(agent, HSA_AGENT_INFO_WORKGROUP_MAX_DIM, &workgroup_max_dim);
			HSAErrorCheck(err);
			
			// Get max number of a grid per dimension
			hsa_dim3_t grid_max_dim;
			err = hsa_agent_get_info(agent, HSA_AGENT_INFO_GRID_MAX_DIM, &grid_max_dim);
			HSAErrorCheck(err);

			// Get max total number of work-items in a grid
			uint32_t grid_max_size = 0;
			err = hsa_agent_get_info(agent, HSA_AGENT_INFO_GRID_MAX_SIZE, &grid_max_size);
			HSAErrorCheck(err);
			
			// Get max number of fbarriers per work group
			uint32_t fbarrier_max_size = 0;
			err = hsa_agent_get_info(agent, HSA_AGENT_INFO_FBARRIER_MAX_SIZE, &fbarrier_max_size);
			HSAErrorCheck(err);

			// Print info for kernel agent
			if(true == fast_f16)
				cout<<"Agent Fast F16 Operation:			TRUE"<<endl;
			cout<<"Agent Wavefront Size:				"<<wavefront_size<<endl;
			cout<<"Agent Workgroup Max Size:			"<<workgroup_max_size<<endl;
			cout<<"Agent Workgroup Max Size Per Dimension:			"<<endl;
			for(int i = 0; i < 3; i++)
			{
				cout<<"  Dim["<<i<<"]:					"<<workgroup_max_dim[i]<<endl;
			}
			cout<<"Agent Grid Max Size:				"<<grid_max_size<<endl;
			cout<<"Agent Grid Max Size per Dimension:"<<endl;
			for(int i = 0; i < 3; i++)
			{
				cout<<"  Dim["<<i<<"]					"<<reinterpret_cast<uint32_t*>(&grid_max_dim)[i]<<endl;
			}
			cout<<"Agent Max number Of fbarriers Per Workgroup:	"<<fbarrier_max_size<<endl;
		}
		
	}	
    return 0;
}


// Implement memory_pool iteration function
int print_memory_pool_info(hsa_amd_memory_pool_t memory_pool) {
    static int memory_id = 0;
	hsa_status_t err;
    memory_id++;
	
	// Get memory_pool segment info
	hsa_amd_segment_t memory_pool_segment;
	err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_SEGMENT, &memory_pool_segment);
	HSAErrorCheck(err);

	cout<<"  Memory Pool #"<<memory_id<<":"<<endl;
	cout<<"    Memory Pool Segment:			";
	switch(memory_pool_segment)
	{
		case HSA_AMD_SEGMENT_GLOBAL:
			cout<<"GLOBAL"<<endl;
			break;
		case HSA_AMD_SEGMENT_READONLY:
			cout<<"READONLY"<<endl;
			break;	
		case HSA_AMD_SEGMENT_PRIVATE:
			cout<<"PRIVATE"<<endl;
		case HSA_AMD_SEGMENT_GROUP:
			cout<<"GROUP"<<endl;
			break;
		default:
			cout<<"Not Supported"<<endl;
			break;
	}

	// Check if the memory_pool is global
	if(HSA_AMD_SEGMENT_GLOBAL == memory_pool_segment)
	{
		uint32_t global_flag = 0;
		err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_GLOBAL_FLAGS, &global_flag);
		HSAErrorCheck(err);
 
		std::vector<std::string> flags;
		cout<<"    Memory Pool Global Flag:			";
		if(HSA_AMD_MEMORY_POOL_GLOBAL_FLAG_KERNARG_INIT & global_flag)
			flags.push_back("KERNARG INIT");
		if(HSA_AMD_MEMORY_POOL_GLOBAL_FLAG_FINE_GRAINED & global_flag)
			flags.push_back("FINE GRAINED");
		if(HSA_AMD_MEMORY_POOL_GLOBAL_FLAG_COARSE_GRAINED & global_flag)
			flags.push_back("COARSE GRAINED");
		if(flags.size() > 0)
		cout<<flags[0];
		for(int i=1; i<flags.size(); i++)
		{
			cout<<", "<<flags[i];
		}
		cout<<endl;
	}
	
	// Get the size of the memory_pool
	size_t memory_pool_size = 0;
 	err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_SIZE, &memory_pool_size);	
	HSAErrorCheck(err);
	cout<<"    Memory Pool Size:				"<<memory_pool_size/1024<<"KB"<<endl;

	// Get memory_pool alloc max size
	/*size_t memory_pool_alloc_max_size = 0;
	err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_ALLOC_MAX_SIZE, &memory_pool_alloc_max_size);
	HSAErrorCheck(err);
	cout<<"    Memory Pool Alloc Max Size:			"<<memory_pool_alloc_max_size/1024<<"KB"<<endl;
    */
	// Check if the memory_pool is allowed to allocate
	bool alloc_allowed = false;
	err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_RUNTIME_ALLOC_ALLOWED, &alloc_allowed);
	HSAErrorCheck(err);
	cout<<"    Memory Pool Allocatable:			";
	if(alloc_allowed)
		cout<<"TRUE"<<endl;
	else
		cout<<"FALSE"<<endl;

	// Get alloc granule
	if(!alloc_allowed)
		return 0;

	size_t alloc_granule = 0;
	err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_RUNTIME_ALLOC_GRANULE, &alloc_granule);
	HSAErrorCheck(err);
	cout<<"    Memory Pool Alloc Granule:			"<<alloc_granule/1024<<"KB"<<endl;

	// Get alloc alignment
	size_t memory_pool_alloc_alignment = 0;
	err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_RUNTIME_ALLOC_ALIGNMENT, &memory_pool_alloc_alignment);
	HSAErrorCheck(err);
	cout<<"    Memory Pool Alloc Alignment:		"<<memory_pool_alloc_alignment/1024<<"KB"<<endl;

    return 0;
}

