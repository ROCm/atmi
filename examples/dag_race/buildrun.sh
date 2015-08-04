#!/bin/bash
set -e
#  Set HSA Environment variables
[ -z $HSA_RUNTIME_PATH ] && HSA_RUNTIME_PATH=/opt/hsa
[ -z $ATMI_PATH ] && ATMI_PATH=/opt/amd/atmi

ATMI_INC=$ATMI_PATH/include

echo 
if [ -f dag_race ] ; then rm dag_race ; fi

echo gcc -c -o players.o dag_race.c -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=players.cl -O3 -I$ATMI_INC
gcc -c -o players.o dag_race.c -g -fplugin=atmi_pifgen.so -fplugin-arg-atmi_pifgen-clfile=players.cl -O3 -I$ATMI_INC

echo gcc -o dag_race players.o dag_race.c.pifdefs.c -g -O3 -lelf -L$ATMI_PATH/lib -latmi_runtime -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include
gcc -o dag_race players.o dag_race.c.pifdefs.c -g -O3 -lelf -L$ATMI_PATH/lib -latmi_runtime -L$HSA_RUNTIME_PATH/lib -lhsa-runtime64 -I$ATMI_INC -I$HSA_RUNTIME_PATH/include

#  Execute
echo
echo ./dag_race 
./dag_race 

