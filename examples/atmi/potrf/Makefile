CC          = gcc
FC          = gfortran
PLUGIN_LIB  = /home/wwu/ATMI/lib/atmi_pifgen.so
CFLAGS      = -g -O0 -lelf -lm
FFLAGS      = -g -O0 -lelf -lm
DPLASMA_INC = -I/home/wwu/opt/include/dplasma
BLAS_INC    = -I/home/wwu/sw/openblas/include
PLASMA_INC  = -I/home/wwu/sw/plasma-openblas/include
DPLASMA_LIB = /home/wwu/dplasma-openblas/dague/libdague.a /home/wwu/dplasma-openblas/dague/libdague-base.a /home/wwu/dplasma-openblas/data_dist/matrix/libdague_distribution_matrix.a /home/wwu/dplasma-openblas/data_dist/libdague_distribution.a /home/wwu/dplasma-openblas/dplasma/lib/libdplasma.a
HWLOC_LIB   = -lhwloc
BLAS_LIB    = -L/home/wwu/sw/openblas/lib -lopenblas
PLASMA_LIB  = -L/home/wwu/sw/plasma-openblas/lib -lcoreblas -llapacke -ltmg -llapack -lhwloc
EXT_LIB     = -lpthread -lgfortran
HSA_RUNTIME_PATH=/opt/hsa
ATMILIB_RUNTIME_PATH=/home/wwu/ATMI
ATMI_INC=-I${ATMILIB_RUNTIME_PATH}/include -I${HSA_RUNTIME_PATH}/include
ATMI_LIB = -L${ATMILIB_RUNTIME_PATH}/lib -latmi_runtime -L${HSA_RUNTIME_PATH}/lib -lhsa-runtime64 

.PHONY: all clean 

all: common.o atmi_spotrf_L.o testing_spotrf.o pifdefs.o testing_spotrf

common.o: common.c
	$(CC) $(CFLAGS) $(DPLASMA_INC) $(BLAS_INC) $(PLASMA_INC) $(ATMI_INC) -c $^ -o $@

atmi_spotrf_L.o: atmi_spotrf_L.c gpuKernels.cl
	$(CC) $(CFLAGS) -fplugin=$(PLUGIN_LIB) -fplugin-arg-atmi_pifgen-pifgenfile=pifdefs.c -fplugin-arg-atmi_pifgen-clfile=gpuKernels.cl $(DPLASMA_INC) $(BLAS_INC) $(PLASMA_INC) $(ATMI_INC) -c atmi_spotrf_L.c -o $@

pifdefs.o: pifdefs.c
	$(CC) $(CFLAGS) $(DPLASMA_INC) $(BLAS_INC) $(PLASMA_INC) $(ATMI_INC) -c $^ -o $@

testing_spotrf.o: testing_spotrf.c
	$(CC) $(CFLAGS) $(DPLASMA_INC) $(BLAS_INC) $(PLASMA_INC) $(ATMI_INC) -c $^ -o $@

testing_spotrf: common.o atmi_spotrf_L.o testing_spotrf.o pifdefs.o
	$(FC) $(FFLAGS) $(DPLASMA_INC) $(BLAS_INC) $(PLASMA_INC) $(ATMI_INC) $^ $(DPLASMA_LIB) $(EXT_LIB) $(HWLOC_LIB) $(BLAS_LIB) $(PLASMA_LIB) $(ATMI_LIB) -o $@

#%.o: %.c
#	$(CC) $(CFLAGS) $(DPLASMA_INC) $(BLAS_INC) $(PLASMA_INC) $(ATMI_INC) -c $^ -o $@

clean:
	rm -f *.o
	rm -f *pifdefs.c
	rm -f testing_spotrf

