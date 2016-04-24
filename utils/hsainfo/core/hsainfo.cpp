#include <iostream>
#include <iomanip>
#include "hsa.h"
#include "hsa_ext_amd.h"
#include "common.hpp"
#include "os.hpp"
#include "isa.hpp"
#include <set>

using namespace std;

static std::vector<hsa_amd_memory_pool_t> g_memory_pools;
static std::vector<hsa_agent_t> g_agents;
// Static var for recording number of agent
static int agent_number = 0;

hsa_status_t get_agent_info(hsa_agent_t, void*);

hsa_status_t get_region_info(hsa_region_t, void*);

hsa_status_t get_memory_pool_info(hsa_amd_memory_pool_t, void*);

int main()
{
	hsa_status_t err;
	// Initialize the runtime
	err = hsa_init();
	ErrorCheck(err);

	// Get the system info first
	{	
		// Get version info
		uint16_t major, minor;
		err = hsa_system_get_info(HSA_SYSTEM_INFO_VERSION_MAJOR, &major);
		ErrorCheck(err);
		err = hsa_system_get_info(HSA_SYSTEM_INFO_VERSION_MINOR, &minor);
		ErrorCheck(err);
	
		// Get timestamp frequency
		uint64_t timestamp_frequency = 0;
		err = hsa_system_get_info(HSA_SYSTEM_INFO_TIMESTAMP_FREQUENCY, &timestamp_frequency);
		ErrorCheck(err);

		// Get maximum duration of a signal wait operation
		uint64_t max_wait = 0;
		err = hsa_system_get_info(HSA_SYSTEM_INFO_SIGNAL_MAX_WAIT, &max_wait);
		ErrorCheck(err);

		// Get Endianness of the system
		hsa_endianness_t endianness;
		err = hsa_system_get_info(HSA_SYSTEM_INFO_ENDIANNESS, &endianness);
		ErrorCheck(err);
		
		// Get machine model info
		hsa_machine_model_t machine_model;
		err = hsa_system_get_info(HSA_SYSTEM_INFO_MACHINE_MODEL, &machine_model);
		ErrorCheck(err);		
		
		// Print out the results
		cout<<"HSA System Info:"<<endl;
		cout<<"Runtime Version:				"<<major<<"."<<minor<<endl;
		cout<<"System Timestamp Frequency: 			"<<timestamp_frequency/1e6<<"MHz"<<endl;
		cout<<"Signal Max Wait Duration:			"<<max_wait<<"(number of timestamp)"<<endl;
		cout<<"Machine Model:					";
		if(HSA_MACHINE_MODEL_SMALL == machine_model) cout<<"SMALL"<<endl;
		else if(HSA_MACHINE_MODEL_LARGE == machine_model)
			cout<<"LARGE"<<endl;
		cout<<"System Endianness:				";	
		if(HSA_ENDIANNESS_LITTLE == endianness)
		cout<<"LITTLE"<<endl;
		else if(HSA_ENDIANNESS_BIG == endianness)
		cout<<"BIG"<<endl;
		cout<<endl;
	}

	// Iterate every agent and get their info
	err = hsa_iterate_agents(get_agent_info, NULL);
	ErrorCheck(err);

    cout << endl << "Agent-Memory Pool Access Matrix" << endl;
    cout << setw(11) << "Agent/Pool#";
    for(int i = 0; i < g_memory_pools.size(); i++) {
        cout << setw(11) << i; 
    }
    cout << endl;
    int i = 0;
    int j = 0;
    for(std::vector<hsa_agent_t>::iterator agent_it = g_agents.begin(); 
                            agent_it != g_agents.end(); agent_it++) {
        cout << setw(11) << i;
        j = 0;
        for(std::vector<hsa_amd_memory_pool_t>::iterator mem_it = g_memory_pools.begin(); 
                            mem_it != g_memory_pools.end(); mem_it++) {
            hsa_amd_memory_pool_access_t access;
            hsa_amd_agent_memory_pool_get_info(*agent_it, 
                                    *mem_it, 
                                    HSA_AMD_AGENT_MEMORY_POOL_INFO_ACCESS, 
                                    &access);
            std::string access_str = (access == 0) ? "NEVER" : (access == 1 ? "YES" : "NO");
            cout << setw(11) << access_str;
            j++;
        }
        cout << endl;
        i++;
    }

	return 0;
		
		
}


