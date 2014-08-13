kernel int matmul(global int *A, global int *B, global int *C, global int *N1, global int *P1) {
   int i = get_global_id(0);
   int j = get_global_id(1);
   int k;
   int P = *P1;
   int N = *N1;

// C[i][j]  for given i and j

   C[i*P+j] = 0;
   for (k=0;k<N;k++) 
      C[i*P+j] += A[i*N+k] * B[k*P+j];
}

