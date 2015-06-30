#!/bin/bash
#
#  cloc.sh:  Convert cl file to brig, hsail, or .o  file
#            using the LLVM to HSAIL backend compiler.
#
#  Written by Greg Rodgers  Gregory.Rodgers@amd.com
#  Maintained by Shreyas Ramalingam Shreyas.Ramalingam@amd.com
#
PROGVERSION=0.9.1
#
# Copyright (c) 2014 ADVANCED MICRO DEVICES, INC.  
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

   cloc.sh: Convert a cl file to brig or hsail using the
            LLVM to HSAIL backend compiler.

   Usage: cloc.sh [ options ] filename.cl

   Options without values:
    -hsail    Generate dissassembled hsail from brig 
    -ll       Generate dissassembled ll from bc, for info only
    -version  Display version of cloc then exit
    -v        Verbose messages
    -n        Dryrun, do nothing, show commands that would execute
    -h        Print this help message
    -k        Keep temporary files

   Options with values:
    -t       <tdir>           Default=/tmp/cloc$$, Temp dir for files
    -o       <outfilename>    Default=<filename>.<ft> ft=brig or hsail
    -opt     <LLVM opt>       Default=2, LLVM optimization level
    -p       <path>           $HSA_LLVM_PATH or <cdir> if HSA_LLVM_PATH not set
                              <cdir> is actual directory of cloc.sh 
    -clopts  <compiler opts>  Default="-cl-std=CL2.0"
    -lkopts  <LLVM link opts> Default="-prelink-opt   \
              -l <cdir>/builtins-hsail.bc -l <cdir>/builtins-gcn.bc   \
              -l <cdir>/builtins-hsail-amd-ci.bc"

   Examples:
    cloc.sh my.cl               /* create my.brig                   */
    cloc.sh  -hsail my.cl       /* create my.hsail and my.brig      */

   You may set environment variables LLVMOPT, HSA_LLVM_PATH, CLOPTS, 
   or LKOPTS instead of providing options -opt -p, -clopts, or -lkopts .
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
      -hsail) 		GEN_IL=true;; 
      -ll) 		GENLL=true;KEEPTDIR=true;; 
      -clopts) 		CLOPTS=$2; shift ;; 
      -opt) 		LLVMOPT=$2; shift ;; 
      -lkopts) 		LKOPTS=$2; shift ;; 
      -o) 		OUTFILE=$2; shift ;; 
      -t) 		TMPDIR=$2; shift ;; 
      -p)               HSA_LLVM_PATH=$2; shift ;;
      -h) 		usage ;; 
      -help) 		usage ;; 
      --help) 		usage ;; 
      -version) 	version ;; 
      --version) 	version ;; 
      -v) 		VERBOSE=true;; 
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
   echo "WARNING:  cloc.sh can only process one .cl file at a time."
   echo "          You can call cloc multiple times to get multiple outputs."
   echo "          Argument $LASTARG will be processed. "
   echo "          These args are ignored: $@"
   echo " "
fi

# All binaries and builtins are expected to be in the same directory as cloc.sh
# unless HSA_LLVM_PATH is set. 
cdir=$(getdname $0)
[ ! -L "$cdir/cloc.sh" ] || cdir=$(getdname `readlink "$cdir/cloc.sh"`)
# If HSA_LLVM_PATH is set use it, else use cdir
HSA_LLVM_PATH=${HSA_LLVM_PATH:-$cdir}

#  Set Default values,  all CMD_ are started from $HSA_LLVM_PATH
LLVMOPT=${LLVMOPT:-2} 
#  no default CLOPTS -cl-std=CL2.0 is a forced option to the clc2 command
CMD_CLC=${CMD_CLC:-clc2 -cl-std=CL2.0 $CLOPTS}
CMD_LLA=${CMD_LLA:-llvm-dis}
LKOPTS=${LKOPTS:--prelink-opt -l $HSA_LLVM_PATH/builtins-hsail.bc -l $HSA_LLVM_PATH/builtins-gcn.bc  -l $HSA_LLVM_PATH/builtins-hsail-amd-ci.bc}
CMD_LLL=${CMD_LLL:-llvm-link $LKOPTS}
CMD_OPT=${CMD_OPT:-opt -O$LLVMOPT -gpu -whole}
CMD_LLC=${CMD_LLC:-llc -O$LLVMOPT -march=hsail-64 -filetype=obj}
CMD_ASM=${CMD_ASM:-hsailasm -disassemble}

RUNDATE=`date`