hsa_status_t get_agent_info(hsa_agent_t agent, void* data)
{
    g_agents.push_back(agent);
    //g_agents.insert(agent);
	int region_number = 0;
	int memory_pool_number = 0;
	hsa_status_t err;
	{
		// Increase the number of agent 
		agent_number++;

		// Get agent name and vendor
		char name[64];
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_NAME, name);
		ErrorCheck(err);
		char vendor_name[64];
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_VENDOR_NAME, &vendor_name);
		ErrorCheck(err);
		
		// Get agent feature
		hsa_agent_feature_t agent_feature;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_FEATURE, &agent_feature);
		ErrorCheck(err);

		// Get profile supported by the agent
		hsa_profile_t agent_profile;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_PROFILE, &agent_profile);
		ErrorCheck(err);

		// Get floating-point rounding mode
  		hsa_default_float_rounding_mode_t float_rounding_mode;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEFAULT_FLOAT_ROUNDING_MODE, &float_rounding_mode);
		ErrorCheck(err);

		// Get max number of queue
		uint32_t max_queue = 0;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUES_MAX, &max_queue);
		ErrorCheck(err);

		// Get queue min size
		uint32_t queue_min_size = 0;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUE_MIN_SIZE, &queue_min_size);
		ErrorCheck(err);
		
		// Get queue max size
		uint32_t queue_max_size = 0;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_max_size);
		ErrorCheck(err);
		
		// Get queue type
		hsa_queue_type_t queue_type;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_QUEUE_TYPE, &queue_type);
		ErrorCheck(err);

		// Get agent node
		uint32_t node;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_NODE, &node);
		ErrorCheck(err);

		// Get device type
		hsa_device_type_t device_type;
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
		ErrorCheck(err);
		
		// Get cache size
		uint32_t cache_size[4];
		err = hsa_agent_get_info(agent, HSA_AGENT_INFO_CACHE_SIZE, cache_size);
		ErrorCheck(err);
	
		// Get chip id
		uint32_t chip_id = 0; 
		err = hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_CHIP_ID, &chip_id);
		ErrorCheck(err);

		// Get cacheline size
		uint32_t cacheline_size = 0;
		err = hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_CACHELINE_SIZE, &cacheline_size);
		ErrorCheck(err);

		// Get Max clock frequency
		uint32_t max_clock_freq = 0;
		err = hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_MAX_CLOCK_FREQUENCY, &max_clock_freq);
		ErrorCheck(err);

		// Get number of Compute Unit
		uint32_t compute_unit = 0;
		err = hsa_agent_get_info(agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_COMPUTE_UNIT_COUNT, &compute_unit);
		ErrorCheck(err);

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
				// Get ISA info
				hsa_isa_t agent_isa;
				err = hsa_agent_get_info(agent, HSA_AGENT_INFO_ISA, &agent_isa);
				ErrorCheck(err);
				
                uint32_t full_name_len;
                err = hsa_isa_get_info(agent_isa, HSA_ISA_INFO_NAME_LENGTH, 0, &full_name_len);
                ErrorCheck(err);

                char *full_name = (char *)malloc(full_name_len);
                err = hsa_isa_get_info(agent_isa, HSA_ISA_INFO_NAME, 0, full_name);
                ErrorCheck(err);
				
                cout<<"Agent Isa:"<<endl;
				cout<<"  Name:						"<< full_name<<endl;
                free(full_name);
                #if 0
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
			ErrorCheck(err);

			// Get wavefront size
			uint32_t wavefront_size = 0;
			err = hsa_agent_get_info(agent, HSA_AGENT_INFO_WAVEFRONT_SIZE, &wavefront_size);
			ErrorCheck(err);

			// Get max total number of work-items in a workgroup
			uint32_t workgroup_max_size = 0;
			err = hsa_agent_get_info(agent, HSA_AGENT_INFO_WORKGROUP_MAX_SIZE, &workgroup_max_size);
			ErrorCheck(err);

			// Get max number of work-items of each dimension of a work-group
			uint16_t workgroup_max_dim[3];
			err = hsa_agent_get_info(agent, HSA_AGENT_INFO_WORKGROUP_MAX_DIM, &workgroup_max_dim);
			ErrorCheck(err);
			
			// Get max number of a grid per dimension
			hsa_dim3_t grid_max_dim;
			err = hsa_agent_get_info(agent, HSA_AGENT_INFO_GRID_MAX_DIM, &grid_max_dim);
			ErrorCheck(err);

			// Get max total number of work-items in a grid
			uint32_t grid_max_size = 0;
			err = hsa_agent_get_info(agent, HSA_AGENT_INFO_GRID_MAX_SIZE, &grid_max_size);
			ErrorCheck(err);
			
			// Get max number of fbarriers per work group
			uint32_t fbarrier_max_size = 0;
			err = hsa_agent_get_info(agent, HSA_AGENT_INFO_FBARRIER_MAX_SIZE, &fbarrier_max_size);
			ErrorCheck(err);

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

	// Get region info
	cout<<"Agent Region Info:"<<endl;
	err = hsa_agent_iterate_regions(agent, get_region_info, &region_number);
	ErrorCheck(err);
	// Get memory pool info
	cout<<"Agent Memory Pool Info:"<<endl;
	err = hsa_amd_agent_iterate_memory_pools(agent, get_memory_pool_info, &memory_pool_number);
	ErrorCheck(err);

    return HSA_STATUS_SUCCESS;
}


