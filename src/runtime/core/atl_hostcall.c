 
/*   
 *   atl_hostcall.c: Implementation of Linked List Queue in c for ATMI to
 *                   implement hostcall.  hostcall buffers and their consumer
 *                    are placed on a linked list queue (hcb).
 *
 *   Written by Greg Rodgers

MIT License

Copyright © 2019 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <stdio.h>
#include <stdlib.h>
#include "hsa_ext_amd.h"
#include "hostcall.h"
#include "hostcall_service_id.h"
#include "atl_internal.h"

typedef struct atl_hcq_element_s atl_hcq_element_t;
struct atl_hcq_element_s {
  hostcall_buffer_t *   hcb;
  hostcall_consumer_t * consumer;
  hsa_queue_t *       	hsa_q;
  atl_hcq_element_t *   next_ptr;
};

/* Persistent static values for the hcq linked list */
atl_hcq_element_t * atl_hcq_front;
atl_hcq_element_t * atl_hcq_rear;
int atl_hcq_count;
 
static int atl_hcq_size() { return atl_hcq_count ;}

atl_hcq_element_t * atl_hcq_push(hostcall_buffer_t * hcb, hostcall_consumer_t * consumer, hsa_queue_t * hsa_q) {
  // FIXME , check rc of these mallocs
  if (atl_hcq_rear == NULL) {
    atl_hcq_rear = (atl_hcq_element_t *) malloc(sizeof(atl_hcq_element_t));
    atl_hcq_front = atl_hcq_rear;
  } else {
    atl_hcq_element_t * new_rear = (atl_hcq_element_t *) malloc(sizeof(atl_hcq_element_t));
    atl_hcq_rear->next_ptr = new_rear;
    atl_hcq_rear = new_rear;
  }
  atl_hcq_rear->next_ptr = NULL;
  atl_hcq_rear->hcb      = hcb; 
  atl_hcq_rear->hsa_q    = hsa_q;
  atl_hcq_rear->consumer = consumer;
  atl_hcq_count++;
  return atl_hcq_rear;
}
 
static void atl_hcq_pop() {
  if (atl_hcq_front  == NULL) {
    printf("\n Error: Trying to pop an element from empty queue");
    return;
  } else {
    if (atl_hcq_front->next_ptr != NULL) {
      atl_hcq_element_t * new_front = atl_hcq_front->next_ptr;
      free(atl_hcq_front);
      atl_hcq_front = new_front;
    } else {
      free(atl_hcq_front);
      atl_hcq_front = NULL;
      atl_hcq_rear = NULL;
    }
    atl_hcq_count--;
  }
}
 
static atl_hcq_element_t  * atl_hcq_find_by_hsa_q(hsa_queue_t * hsa_q) {
  atl_hcq_element_t * this_front = atl_hcq_front;
  int reverse_counter = atl_hcq_size();
  while (reverse_counter) {
    if (this_front->hsa_q == hsa_q)
       return this_front;
    this_front = this_front->next_ptr;
    reverse_counter--;
  }
  return NULL;
}

static hostcall_buffer_t * atl_hcq_create_buffer(unsigned int num_packets,
	 hsa_amd_memory_pool_t finegrain_pool) {
    if (num_packets == 0) {
	printf("num_packets cannot be zero \n");
	abort();
    }
    auto size  = hostcall_get_buffer_size(num_packets);
    auto align = hostcall_get_buffer_alignment();
    void *newbuffer = NULL;
    hsa_status_t err = hsa_amd_memory_pool_allocate(finegrain_pool, size, 0, &newbuffer);
    if (!newbuffer) {
	    printf("call to  hsa_amd_memory_pool_allocate failed \n");
	    abort();
    }
    if (hostcall_initialize_buffer(newbuffer, num_packets) != HOSTCALL_SUCCESS) {
	    printf("call to  hostcall_initialize_buffer failed \n");
	    abort();
    }
    // printf("created hostcall buffer %p with %d packets \n", newbuffer, num_packets);
    return (hostcall_buffer_t *) newbuffer;
}

