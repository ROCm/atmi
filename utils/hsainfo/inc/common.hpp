#ifndef COMMON_COMMON_HPP
#define COMMON_COMMON_HPP

#include <cstdlib>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdio>
#include "hsa.h"
//#include "hsa_ext_finalize.h"
//#include "hsa_ext_amd.h"

using namespace std;

#if defined(_MSC_VER)
  #define ALIGNED_(x) __declspec(align(x))
#else
  #if defined(__GNUC__)
    #define ALIGNED_(x) __attribute__ ((aligned(x)))
  #endif // __GNUC__
#endif // _MSC_VER

#define MULTILINE(...) # __VA_ARGS__

#define HSA_ARGUMENT_ALIGN_BYTES 16


#define ErrorCheck(x) error_check(x, __LINE__, __FILE__)

//@Brief: Check HSA API return value
void error_check(hsa_status_t hsa_error_code, int line_num, string str);

//@Brief: Find the first avaliable GPU device
hsa_status_t FindGpuDevice(hsa_agent_t agent, void *data);

//@Brief: Find the agent's global region
hsa_status_t FindGlobalRegion(hsa_region_t region, void* data);

//@Brief: Find the agent's group region
hsa_status_t FindGroupRegion(hsa_region_t region, void* data);

//@Brief: Calculate the mean number of the vector
double CalcMean(vector<double> scores);

//@Brief: Calculate the Median valud of the vector
double CalcMedian(vector<double> scores);

//@Brief: Calculate the standard deviation of the vector
double CalcStdDeviation(vector<double> scores, int score_mean);


int CalcConcurrentQueues(vector<double> scores);

typedef uint32_t BrigCodeOffset32_t;
typedef uint32_t BrigDataOffset32_t;
typedef uint16_t BrigKinds16_t;
typedef uint8_t BrigLinkage8_t;
typedef uint8_t BrigExecutableModifier8_t;
typedef BrigDataOffset32_t BrigDataOffsetString32_t;

typedef struct BrigData BrigData;
struct BrigData {
  uint32_t byteCount;
  uint8_t bytes[1];
};

enum BrigKinds
{
    BRIG_KIND_NONE = 0x0000,
    BRIG_KIND_DIRECTIVE_BEGIN = 0x1000,
    BRIG_KIND_DIRECTIVE_KERNEL = 0x1008,
};


typedef struct BrigExecutableModifier BrigExecutableModifier;
struct BrigExecutableModifier {
    BrigExecutableModifier8_t allBits;
};

typedef struct BrigDirectiveExecutable BrigDirectiveExecutable;
struct BrigDirectiveExecutable {
    uint16_t byteCount;
    BrigKinds16_t kind;
    BrigDataOffsetString32_t name;
    uint16_t outArgCount;
    uint16_t inArgCount;
    BrigCodeOffset32_t firstInArg;
    BrigCodeOffset32_t firstCodeBlockEntry;
    BrigCodeOffset32_t nextModuleEntry;
    uint32_t codeBlockEntryCount;
    BrigExecutableModifier modifier;
    BrigLinkage8_t linkage;
    uint16_t reserved;
};

typedef struct BrigBase BrigBase;
struct BrigBase {
    uint16_t byteCount;
    BrigKinds16_t kind;
};


#endif // COMMON_COMMON_HPP
