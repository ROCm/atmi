#!/bin/bash
#
#  cloc.sh: Compile cl file into an HSA Code object file (.hsaco)  
#           using the LLVM Ligntning Compiler. An hsaco file contains 
#           the amdgpu isa that can be loaded by the HSA Runtime.
#
#  Old options -hsail and -brig use HLC that will be deprecated
#
#  Written by Greg Rodgers  Gregory.Rodgers@amd.com
#
PROGVERSION=1.3.2a
#
# Copyright (c) 2016 ADVANCED MICRO DEVICES, INC.  
# 
# AMD is granting you permission to use this software and documentation (if any) (collectively, the 
# Materials) pursuant to the terms and conditions of the Software License Agreement included with the 
# Materials.  If you do not have a copy of the Software License Agreement, contact your AMD 
# representative for a copy.
# 
# You agree that you will not reverse engineer or decompile the Materials, in whole or in part, except for 
# example code which is provided in source code form and as allowed by applicable law.
# 
# WARRANTY DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
# KIND.  AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT 
# LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
# PURPOSE, TITLE, NON-INFRINGEMENT, THAT THE SOFTWARE WILL RUN UNINTERRUPTED OR ERROR-
# FREE OR WARRANTIES ARISING FROM CUSTOM OF TRADE OR COURSE OF USAGE.  THE ENTIRE RISK 
# ASSOCIATED WITH THE USE OF THE SOFTWARE IS ASSUMED BY YOU.  Some jurisdictions do not 
# allow the exclusion of implied warranties, so the above exclusion may not apply to You. 
# 
# LIMITATION OF LIABILITY AND INDEMNIFICATION:  AMD AND ITS LICENSORS WILL NOT, 
# UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT, INCIDENTAL, 
# INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF THE SOFTWARE OR THIS 
# AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH 
# DAMAGES.  In no event shall AMD's total liability to You for all damages, losses, and 
# causes of action (whether in contract, tort (including negligence) or otherwise) 
# exceed the amount of $100 USD.  You agree to defend, indemnify and hold harmless 
# AMD and its licensors, and any of their directors, officers, employees, affiliates or 
# agents from and against any and all loss, damage, liability and other expenses 
# (including reasonable attorneys' fees), resulting from Your use of the Software or 
# violation of the terms and conditions of this Agreement.  
# 
# U.S. GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with "RESTRICTED RIGHTS." 
# Use, duplication, or disclosure by the Government is subject to the restrictions as set 
# forth in FAR 52.227-14 and DFAR252.227-7013, et seq., or its successor.  Use of the 
# Materials by the Government constitutes acknowledgement of AMD's proprietary rights in them.
# 
# EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as stated in the 
# Software License Agreement.
# 
function usage(){
/bin/cat 2>&1 <<"EOF" 

   cloc.sh: Compile a cl or cu file into an HSA Code object file (.hsaco)  
            using the LLVM Ligntning Compiler. An hsaco file contains 
            the amdgpu isa that can be loaded by the HSA Runtime.
            As of amdcloc 1.3.1, use of cuda is only experimental.  
            Generated kernels from cuda will not execute. 

   Usage: cloc.sh [ options ] filename.cl

   Options without values:
    -ll       Generate IR for LLVM steps before generating hsaco
    -s        Generate dissassembled gcn from hsaco
    -g        Generate debug information
    -noqp     No quickpath, Use LLVM commands instead of clang driver
    -noshared Do not link hsaco as shared object, forces noqp
    -version  Display version of cloc then exit
    -v        Verbose messages
    -n        Dryrun, do nothing, show commands that would execute
    -h        Print this help message
    -k        Keep temporary files
    -brig     Generate brig instead of hsaco (deprecated)
    -hsail    Generate dissassembled hsail for brig  (deprecated)

   Options with values:
    -amdllvm   <path>           $AMDLLVM or /opt/amd/llvm
    -libgcn    <path>           $LIBGCN or /opt/rocm/libamdgcn  
    -hlcpath   <path>           $HLC_PATH or /opt/rocm/hlc3.2/bin  
    -cuda-path <path>           $CUDA_PATH or /usr/local/cuda
    -mcpu      <cputype>        Default= value returned by mymcpu
    -bclib     <bcfile>         Add a bc library for llvm-link
    -clopts    <compiler opts>  Addtional options for cl frontend
    -cuopts    <compiler opts>  Additonal options for cu frontend
    -I         <include dir>    Provide one directory per -I option
    -lkopts    <LLVM link opts> Default=$LIBGCN/lib/libamdgcn.$mcpu.bc
    -hsaillib  <fname>          Filename of hsail library. (deprecated)
    -opt       <LLVM opt>       LLVM optimization level
    -o         <outfilename>    Default=<filename>.<ft> ft=brig or hsail
    -t         <tdir>           Temporary directory or intermediate files
                                Default=/tmp/cloc-tmp-$$
   Examples:
    cloc.sh my.cl             /* creates my.hsaco                    */
    cloc.sh whybother.cu      /* creates whybother.hsaco             */

   Note: Instead of providing these command line options:
   -opt,-hlcpath,-amdllvm,-libgcn,-cuda-path,-mcpu,-clopts, or -lkopts 

   You may set these environment variables, respectively:
   LLVMOPT,HLC_PATH,AMDLLVM,LIBGCN,CUDA_PATH,LC_MCPU,CLOPTS, or LKOPTS 

   Command line options will take precedence over environment variables. 

   Copyright (c) 2016 ADVANCED MICRO DEVICES, INC.

EOF
   exit 1 
}

