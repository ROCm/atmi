#!/bin/bash

#!/bin/bash
#
#MIT License 
#
#Copyright © 2016 Advanced Micro Devices, Inc.  
#
#Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
#without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
#persons to whom the Software is furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
#PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
#OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

if [ $# -eq 1 ] && [ "$1" = "--help" ]; then 
	echo "usage: ./build.sh -l <layout> -a <allocation> -t <agent> -c -v -m <mode>"
	exit 0
fi

[ -z ${HSA_RUNTIME_PATH} ] && HSA_RUNTIME_PATH=/opt/rocm/hsa

ATMIPATH=/opt/rocm/libatmi
CLOC_PATH=/usr/bin
CC=/usr/bin/gcc
CXX=g++

while [ $# -gt 0 ]; do
  key="$1"
  case $key in
    -l|--layout)
      layout="$2"
      shift # option has parameter
			;;
	-m|--mode)
			mode="$2"
			shift
			;;
    -c|--copy)
      copy="COPY"
			;;
    -b|--brig)
      module_type="MODULE_BRIG"
            ;;
    -v|--verbose)
      verbose="VERBOSE"
			;;
    -a|--alloc)
      alloc="$2"
			shift
			;;
		-t|--agent)
			agent="$2"
			shift
			;;
    *)
			echo "Unknown option:" $key
			exit 0
			;;
  esac
  shift
done

[ "$layout" ] || { layout="AOS"; }
[ "$mode" ] || { mode="build";} 
[ "$agent" ] || { agent="DEVICE";}
[ "$copy" ] || { copy="NOCOPY";}
[ "$alloc" ] || { alloc="FINE";}
[ "$verbose" ] || { verbose="CURT";}
[ "$module_type" ] || { module_type="MODULE_GCN";}

host=`hostname`
case $host in 
	xn0|xn1|xn2|xn3|xn4|xn5|xn6|xn7|xn8|xn9)
		node="kaveri"
		;;
	c0|c1|c2|c3)
		node="carrizo"
		;;
	t1|ROCNREDLINE)
		node="fiji"
		;;
	*)
		echo "unknown host node" $host
		exit 0
esac


if [ $mode = "clean" ]; then
	rm -rf *.o  *~ grayscale_hsaco.h kernel.[ch]
	for l in AOS DA; do
		rm -rf dlbench.atmi_${l}
	done
fi	

# build from C source 
if [ $mode = "build" ]; then
	echo "${CLOC_PATH}/cloc.sh -mcpu fiji -opt 2 grayscale.cl"
	${CLOC_PATH}/cloc.sh -mcpu fiji -opt 2 grayscale.cl
	echo "${CXX} -g -I${ATMIPATH}/include -I${HSA_RUNTIME_PATH}/include -D${module_type} -D${alloc} -D${layout} -D${verbose} -I. -c dlbench.atmi.c -std=c++11"
	${CXX} -g -I${ATMIPATH}/include -I${HSA_RUNTIME_PATH}/include -D${module_type} -D${alloc} -D${layout} -D${verbose} -I. -c dlbench.atmi.c -std=c++11
	echo "${CXX} -o dlbench.atmi_${layout} dlbench.atmi.o ${LFLAGS} -L${ATMIPATH}/lib -latmi_runtime "
	${CXX} -o dlbench.atmi_${layout} dlbench.atmi.o ${LFLAGS} -L${ATMIPATH}/lib -latmi_runtime 
fi

