/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <set>
#include <string>
#include <sys/time.h>

#include "nw.h"
#include "atmi_runtime.h"

#define ErrorCheck(status) \
  if (status != ATMI_STATUS_SUCCESS) { \
    printf("Error at [%s:%d]\n", __FILE__, __LINE__); \
    exit(1); \
  }

void lparm_init(atmi_lparm_t* X) {
  X->gridDim[0]=64;
  X->gridDim[1]=1;
  X->gridDim[2]=1;

  X->groupDim[0]=64;
  X->groupDim[1]=1;
  X->groupDim[2]=1;

  X->group = NULL;
  X->groupable = ATMI_FALSE;
  X->synchronous=ATMI_FALSE;

  X->acquire_scope=ATMI_FENCE_SCOPE_SYSTEM;
  X->release_scope=ATMI_FENCE_SCOPE_SYSTEM;

  X->num_required=0;
  X->requires=NULL;
  X->num_required_groups=0;
  X->required_groups=NULL;

  //X->profilable=ATMI_TRUE;
  X->profilable=ATMI_FALSE;
  X->atmi_id=ATMI_VRM;
  X->kernel_id=-1;
  //X->place=ATMI_PLACE_ANY(0);
  X->place = ATMI_PLACE_GPU(0, 0);
  X->task_info = NULL;
  X->continuation_task = ATMI_NULL_TASK_HANDLE;
}

//global variables

int blosum62[24][24] = {
  { 4, -1, -2, -2,  0, -1, -1,  0, -2, -1, -1, -1, -1, -2, -1,  1,  0, -3, -2,  0, -2, -1,  0, -4},
  {-1,  5,  0, -2, -3,  1,  0, -2,  0, -3, -2,  2, -1, -3, -2, -1, -1, -3, -2, -3, -1,  0, -1, -4},
  {-2,  0,  6,  1, -3,  0,  0,  0,  1, -3, -3,  0, -2, -3, -2,  1,  0, -4, -2, -3,  3,  0, -1, -4},
  {-2, -2,  1,  6, -3,  0,  2, -1, -1, -3, -4, -1, -3, -3, -1,  0, -1, -4, -3, -3,  4,  1, -1, -4},
  { 0, -3, -3, -3,  9, -3, -4, -3, -3, -1, -1, -3, -1, -2, -3, -1, -1, -2, -2, -1, -3, -3, -2, -4},
  {-1,  1,  0,  0, -3,  5,  2, -2,  0, -3, -2,  1,  0, -3, -1,  0, -1, -2, -1, -2,  0,  3, -1, -4},
  {-1,  0,  0,  2, -4,  2,  5, -2,  0, -3, -3,  1, -2, -3, -1,  0, -1, -3, -2, -2,  1,  4, -1, -4},
  { 0, -2,  0, -1, -3, -2, -2,  6, -2, -4, -4, -2, -3, -3, -2,  0, -2, -2, -3, -3, -1, -2, -1, -4},
  {-2,  0,  1, -1, -3,  0,  0, -2,  8, -3, -3, -1, -2, -1, -2, -1, -2, -2,  2, -3,  0,  0, -1, -4},
  {-1, -3, -3, -3, -1, -3, -3, -4, -3,  4,  2, -3,  1,  0, -3, -2, -1, -3, -1,  3, -3, -3, -1, -4},
  {-1, -2, -3, -4, -1, -2, -3, -4, -3,  2,  4, -2,  2,  0, -3, -2, -1, -2, -1,  1, -4, -3, -1, -4},
  {-1,  2,  0, -1, -3,  1,  1, -2, -1, -3, -2,  5, -1, -3, -1,  0, -1, -3, -2, -2,  0,  1, -1, -4},
  {-1, -1, -2, -3, -1,  0, -2, -3, -2,  1,  2, -1,  5,  0, -2, -1, -1, -1, -1,  1, -3, -1, -1, -4},
  {-2, -3, -3, -3, -2, -3, -3, -3, -1,  0,  0, -3,  0,  6, -4, -2, -2,  1,  3, -1, -3, -3, -1, -4},
  {-1, -2, -2, -1, -3, -1, -1, -2, -2, -3, -3, -1, -2, -4,  7, -1, -1, -4, -3, -2, -2, -1, -2, -4},
  { 1, -1,  1,  0, -1,  0,  0,  0, -1, -2, -2,  0, -1, -2, -1,  4,  1, -3, -2, -2,  0,  0,  0, -4},
  { 0, -1,  0, -1, -1, -1, -1, -2, -2, -1, -1, -1, -1, -2, -1,  1,  5, -2, -2,  0, -1, -1,  0, -4},
  {-3, -3, -4, -4, -2, -2, -3, -2, -2, -3, -2, -3, -1,  1, -4, -3, -2, 11,  2, -3, -4, -3, -2, -4},
  {-2, -2, -2, -3, -2, -1, -2, -3,  2, -1, -1, -2, -1,  3, -3, -2, -2,  2,  7, -1, -3, -2, -1, -4},
  { 0, -3, -3, -3, -1, -2, -2, -3, -3,  3,  1, -2,  1, -1, -2, -2,  0, -3, -1,  4, -3, -2, -1, -4},
  {-2, -1,  3,  4, -3,  0,  1, -1,  0, -3, -4,  0, -3, -3, -2,  0, -1, -4, -3, -3,  4,  1, -1, -4},
  {-1,  0,  0,  1, -3,  3,  4, -2,  0, -3, -3,  1, -1, -3, -1,  0, -1, -3, -2, -2,  1,  4, -1, -4},
  { 0, -1, -1, -1, -2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2,  0,  0, -2, -1, -1, -1, -1, -1, -4},
  {-4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4,  1}
};

