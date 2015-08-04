#include <atmi.h>
#include <hsa_cl.h>
__kernel void racerKernel(ATMI_myTask, const int myNumber,
   __global int*winner, 
   __global int*loser, 
   __global int*finishers)
{
    if(get_global_id(0) != 0) return;
    int ret = hsa_atomic_cas_system(&(finishers[0]), 0, myNumber);
    if(ret == 0) { // winner 
        *winner = myNumber;
    } 
    else {
        int second_place = hsa_atomic_cas_system(&(finishers[1]), 0, myNumber);
        if(second_place != 0) { // third place
            finishers[2] = myNumber;
            *loser = myNumber;
        }
    }
}   
