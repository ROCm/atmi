kernel int memApp(global int *A, global int *N1,global int *start1, global int *stride1, global int *laps1,global int *skip1, global int *epocs1) {
   int N = *N1; int start = *start1; int stride = *stride1;
   int laps = *laps1;int skip = *skip1; int epocs = *epocs1;

   int i = get_global_id(0);
   int k = start;
   int l, p;

   for (l=0;l<epocs;l++) {
      p = 0;
      while ((p++ < laps) && (k < N)) {
 	   A[k] = i + k;
	   k += stride;
      }
      k = k + skip;
   }
}

