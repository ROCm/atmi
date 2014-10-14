#include "matmul.h"

/*************************** 

   simple_sgemm_tt:  Simple matrix multiplication 

   Assumptions:
    - A,B,C are in row-major order
    - A,B,C are initialized. Typically C=0.0

***************************/

__kernel void simple_sgemm_tt(const int M, const int N, const int K, const float alpha, const float * A, const int LDA, const float * B, const int LDB, const float beta ,  float* C, const int LDC){ 

   int C_row = get_global_id(0);
   int C_col = get_global_id(1);

   float Cvalue = 0.0;
   for (int i = 0; i < K ; ++i) Cvalue += A[(C_row*LDA)+i]*B[(i*LDB)+C_col]; 
   C[(C_row*LDC)+C_col] = alpha*Cvalue +  beta*C[(C_row*LDC)+C_col] ;
}

/*************************** 

   tiled_sgemm_tt:  Tiled matrix multiplication: 

   Same assumptions as simple_sgemm_tt

***************************/

__kernel void tiled_sgemm_tt(const int M, const int N, const int K, const float alpha, const float * A, const int LDA, const float * B, const int LDB, const float beta ,  float* C, const int LDC){ 

   int C_row = get_global_id(0);
   int C_col = get_global_id(1);
   __private int kblock,e,A_col,B_row;

   // Local memory used to store submatrices As and Bs 
   __local float As[BLOCK_SIZE][BLOCK_SIZE];
   __local float Bs[BLOCK_SIZE][BLOCK_SIZE];

   // Thread row and column 
   int row = get_local_id(0);
   int col = get_local_id(1);

   // A_row and B_col are constant for this group
   int A_row = (BLOCK_SIZE*get_group_id(0)) + row;
   int B_col = (BLOCK_SIZE*get_group_id(1)) + col;

   // Each thread computes one element of C
   float Cvalue = 0.0;

   // Loop over all the sub-matrices of A and B required to compute CVALUE
   for (kblock = 0; kblock  < (((K-1) / BLOCK_SIZE) + 1) ; ++kblock ) {
      // Copy global memory values to local memory
      A_col = (BLOCK_SIZE*kblock) + col;
      As[row][col] = A[(A_row*LDA)+ A_col];
      B_row = (BLOCK_SIZE*kblock) + row;
      Bs[row][col] = B[(B_row*LDB)+ B_col];

      // Synchronize to ensure As and Bs are loaded before starting computation
      barrier(CLK_LOCAL_MEM_FENCE);

      // Multiply As and Bs and accumulate in Cvalue
      for (e = 0; e < BLOCK_SIZE; ++e) Cvalue += As[row][e] * Bs[e][col];

      // Synchronize to ensure computation is done before next iteration
      barrier(CLK_LOCAL_MEM_FENCE);

   }

   // Fetch C element and write Cvalue to global memory
   C[(C_row*LDC)+C_col] = alpha*Cvalue +  beta*C[(C_row*LDC)+C_col] ;
}

/*************************** 

   tiled_sgemm_tn:  Tiled matrix multiplication: 

    - A,C are in row-major order
    - B is in column-major order 
    - A,B,C are initialized. Typically C=0.0

***************************/

__kernel void tiled_sgemm_tn(const int M, const int N, const int K, const float alpha, const float * A, const int LDA, const float * B, const int LDB, const float beta ,  float* C, const int LDC){ 

   int C_row = get_global_id(0);
   int C_col = get_global_id(1);

   // Local memory used to store submatrices As and Bs 
   __local float As[BLOCK_SIZE][BLOCK_SIZE];
   __local float Bs[BLOCK_SIZE][BLOCK_SIZE];

   // Thread row and column 
   int row = get_local_id(0);
   int col = get_local_id(1);

   // A_row and B_col are constant for this group and thread
   int A_row = (BLOCK_SIZE*get_group_id(0)) + row;
   int B_col = (BLOCK_SIZE*get_group_id(1)) + col;

   // Each thread computes one element of C
   float Cvalue = 0.0;

   // Loop over all the sub-matrices of A and B required to compute CVALUE
   for (__private int kblock = 0; kblock  < (((K-1) / BLOCK_SIZE) + 1); kblock++ ) {
      // Copy global memory values to local memory
      As[row][col] = A[(A_row*LDA) + (BLOCK_SIZE*kblock) + col ];
      Bs[row][col] = B[(B_col*LDB) + (BLOCK_SIZE*kblock) + row ];

      // Synchronize to ensure As and Bs are loaded before starting computation
      barrier(CLK_LOCAL_MEM_FENCE);

      // Multiply As and Bs and accumulate in Cvalue
      for (__private int e = 0; e < BLOCK_SIZE; ++e) Cvalue += As[row][e] * Bs[e][col];

      // Synchronize to ensure computation is done before next iteration
      barrier(CLK_LOCAL_MEM_FENCE);

   }

   // Fetch C element and write Cvalue to global memory
   C[(C_row*LDC)+C_col] = alpha*Cvalue +  beta*C[(C_row*LDC)+C_col] ;
}
