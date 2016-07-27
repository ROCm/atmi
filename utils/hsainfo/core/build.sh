HSA_PATH=/opt/rocm/hsa
g++ -g -o hsainfo -I ${HSA_PATH}/include -I../inc hsainfo.cpp common.cpp -L ${HSA_PATH}/lib -lhsa-runtime64 -std=c++11
#g++ -g -Wl,--unresolved-symbols=ignore-in-shared-libs -o hsainfo -I ${HSA_PATH}/include hsainfo.cpp common.cpp -L ${HSA_PATH}/lib -lhsa-runtime64 -std=c++11