DEADRC=12

#  Utility Functions
function do_err(){
   if [ $NEWTMPDIR ] ; then 
      if [ $KEEPTDIR ] ; then 
         cp -rp $TMPDIR $OUTDIR
         [ $VERBOSE ] && echo "#Info:  Temp files copied to $OUTDIR/$TMPNAME"
      fi
      rm -rf $TMPDIR
   else 
      if [ $KEEPTDIR ] ; then 
         [ $VERBOSE ] && echo "#Info:  Temp files kept in $TMPDIR"
      fi 
   fi
   [ $VERBOSE ] && echo "#Info:  Done"
   exit $1
}

function version(){
   echo $PROGVERSION
   exit 0
}

function runcmd(){
   THISCMD=$1
   if [ $DRYRUN ] ; then
      echo "$THISCMD"
   else 
      [ $VV ] && echo "$THISCMD"
      $THISCMD
      rc=$?
      if [ $rc != 0 ] ; then 
         echo "ERROR:  The following command failed with return code $rc."
         echo "        $THISCMD"
         do_err $rc
      fi
   fi
}

function getdname(){
   local __DIRN=`dirname "$1"`
   if [ "$__DIRN" = "." ] ; then 
      __DIRN=$PWD; 
   else
      if [ ${__DIRN:0:1} != "/" ] ; then 
         if [ ${__DIRN:0:2} == ".." ] ; then 
               __DIRN=`dirname $PWD`/${__DIRN:3}
         else
            if [ ${__DIRN:0:1} = "." ] ; then 
               __DIRN=$PWD/${__DIRN:2}
            else
               __DIRN=$PWD/$__DIRN
            fi
         fi
      fi
   fi
   echo $__DIRN
}

function ptxll2gcnll(){
   LLFILE=$1
   # change IR from ptx to amdgcn
   sed -i -e"s/nvptx64-nvidia-cuda/amdgcn--amdhsa/" $LLFILE
   sed -i -e"s/e-i64:64-v16:16-v32:32-n16:32:64/e-p:32:32-p1:64:64-p2:64:64-p3:32:32-p4:64:64-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64/" $LLFILE
   sed -i -e"s/\"target-features\"=\"+ptx42\" //" $LLFILE
   sed -i -e"s/\"sm_35\"/\"$LC_MCPU\"/" $LLFILE
   sed -i -e"s/^define /define amdgpu_kernel /" $LLFILE
   while read ptxintr hsaintr ; do 
      sed -i -e "s/$ptxintr/$hsaintr/" $LLFILE
   done < $CLOC_PATH/intrinsics.map
}

