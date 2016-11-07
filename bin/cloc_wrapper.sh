#!/bin/bash
#MIT License
#
#Copyright © 2016 Advanced Micro Devices, Inc.
#
#Permission is hereby granted, free of charge, to any person obtaining a copy of
#this software and associated documentation files (the "Software"), to deal in
#the Software
#without restriction, including without limitation the rights to use, copy,
#modify, merge, publish, distribute, sublicense, and/or sell copies of the
#Software, and to permit
#persons to whom the Software is furnished to do so, subject to the following
#conditions:
#
#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR
#PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
#BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
#CONTRACT, TORT OR
#OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
#OR OTHER DEALINGS IN THE SOFTWARE.
#
#  Written by Ashwin Aji Ashwin.Aji@amd.com
#
PROGVERSION=0.3.0

function usage(){
/bin/cat 2>&1 <<"EOF" 

   Converts a CL file to the BRIG char array that can be later loaded in
   HSA modules.

   Usage: cloc_wrapper.sh [ options ] filename.cl

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
                             <sdir> is actual directory of cloc_wrapper.sh
    -cp       <path>         $HSA_LLVM_PATH or /usr/bin
    -rp       <HSA RT path>  Default=$HSA_RUNTIME_PATH or /opt/hsa
    -o        <outfilename>  Default=<file>_brig.h or <file>_hof.h
    -hsaillib <hsail file>   Add HSAIL library to kernel code 
    -clopts   <compiler opts>  Default="-cl-std=CL2.0"

   Examples:
    cloc_wrapper.sh my.cl              /* create my_brig.h                  */
    cloc_wrapper.sh -hof my.cl         /* create my_hof.h (finalized)       */
    cloc_wrapper.sh -hsaillib mylib.hsail my.cl  
                                   /* creates my_brig.h, a composition  *
                                    * of my.cl & mylib.hsail            */
    cloc_wrapper.sh -hof -hsaillib mylib.hsail my.cl  
                                   /* creates my_hof.h, a composition   *
                                    * of my.cl & mylib.hsail (finalized)*/
    cloc_wrapper.sh -t /tmp/foo my.cl  /* will automatically set -k         */

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
[ ! -L "$sdir/cloc_wrapper.sh" ] || sdir=$(getdname `readlink "$sdir/cloc_wrapper.sh"`)
ATMI_PATH=${ATMI_PATH:-$sdir/..}
HSA_LLVM_PATH=${HSA_LLVM_PATH:-/usr/bin}
HSA_HLC_BIN_PATH=/opt/rocm/hlc3.2/bin
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

[ $VERBOSE ] && echo "#Info:  Version:	cloc_wrapper.sh $PROGVERSION" 
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
        echo $ATMI_PATH/bin/hof -o $BRIGDIR/$FNAME.o -b $BRIGDIR/$BRIGNAME 
    else
        echo $ATMI_PATH/bin/hof -o $BRIGDIR/$FNAME.o -b $BRIGDIR/$BRIGNAME 
        $ATMI_PATH/bin/hof -o $BRIGDIR/$FNAME.o -b $BRIGDIR/$BRIGNAME 
        #LD_LIBRARY_PATH=$HSA_RUNTIME_PATH/lib:$LD_LIBRARY_PATH hof -o $BRIGDIR/$FNAME.o -b $BRIGDIR/$BRIGNAME 
        rc=$?
        if [ $rc != 0 ] ; then 
            echo "ERROR:  The hof command failed with return code $rc."
            exit $rc
        fi
        cp $BRIGDIR/$FNAME.o $FULLOUTFILE
    fi
else 
    cp $BRIGDIR/$BRIGNAME $FULLOUTFILE
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
