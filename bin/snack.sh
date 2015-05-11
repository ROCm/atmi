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
PROGVERSION=0.9.1
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

   snack: Generate host-callable "snack" functions for GPU kernels.
          Snack generates the source code and headers for each kernel 
          in the input filename.cl file.  The -c option will compile 
          the source with gcc so you can link with your host application.
          Host applicaton requires no API to use snack functions.

   Usage: snack.sh [ options ] filename.cl

   Options without values:
    -c        Compile generated source code to create .o file
    -hsail    Generate text hsail for manual optimization
    -version  Display version of snack then exit
    -v        Verbose messages
    -vv       Get additional verbose messages from cloc.sh
    -n        Dryrun, do nothing, show commands that would execute
    -h        Print this help message
    -k        Keep temporary files
    -fort     Generate fortran function names
    -noglobs  Do not generate global functions 
    -str      Depricated, create .o file needed for okra

   Options with values:
    -opt      <LLVM opt>     Default=2, passed to cloc.sh to build HSAIL 
    -gccopt   <gcc opt>      Default=2, gcc optimization for snack wrapper
    -t        <tempdir>      Default=/tmp/snk_$$, Temp dir for files
    -s        <symbolname>   Default=filename 
    -p        <path>         $HSA_LLVM_PATH or <sdir> if HSA_LLVM_PATH not set
                             <sdir> is actual directory of snack.sh 
    -rp       <HSA RT path>  Default=$HSA_RUNTIME_PATH or /opt/hsa
    -snk      <SNACK RT path> Default=$SNACK_RUNTIME_PATH or /opt/amd/cloc
    -o        <outfilename>  Default=<filename>.<ft> 

   Examples:
    snack.sh my.cl              /* create my.snackwrap.c and my.h    */
    snack.sh -c my.cl           /* gcc compile to create  my.o       */
    snack.sh -hsail my.cl       /* create hsail and snackwrap.c      */
    snack.sh -c -hsail my.cl    /* create hsail snackwrap.c and .o   */
    snack.sh -t /tmp/foo my.cl  /* will automatically set -k         */

   You may set environment variables HSA_LLVM_PATH, HSA_RUNTIME_PATH, 
   instead of providing options -p, -rp.
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
      -n) 		DRYRUN=true;;
      -c) 		MAKEOBJ=true;;  
      -fort) 		FORTRAN=1;;  
      -noglobs)  	NOGLOBFUNS=1;;  
      -str) 		MAKESTR=true;; 
      -hsail) 		GEN_IL=true;; 
      -opt) 		LLVMOPT=$2; shift ;; 
      -gccopt) 		GCCOPT=$2; shift ;; 
      -s) 		SYMBOLNAME=$2; shift ;; 
      -o) 		OUTFILE=$2; shift ;; 
      -t) 		TMPDIR=$2; shift ;; 
      -p)               HSA_LLVM_PATH=$2; shift ;;
      -rp)              HSA_RUNTIME_PATH=$2; shift ;;
      -snk)             SNACK_RUNTIME_PATH=$2; shift ;;
      -h) 		usage ;; 
      -help) 		usage ;; 
      --help) 		usage ;; 
      -version) 	version ;; 
      --version) 	version ;; 
      -v) 		VERBOSE=true;; 
      -vv) 		CLOCVERBOSE=true;; 
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

sdir=$(getdname $0)
[ ! -L "$sdir/snack.sh" ] || sdir=$(getdname `readlink "$sdir/snack.sh"`)
HSA_LLVM_PATH=${HSA_LLVM_PATH:-$sdir}

#  Set Default values
GCCOPT=${GCCOPT:-3}
LLVMOPT=${LLVMOPT:-2}
HSA_RUNTIME_PATH=${HSA_RUNTIME_PATH:-/opt/hsa}
SNACK_RUNTIME_PATH=${SNACK_RUNTIME_PATH:-/opt/amd/cloc}
CMD_BRI=${CMD_BRI:-hsailasm }

FORTRAN=${FORTRAN:-0};
NOGLOBFUNS=${NOGLOBFUNS:-0};

RUNDATE=`date`

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
   echo "        Set env variable HSA_LLVM_PATH or use -p option"
   exit $DEADRC
fi
if [ $MAKEOBJ ] && [ ! -d "$HSA_RUNTIME_PATH/lib" ] && [ ! -d "$SNACK_RUNTIME_PATH/lib" ] ; then 
   echo "ERROR:  snack.sh -c option needs HSA_RUNTIME_PATH and SNACK_RUNTIME_PATH"
   echo "        Missing directory $HSA_RUNTIME_PATH/lib or $SNACK_RUNTIME_PATH/lib"
   echo "        Set env variable HSA_RUNTIME_PATH or use -rp option"
   echo "        Set env variable SNACK_RUNTIME_PATH or use -snk option"
   exit $DEADRC