#  --------  The main code starts here -----
INCLUDES=""
#  Argument processing
while [ $# -gt 0 ] ; do 
   case "$1" in 
      -q)               QUIET=true;;
      --quiet)          QUIET=true;;
      -k) 		KEEPTDIR=true;; 
      -n) 		DRYRUN=true;; 
      -hsail) 		GEN_IL=true;; 
      -brig) 		GEN_BRIG=true;; 
      -g) 		GEN_DEBUG=true;; 
      -ll) 		GENLL=true;;
      -s) 		GENASM=true;;
      -noqp) 		NOQP=true;;
      -noshared) 	NOSHARED=true;;
      -clopts) 		CLOPTS=$2; shift ;; 
      -cuopts) 		CUOPTS=$2; shift ;; 
      -I) 		INCLUDES="$INCLUDES -I $2"; shift ;; 
      -opt) 		LLVMOPT=$2; shift ;; 
      -lkopts) 		LKOPTS=$2; shift ;; 
      -o) 		OUTFILE=$2; shift ;; 
      -t)		TMPDIR=$2; shift ;; 
      -hsaillib) 	HSAILLIB=$2; shift ;; 
      -bclib)		EXTRABCLIB=$2; shift ;; 
      -mcpu)            LC_MCPU=$2; shift ;;
      -atmipath)        ATMI_PATH=$2; shift ;;
      -amdllvm)         AMDLLVM=$2; shift ;;
      -libgcn)          LIBGCN=$2; shift ;;
      -hlcpath)         HLC_PATH=$2; shift ;;
      -cuda-path)       CUDA_PATH=$2; shift ;;
      -h) 	        usage ;; 
      -help) 	        usage ;; 
      --help) 	        usage ;; 
      -version) 	version ;; 
      --version) 	version ;; 
      -v) 		VERBOSE=true;; 
      -vv) 		VV=true;; 
      --) 		shift ; break;;
      *) 		break;echo $1 ignored;
   esac
   shift
done

# The above while loop is exited when last string with a "-" is processed
LASTARG=$1
shift

#  Allow output specifier after the cl file
if [ "$1" == "-o" ]; then 
   OUTFILE=$2; shift ; shift; 
fi

if [ ! -z $1 ]; then 
   echo " "
   echo "WARNING:  cloc.sh can only process one .cl or .cu file at a time."
   echo "          You can call cloc multiple times to get multiple outputs."
   echo "          Argument $LASTARG will be processed. "
   echo "          These args are ignored: $@"
   echo " "
fi

cdir=$(getdname $0)
[ ! -L "$cdir/cloc.sh" ] || cdir=$(getdname `readlink "$cdir/cloc.sh"`)

# These are default locations of the lightning compiler, libamdgcn, and HLC
AMDLLVM=${AMDLLVM:-/opt/amd/llvm}
LIBGCN=${LIBGCN:-/opt/rocm/libamdgcn}
HLC_PATH=${HLC_PATH:-/opt/rocm/hlc3.2/bin}
CUDA_PATH=${CUDA_PATH:-/usr/local/cuda}
ATMI_PATH=${ATMI_PATH:-/opt/rocm/atmi}
#CUOPTS=${CUOPTS:--S -emit-llvm -DGPUCC_AMDGCN -Wno-format-security --cuda-device-only --cuda-gpu-arch=sm_35 --cuda-path=$CUDA_PATH -include $AMDLLVM/lib/clang/4.0.0/include/cuda_builtin_vars.h}
CUOPTS=${CUOPTS:--S -emit-llvm -DGPUCC_AMDGCN -Wno-format-security --cuda-device-only --cuda-gpu-arch=sm_35 --cuda-path=$CUDA_PATH -include $AMDLLVM/lib/clang/4.0.0/include/__clang_cuda_builtin_vars.h}

if [ ! $LC_MCPU ] ; then 
   LC_MCPU=`mymcpu`
   if [ "$LC_MCPU" == "" ] ; then 
      LC_MCPU="fiji"
   fi
fi
#LC_MCPU="gfx803"