int maximum( int a,
    int b,
    int c){

  int k;
  if( a <= b )
    k = b;
  else
    k = a;
  if( k <=c )
    return(c);
  else
    return(k);
}

void usage(int argc, char **argv)
{
  fprintf(stderr, "Usage: %s <max_rows/max_cols> <penalty> <task_size>\n", argv[0]);
  fprintf(stderr, "\t<dimension>  - x and y dimensions\n");
  fprintf(stderr, "\t<penalty> - penalty(positive integer)\n");
  fprintf(stderr, "\t<file> - filename\n");
  exit(1);
}

typedef struct __stopwatch_t{
  struct timeval begin;
  struct timeval end;
}stopwatch;

void stopwatch_stop(stopwatch *sw){
  if (sw == NULL)
    return;

  gettimeofday(&sw->end, NULL);
}

void stopwatch_start(stopwatch *sw){
  if (sw == NULL)
    return;

  bzero(&sw->begin, sizeof(struct timeval));
  bzero(&sw->end  , sizeof(struct timeval));

  gettimeofday(&sw->begin, NULL);
}

double
get_interval_by_sec(stopwatch *sw){
  if (sw == NULL)
    return 0;
  return ((double)(sw->end.tv_sec-sw->begin.tv_sec)+(double)(sw->end.tv_usec-sw->begin.tv_usec)/1000000);
}