filetype=${LASTARG##*\.}
if [ "$filetype" != "cl" ]  ; then 
   echo "ERROR:  $0 requires one argument with file type cl"
   exit $DEADRC 
fi

if [ ! -e "$LASTARG" ]  ; then 
   echo "ERROR:  The file $LASTARG does not exist."
   exit $DEADRC
fi

# Parse LASTARG for directory, filename, and symbolname
INDIR=$(getdname $LASTARG)
CLNAME=${LASTARG##*/}
# FNAME has the .cl extension removed, used for naming intermediate filenames
FNAME=`echo "$CLNAME" | cut -d'.' -f1`

if [ -z $OUTFILE ] ; then 
#  Output file not specified so use input directory
   OUTDIR=$INDIR
#  Make up the output file name based on last step 
   OUTFILE=${FNAME}.brig
else 
#  Use the specified OUTFILE
   OUTDIR=$(getdname $OUTFILE)
   OUTFILE=${OUTFILE##*/}
   if [ ${OUTFILE##*.} == "h" ] ; then 
      OUTFILE="${OUTFILE%.*}.brig"
   fi
fi 

TMPDIR=${TMPDIR:-/tmp/cloc$$}
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
   echo "ERROR:  Missing binary hsailasm in $HSA_LLVM_PATH"
   exit $DEADRC
fi 
if [ ! -d $OUTDIR ] && [ ! $DRYRUN ]  ; then 
   echo "ERROR:  The output directory $OUTDIR does not exist"
   exit $DEADRC
fi 

if [ $GEN_IL ]  ; then 
   HSAILDIR=$OUTDIR
   HSAILNAME=$FNAME.hsail
fi 

[ $VERBOSE ] && echo "#Info:  Version:	cloc.sh $PROGVERSION" 
[ $VERBOSE ] && echo "#Info:  Input file:	$INDIR/$CLNAME"
[ $VERBOSE ] && echo "#Info:  Brig file:	$OUTDIR/$OUTFILE"
if [ $GEN_IL ] ; then 
   [ $VERBOSE ] && echo "#Info:  HSAIL file:	$HSAILDIR/$HSAILNAME"
fi
[ $VERBOSE ] && echo "#Info:  Run date:	$RUNDATE" 
[ $VERBOSE ] && echo "#Info:  LLVM path:	$HSA_LLVM_PATH"
[ $KEEPTDIR ] && [ $VERBOSE ] && echo "#Info:  Temp dir:	$TMPDIR" 
rc=0

[ $VERBOSE ] && echo "#Step:  Compile(clc2)	cl --> bc ..."
if [ $DRYRUN ] ; then
   echo $CMD_CLC -o $TMPDIR/$FNAME.bc $INDIR/$CLNAME
else
   $HSA_LLVM_PATH/$CMD_CLC -o $TMPDIR/$FNAME.bc $INDIR/$CLNAME
   rc=$?
   if [ $rc != 0 ] ; then 
      echo "ERROR:  The following command failed with return code $rc."
      echo "        $CMD_CLC -o $TMPDIR/$FNAME.bc $INDIR/$CLNAME"
      do_err $rc
   fi
fi

if [ $GENLL ] ; then
   [ $VERBOSE ] && echo "#Step:  Disassmble	bc --> ll ..."
   if [ $DRYRUN ] ; then
      echo $CMD_LLA -o $TMPDIR/$FNAME.ll $TMPDIR/$FNAME.bc
   else 
      $HSA_LLVM_PATH/$CMD_LLA -o $TMPDIR/$FNAME.ll $TMPDIR/$FNAME.bc
      rc=$?
   fi
   if [ $rc != 0 ] ; then 
      echo "ERROR:  The following command failed with return code $rc."
      echo "        $CMD_LLA -o $TMPDIR/$FNAME.ll $TMPDIR/$FNAME.bc"
      do_err $rc
   fi
fi

[ $VERBOSE ] && echo "#Step:  Link(llvm-link)	bc --> lnkd.bc ..."
if [ $DRYRUN ] ; then
   echo $CMD_LLL -o $TMPDIR/$FNAME.lnkd.bc $TMPDIR/$FNAME.bc  
else
   $HSA_LLVM_PATH/$CMD_LLL -o $TMPDIR/$FNAME.lnkd.bc $TMPDIR/$FNAME.bc 
   rc=$?
fi
if [ $rc != 0 ] ; then 
   echo "ERROR:  The following command failed with return code $rc."
   echo "        $HSA_LLVM_PATH/$CMD_LLL -o $TMPDIR/$FNAME.lnkd.bc $TMPDIR/$FNAME.bc"
   do_err $rc
fi

[ $VERBOSE ] && echo "#Step:  Optimize(opt)	lnkd.bc --> opt.bc -O$LLVMOPT ..."
if [ $DRYRUN ] ; then
   echo $CMD_OPT -o $TMPDIR/$FNAME.opt.bc $TMPDIR/$FNAME.lnkd.bc
else
   $HSA_LLVM_PATH/$CMD_OPT -o $TMPDIR/$FNAME.opt.bc $TMPDIR/$FNAME.lnkd.bc
   rc=$?
fi
if [ $rc != 0 ] ; then 
   echo "ERROR:  The following command failed with return code $rc."
   echo "        $CMD_OPT -o $TMPDIR/$FNAME.opt.bc $TMPDIR/$FNAME.lnkd.bc"
   do_err $rc
fi

 
[ $VERBOSE ] && echo "#Step:  llc arch=hsail	opt.bc --> $OUTFILE -O$LLVMOPT ..."
if [ $DRYRUN ] ; then
   echo $CMD_LLC -o $OUTDIR/$OUTFILE $TMPDIR/$FNAME.opt.bc
else
   $HSA_LLVM_PATH/$CMD_LLC -o $OUTDIR/$OUTFILE $TMPDIR/$FNAME.opt.bc
   rc=$?
fi
if [ $rc != 0 ] ; then 
   echo "ERROR:  The following command failed with return code $rc."
   echo "        $CMD_LLC -o $OUTDIR/$OUTFILE $TMPDIR/$FNAME.opt.bc"
   do_err $rc
fi

if [ $GEN_IL ] ; then 
   [ $VERBOSE ] && echo "#Step:  hsailasm   	brig --> $HSAILNAME ..."
   if [ $DRYRUN ] ; then
      echo $CMD_ASM -o $HSAILDIR/$HSAILNAME $OUTDIR/$OUTFILE
   else
      $HSA_LLVM_PATH/$CMD_ASM -o $HSAILDIR/$HSAILNAME $OUTDIR/$OUTFILE
      rc=$?
   fi
   if [ $rc != 0 ] ; then 
      echo "ERROR:  The following command failed with return code $rc."
      echo "        $CMD_ASM -o $HSAILDIR/$HSAILNAME $OUTDIR/$OUTFILE"
      do_err $rc
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

[ $VERBOSE ] && echo "#Info:  Done"

exit 0
