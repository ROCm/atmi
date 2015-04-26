#Set HSA Environment variables
HSA_RUNTIME_PATH=/opt/hsa
HSA_LIBHSAIL_PATH=/opt/hsa/lib
HSA_LLVM_PATH=/opt/amd/cloc/bin

default:
	g++ -o snk_genw.o -I${HSA_RUNTIME_PATH}/include -O2 -c snk_genw.c

clean:
	rm -rf snk_genw.o
