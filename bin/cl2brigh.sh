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
#
PROGVERSION=0.1.1
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

   Converts a CL file to the BRIG char array that can be later loaded in
   HSA modules.

   Usage: cl2brigh.sh [ options ] filename.cl

   Options without values:
    -version  Display version of this tool and exit
    -v        Verbose messages
    -vv       Get additional verbose messages from cloc.sh
    -n        Dryrun, do nothing, show commands that would execute
    -h        Print this help message
    -k        Keep temporary files
    -hof      Use HSA Offline Finalizer to create machine ISA 

   Options with values:
    -opt      <LLVM opt>     Default=2, passed to cloc.sh to build HSAIL 
    -t        <tempdir>      Default=/tmp/atmi_$$, Temp dir for files
    -s        <symbolname>   Default=filename 
    -p        <path>         $ATMI_PATH or <sdir> if ATMI_PATH not set
                             <sdir> is actual directory of cl2brigh.sh
    -cp       <path>         $HSA_LLVM_PATH or /opt/amd/cloc/bin
    -rp       <HSA RT path>  Default=$HSA_RUNTIME_PATH or /opt/hsa
    -o        <outfilename>  Default=<file>_brig.h or <file>_hof.h
    -hsaillib <hsail file>   Add HSAIL library to kernel code 
    -clopts   <compiler opts>  Default="-cl-std=CL2.0"

   Examples:
    cl2brigh.sh my.cl              /* create my_brig.h                  */
    cl2brigh.sh -hof my.cl         /* create my_hof.h (finalized)       */
    cl2brigh.sh -hsaillib mylib.hsail my.cl  
                                   /* creates my_brig.h, a composition  *
                                    * of my.cl & mylib.hsail            */
    cl2brigh.sh -hof -hsaillib mylib.hsail my.cl  
                                   /* creates my_hof.h, a composition   *
                                    * of my.cl & mylib.hsail (finalized)*/
    cl2brigh.sh -t /tmp/foo my.cl  /* will automatically set -k         */

   You may set environment variables ATMI_PATH,HSA_LLVM_PATH, or HSA_RUNTIME_PATH, 
   instead of providing options -p, -cp, -rp.
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
      -k) 		KEEPTDIR=true;; 
      --keep) 		KEEPTDIR=true;; 
      -n) 		DRYRUN=true;; 
      -hof)      	HOF=true;;  
      -opt) 		LLVMOPT=$2; shift ;; 
      -s) 		SYMBOLNAME=$2; shift ;; 
      -o) 		OUTFILE=$2; shift ;; 
      -t) 		TMPDIR=$2; shift ;; 
      -hsaillib)        HSAILLIB=$2; shift ;; 
      -clopts) 		CLOPTS=$2; shift ;; 
      -p)               ATMI_PATH=$2; shift ;;
      -cp)              HSA_LLVM_PATH=$2; shift ;;
      -rp)              HSA_RUNTIME_PATH=$2; shift ;;
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
   echo "WARNING:  This script can only process one .cl file at a time."
   echo "          You can call the script multiple times to get multiple outputs."
   echo "          Argument $LASTARG will be processed. "
   echo "          These args are ignored: $@"
   echo " "
fi

sdir=$(getdname $0)
[ ! -L "$sdir/cl2brigh.sh" ] || sdir=$(getdname `readlink "$sdir/cl2brigh.sh"`)
ATMI_PATH=${ATMI_PATH:-$sdir/..}
HSA_LLVM_PATH=${HSA_LLVM_PATH:-/opt/amd/cloc/bin}
HSA_HLC_BIN_PATH=${HSA_LLVM_PATH}/../../hlc3.2/bin
#  Set Default values
LLVMOPT=${LLVMOPT:-2}
HSA_RUNTIME_PATH=${HSA_RUNTIME_PATH:-/opt/hsa}
CMD_BRI=${CMD_BRI:-HSAILasm }

RUNDATE=`date`

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

