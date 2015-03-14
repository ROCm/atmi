#!/bin/bash
#
#  snack: Structured No API Compiled Kernels .  Snack is used to
#         generate host-callable functions that launch compiled 
#         GPU and CPU kernels without an API.  Snack generates the
#         wrapper source code for these host-callable functions
#         that embeds compiled kernels into the source. The generated 
#         source code uses the HSA API to launch these kernels 
#         with various synchrnous and asynchronous features.
#         
#         The generated functions are called a "snack" functions
#         An application calls snack functions with the programmer
#         defined name and argument list. An extra argument is 
#         added to the programmer defined argument list to specify
#         the launch parameters. Since the host application directly 
#         calls snack functions and the launch attributes are 
#         specified in a data structure, there is no host API required.
#
#         The snack command requires the cloc.sh tool to generate
#         HSAIL for GPU kernels. Snack is distributed with the 
#         snack github repository.  
#
#  Written by Greg Rodgers  Gregory.Rodgers@amd.com
#  Maintained by Shreyas Ramalingam Shreyas.Ramalingam@amd.com
#
PROGVERSION=0.8.0
#
# Copyright (c) 2015 ADVANCED MICRO DEVICES, INC.  Patent pending.
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

   snack: Generate snack functions from source code.
          Header .h files will be created with .o files. 

   Usage: snack [ options ] filename.cl

   Options without values:
    -c      Create .o object file for SNACK
    -str    Create .o file with string for brig or hsail. e.g. to use with okra
    -hsail  Generate dissassembled hsail from brig 
    -v      Display version of snack then exit
    -q      Run quietly, no messages 
    -n      Dryrun, do nothing, show commands that would execute
    -h      Print this help message
    -k      Keep temporary files
    -nq     Shortcut for -n -q, Show commands without messages. 
    -fort   Generate fortran function names for -c option
    -noglobs Do not generate global functions 

   Options with values:
    -clopts  <compiler opts>  Default="-cl-std=CL2.0"
    -lkopts  <linker opts>    Read snack script for defaults
    -s       <symbolname>     Default=filename (only with -str option)
    -t       <tdir>           Default=/tmp/cloc$$, Temp dir for files
    -o       <outfilename>    Default=<filename>.<ft> ft=snackwrap.c or o
    -p1     <path>            Default=$HSA_LLVM_PATH or /opt/amd/bin
    -p2     <path>            Default=$HSA_RUNTIME_PATH or /opt/hsa
    -opt    <cl opt>          Default=2
    -gccopt <gcc opt>         Default=2

   Examples:
    snack mykernel.cl              /* create mykernel.snackwrap.c     */
    snack -c mykernel.cl           /* gcc compile to mykernel.o       */
    snack -str mykernel.cl         /* create mykernel.o for okra      */
    snack -hsail mykernel.cl       /* create hsail and snackwrap.c    */
    snack -t /tmp/foo mykernel.cl  /* will automatically set -k       */

   You may set environment variables HSA_LLVM_PATH, HSA_RUNTIME_PATH, 
   CLOPTS, or LKOPTS instead of providing options -p1, -p2, -clopts, 
   or -lkopts respectively.  
   Command line options will take precedence over environment variables. 

   Copyright (c) 2015 ADVANCED MICRO DEVICES, INC.

EOF
   exit 1 
}

DEADRC=12

#  Utility Functions
function do_err(){
   if [ ! $KEEPTDIR ] ; then 
      rm -rf $TMPDIR
   fi
   exit $1
}
function version(){
   echo $PROGVERSION
   exit 0
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

#  --------  The main code starts here -----

#  Argument processing
while [ $# -gt 0 ] ; do 
   case "$1" in 
      -q)               QUIET=true;;
      --quiet)          QUIET=true;;
      -k) 		KEEPTDIR=true;; 
      --keep) 		KEEPTDIR=true;; 
      -n) 		DRYRUN=true;; 
      -nq) 		DRYRUN=true;QUIET=true;;
      -qn) 		DRYRUN=true;QUIET=true;; 
      -c) 		MAKEOBJ=true;;  
      -fort) 		FORTRAN=1;;  
      -noglobs)  	NOGLOBFUNS=1;;  
      -str) 		MAKESTR=true;; 
      -hsail) 		GEN_IL=true;; 
      -clopts) 		CLOPTS=$2; shift ;; 
      -opt) 		LLVMOPT=$2; shift ;; 
      -gccopt) 		GCCOPT=$2; shift ;; 
      -lkopts) 		LKOPTS=$2; shift ;; 
      -s) 		SYMBOLNAME=$2; shift ;; 
      -o) 		OUTFILE=$2; shift ;; 
      -t) 		TMPDIR=$2; shift ;; 
      -p1)              HSA_LLVM_PATH=$2; shift ;;
      -p2)              HSA_RUNTIME_PATH=$2; shift ;;
      -h) 		usage ;; 
      -help) 		usage ;; 
      --help) 		usage ;; 
      -v) 		version ;; 
      --version) 	version ;; 
      --) 		shift ; break;;
      -*) 		usage ;;
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
   echo "WARNING:  Snack can only process one .cl file at a time."
   echo "          You can call snack multiple times to get multiple outputs."
   echo "          Argument $LASTARG will be processed. "
   echo "          These args are ignored: $@"
   echo " "