int main(int argc, char **argv){
  ErrorCheck(atmi_init(ATMI_DEVTYPE_GPU));

  const char *module = "nw.hsaco";
  atmi_platform_type_t module_type = AMDGCN;
  ErrorCheck(atmi_module_register(&module, &module_type, 1));

  int max_rows, max_cols, penalty;
  char * tempchar;
  stopwatch sw;
  int task_sze = 1; // Number of workgroups per task
  // the lengths of the two sequences should be able to divided by 16.
  // And at current stage  max_rows needs to equal max_cols
  if (argc == 4)
  {
    max_rows = atoi(argv[1]);
    max_cols = atoi(argv[1]);
    penalty = atoi(argv[2]);
    task_sze = atoi(argv[3]);
    //tempchar = argv[3];
  }
  else{
    usage(argc, argv);
  }

  if(atoi(argv[1])%16!=0){
    fprintf(stderr,"The dimension values must be a multiple of 16\n");
    exit(1);
  }

  max_rows = max_rows + 1;
  max_cols = max_cols + 1;

  int *reference;
  int *input_itemsets;
  int *d_reference;
  int *d_input_itemsets;
  int *output_itemsets;

  reference = (int *)malloc( max_rows * max_cols * sizeof(int) );
  input_itemsets = (int *)malloc( max_rows * max_cols * sizeof(int) );
  output_itemsets = (int *)malloc( max_rows * max_cols * sizeof(int) );

  srand(7);

  //initialization
  for (int i = 0 ; i < max_cols; i++){
    for (int j = 0 ; j < max_rows; j++){
      input_itemsets[i*max_cols+j] = 0;
    }
  }

  for( int i=1; i< max_rows ; i++){    //initialize the cols
    input_itemsets[i*max_cols] = rand() % 10 + 1;
  }

  for( int j=1; j< max_cols ; j++){    //initialize the rows
    input_itemsets[j] = rand() % 10 + 1;
  }

  for (int i = 1 ; i < max_cols; i++){
    for (int j = 1 ; j < max_rows; j++){
      reference[i*max_cols+j] = blosum62[input_itemsets[i*max_cols]][input_itemsets[j]];
    }
  }

  for( int i = 1; i< max_rows ; i++)
    input_itemsets[i*max_cols] = -i * penalty;
  for( int j = 1; j< max_cols ; j++)
    input_itemsets[j] = -j * penalty;

  int gpu_id = 0;
  int cpu_id = 0;
  atmi_mem_place_t gpu = ATMI_MEM_PLACE(ATMI_DEVTYPE_GPU, gpu_id, 0);
  atmi_mem_place_t cpu = ATMI_MEM_PLACE(ATMI_DEVTYPE_CPU, cpu_id, 0);
  ErrorCheck(atmi_malloc((void **)&d_reference, max_rows * max_cols * sizeof(int), gpu));
  ErrorCheck(atmi_memcpy(d_reference, reference, max_rows * max_cols * sizeof(int)));
  ErrorCheck(atmi_malloc((void **)&d_input_itemsets, max_rows * max_cols * sizeof(int), gpu));
  ErrorCheck(atmi_memcpy(d_input_itemsets, input_itemsets, max_rows * max_cols * sizeof(int)));

  size_t nworkitems, workgroupsize = 0;
  nworkitems = 16;

  if(nworkitems < 1 || workgroupsize < 0){
    printf("ERROR: invalid or missing <num_work_items>[/<work_group_size>]\n");
    return -1;
  }

  // set global and local workitems
  size_t local_work[3] = { (workgroupsize>0)?workgroupsize:1, 1, 1 };
  size_t global_work[3] = { nworkitems, 1, 1 }; //nworkitems = no. of GPU threads

  int worksize = max_cols - 1;
  printf("worksize = %d\n", worksize);
  //these two parameters are for extension use, don't worry about it.
  int offset_r = 0, offset_c = 0;
  int block_width = worksize/BLOCK_SIZE ;

  //int *tmp_var = new int;
  int *tmp_var;
  ErrorCheck(atmi_malloc((void **)&tmp_var, sizeof(int), cpu));
  *tmp_var = 0;
  atmi_kernel_t dummy_kernel;
  const unsigned int dummy_num_args = 1;
  size_t dummy_arg_sizes[dummy_num_args];
  dummy_arg_sizes[0] = sizeof(int *);
  void *dummy_args[] = {&tmp_var};
  ErrorCheck(atmi_kernel_create(&dummy_kernel, dummy_num_args, dummy_arg_sizes, 1,
        ATMI_DEVTYPE_GPU, "dummy_kernel_gpu"));

  atmi_kernel_t nw_kernel1;
  atmi_kernel_t nw_kernel2;
  const unsigned int nw1_num_args = 11;
  size_t nw1_arg_sizes[nw1_num_args];
  for(int i = 0; i < 3; i++) nw1_arg_sizes[i] = sizeof(int *);
  for(int i = 3; i < nw1_num_args; i++) nw1_arg_sizes[i] = sizeof(int);
  ErrorCheck(atmi_kernel_create(&nw_kernel1, nw1_num_args, nw1_arg_sizes, 1,
        ATMI_DEVTYPE_GPU, "nw_kernel1_gpu"));
  ErrorCheck(atmi_kernel_create(&nw_kernel2, nw1_num_args, nw1_arg_sizes, 1,
        ATMI_DEVTYPE_GPU, "nw_kernel2_gpu"));

  ATMI_LPARM_1D(dummy_lp, 1);
  dummy_lp->place = ATMI_PLACE_GPU(0, 0);
  dummy_lp->synchronous = ATMI_TRUE;
  dummy_lp->kernel_id = 0;
  atmi_task_handle_t dummy_task = atmi_task_launch(dummy_lp, dummy_kernel, dummy_args);
  if(dummy_task == ATMI_NULL_TASK_HANDLE) {
    fprintf(stderr, "Dummy kernel failed.\n");
  }
  printf("Tmp Var: %d\n", *tmp_var);
  //delete tmp_var;
  ErrorCheck(atmi_free(tmp_var));


  int num_diagonal = worksize / BLOCK_SIZE;
  int num_tasks = 0;
  for( int blk = 1 ; blk <= worksize/BLOCK_SIZE ; blk++) {
    int num_tasks_this_iter = (blk + (task_sze - 1))/task_sze;
    num_tasks += num_tasks_this_iter;
  }
  for( int blk =  worksize/BLOCK_SIZE - 1  ; blk >= 1 ; blk--){
    int num_tasks_this_iter = (blk + (task_sze - 1))/task_sze;
    num_tasks += num_tasks_this_iter;
  }
  printf("# of tasks = %d\n",num_tasks);

  atmi_lparm_t* lparm_nw = (atmi_lparm_t*)malloc(num_tasks * sizeof(atmi_lparm_t));
  typedef atmi_task_handle_t task_handle;
  task_handle* nw_tasks = (task_handle*)malloc(num_tasks * sizeof(task_handle));
  task_handle* task_deps_list = (task_handle*)malloc(4 * num_tasks * sizeof(task_handle));

  for (int i = 0; i < num_tasks; i++) {
    lparm_init(&lparm_nw[i]);
    lparm_nw[i].gridDim[1] = 1;
    lparm_nw[i].groupDim[0] = BLOCK_SIZE;
    lparm_nw[i].groupDim[1] = BLOCK_SIZE;
    lparm_nw[i].synchronous = ATMI_FALSE;
  }

  int nw_task_index = -1;
  int nOrw_task_index = -1;
  int last_task_index = -1;
  int task_index = 0;
  printf("Processing upper-left matrix\n");
  /* beginning of timing point */
  stopwatch_start(&sw);
  for( int blk = 1 ; blk <= worksize/BLOCK_SIZE ; blk++) {
    int num_tasks_this_iter = (blk + (task_sze - 1))/task_sze;
    int last_task_size = blk - task_sze *  (num_tasks_this_iter - 1);

    nw_task_index = nOrw_task_index;
    nOrw_task_index = last_task_index;
    last_task_index = task_index;

    for (int i = 0; i < num_tasks_this_iter; i++) {
      int this_task_sze = (i == num_tasks_this_iter - 1) ? last_task_size : task_sze;
      std::set<int> dep_task_list;
      for (int j = 0; j < this_task_sze; j++) {
        int nw_neighbour = (i * task_sze) + j - 1;
        int n_neighbour = (i * task_sze) + j;
        int w_neighbour = (i * task_sze) + j - 1;
        // Inserting west task to dependency list
        int tmp_tsk;
        if (w_neighbour >= 0) {
          tmp_tsk = nOrw_task_index + w_neighbour/task_sze;
          //printf("Inserting task:%d to dep list of task:%d; w_index:%d, w_neighbour:%d, tsk_sze:%d, blk:%d, i:%d, j%d\n", tmp_tsk, task_index, nOrw_task_index, w_neighbour, task_sze, blk, i, j);
          if (tmp_tsk >= 0)
            dep_task_list.insert(tmp_tsk);
        }
        // Inserting northwest task to dependency list
        if (nw_neighbour >= 0 &&
            nw_neighbour < ( blk - 2)) {
          tmp_tsk = nw_task_index + nw_neighbour/task_sze;
          //printf("Inserting task:%d to dep list of task:%d; nw_index:%d, nw_neighbour:%d, tsk_sze:%d, blk:%d, i:%d, j%d\n", tmp_tsk, task_index, nw_task_index, nw_neighbour, task_sze, blk, i, j);
          if (tmp_tsk >= 0)
            dep_task_list.insert(tmp_tsk);
        }
        // Inserting north task to dependency list
        if (n_neighbour >= 0 &&
            n_neighbour < (blk -1)) {
          tmp_tsk = nOrw_task_index + n_neighbour/task_sze;
          //printf("Inserting task:%d to dep list of task:%d; n_index:%d, n_neighbour:%d, tsk_sze:%d, blk:%d, i:%d, j%d\n", tmp_tsk, task_index, nOrw_task_index, n_neighbour, task_sze, blk, i, j);
          if (tmp_tsk >= 0)
            dep_task_list.insert(tmp_tsk);
        }
      }
      assert(dep_task_list.size() <= 4);
      lparm_nw[task_index].num_required = dep_task_list.size();
      std::set<int>::iterator it;
      //printf("Task:%d is dependent on ",task_index);
      int it_index = 0;
      for (it = dep_task_list.begin(); it != dep_task_list.end(); ++it) {
        //printf("task:%d\t",*it);
        task_deps_list[4 * task_index + it_index] = nw_tasks[*it];
        it_index++;
      }
      //printf("\n");
      lparm_nw[task_index].requires = &task_deps_list[4 * task_index];
      if (i == num_tasks_this_iter - 1) { // Is last task?
        lparm_nw[task_index].gridDim[0] = BLOCK_SIZE * last_task_size;
      } else {
        lparm_nw[task_index].gridDim[0] = BLOCK_SIZE * task_sze;
      }
      int nw_kernel1_idx = i * task_sze;
      void *nw_kernel1_args[] = {
        &d_reference,
        &d_input_itemsets,
        &output_itemsets,
        &max_cols,
        &penalty,
        &blk,
        &block_width,
        &worksize,
        &offset_r,
        &offset_c,
        &nw_kernel1_idx
          //(i * task_sze)
      };
      nw_tasks[task_index] = atmi_task_launch(&lparm_nw[task_index], nw_kernel1, nw_kernel1_args);
      /*nw_kernel1(&lparm_nw[task_index],
        reference,
        input_itemsets,
        output_itemsets,
        max_cols,
        penalty,
        blk,
        block_width,
        worksize,
        offset_r,
        offset_c,
        (i * task_sze));
        */
      task_index++;
    }
  }

  printf("Processing lower-right matrix\n");
  for( int blk =  worksize/BLOCK_SIZE - 1  ; blk >= 1 ; blk--){
    int num_tasks_this_iter = (blk + (task_sze - 1))/task_sze;
    int last_task_size = blk - task_sze *  (num_tasks_this_iter - 1);

    nw_task_index = nOrw_task_index;
    nOrw_task_index = last_task_index;
    last_task_index = task_index;

    for (int i = 0; i < num_tasks_this_iter; i++) {
      int this_task_sze = (i == num_tasks_this_iter - 1) ? last_task_size : task_sze;
      std::set<int> dep_task_list;
      for (int j = 0; j < this_task_sze; j++) {
        int nw_neighbour = (blk == worksize/BLOCK_SIZE - 1) ? (i * task_sze) + j  : (i * task_sze) + j + 1;
        int n_neighbour = (i * task_sze) + j + 1;
        int w_neighbour = (i * task_sze) + j;
        // Inserting north, west and northwest task to dependency list
        int tmp_tsk = nOrw_task_index + n_neighbour/task_sze;
        dep_task_list.insert(tmp_tsk);
        tmp_tsk = nOrw_task_index+ w_neighbour/task_sze;
        dep_task_list.insert(tmp_tsk);
        tmp_tsk = nw_task_index + nw_neighbour/task_sze;
        dep_task_list.insert(tmp_tsk);
      }
      assert(dep_task_list.size() <= 4);
      lparm_nw[task_index].num_required = dep_task_list.size();
      std::set<int>::iterator it;
      int it_index = 0;
      //printf("Task:%d is dependent on ",task_index);
      for (it = dep_task_list.begin(); it != dep_task_list.end(); ++it) {
        //printf("task:%d\t",*it);
        task_deps_list[4 * task_index + it_index] = nw_tasks[*it];
        it_index++;
      }
      //printf("\n");
      lparm_nw[task_index].requires = &task_deps_list[4 * task_index];
      if (i == num_tasks_this_iter - 1) { // Is last task?
        lparm_nw[task_index].gridDim[0] = BLOCK_SIZE * last_task_size;
      } else {
        lparm_nw[task_index].gridDim[0] = BLOCK_SIZE * task_sze;
      }
      int nw_kernel2_idx = i * task_sze;
      void *nw_kernel2_args[] = {
        &d_reference,
        &d_input_itemsets,
        &output_itemsets,
        &max_cols,
        &penalty,
        &blk,
        &block_width,
        &worksize,
        &offset_r,
        &offset_c,
        &nw_kernel2_idx
          //(i * task_sze)
      };
      nw_tasks[task_index] = atmi_task_launch(&lparm_nw[task_index], nw_kernel2, nw_kernel2_args);
      /*nw_tasks[task_index] =  nw_kernel2(&lparm_nw[task_index],
        reference,
        input_itemsets,
        output_itemsets,
        max_cols,
        penalty,
        blk,
        block_width,
        worksize,
        offset_r,
        offset_c,
        (i * task_sze));
        */
      task_index++;
    }
  }
  ErrorCheck(atmi_task_wait(nw_tasks[last_task_index]));
  //SYNC_STREAM(NULL);
  /* end of timing point */
  stopwatch_stop(&sw);
  printf("Time consumed(ms): %lf\n", 1000*get_interval_by_sec(&sw));

  //memcpy(output_itemsets, input_itemsets, max_cols * max_rows * sizeof(int));
  ErrorCheck(atmi_memcpy(output_itemsets, d_input_itemsets, max_rows * max_cols * sizeof(int)));

#define TRACEBACK
#ifdef TRACEBACK

  FILE *fpo = fopen("result.txt","w");
  fprintf(fpo, "print traceback value GPU:\n");

  for (int i = max_rows - 2,  j = max_rows - 2; i>=0, j>=0;){
    int nw, n, w, traceback;
    if ( i == max_rows - 2 && j == max_rows - 2 )
      fprintf(fpo, "%d ", output_itemsets[ i * max_cols + j]); //print the first element
    if ( i == 0 && j == 0 )
      break;

    if ( i > 0 && j > 0 ){
      nw = output_itemsets[(i - 1) * max_cols + j - 1];
      w  = output_itemsets[ i * max_cols + j - 1 ];
      n  = output_itemsets[(i - 1) * max_cols + j];
    }
    else if ( i == 0 ){
      nw = n = LIMIT;
      w  = output_itemsets[ i * max_cols + j - 1 ];
    }
    else if ( j == 0 ){
      nw = w = LIMIT;
      n  = output_itemsets[(i - 1) * max_cols + j];
    }
    else{
    }

    //traceback = maximum(nw, w, n);
    int new_nw, new_w, new_n;
    new_nw = nw + reference[i * max_cols + j];
    new_w = w - penalty;
    new_n = n - penalty;

    traceback = maximum(new_nw, new_w, new_n);
    if(traceback == new_nw)
      traceback = nw;
    if(traceback == new_w)
      traceback = w;
    if(traceback == new_n)
      traceback = n;

    fprintf(fpo, "%d ", traceback);

    if(traceback == nw )
    {i--; j--; continue;}

    else if(traceback == w )
    {j--; continue;}

    else if(traceback == n )
    {i--; continue;}

    else
      ;
  }

  fclose(fpo);

#endif

  printf("Computation Done\n");

  ErrorCheck(atmi_kernel_release(dummy_kernel));
  ErrorCheck(atmi_kernel_release(nw_kernel1));
  ErrorCheck(atmi_kernel_release(nw_kernel2));

  ErrorCheck(atmi_free(d_reference));
  ErrorCheck(atmi_free(d_input_itemsets));
  free(reference);
  free(input_itemsets);
  free(output_itemsets);

}