LLVMOPT=${LLVMOPT:-3}

if [ $VV ]  ; then 
   VERBOSE=true
fi

#BCFILES="$LIBGCN/$LC_MCPU/lib/opencl.amdgcn.bc"
#BCFILES="$BCFILES $LIBGCN/$LC_MCPU/lib/ockl.amdgcn.bc"
#BCFILES="$BCFILES $LIBGCN/$LC_MCPU/lib/ocml.amdgcn.bc"
#BCFILES="$BCFILES $LIBGCN/$LC_MCPU/lib/irif.amdgcn.bc"
BCFILES=""
BCFILES="$BCFILES $ATMI_PATH/lib/atmi.amdgcn.bc"
BCFILES="$BCFILES $AMDLLVM/lib/opencl.amdgcn.bc"
BCFILES="$BCFILES $AMDLLVM/lib/ockl.amdgcn.bc"
BCFILES="$BCFILES $AMDLLVM/lib/ocml.amdgcn.bc"
BCFILES="$BCFILES $AMDLLVM/lib/irif.amdgcn.bc"
#BCFILES="$BCFILES $AMDLLVM/lib/oclc_isa_version_700.amdgcn.bc"
BCFILES="$BCFILES $AMDLLVM/lib/oclc_isa_version_803.amdgcn.bc"
#LINKOPTS="-Xclang -mlink-bitcode-file -Xclang $LIBGCN/lib/libamdgcn.$LC_MCPU.bc"

if [ $EXTRABCLIB ] ; then 
   if [ -f $EXTRABCLIB ] ; then 
#     EXTRABCFILE will force QP off so LINKOPTS not used.
      BCFILES="$EXTRABCLIB $BCFILES"
   else
      echo "ERROR: Environment variable EXTRABCLIB is set to $EXTRABCLIB"
      echo "       File $EXTRABCLIB does not exist"
      exit $DEADRC
   fi
fi