fi

CLOCPATH=$(getdname $0)

if ! [ $QUIET ] ; then  VERBOSE=true ; fi

#  Set Default values
GCCOPT=${GCCOPT:-3}
LLVMOPT=${LLVMOPT:-2}
HSA_RUNTIME_PATH=${HSA_RUNTIME_PATH:-/opt/hsa}
HSA_LLVM_PATH=${HSA_LLVM_PATH:-/opt/amd/bin}
#  no default CLOPTS -cl-std=CL2.0 is a forced option to the clc2 command
CMD_CLC=${CMD_CLC:-clc2 -cl-std=CL2.0 $CLOPTS}
CMD_LLA=${CMD_LLA:-llvm-dis}
LKOPTS=${LKOPTS:--prelink-opt -l $HSA_LLVM_PATH/builtins-hsail.bc}
CMD_LLL=${CMD_LLL:-llvm-link $LKOPTS}
CMD_OPT=${CMD_OPT:-opt -O$LLVMOPT -gpu -whole}
CMD_LLC=${CMD_LLC:-llc -O$LLVMOPT -march=hsail-64 -filetype=obj}
CMD_ASM=${CMD_ASM:-hsailasm -disassemble}
CMD_BRI=${CMD_BRI:-hsailasm }


FORTRAN=${FORTRAN:-0};
NOGLOBFUNS=${NOGLOBFUNS:-0};

RUNDATE=`date`

#$HSA_LLVM_PATH/$CMD_LLC -version  | grep -q "Jul 21 2014"
#We assume that the HSAIL_HLC_Stable always generates dummy arguments
GENW_ADD_DUMMY=t
export GENW_ADD_DUMMY