// ---------------------------- snip -------------------------------------------
// FIXME: Move all these handlers into the hostcall repository and create an
//  external API hostcall_register_all_handlers(hostcall_consumer_t c);
//  that will register all handlers for a consumer
//
void handler_HOSTCALL_SERVICE_PRINTF(void *ignored, uint32_t service, uint64_t *payload) {
    *payload = *payload + 1;
    printf("=================== Printf Handler called\n");
}

void handler_HOSTCALL_SERVICE_MALLOC(void *ignored, uint32_t service, uint64_t *payload) {
	payload_t * p = (payload_t  *) payload;

    printf("===================malloc Handler called for service %d pl0:%ld \n",service,
		    payload[0]);
    payload[1] = 42; // Attempt a temporary LOOPTEST. 
    // The LOOPTeST IS TO TAKE result0 FROM FIRST hostcall_invoke and use it as input0 in 2nd call to invoke.
    // We should see this handler get a 42. 
    if (payload[0] == 42 ) {
	    printf("temporary LOOPTeST is success the 42 was returned in a 2nd call to hostall\n");
	    abort();
    }
}
void handler_HOSTCALL_SERVICE_FREE(void *ignored, uint32_t service, uint64_t *payload) {
}
void handler_HOSTCALL_SERVICE_DEMO(void *ignored, uint32_t service, uint64_t *payload) {
}
// ---------------------------- snip -------------------------------------------

// ----   External functions start here  -----
unsigned long atl_hostcall_assign_buffer(unsigned int minpackets, 
		hsa_queue_t * this_Q, hsa_amd_memory_pool_t finegrain_pool) {
  void * hostcall_ptrs;

    atl_hcq_element_t * llq_elem ;
    llq_elem  = atl_hcq_find_by_hsa_q(this_Q);
    if (!llq_elem) {
       //  For now, we create one bufer and one consumer per hsa queue
       hostcall_buffer_t * hcb  = atl_hcq_create_buffer(minpackets,
		 finegrain_pool) ;
       
       hostcall_consumer_t * c;
       c = hostcall_create_consumer();
       // FIXME  call hostcall_register_all_handlers(c) when hostcall gets this api
       hostcall_register_service(c,HOSTCALL_SERVICE_PRINTF, handler_HOSTCALL_SERVICE_PRINTF, nullptr);
       hostcall_register_service(c,HOSTCALL_SERVICE_MALLOC, handler_HOSTCALL_SERVICE_MALLOC, nullptr);
       hostcall_register_service(c,HOSTCALL_SERVICE_FREE, handler_HOSTCALL_SERVICE_FREE, nullptr);
       hostcall_register_service(c,HOSTCALL_SERVICE_DEMO, handler_HOSTCALL_SERVICE_DEMO, nullptr);
       hostcall_register_buffer(c,hcb);
       allow_access_to_all_gpu_agents(hcb);
       // Now add a new hcq element to hcq
       llq_elem = atl_hcq_push( hcb , c, this_Q);
       hostcall_launch_consumer(c);
    }
    return (unsigned long) llq_elem->hcb;
}

hsa_status_t atl_hostcall_init() {
   atl_hcq_count = 0;
   atl_hcq_front = atl_hcq_rear = NULL;
   return HSA_STATUS_SUCCESS;
}

hsa_status_t atl_hostcall_terminate() {
   hostcall_consumer_t * c;
   atl_hcq_element_t * this_front = atl_hcq_front;
   atl_hcq_element_t * last_front;
   int reverse_counter = atl_hcq_size();
   while (reverse_counter) {
      if (c = this_front->consumer) {
        hostcall_destroy_consumer(c);
      }
      hsa_memory_free(this_front->hcb);
      last_front = this_front;
      this_front = this_front->next_ptr;
      free(last_front);
      reverse_counter--;
   }
   atl_hcq_count = 0;
   atl_hcq_front = atl_hcq_rear = NULL;
   return HSA_STATUS_SUCCESS;
}