[ -z $HSAILLIB  ] && HSAILLIB=$ATMI_PATH/bin/builtins-hsail.hsail
if [ "$HSAILLIB" != "" ] ; then 
   if [ ! -f $HSAILLIB ] ; then 
      echo "ERROR:  The HSAIL file $HSAILLIB does not exist."
      exit $DEADRC
   fi
fi

# Parse LASTARG for directory, filename, and symbolname
INDIR=$(getdname $LASTARG)
CLOPTS=${CLOPTS:-cl-std=CL2.0 -I$INDIR -I$ATMI_PATH/include}
CLNAME=${LASTARG##*/}
# FNAME has the .cl extension removed, used for symbolname and intermediate filenames
FNAME=`echo "$CLNAME" | cut -d'.' -f1`
SYMBOLNAME=${SYMBOLNAME:-$FNAME}
BRIGOFILE="${SYMBOLNAME}.brig"
HOFOFILE="${SYMBOLNAME}.hof"
BRIGHFILE="${SYMBOLNAME}_brig.h"
HOFHFILE="${SYMBOLNAME}_hof.h"
OTHERCLOCFLAGS="-opt $LLVMOPT"

if [ -z $OUTFILE ] ; then 
#  Output file not specified so use input directory
   OUTDIR=$INDIR
#  Make up the output file name based on last step 
   if [ $HOF ]; then 
      OUTFILE=$HOFOFILE
   else
      OUTFILE=$BRIGOFILE
   fi
else 
#  Use the specified OUTFILE.  Bad idea for snack
   OUTDIR=$(getdname $OUTFILE)
   OUTFILE=${OUTFILE##*/}
fi 

if [ $CLOCVERBOSE ] ; then 
   VERBOSE=true
fi

if [ $HSAIL_OPT_STEP2 ] ; then 
   [ $VERBOSE ] && echo " " && echo "#WARN:  ***** Step 2 of manual HSAIL optimization process detected. ***** "
fi

TMPDIR=${TMPDIR:-/tmp/atmi_$$}
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
if [ ! -e $HSA_HLC_BIN_PATH/$CMD_BRI ] ; then 
   echo "ERROR:  Missing HSAILasm in $HSA_LLVM_PATH"
   echo "        Set env variable HSA_LLVM_PATH or use -p option"
   exit $DEADRC
fi 
if [ ! -d $OUTDIR ] && [ ! $DRYRUN ]  ; then 
   echo "ERROR:  The output directory $OUTDIR does not exist"
   exit $DEADRC
fi
FULLOUTFILE=$OUTDIR/$OUTFILE

[ $VERBOSE ] && echo "#Info:  Version:	cl2brigh.sh $PROGVERSION" 
[ $VERBOSE ] && echo "#Info:  Input Files:	"
[ $VERBOSE ] && echo "#           File:	       $INDIR/$CLNAME"
[ $VERBOSE ] && echo "#Info:  Output Files:"
if [ $HOF ] ; then 
   [ $VERBOSE ] && echo "#           HOF incl:	   $FULLOUTFILE"
else
   [ $VERBOSE ] && echo "#           BRIG incl:	   $FULLOUTFILE"
fi

[ $VERBOSE ] && echo "#Info:  Run date:	$RUNDATE" 
[ $VERBOSE ] && echo "#Info:  LLVM path:	$HSA_LLVM_PATH"
[ $MAKEOBJ ] && [ $VERBOSE ] && echo "#Info:  Runtime:	$HSA_RUNTIME_PATH"
[ $MAKEOBJ ] && [ $VERBOSE ] && echo "#Info:  ATMI Runtime:	$ATMI_PATH"
[ $KEEPTDIR ] && [ $VERBOSE ] && echo "#Info:  Temp dir:	$TMPDIR" 
rc=0

if [ $HSAIL_OPT_STEP2 ] ; then 
#  This is step 2 of manual HSAIL
   BRIGDIR=$TMPDIR
   BRIGNAME=$FNAME.brig
   [ $VERBOSE ] && echo "#Step:  		hsail --> brig  ..."
   if [ $DRYRUN ] ; then
      echo "$HSA_HLC_BIN_PATH/$CMD_BRI -o $BRIGDIR/$BRIGNAME $INDIR/$FNAME.hsail"
   else
      echo "$HSA_HLC_BIN_PATH/$CMD_BRI -o $BRIGDIR/$BRIGNAME $INDIR/$FNAME.hsail"
      $HSA_HLC_BIN_PATH/$CMD_BRI -o $BRIGDIR/$BRIGNAME $INDIR/$FNAME.hsail
      rc=$?
      if [ $rc != 0 ] ; then 
         echo "ERROR:  The following command failed with return code $rc."
         echo "        $HSA_HLC_BIN_PATH/$CMD_BRI -o $BRIGDIR/$BRIGNAME $INDIR/$FNAME.hsail"
         do_err $rc
      fi
   fi

else
  
#  Not step 2, do normal steps
[ $VERBOSE ] && echo "Step:   Copy:           $INDIR/$CLNAME --> $TMPDIR/updated.cl"
cp $INDIR/$CLNAME $TMPDIR/updated.cl

#  Call cloc to generate brig
   if [ $CLOCVERBOSE ] ; then 
      OTHERCLOCFLAGS="$OTHERCLOCFLAGS -v"
   fi
   [ $VERBOSE ] && echo "#Step:  cloc.sh		cl --> brig ..."
   if [ $DRYRUN ] ; then
      echo "$HSA_LLVM_PATH/cloc.sh -brig -t $TMPDIR -k -clopts ""-I$INDIR -I$ATMI_PATH/include"" $OTHERCLOCFLAGS $TMPDIR/updated.cl"
   else 
      [ $CLOCVERBOSE ] && echo " " && echo "#------ Start cloc.sh output ------"
      [ $CLOCVERBOSE ] && echo "$HSA_LLVM_PATH/cloc.sh -brig -t $TMPDIR -k -clopts "-I$INDIR -I$ATMI_PATH/include" $OTHERCLOCFLAGS $TMPDIR/updated.cl"
      $HSA_LLVM_PATH/cloc.sh -brig -t $TMPDIR -k -clopts "-I$INDIR -I$ATMI_PATH/include" $OTHERCLOCFLAGS $TMPDIR/updated.cl
      rc=$?
      [ $CLOCVERBOSE ] && echo "#------ End cloc.sh output ------" && echo " " 
      if [ $rc != 0 ] ; then 
         echo "ERROR:  cloc.sh failed with return code $rc.  Command was:"
         echo "        $HSA_LLVM_PATH/cloc.sh -brig -t $TMPDIR -k -clopts "-I$INDIR -I$ATMI_PATH/include" $OTHERCLOCFLAGS $TMPDIR/updated.cl"
         do_err $rc
      fi
   fi
   BRIGDIR=$TMPDIR
   BRIGNAME=updated.brig

fi

if [ "$HSAILLIB" != "" ] ; then 
   # disassemble brig $BRIGDIR/$BRIGNAME to composite.hsail
   [ $VERBOSE ] && echo "#Step:  Add HSAIL		brig --> hsail+hsaillib --> $BRIGHFILE ..."
   if [ $DRYRUN ] ; then
      echo $HSA_HLC_BIN_PATH/$CMD_BRI -disassemble -o $TMPDIR/composite.hsail $BRIGDIR/$BRIGNAME
   else
      $HSA_HLC_BIN_PATH/$CMD_BRI -disassemble -o $TMPDIR/composite.hsail $BRIGDIR/$BRIGNAME
   fi

   # Inject ATMI_CONTEXT
   sed -i -e "5i\
alloc(agent) global_u64 &ATMI_CONTEXT = 0;\n\
       " $TMPDIR/composite.hsail

   entry_lines=($(grep -n "@__OpenCL_" $TMPDIR/composite.hsail | grep -Eo '^[^:]+'))

   num_kernels=${#entry_lines[@]}
   offset=2;
   for ((i=0; i<${num_kernels}; i++))
   do
       entry_line=$((${entry_lines[$i]} + $offset))
       offset=$(($offset + 4))
       sed -i -e "${entry_line}i\
    //init ATMI_CONTEXT\n\
    ld_kernarg_align(8)_width(all)_u64  \$d0, [%__printf_buffer];\n\
    st_global_align(8)_u64 \$d0, [&ATMI_CONTEXT];\n\
           " $TMPDIR/composite.hsail 
   done
 
   # Add $HSAILLIB to file 
   if [ $DRYRUN ] ; then
      echo cat $HSAILLIB >> $TMPDIR/composite.hsail
   else
      cat $HSAILLIB >> $TMPDIR/composite.hsail
   fi

   # assemble complete hsail file to brig $BRIGDIR/$BRIGNAME
   if [ $DRYRUN ] ; then
      echo $HSA_HLC_BIN_PATH/$CMD_BRI -o $BRIGDIR/$BRIGNAME $TMPDIR/composite.hsail
      rc=0
   else
      $HSA_HLC_BIN_PATH/$CMD_BRI -o $BRIGDIR/$BRIGNAME $TMPDIR/composite.hsail
      rc=$?
   fi
   if [ $rc != 0 ] ; then 
      echo "ERROR:  HSAIL assembly of HSAILLIB failed with return code $rc. Command was:"
      echo "        $HSA_HLC_BIN_PATH/$CMD_BRI -o $BRIGDIR/$BRIGNAME $TMPDIR/composite.hsail"
      do_err $rc
   fi
fi

#   HSA Offline Finalizer 
if [ $HOF ] ; then 
    [ $VERBOSE ] && echo "#Step:  offline finalization of brig --> $OUTFILE ..."
    if [ $DRYRUN ] ; then
        echo $ATMI_PATH/bin/atmi_hof -o $BRIGDIR/$FNAME.o -b $BRIGDIR/$BRIGNAME 
    else
        echo $ATMI_PATH/bin/atmi_hof -o $BRIGDIR/$FNAME.o -b $BRIGDIR/$BRIGNAME 
        $ATMI_PATH/bin/atmi_hof -o $BRIGDIR/$FNAME.o -b $BRIGDIR/$BRIGNAME 
        #LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib:$LD_LIBRARY_PATH atmi_hof -o $BRIGDIR/$FNAME.o -b $BRIGDIR/$BRIGNAME 
        rc=$?
        if [ $rc != 0 ] ; then 
            echo "ERROR:  The hof command failed with return code $rc."
            exit $rc
        fi
        cp $BRIGDIR/$FNAME.o $FULLOUTFILE
        #echo "char ${SYMBOLNAME}_HSA_HofMem[] = {" > $FULLOUTFILE
        #hexdump -v -e '"0x" 1/1 "%02X" ","' $BRIGDIR/$FNAME.hof >> $FULLOUTFILE
        #rc=$?
        #if [ $rc != 0 ] ; then 
        #    echo "ERROR:  The hexdump command failed with return code $rc."
        #    exit $rc
        #fi
        #echo "};" >> $FULLOUTFILE
        #echo "size_t ${SYMBOLNAME}_HSA_HofMemSz = sizeof(${SYMBOLNAME}_HSA_HofMem);" >> $FULLOUTFILE
    fi
else 
    cp $BRIGDIR/$BRIGNAME $FULLOUTFILE
    #[ $VERBOSE ] && echo "#Step:  hexdump		brig --> $BRIGHFILE ..."
    #if [ $DRYRUN ] ; then
    #    echo "hexdump -v -e '""0x"" 1/1 ""%02X"" "",""' $BRIGDIR/$BRIGNAME "
    #else
    #    echo "char ${SYMBOLNAME}_HSA_BrigMem[] = {" > $FULLOUTFILE
    #    hexdump -v -e '"0x" 1/1 "%02X" ","' $BRIGDIR/$BRIGNAME >> $FULLOUTFILE
    #    rc=$?
    #    if [ $rc != 0 ] ; then 
    #        echo "ERROR:  The hexdump command failed with return code $rc."
    #        exit $rc
    #    fi
    #    echo "};" >> $FULLOUTFILE
    #    echo "size_t ${SYMBOLNAME}_HSA_BrigMemSz = sizeof(${SYMBOLNAME}_HSA_BrigMem);" >> $FULLOUTFILE
    #fi
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