filetype=${LASTARG##*\.}
if [ "$filetype" != "cl" ]  ; then 
   if [ "$filetype" != "cu" ] ; then 
      echo "ERROR:  $0 requires one argument with file type cl or cu"
      exit $DEADRC 
   else
      GPUCC=true
      echo "WARNING:  Use of GPUCC for cuda to amdgcn code object (hsaco) is very experimental\n"
      if [ ! -d $CUDA_PATH ] ; then 
         echo "ERROR:  No CUDA_PATH directory at $CUDA_PATH "
         exit $DEADRC
      fi
   fi
fi

#  Define the subcomands
if [ $GPUCC ] ; then 
   INCLUDES="-I $CUDA_PATH/include ${INCLUDES}"
   CMD_CLC=${CMD_CLC:-clang++ $CUOPTS $INCLUDES} 
else
   INCLUDES="-I ${HOME}/opt/include -I /home/aaji/git/ROCm-Device-Libs/ockl/inc -I /home/aaji/git/ROCm-Device-Libs/ocml/inc -I /home/aaji/git/ROCm-Device-Libs/irif/inc -I /opt/rocm/include -I ${LIBGCN}/include ${INCLUDES}" 
   CMD_CLC=${CMD_CLC:-clang -x cl -Xclang -cl-std=CL2.0 $CLOPTS $LINKOPTS $INCLUDES -include opencl-c.h -Dcl_clang_storage_class_specifiers -Dcl_khr_fp64 -target amdgcn--amdhsa -mcpu=$LC_MCPU } 
fi
CMD_LLA=${CMD_LLA:-llvm-dis}
CMD_LLL=${CMD_LLL:-llvm-link}
CMD_OPT=${CMD_OPT:-opt -O$LLVMOPT -mcpu=$LC_MCPU -amdgpu-annotate-kernel-features}
CMD_LLC=${CMD_LLC:-llc -mtriple amdgcn--amdhsa -mcpu=$LC_MCPU -filetype=obj}

RUNDATE=`date`

if [ ! -e "$LASTARG" ]  ; then 
   echo "ERROR:  The file $LASTARG does not exist."
   exit $DEADRC
fi

if [ "$HSAILLIB" != "" ] ; then 
   if [ ! -f $HSAILLIB ] ; then 
      echo "ERROR:  The HSAIL library file $HSAILLIB does not exist "
      exit $DEADRC
   fi
fi

# Parse LASTARG for directory, filename, and symbolname
INDIR=$(getdname $LASTARG)
FILENAME=${LASTARG##*/}
# FNAME has the .cl extension removed, used for naming intermediate filenames
FNAME=${FILENAME%.*}

if [ -z $OUTFILE ] ; then 
#  Output file not specified so use input directory
   OUTDIR=$INDIR
#  Make up the output file name based on last step 
   if [ $GEN_BRIG ] || [ $GEN_IL ] ; then
      OUTFILE=${FNAME}.brig
   else
      OUTFILE=${FNAME}.hsaco
   fi
else 
#  Use the specified OUTFILE
   OUTDIR=$(getdname $OUTFILE)
   OUTFILE=${OUTFILE##*/}
fi 

sdir=$(getdname $0)
[ ! -L "$sdir/cloc.sh" ] || sdir=$(getdname `readlink "$sdir/cloc.sh"`)
CLOC_PATH=${CLOC_PATH:-$sdir}

TMPNAME="cloc-tmp-$$"
TMPDIR=${TMPDIR:-/tmp/$TMPNAME}
if [ -d $TMPDIR ] ; then 
   KEEPTDIR=true
else 
   if [ $DRYRUN ] ; then
      echo "mkdir -p $TMPDIR"
   else
      mkdir -p $TMPDIR
      NEWTMPDIR=true
   fi
fi

# Be sure not to delete the output directory
if [ $TMPDIR == $OUTDIR ] ; then 
   KEEPTDIR=true
fi
if [ ! -d $TMPDIR ] && [ ! $DRYRUN ] ; then 
   echo "ERROR:  Directory $TMPDIR does not exist or could not be created"
   exit $DEADRC
fi 
if [ ! -d $OUTDIR ] && [ ! $DRYRUN ]  ; then 
   echo "ERROR:  The output directory $OUTDIR does not exist"
   exit $DEADRC
fi 

#  Print Header block
if [ $VERBOSE ] ; then 
   echo "#   "
   echo "#Info:  CLOC Version:	$PROGVERSION" 
   echo "#Info:  Run date:	$RUNDATE" 
   echo "#Info:  Input file:	$INDIR/$FILENAME"
   if [ $GEN_BRIG ] || [ $GEN_IL ]  ; then
      echo "#Info:  Brig file:	$OUTDIR/$OUTFILE"
      [ $GEN_IL ] && echo "#Info:  HSAIL file:	$OUTDIR/$FNAME.hsail"
      echo "#Info:  HLC path:	$HLC_PATH"
   else
      echo "#Info:  Code object:	$OUTDIR/$OUTFILE"
   fi
   echo "#Info:  AMDLLVM:	$AMDLLVM"
   echo "#Info:  ATMI Path:	$ATMI_PATH"
   [ $KEEPTDIR ] &&  echo "#Info:  Temp dir:	$TMPDIR" 
   echo "#   "
fi 

rc=0

if [ ! $GEN_IL ] && [ ! $GEN_BRIG ] ; then 

   # No HSAIL or Brig.  This is the new code object path
   # Use the Lightning Compiler in /opt/amd/llvm  to generate HSA code object

   if [ $VV ]  ; then 
      CLOPTS="-v $CLOPTS"
   fi

   if [ $NOQP ] || [ $GENLL ] || [ $NOSHARED ] || [ $EXTRABCLIB ] ; then 
      quickpath="false"
   else
      quickpath="true"
   fi
   #  Fixme :  need long path for linking multiple libs
   quickpath="false"

   if [ "$quickpath" == "true" ] ; then 

      [ $VERBOSE ] && echo "#Step:  Compile cl	cl --> hsaco ..."
      runcmd "$AMDLLVM/bin/$CMD_CLC -o $OUTDIR/$OUTFILE $INDIR/$FILENAME"

   else 
      # Run 4 steps, clang,link,opt,llc
      if [ $GPUCC ] ; then 
         [ $VERBOSE ] && echo "#Step:  GPUCC		cu --> ll  ..."
         runcmd "$AMDLLVM/bin/$CMD_CLC -o $TMPDIR/$FNAME.ll $INDIR/$FILENAME"

      #  echo
         cmd="$AMDLLVM/bin/llvm-as -o $TMPDIR/$FNAME.bc $TMPDIR/$FNAME.ll"
      #  echo $cmd; runcmd "$cmd"
         passes="-argpromotion"
         cmd="$AMDLLVM/bin/opt $passes -o $TMPDIR/$FNAME.lowpass.bc $TMPDIR/$FNAME.bc"
      #  echo $cmd; runcmd "$cmd"
         cmd="$AMDLLVM/bin/llvm-dis -o $TMPDIR/$FNAME.ll $TMPDIR/$FNAME.lowpass.bc"
      #  echo $cmd; runcmd "$cmd"
      #  echo

         # fix up the ptx ll to work with amdgcn
         [ $VERBOSE ] && echo "#Step:  Fix LLVM IR	ll --> ll  ..."
         runcmd "ptxll2gcnll $TMPDIR/$FNAME.ll"
         [ $VERBOSE ] && echo "#Step:  Assemble	ll --> bc ..."
         runcmd "$AMDLLVM/bin/llvm-as -o $TMPDIR/$FNAME.bc $TMPDIR/$FNAME.ll"
         
      else 
         [ $VERBOSE ] && echo "#Step:  Compile cl	cl --> bc ..."
         runcmd "$AMDLLVM/bin/$CMD_CLC -c -emit-llvm -o $TMPDIR/$FNAME.bc $INDIR/$FILENAME"
      fi

      if [ $GENLL ] ; then
         [ $VERBOSE ] && echo "#Step:  Disassemble	bc --> ll ..."
         runcmd "$AMDLLVM/bin/$CMD_LLA -o $TMPDIR/$FNAME.ll $TMPDIR/$FNAME.bc"
         if [ "$OUTDIR" != "$TMPDIR" ] ; then
            runcmd "cp $TMPDIR/$FNAME.ll $OUTDIR/$FNAME.ll"
         fi
      fi

      [ $VERBOSE ] && echo "#Step:  Link(llvm-link)	bc --> lnkd.bc ..."
      runcmd "$AMDLLVM/bin/$CMD_LLL $TMPDIR/$FNAME.bc $BCFILES -o $TMPDIR/$FNAME.lnkd.bc" 

      if [ $GENLL ] ; then
         [ $VERBOSE ] && echo "#Step:  Disassemble	lnkd.bc --> lnkd.ll ..."
         runcmd "$AMDLLVM/bin/$CMD_LLA -o $TMPDIR/$FNAME.lnkd.ll $TMPDIR/$FNAME.lnkd.bc"
         if [ "$OUTDIR" != "$TMPDIR" ] ; then
            runcmd "cp $TMPDIR/$FNAME.lnkd.ll $OUTDIR/$FNAME.lnkd.ll"
         fi
      fi 

      if [ $LLVMOPT != 0 ] ; then 
         [ $VERBOSE ] && echo "#Step:  Optimize(opt)	lnkd.bc --> opt.bc -O$LLVMOPT ..."
         runcmd "$AMDLLVM/bin/$CMD_OPT -o $TMPDIR/$FNAME.opt.bc $TMPDIR/$FNAME.lnkd.bc"

         if [ $GENLL ] ; then
            [ $VERBOSE ] && echo "#Step:  Disassemble	opt.bc --> opt.ll ..."
            runcmd "$AMDLLVM/bin/$CMD_LLA -o $TMPDIR/$FNAME.opt.ll $TMPDIR/$FNAME.opt.bc"
            if [ "$OUTDIR" != "$TMPDIR" ] ; then
               runcmd "cp $TMPDIR/$FNAME.opt.ll $OUTDIR/$FNAME.opt.ll"
            fi 
         fi 
         LLC_BC="opt"
      else
         # No optimization so generate object for lnkd bc.
         LLC_BC="lnkd"
      fi 

      [ $VERBOSE ] && echo "#Step:  llc mcpu=$LC_MCPU	$LLC_BC.bc --> amdgcn ..."
      runcmd "$AMDLLVM/bin/$CMD_LLC -o $TMPDIR/$FNAME.gcn $TMPDIR/$FNAME.$LLC_BC.bc"

      [ $VERBOSE ] && echo "#Step:	ld.lld		gcn --> hsaco ..."
      if [ $NOSHARED ] ; then 
           SHAREDARG=""
      else 
           SHAREDARG="-shared"
      fi
      #  FIXME:  Why does shared sometimes cause the -fPIC problem ?
      runcmd "$AMDLLVM/bin/ld.lld $TMPDIR/$FNAME.gcn --no-undefined $SHAREDARG -o $OUTDIR/$OUTFILE"
 

   fi # end of if quickpath then ... else  ...

   if [ $GENASM ] ; then
      [ $VERBOSE ] && echo "#Step:  llvm-objdump 	hsaco --> .s ..."
      textstarthex=`readelf -S -W  $OUTDIR/$OUTFILE | grep .text | awk '{print $6}'`
      textstart=$((0x$textstarthex))
      textszhex=`readelf -S -W $OUTDIR/$OUTFILE | grep .text | awk '{print $7}'`
      textsz=$((0x$textszhex))
      countclause=" count=$textsz skip=$textstart"
      dd if=$OUTDIR/$OUTFILE of=$OUTDIR/$FNAME.raw bs=1 $countclause 2>/dev/null
      hexdump -v -e '/1 "0x%02X "' $OUTDIR/$FNAME.raw | $AMDLLVM/bin/llvm-mc -arch=amdgcn -mcpu=$LC_MCPU -disassemble >$OUTDIR/$FNAME.s 2>$OUTDIR/$FNAME.s.err
      rm $OUTDIR/$FNAME.raw
      if [ "$LC_MCPU" == "kaveri" ] ; then 
         echo "WARNING:  Disassembly not supported for Kaveri. See $FNAME.s.err"
      else
         rm $OUTDIR/$FNAME.s.err
         echo "#INFO File $OUTDIR/$FNAME.s contains amdgcn assembly"
      fi
   fi

else 

   # -hsail or -brig specified.   This is the OLD code path with HLC

   LLVMOPT=${LLVMOPT:-2} 
   CMD_HLC_CLC=${CMD_HLC_CLC:-clc2 -cl-std=CL2.0 $CLOPTS $INCLUDES}
   CMD_HLC_LLA=${CMD_HLC_LLA:-llvm-dis}
   HLC_LKOPTS=${HLC_LKOPTS:--prelink-opt -l $HLC_PATH/../lib/builtins-hsail.bc -l $HLC_PATH/../lib/builtins-gcn.bc  -l $HLC_PATH/../lib/builtins-hsail-amd-ci.bc}
   CMD_HLC_LLL=${CMD_HLC_LLL:-llvm-link $HLC_LKOPTS}
   CMD_HLC_OPT=${CMD_HLC_OPT:-opt -O$LLVMOPT -gpu -whole}
   CMD_HLC_LLC=${CMD_HLC_LLC:-llc -O$LLVMOPT -march=hsail-64 -filetype=asm}
   CMD_HLC_BRI=${CMD_HLC_BRI:-HSAILasm}
   CMD_HLC_ASM=${CMD_HLC_ASM:-HSAILasm -disassemble}
   if [ $GEN_DEBUG ]  ; then 
      export LIBHSAIL_OPTIONS_APPEND="-g -include-source"
   fi

   [ $VERBOSE ] && echo "#Step:  Compile(clc2)	cl --> bc ..."
   runcmd "$HLC_PATH/$CMD_HLC_CLC -o $TMPDIR/$FNAME.bc $INDIR/$FILENAME"

   [ $VERBOSE ] && echo "#Step:  Link(llvm-link)	bc --> lnkd.bc ..."
   runcmd "$HLC_PATH/$CMD_HLC_LLL -o $TMPDIR/$FNAME.lnkd.bc $TMPDIR/$FNAME.bc "

   [ $VERBOSE ] && echo "#Step:  Optimize(opt)	lnkd.bc --> opt.bc -O$LLVMOPT ..."
   runcmd "$HLC_PATH/$CMD_HLC_OPT -o $TMPDIR/$FNAME.opt.bc $TMPDIR/$FNAME.lnkd.bc"

   if [ $GENLL ] ; then
     [ $VERBOSE ] && echo "#Step:  Disassemble	opt.bc --> opt.ll ..."
     runcmd "$HLC_PATH/$CMD_LLA -o $TMPDIR/$FNAME.ll $TMPDIR/$FNAME.bc"
     if [ "$OUTDIR" != "$TMPDIR" ] ; then
        runcmd "cp $TMPDIR/$FNAME.opt.ll $OUTDIR/$FNAME.opt.ll"
     fi
   fi

   [ $VERBOSE ] && echo "#Step:  llc arch=hsail	opt.bc --> hsail ..."
   runcmd "$HLC_PATH/$CMD_HLC_LLC -o $TMPDIR/$FNAME.hsail $TMPDIR/$FNAME.opt.bc"

   if [ "$HSAILLIB" != "" ] ; then 
   # An HSAILLIB hsail_lib.hsail requires a corresponding hsail_lib.h that 
   # the programmer included in his .cl.  This header resulted in the generation of 
   # "decl prog functions" at the top of his hsail for every function in his header functions. 
   # We must remove these and add the HSAILLIB in its place. 
   
   [ $VERBOSE ] && echo "#Step:  HSAILLIB 	Add hsaillib to hsail"
   if [ $DRYRUN ] ; then
      echo "DRYRUN commands for adding HSAILLIB not available "
   else
      # build a sedstring with names of functions in HSAILLIB
      sedstringfile=$TMPDIR/sedstring$$
      grep "decl function" $HSAILLIB | while read line  ; do 
         name=`echo $line | cut -d" " -f3 | cut -d"(" -f1`
         echo -n "${sep}^decl\sprog\sfunction\s${name}("
      sep="\|"
      done >$sedstringfile
      sedstring=`cat $sedstringfile`
      rm $sedstringfile
      # Rip out "decl prog function" prototypes from original hsail
      noprogfile=$TMPDIR/noproghsail.hsail
      finalfile=$TMPDIR/finalhsail.hsail
      cmd="/bin/sed -e ""/${sedstring}/,/;/d"" "
      $cmd $TMPDIR/$FNAME.hsail >$noprogfile
      # Piece together final hsail with header, HSAILLIB, and remaining hsail
      #  Keep only up to extension "IMAGE";
      cmd="sed -n ""1,/extension\s\"IMAGE\";/p"" "
      $cmd $noprogfile >$finalfile
      echo "//  START of HSAILLIB $HSAILLIB" >>$finalfile
      cat $HSAILLIB >>$finalfile
      echo "//  END of HSAILLIB $HSAILLIB" >>$finalfile
      echo "decl prog function &abort()();" >>$finalfile
      #  Keep everything after "prog kernel" 
      cmd="sed -n ""/^prog\skernel/,\$p "" "
      $cmd $noprogfile >>$finalfile
      cp $finalfile $TMPDIR/$FNAME.hsail
      rm $finalfile
      rm $noprogfile
   fi
   fi

   [ $VERBOSE ] && echo "#Step:  HSAILasm	hsail --> $OUTFILE -O$LLVMOPT ..."   
   runcmd "$HLC_PATH/$CMD_HLC_BRI -o $OUTDIR/$OUTFILE $TMPDIR/$FNAME.hsail"

   if [ $GEN_IL ] ; then 
      [ $VERBOSE ] && echo "#Step:  HSAILasm   	brig --> $FNAME.hsail ..."
      runcmd "$HLC_PATH/$CMD_HLC_ASM -o $OUTDIR/$FNAME.hsail $OUTDIR/$OUTFILE"
   fi

fi

# cleanup
do_err 0
exit 0
