/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef __ATL_MACHINE__
#define __ATL_MACHINE__
#include "atmi.h"
#include <hsa.h>
#include <hsa_ext_amd.h>
#include "atl_internal.h"
#include <vector>

class ATLMemory;

class ATLProcessor {
    public:
        ATLProcessor(hsa_agent_t agent) : _next_best_queue_id(0), _agent(agent) {
            _queues.clear();
            _memories.clear();
        }
        void addMemory(ATLMemory &p);
        hsa_agent_t getAgent() const {return _agent; }
        // TODO: Do we need this or are we building the machine structure just once in the program? 
        // void removeMemory(ATLMemory &p); 
        std::vector<ATLMemory> &getMemories();
        virtual atmi_devtype_t getType() {return ATMI_DEVTYPE_ALL; }

        virtual void createQueues(const int count) {}
        virtual void destroyQueues();
        virtual hsa_queue_t *getQueue(const int index);
        std::vector<hsa_queue_t *> getQueues() const {return _queues; }
        virtual hsa_queue_t *getBestQueue(atmi_scheduler_t sched);
        int getNumCUs() const;
        int getWavefrontSize() const;
    protected:
        hsa_agent_t     _agent;
        std::vector<hsa_queue_t *> _queues;
        unsigned int _next_best_queue_id; // schedule queues by setting this to best queue ID
        
        std::vector<ATLMemory> _memories;
};


class ATLCPUProcessor : public ATLProcessor {
    public:
        ATLCPUProcessor(hsa_agent_t agent) : ATLProcessor(agent) {
            _agents.clear();
        }
        atmi_devtype_t getType() const { return ATMI_DEVTYPE_CPU; }
        void createQueues(const int count);
        
        agent_t *getThreadAgent(const int index);
        const std::vector<agent_t *> &getThreadAgents() const {return _agents;}
        // misc helper functions needed by ATMI DP
        hsa_signal_t *get_worker_sig(hsa_queue_t *q);
    private:
        std::vector<agent_t *> _agents;
};

class ATLGPUProcessor : public ATLProcessor {
    public:
        ATLGPUProcessor(hsa_agent_t agent, atmi_devtype_t type = ATMI_DEVTYPE_dGPU) 
                    : ATLProcessor(agent) {_type = type;}
        atmi_devtype_t getType() const { return _type; }
        void createQueues(const int count);
    private:
        atmi_devtype_t _type;
};

class ATLDSPProcessor : public ATLProcessor {
    public:
        ATLDSPProcessor(hsa_agent_t agent) : ATLProcessor(agent) {}
        atmi_devtype_t getType() const { return ATMI_DEVTYPE_DSP; }
        void createQueues(const int count);
};

class ATLMemory {
    public:
        ATLMemory(hsa_amd_memory_pool_t pool, ATLProcessor p, atmi_memtype_t t) :
            _memory_pool(pool), _processor(p), _type(t) {} 
        ATLProcessor &getProcessor()  { return _processor; }
        hsa_amd_memory_pool_t getMemory() const {return _memory_pool; }
        
        atmi_memtype_t getType() const {return _type; }
        // uint32_t getAccessType () { return fine of coarse grained? ;}
        /* memory alloc/free */
        void *alloc(size_t s); 
        void free(void *p);
        //atmi_task_handle_t copy(ATLMemory &m, bool async = false);
    private:
        hsa_amd_memory_pool_t   _memory_pool; 
        ATLProcessor    _processor;
        atmi_memtype_t  _type;
};

class ATLMachine {
    public:
        ATLMachine() {
            _cpu_processors.clear();
            _gpu_processors.clear();
            _dsp_processors.clear();
        }
        template<typename T> void addProcessor(T &p);
        template<typename T> std::vector<T> &getProcessors();
        template<typename T> size_t getProcessorCount(){return getProcessors<T>().size();}
    private:
        std::vector<ATLCPUProcessor> _cpu_processors;
        std::vector<ATLGPUProcessor> _gpu_processors;
        std::vector<ATLDSPProcessor> _dsp_processors;
};

hsa_amd_memory_pool_t get_memory_pool(ATLProcessor &proc, const int mem_id);

#include "ATLMachine.tcc"

#endif // __ATL_MACHINE__