fi
if [ $MAKEOBJ ] && [ ! -f $HSA_RUNTIME_PATH/include/hsa.h ] ; then 
   echo "ERROR:  Missing $HSA_RUNTIME_PATH/include/hsa.h"
   echo "        snack.sh requires HSA includes"
   exit $DEADRC
fi
if [ $MAKEOBJ ] && [ ! -f $SNACK_RUNTIME_PATH/include/snk.h ] ; then 
   echo "ERROR:  Missing $SNACK_RUNTIME_PATH/include/snk.h"
   echo "        snack.sh requires HSA includes"
   exit $DEADRC
fi

# Parse LASTARG for directory, filename, and symbolname
INDIR=$(getdname $LASTARG)
CLNAME=${LASTARG##*/}
# FNAME has the .cl extension removed, used for symbolname and intermediate filenames
FNAME=`echo "$CLNAME" | cut -d'.' -f1`
SYMBOLNAME=${SYMBOLNAME:-$FNAME}
BRIGHFILE="${SYMBOLNAME}_brig.h"
OTHERCLOCFLAGS="-opt $LLVMOPT"

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

if [ $CLOCVERBOSE ] ; then 
   VERBOSE=true
fi

if [ $GEN_IL ] ; then
#   -hsail specified.  This should be step 1 of HSAIL optimization process 
   if [ $HSAIL_OPT_STEP2 ] ; then 
      echo "ERROR:  Step 2 of manual HSAIL optimization process for file: $FNAME.hsail "
      echo "        For step 2, do not use the -hsail option."
      echo "        -hsail is used for step 1 to create hsail."
      exit $DEADRC
   fi
   OTHERCLOCFLAGS="$OTHERCLOCFLAGS -hsail"
fi

if [ $HSAIL_OPT_STEP2 ] ; then 
   if [ ! -f $INDIR/$FNAME.snackwrap.c ] ; then 
      echo "        "
      echo "ERROR:  Step 2 of manual HSAIL optimization process requires: $INDIR/$FNAME.snackwrap.c"
      echo "        Complete step 1 by building files with \"snack -c -hsail $FNAME.cl \" command "
      echo "        "
      exit $DEADRC
   fi
   [ $VERBOSE ] && echo " " && echo "#WARN:  ***** Step 2 of manual HSAIL optimization process detected. ***** "
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
   echo "        Set env variable HSA_LLVM_PATH or use -p option"
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

if [ $MAKESTR ] && [ $GEN_IL ] ; then 
   echo "ERROR:  The use of -hsail with -str is depricated "
   echo "        In the future , -str will be completely depricated"
   exit $DEADRC
fi 

[ $VERBOSE ] && echo "#Info:  Version:	snack.sh $PROGVERSION" 

if [ $HSAIL_OPT_STEP2 ] ; then 
   CWRAPFILE=$OUTDIR/$FNAME.snackwrap.c
   [ $VERBOSE ] && echo "#Info:  Input Files:	"
   [ $VERBOSE ] && echo "#           HSAIL file:	   $INDIR/$FNAME.hsail"
   [ $VERBOSE ] && echo "#           Wrapper:       $CWRAPFILE"
   [ $VERBOSE ] && echo "#           Headers:	   $OUTDIR/$FNAME.h"
   [ $VERBOSE ] && echo "#Info:  Output Files:"
   FULLBRIGHFILE=$OUTDIR/$BRIGHFILE
   [ $VERBOSE ] && echo "#           Brig incl:	   $FULLBRIGHFILE"
else
   [ $VERBOSE ] && echo "#Info:  Input File:	"
   [ $VERBOSE ] && echo "#           CL file:	   $INDIR/$CLNAME"
   [ $VERBOSE ] && echo "#Info:  Output Files:"
   if [ $GEN_IL ] ; then 
      [ $VERBOSE ] && echo "#WARN:  ***** Step 1 of manual HSAIL optimization process detected. ***** "
      CWRAPFILE=$OUTDIR/$FNAME.snackwrap.c
      FULLBRIGHFILE=$OUTDIR/$BRIGHFILE
      [ $VERBOSE ] && echo "#           Wrapper:       $CWRAPFILE"
      [ $VERBOSE ] && echo "#           Brig incl:	   $FULLBRIGHFILE"
      [ $VERBOSE ] && echo "#           HSAIL:	   $OUTDIR/$FNAME.hsail"
   else
      if [ $MAKESTR ] || [ $MAKEOBJ ] ; then 
         FULLBRIGHFILE=$TMPDIR/$BRIGHFILE
         CWRAPFILE=$TMPDIR/$FNAME.snackwrap.c
      else
         CWRAPFILE=$OUTDIR/$FNAME.snackwrap.c
         FULLBRIGHFILE=$OUTDIR/$BRIGHFILE
         [ $VERBOSE ] && echo "#           Wrapper:       $CWRAPFILE"
         [ $VERBOSE ] && echo "#           Brig incl:	   $FULLBRIGHFILE"
      fi
   fi
   if [ $MAKESTR ] || [ $MAKEOBJ ] ; then 
      [ $VERBOSE ] && echo "#           Object:	   $OUTDIR/$OUTFILE"
   fi
   [ $VERBOSE ] && echo "#           Headers:	   $OUTDIR/$FNAME.h"
fi

[ $VERBOSE ] && echo "#Info:  Run date:	$RUNDATE" 
[ $VERBOSE ] && echo "#Info:  LLVM path:	$HSA_LLVM_PATH"
[ $MAKEOBJ ] && [ $VERBOSE ] && echo "#Info:  Runtime:	$HSA_RUNTIME_PATH"
[ $MAKEOBJ ] && [ $VERBOSE ] && echo "#Info:  SNACK Runtime:	$SNACK_RUNTIME_PATH"
[ $KEEPTDIR ] && [ $VERBOSE ] && echo "#Info:  Temp dir:	$TMPDIR" 
if [ $MAKESTR ] || [ $MAKEOBJ ] ; then  
   [ $VERBOSE ] && echo "#Info:  gcc loc:	$CMD_GCC" 
fi
rc=0

if [ $HSAIL_OPT_STEP2 ] ; then 
#  This is step 2 of manual HSAIL
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
  
#  Not step 2, do normal steps
   [ $VERBOSE ] && echo "#Step:  genw  		cl --> $FNAME.snackwrap.c + $FNAME.h ..."
   if [ $DRYRUN ] ; then
      echo "$SNACK_RUNTIME_PATH/bin/snk_genw.sh $SYMBOLNAME $INDIR/$CLNAME $PROGVERSION $TMPDIR $CWRAPFILE $OUTDIR/$FNAME.h $TMPDIR/updated.cl $FORTRAN $NOGLOBFUNS" 
   else
      echo "$SNACK_RUNTIME_PATH/bin/snk_genw.sh $SYMBOLNAME $INDIR/$CLNAME $PROGVERSION $TMPDIR $CWRAPFILE $OUTDIR/$FNAME.h $TMPDIR/updated.cl $FORTRAN $NOGLOBFUNS" 
      $SNACK_RUNTIME_PATH/bin/snk_genw.sh $SYMBOLNAME $INDIR/$CLNAME $PROGVERSION $TMPDIR $CWRAPFILE $OUTDIR/$FNAME.h $TMPDIR/updated.cl $FORTRAN $NOGLOBFUNS
      rc=$?
      if [ $rc != 0 ] ; then 
         echo "ERROR:  The following command failed with return code $rc."
         echo "        $SNACK_RUNTIME_PATH/bin/snk_genw.sh $SYMBOLNAME $INDIR/$CLNAME $PROGVERSION $TMPDIR $CWRAPFILE $OUTDIR/$FNAME.h $TMPDIR/updated.cl $FORTRAN $NOGLOBFUNS"
         do_err $rc
      fi
   fi

#  Call cloc to generate brig
   if [ $CLOCVERBOSE ] ; then 
      OTHERCLOCFLAGS="$OTHERCLOCFLAGS -v"
   fi
   [ $VERBOSE ] && echo "#Step:  cloc.sh		cl --> brig ..."
   if [ $DRYRUN ] ; then
      echo "$HSA_LLVM_PATH/cloc.sh -t $TMPDIR -k -clopts ""-I$INDIR"" $OTHERCLOCFLAGS $TMPDIR/updated.cl"
   else 
      [ $CLOCVERBOSE ] && echo " " && echo "#------ Start cloc.sh output ------"
      [ $CLOCVERBOSE ] && echo "$HSA_LLVM_PATH/cloc.sh -t $TMPDIR -k -clopts "-I$INDIR" $OTHERCLOCFLAGS $TMPDIR/updated.cl"
      $HSA_LLVM_PATH/cloc.sh -t $TMPDIR -k -clopts "-I$INDIR" $OTHERCLOCFLAGS $TMPDIR/updated.cl
      rc=$?
      [ $CLOCVERBOSE ] && echo "#------ End cloc.sh output ------" && echo " " 
      if [ $rc != 0 ] ; then 
         echo "ERROR:  cloc.sh failed with return code $rc.  Command was:"
         echo "        $HSA_LLVM_PATH/cloc.sh -t $TMPDIR -k -clopts "-I$INDIR" $OTHERCLOCFLAGS $TMPDIR/updated.cl"
         do_err $rc
      fi
      if [ $GEN_IL ] ; then 
         cp $TMPDIR/updated.hsail $OUTDIR/$FNAME.hsail
      fi
   fi
   BRIGDIR=$TMPDIR
   BRIGNAME=updated.brig

fi

#  This section will be depricated with Okra and -str option
if [ $MAKESTR ] ; then 
      [ $VERBOSE ] && echo "#Step:  hexdump  	brig --> c char array ..."
      if [ $DRYRUN ] ; then
         echo "hexdump -v -e '""0x"" 1/1 ""%02X"" "",""' $TMPDIR/$BRIGNAME "
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
      fi
      [ $VERBOSE ] && echo "#Step:  gcc  	 	c char array --> $OUTFILE ..."
      if [ $DRYRUN ] ; then
         echo $CMD_GCC -O$GCCOPT -o $OUTDIR/$OUTFILE -c $TMPDIR/$FNAME.c
      else
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

else

#   Not depricated option -str 
[ $VERBOSE ] && echo "#Step:  hexdump		brig --> $BRIGHFILE ..."
if [ $DRYRUN ] ; then
   echo "hexdump -v -e '""0x"" 1/1 ""%02X"" "",""' $BRIGDIR/$BRIGNAME "
else
   echo "char ${SYMBOLNAME}_HSA_BrigMem[] = {" > $FULLBRIGHFILE
   hexdump -v -e '"0x" 1/1 "%02X" ","' $BRIGDIR/$BRIGNAME >> $FULLBRIGHFILE
   rc=$?
   if [ $rc != 0 ] ; then 
      echo "ERROR:  The hexdump command failed with return code $rc."
      exit $rc
   fi
   echo "};" >> $FULLBRIGHFILE
   echo "size_t ${SYMBOLNAME}_HSA_BrigMemSz = sizeof(${SYMBOLNAME}_HSA_BrigMem);" >> $FULLBRIGHFILE
fi


if [ $MAKEOBJ ] ; then 
   [ $VERBOSE ] && echo "#Step:  gcc		snackwrap.c + _brig.h --> $OUTFILE  ..."
   if [ $DRYRUN ] ; then
      echo "$CMD_GCC -O$GCCOPT -I$TMPDIR -I$INDIR -I$SNACK_RUNTIME_PATH/include -I$HSA_RUNTIME_PATH/include -o $OUTDIR/$OUTFILE -c $CWRAPFILE"
   else
      echo "$CMD_GCC -O$GCCOPT -I$TMPDIR -I$INDIR -I$SNACK_RUNTIME_PATH/include -I$HSA_RUNTIME_PATH/include -o $OUTDIR/$OUTFILE -c $CWRAPFILE"
      $CMD_GCC -O$GCCOPT -I$TMPDIR -I$INDIR -I$SNACK_RUNTIME_PATH/include -I$HSA_RUNTIME_PATH/include -o $OUTDIR/$OUTFILE -c $CWRAPFILE
      rc=$?
      if [ $rc != 0 ] ; then 
         echo "ERROR:  The following command failed with return code $rc."
         echo "        $CMD_GCC -O$GCCOPT -I$TMPDIR -I$INDIR -I$SNACK_RUNTIME_PATH/include -I$HSA_RUNTIME_PATH/include -o $OUTDIR/$OUTFILE -c $CWRAPFILE"
        do_err $rc
      fi
   fi
fi

# end of NOT -str
fi

# cleanup
if [ ! $KEEPTDIR ] ; then 
   if [ $DRYRUN ] ; then 
      echo "rm -rf $TMPDIR"
   else
      rm -rf $TMPDIR
   fi
fi

[ $GEN_IL ] && [ $VERBOSE ] && echo " " &&  echo "#WARN:  ***** For Step 2, Make hsail updates then run \"snack.sh -c $FNAME.hsail \" ***** "
[ $VERBOSE ] && echo "#Info:  Done"

exit 0
