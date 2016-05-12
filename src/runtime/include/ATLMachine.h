#ifndef __ATL_MACHINE__
#define __ATL_MACHINE__
#include "atmi.h"
#include <hsa.h>
#include <hsa_ext_amd.h>
#include <vector>

class ATLFineMemory;
class ATLCoarseMemory;

class ATLProcessor {
    public:
        ATLProcessor(hsa_agent_t agent) : _agent(agent) {}
        template<typename T> void addMemory(T &p);
        hsa_agent_t getAgent() const {return _agent; }
        // TODO: Do we need this or are we building the machine structure just once in the program? 
        // void removeMemory(ATLMemory &p); 
        template<typename T> std::vector<T> &getMemories();
        virtual atmi_devtype_t getType() {return ATMI_DEVTYPE_ALL; }
    private:
        hsa_agent_t     _agent;
        std::vector<ATLFineMemory> _dram_memories;
        std::vector<ATLCoarseMemory> _gddr_memories;
};

class ATLCPUProcessor : public ATLProcessor {
    public:
        ATLCPUProcessor(hsa_agent_t agent) : ATLProcessor(agent) {}
        atmi_devtype_t getType() const { return ATMI_DEVTYPE_CPU; }
};

class ATLGPUProcessor : public ATLProcessor {
    public:
        ATLGPUProcessor(hsa_agent_t agent) : ATLProcessor(agent) {}
        atmi_devtype_t getType() const { return ATMI_DEVTYPE_GPU; }
};

class ATLDSPProcessor : public ATLProcessor {
    public:
        ATLDSPProcessor(hsa_agent_t agent) : ATLProcessor(agent) {}
        atmi_devtype_t getType() const { return ATMI_DEVTYPE_DSP; }
};

class ATLMemory {
    public:
        ATLMemory(hsa_amd_memory_pool_t pool, ATLProcessor p) :
            _memory_pool(pool), _processor(p) {} 
        ATLProcessor &getProcessor()  { return _processor; }
        hsa_amd_memory_pool_t getMemory() const {return _memory_pool; }
        
        atmi_memtype_t getType() const {return ATMI_MEMTYPE_ANY; }
        // uint32_t getAccessType () { return fine of coarse grained? ;}
        /* memory alloc/free */
        void *alloc(size_t s); 
        void free(void *p);
        //atmi_task_handle_t copy(ATLMemory &m, bool async = false);
    private:
        hsa_amd_memory_pool_t   _memory_pool; 
        ATLProcessor    _processor;
};

class ATLFineMemory : public ATLMemory {
    public:
        ATLFineMemory(hsa_amd_memory_pool_t pool, ATLProcessor p) :
            ATLMemory(pool, p) {} 
        atmi_memtype_t getType() const { return ATMI_MEMTYPE_FINE_GRAINED; }
};

class ATLCoarseMemory : public ATLMemory {
    public:
        ATLCoarseMemory(hsa_amd_memory_pool_t pool, ATLProcessor p) :
            ATLMemory(pool, p) {} 
        atmi_memtype_t getType() const { return ATMI_MEMTYPE_COARSE_GRAINED; }
};

class ATLMachine {
    public:
        template<typename T> void addProcessor(T &p);
        template<typename T> std::vector<T> &getProcessors();
    private:
        std::vector<ATLCPUProcessor> _cpu_processors;
        std::vector<ATLGPUProcessor> _gpu_processors;
        std::vector<ATLDSPProcessor> _dsp_processors;
};

#endif // __ATL_MACHINE__