// Implement memory_pool iteration function
hsa_status_t get_memory_pool_info(hsa_amd_memory_pool_t memory_pool, void* data)
{
    g_memory_pools.push_back(memory_pool);
    //g_memory_pools.insert(memory_pool);
	int* p_int = reinterpret_cast<int*>(data);
	(*p_int)++;
	
	hsa_status_t err;
	
	// Get memory_pool segment info
	hsa_amd_segment_t memory_pool_segment;
	err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_SEGMENT, &memory_pool_segment);
	ErrorCheck(err);

	cout<<"  Memory Pool #"<<*p_int<<":"<<endl;
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
		ErrorCheck(err);
 
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
	ErrorCheck(err);
	cout<<"    Memory Pool Size:				"<<memory_pool_size/1024<<"KB"<<endl;

	// Get memory_pool alloc max size
	/*size_t memory_pool_alloc_max_size = 0;
	err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_ALLOC_MAX_SIZE, &memory_pool_alloc_max_size);
	ErrorCheck(err);
	cout<<"    Memory Pool Alloc Max Size:			"<<memory_pool_alloc_max_size/1024<<"KB"<<endl;
    */
	// Check if the memory_pool is allowed to allocate
	bool alloc_allowed = false;
	err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_RUNTIME_ALLOC_ALLOWED, &alloc_allowed);
	ErrorCheck(err);
	cout<<"    Memory Pool Allocatable:			";
	if(alloc_allowed)
		cout<<"TRUE"<<endl;
	else
		cout<<"FALSE"<<endl;

	// Get alloc granule
	if(!alloc_allowed)
		return HSA_STATUS_SUCCESS;

	size_t alloc_granule = 0;
	err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_RUNTIME_ALLOC_GRANULE, &alloc_granule);
	ErrorCheck(err);
	cout<<"    Memory Pool Alloc Granule:			"<<alloc_granule/1024<<"KB"<<endl;

	// Get alloc alignment
	size_t memory_pool_alloc_alignment = 0;
	err = hsa_amd_memory_pool_get_info(memory_pool, HSA_AMD_MEMORY_POOL_INFO_RUNTIME_ALLOC_ALIGNMENT, &memory_pool_alloc_alignment);
	ErrorCheck(err);
	cout<<"    Memory Pool Alloc Alignment:		"<<memory_pool_alloc_alignment/1024<<"KB"<<endl;
	return HSA_STATUS_SUCCESS;
}