filetype=${LASTARG##*\.}
if [ "$filetype" != "cl" ]  ; then 
   if [ "$filetype" == "hsail" ]  ; then 
      HSAIL_OPT_STEP2=true
   else
      echo "ERROR:  $0 requires one argument with file type cl or hsail "
      exit $DEADRC 
   fi
fi

if [ ! -e "$LASTARG" ]  ; then 
   echo "ERROR:  The file $LASTARG does not exist."
   exit $DEADRC
fi
if [ ! -d $HSA_LLVM_PATH ] ; then 
   echo "ERROR:  Missing directory $HSA_LLVM_PATH "
   echo "        Set env variable HSA_LLVM_PATH or use -p1 option"
   exit $DEADRC
fi
#  We need RUNTIME with -c option
if [ ! -d $HSA_RUNTIME_PATH ] ; then 
   echo "ERROR:  Snack needs HSA_RUNTIME_PATH"
   echo "        Missing directory $HSA_RUNTIME_PATH "
   echo "        Set env variable HSA_RUNTIME_PATH or use -p2 option"
   exit $DEADRC
fi
if [ ! -f $HSA_RUNTIME_PATH/include/hsa.h ] ; then 
   echo "ERROR:  Missing $HSA_RUNTIME_PATH/include/hsa.h"
   echo "        The -c option requires HSA includes"
   exit $DEADRC
fi

# Parse LASTARG for directory, filename, and symbolname
INDIR=$(getdname $LASTARG)
CLNAME=${LASTARG##*/}
# FNAME has the .cl extension removed, used for symbolname and intermediate filenames
FNAME=`echo "$CLNAME" | cut -d'.' -f1`
SYMBOLNAME=${SYMBOLNAME:-$FNAME}

if [ -z $OUTFILE ] ; then 
#  Output file not specified so use input directory
   OUTDIR=$INDIR
#  Make up the output file name based on last step 
   if [ $MAKESTR ] || [ $MAKEOBJ ] ; then 
      OUTFILE=${FNAME}.o
   else
#     Output is snackwrap.c
      OUTFILE=${FNAME}.snackwrap.c
   fi
else 
#  Use the specified OUTFILE.  Bad idea for snack
   OUTDIR=$(getdname $OUTFILE)
   OUTFILE=${OUTFILE##*/}
fi 

if [ $GEN_IL ] ; then
   HSAIL_OPT_STEP1=true
#   -hsail specified.  This should be step 1 of HSAIL optimization process 
   if [ $HSAIL_OPT_STEP2 ] ; then 
      echo "ERROR:  Step 2 of manual HSAIL optimization process for file: $FNAME.hsail "
      echo "        For step 2, do not use the -hsail option."
      echo "        -hsail is used for step 1 to create hsail."
      exit $DEADRC
   fi
fi

if [ $HSAIL_OPT_STEP2 ] ; then 
   if [ ! -f $INDIR/$FNAME.snackwrap.c ] ; then 
      echo "        "
      echo "ERROR:  Step 2 of manual HSAIL optimization process requires: $INDIR/$FNAME.snackwrap.c"
      echo "        Complete step 1 by building files with \"snack -c -hsail $FNAME.cl \" command "
      echo "        "
      exit $DEADRC
   fi
fi

TMPDIR=${TMPDIR:-/tmp/snk_$$}
if [ -d $TMPDIR ] ; then 
   KEEPTDIR=true
else 
   if [ $DRYRUN ] ; then
      echo "mkdir -p $TMPDIR"
   else
      mkdir -p $TMPDIR
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
if [ ! -e $HSA_LLVM_PATH/hsailasm ] ; then 
   echo "ERROR:  Missing hsailasm in $HSA_LLVM_PATH"
   echo "        Set env variable HSA_LLVM_PATH or use -p1 option"
   exit $DEADRC
fi 
if [ ! -d $OUTDIR ] && [ ! $DRYRUN ]  ; then 
   echo "ERROR:  The output directory $OUTDIR does not exist"
   exit $DEADRC
fi 

# Snack only needs to compile if -c or -str specified
if [ $MAKESTR ] || [ $MAKEOBJ ] ; then 
   CMD_GCC=`which gcc`
   if [ -z "$CMD_GCC" ] ; then  
      echo "ERROR:  No gcc compiler found."
      exit $DEADRC
   fi
fi

[ $VERBOSE ] && echo "#Info:  Version:	$PROGVERSION" 
[ $VERBOSE ] && echo "#Info:  Input file:	$INDIR/$CLNAME"
if [ $HSAIL_OPT_STEP1 ] ; then 
   CWRAPFILE=$OUTDIR/$FNAME.snackwrap.c
   [ $VERBOSE ] && echo "#WARN:  ***** Step 1 of manual HSAIL optimization process detected. ***** "
   [ $VERBOSE ] && echo "#Info:  Output Files:"
   [ $VERBOSE ] && echo "#Info:     Object:	$OUTDIR/$OUTFILE"
   [ $VERBOSE ] && echo "#Info:     HSAIL:	$OUTDIR/$FNAME.hsail"
   [ $VERBOSE ] && echo "#Info:     Wrapper:	$CWRAPFILE"
   [ $VERBOSE ] && echo "#Info:     Headers:	$OUTDIR/$FNAME.h"
else
   if [ $MAKESTR ] || [ $MAKEOBJ ] ; then 
      CWRAPFILE=$TMPDIR/$FNAME.snackwrap.c
   else 
      CWRAPFILE=$OUTDIR/$FNAME.snackwrap.c
   fi
   [ $VERBOSE ] && echo "#Info:  Output file:	$OUTDIR/$OUTFILE"
   [ $VERBOSE ] && echo "#Info:  Output headers:	$OUTDIR/$FNAME.h"
fi
[ $VERBOSE ] && echo "#Info:  Run date:	$RUNDATE" 
[ $VERBOSE ] && echo "#Info:  LLVM path:	$HSA_LLVM_PATH"
[ $MAKEOBJ ] && [ $VERBOSE ] && echo "#Info:  Runtime:	$HSA_RUNTIME_PATH"
[ $KEEPTDIR ] && [ $VERBOSE ] && echo "#Info:  Temp dir:	$TMPDIR" 
if [ $MAKESTR ] || [ $MAKEOBJ ] ; then  
   [ $VERBOSE ] && echo "#Info:  gcc loc:	$CMD_GCC" 
fi
rc=0

if [ $HSAIL_OPT_STEP2 ] ; then 

   [ $VERBOSE ] && echo " " && echo "#WARN:  ***** Step 2 of manual HSAIL optimization process detected. ***** "
   BRIGDIR=$TMPDIR
   BRIGNAME=$FNAME.brig
   CWRAPFILE=$INDIR/$FNAME.snackwrap.c
   [ $VERBOSE ] && echo "#Step:  gcc		hsail --> brig  ..."
   if [ $DRYRUN ] ; then
      echo "$HSA_LLVM_PATH/$CMD_BRI -o $BRIGDIR/$BRIGNAME $INDIR/$FNAME.hsail"
   else
      $HSA_LLVM_PATH/$CMD_BRI -o $BRIGDIR/$BRIGNAME $INDIR/$FNAME.hsail
      rc=$?
      if [ $rc != 0 ] ; then 
         echo "ERROR:  The following command failed with return code $rc."
         echo "        $HSA_LLVM_PATH/$CMD_BRI -o $BRIGDIR/$BRIGNAME $INDIR/$FNAME.hsail"
         do_err $rc
      fi
   fi

else
  
#  This is not step 2 of manual HSAIL, so do all the normal steps

   [ $VERBOSE ] && echo "#Step:  genw  		cl --> $FNAME.snackwrap.c + $FNAME.h ..."
   if [ $DRYRUN ] ; then
      echo "$CLOCPATH/snk_genw.sh $SYMBOLNAME $INDIR/$CLNAME $PROGVERSION $TMPDIR $CWRAPFILE $OUTDIR/$FNAME.h $TMPDIR/updated.cl $FORTRAN $NOGLOBFUNS" 
   else
      $CLOCPATH/snk_genw.sh $SYMBOLNAME $INDIR/$CLNAME $PROGVERSION $TMPDIR $CWRAPFILE $OUTDIR/$FNAME.h $TMPDIR/updated.cl $FORTRAN $NOGLOBFUNS
      rc=$?
      if [ $rc != 0 ] ; then 
         echo "ERROR:  The following command failed with return code $rc."
         echo "        $CLOCPATH/snk_genw.sh $SYMBOLNAME $INDIR/$CLNAME $PROGVERSION $TMPDIR $CWRAPFILE $OUTDIR/$FNAME.h $TMPDIR/updated.cl $FORTRAN $NOGLOBFUNS"
         do_err $rc
      fi
   fi

#  Call cloc to generate brig
   [ $VERBOSE ] && echo "#Step:  cloc		Updated cl --> brig ..."
   $CLOCPATH/cloc.sh -t $TMPDIR -k -clopts "-I$INDIR" $TMPDIR/updated.cl

   BRIGDIR=$TMPDIR
   BRIGNAME=updated.brig

   if [ $GEN_IL ]  ; then 
      HSAILDIR=$OUTDIR
      HSAILNAME=$FNAME.hsail
   fi
 
if [ $MAKESTR ] ; then 
   if [ $GEN_IL ] ; then 
      [ $VERBOSE ] && echo "#Step:  gcc  	 	hsail --> $OUTFILE ..."
      if [ $DRYRUN ] ; then
         echo $CMD_GCC -O$GCCOPT -o $OUTDIR/$OUTFILE -c $TMPDIR/$FNAME.c
      else
#        A cool macro for creating big stings in c
         echo "#define MULTILINE(...) # __VA_ARGS__" > $TMPDIR/$FNAME.c
         echo "char ${SYMBOLNAME}[] = MULTILINE(" >> $TMPDIR/$FNAME.c
         cat $TMPDIR/$HSAILNAME >> $TMPDIR/$FNAME.c
         echo ");" >> $TMPDIR/$FNAME.c
         $CMD_GCC -O$GCCOPT -o $OUTDIR/$OUTFILE -c $TMPDIR/$FNAME.c
         rc=$?
         if [ $rc != 0 ] ; then 
            echo "ERROR:  The following command failed with return code $rc."
            echo "        $CMD_GCC -O$GCCOPT -o $OUTDIR/$OUTFILE.o -c $TMPDIR/$FNAME.c"
            do_err $rc
         fi
#        Make the header file, no sz needed for hsail
         echo "extern char ${SYMBOLNAME}[];" > $OUTDIR/$FNAME.h
      fi
   else
      [ $VERBOSE ] && echo "#Step:  gcc  	 	brig --> $OUTFILE ..."
      if [ $DRYRUN ] ; then
         echo $CMD_GCC -O$GCCOPT -o $OUTDIR/$OUTFILE -c $TMPDIR/$FNAME.c
      else
         echo "#include <stddef.h>" > $TMPDIR/$FNAME.c
         echo "char ${SYMBOLNAME}[] = {" >> $TMPDIR/$FNAME.c
         hexdump -v -e '"0x" 1/1 "%02X" ","' $TMPDIR/$BRIGNAME >> $TMPDIR/$FNAME.c
         rc=$?
         if [ $rc != 0 ] ; then 
            echo "ERROR:  The hexdump command failed with return code $rc."
            do_err $rc
         fi
         echo "};" >> $TMPDIR/$FNAME.c
#        okra needs the size of brig for createKernelFromBinary()
         echo "size_t ${SYMBOLNAME}sz = sizeof($SYMBOLNAME);" >> $TMPDIR/$FNAME.c
         $CMD_GCC -O$GCCOPT -o $OUTDIR/$OUTFILE -c $TMPDIR/$FNAME.c
         rc=$?
         if [ $rc != 0 ] ; then 
            echo "ERROR:  The following command failed with return code $rc."
            echo "        $CMD_GCC -O$GCCOPT -o $OUTDIR/$OUTFILE -c $TMPDIR/$FNAME.c"
            do_err $rc
         fi
#        Make the header file
         echo "extern char ${SYMBOLNAME}[];" > $OUTDIR/$FNAME.h
         echo "extern size_t ${SYMBOLNAME}sz;" >> $OUTDIR/$FNAME.h
      fi
   fi
fi

fi   

#  Add the brig to the c as local binary string and compile c
if [ $MAKEOBJ ] ; then 
   if [ $DRYRUN ] ; then
      [ $VERBOSE ] && echo "#Step:  gcc		c --> $OUTFILE  ..."
      echo "$CMD_GCC -O$GCCOPT -I$TMPDIR -I$INDIR -I$HSA_RUNTIME_PATH/include -o $OUTDIR/$OUTFILE -c $CWRAPFILE"
   else
      [ $VERBOSE ] && echo "#Step:  gcc		c --> $OUTFILE  ..."
      echo "char brigMem[] = {" > $TMPDIR/binarybrig.h
      hexdump -v -e '"0x" 1/1 "%02X" ","' $BRIGDIR/$BRIGNAME >> $TMPDIR/binarybrig.h
      rc=$?
      if [ $rc != 0 ] ; then 
         echo "ERROR:  The hexdump command failed with return code $rc."
         exit $rc
      fi
      echo "};" >> $TMPDIR/binarybrig.h
      echo "size_t brigMemSz = sizeof(brigMem);" >> $TMPDIR/binarybrig.h
      $CMD_GCC -O$GCCOPT -I$TMPDIR -I$INDIR -I$HSA_RUNTIME_PATH/include -o $OUTDIR/$OUTFILE -c $CWRAPFILE
      rc=$?
      if [ $rc != 0 ] ; then 
         echo "ERROR:  The following command failed with return code $rc."
         echo "        $CMD_GCC -O$GCCOPT -I$TMPDIR -I$INDIR -I$HSA_RUNTIME_PATH/include -o $OUTDIR/$OUTFILE -c $CWRAPFILE"
         do_err $rc
      fi
   fi
fi

# cleanup
if [ ! $KEEPTDIR ] ; then 
   if [ $DRYRUN ] ; then 
      echo "rm -rf $TMPDIR"
   else
      rm -rf $TMPDIR
   fi
fi

[ $HSAIL_OPT_STEP1 ] && [ $VERBOSE ] && echo " " &&  echo "#WARN:  ***** For Step 2, Make hsail updates then run \"snack -c $FNAME.hsail \" ***** "
[ $VERBOSE ] && echo "#Info:  Done"

exit 0