// Implement region iteration function
hsa_status_t get_region_info(hsa_region_t region, void* data)
{
	int* p_int = reinterpret_cast<int*>(data);
	(*p_int)++;
	
	hsa_status_t err;
	
	// Get region segment info
	hsa_region_segment_t region_segment;
	err = hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &region_segment);
	ErrorCheck(err);

	cout<<"  Region #"<<*p_int<<":"<<endl;
	cout<<"    Region Segment:				";
	switch(region_segment)
	{
		case HSA_REGION_SEGMENT_GLOBAL:
			cout<<"GLOBAL"<<endl;
			break;
		case HSA_REGION_SEGMENT_READONLY:
			cout<<"READONLY"<<endl;
			break;	
		case HSA_REGION_SEGMENT_PRIVATE:
			cout<<"PRIVATE"<<endl;
		case HSA_REGION_SEGMENT_GROUP:
			cout<<"GROUP"<<endl;
			break;
		default:
			cout<<"Not Supported"<<endl;
			break;
	}

	// Check if the region is global
	if(HSA_REGION_SEGMENT_GLOBAL == region_segment)
	{
		uint32_t global_flag = 0;
		err = hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &global_flag);
		ErrorCheck(err);
 
		std::vector<std::string> flags;
		cout<<"    Region Global Flag:				";
		if(HSA_REGION_GLOBAL_FLAG_KERNARG & global_flag)
			flags.push_back("KERNARG");
		if(HSA_REGION_GLOBAL_FLAG_FINE_GRAINED & global_flag)
			flags.push_back("FINE GRAINED");
		if(HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED & global_flag)
			flags.push_back("COARSE GRAINED");
		if(flags.size() > 0)
		cout<<flags[0];
		for(int i=1; i<flags.size(); i++)
		{
			cout<<", "<<flags[i];
		}
		cout<<endl;
	}
	
	// Get the size of the region
	size_t region_size = 0;
 	err = hsa_region_get_info(region, HSA_REGION_INFO_SIZE, &region_size);	
	ErrorCheck(err);
	cout<<"    Region Size:				"<<region_size/1024<<"KB"<<endl;

	// Get region alloc max size
	size_t region_alloc_max_size = 0;
	err = hsa_region_get_info(region, HSA_REGION_INFO_ALLOC_MAX_SIZE, &region_alloc_max_size);
	ErrorCheck(err);
	cout<<"    Region Alloc Max Size:			"<<region_alloc_max_size/1024<<"KB"<<endl;

	// Check if the region is allowed to allocate
	bool alloc_allowed = false;
	err = hsa_region_get_info(region, HSA_REGION_INFO_RUNTIME_ALLOC_ALLOWED, &alloc_allowed);
	ErrorCheck(err);
	cout<<"    Region Allocatable:				";
	if(alloc_allowed)
		cout<<"TRUE"<<endl;
	else
		cout<<"FALSE"<<endl;

	// Get alloc granule
	if(!alloc_allowed)
		return HSA_STATUS_SUCCESS;

	size_t alloc_granule = 0;
	err = hsa_region_get_info(region, HSA_REGION_INFO_RUNTIME_ALLOC_GRANULE, &alloc_granule);
	ErrorCheck(err);
	cout<<"    Region Alloc Granule:			"<<alloc_granule/1024<<"KB"<<endl;

	// Get alloc alignment
	size_t region_alloc_alignment = 0;
	err = hsa_region_get_info(region, HSA_REGION_INFO_RUNTIME_ALLOC_ALIGNMENT, &region_alloc_alignment);
	ErrorCheck(err);
	cout<<"    Region Alloc Alignment:			"<<region_alloc_alignment/1024<<"KB"<<endl;
	return HSA_STATUS_SUCCESS;
}
