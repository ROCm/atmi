CLOC - Version 0.6.2
====================

CLOC: Convert an CL (Kernel c Language) file to brig, hsail, or object

Table of contents
-----------------

- [Copyright and Disclaimer](#Copyright)
- [License](#License)
- [Help Text](#Help)
- [Install](INSTALL.md)

<A NAME="Copyright">
Copyright and Disclaimer
------------------------

Copyright 2014 ADVANCED MICRO DEVICES, INC.  

AMD is granting you permission to use this software (the Materials) pursuant to the 
terms and conditions of the Software License Agreement included with the Materials.  
If you do not have a copy of the Software License Agreement, contact your AMD 
representative for a copy.

You agree that you will not reverse engineer or decompile the Materials, in whole or 
in part, except as allowed by applicable law.

WARRANTY DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
KIND.  AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT 
LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
PURPOSE, TITLE, NON-INFRINGEMENT, THAT THE SOFTWARE WILL RUN UNINTERRUPTED OR ERROR-
FREE OR WARRANTIES ARISING FROM CUSTOM OF TRADE OR COURSE OF USAGE.  THE ENTIRE RISK 
ASSOCIATED WITH THE USE OF THE SOFTWARE IS ASSUMED BY YOU.  Some jurisdictions do not 
allow the exclusion of implied warranties, so the above exclusion may not apply to You. 

LIMITATION OF LIABILITY AND INDEMNIFICATION:  AMD AND ITS LICENSORS WILL NOT, 
UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT, INCIDENTAL, 
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF THE SOFTWARE OR THIS 
AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGES.  In no event shall AMD's total liability to You for all damages, losses, and 
causes of action (whether in contract, tort (including negligence) or otherwise) 
exceed the amount of $100 USD.  You agree to defend, indemnify and hold harmless 
AMD and its licensors, and any of their directors, officers, employees, affiliates or 
agents from and against any and all loss, damage, liability and other expenses 
(including reasonable attorneys' fees), resulting from Your use of the Software or 
violation of the terms and conditions of this Agreement.  

U.S. GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with "RESTRICTED RIGHTS." 
Use, duplication, or disclosure by the Government is subject to the restrictions as set 
forth in FAR 52.227-14 and DFAR252.227-7013, et seq., or its successor.  Use of the 
Materials by the Government constitutes acknowledgement of AMD's proprietary rights in them.

EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as stated in the 
Software License Agreement.

<A NAME="License">
License
-------

```
/* Copyright 2014 HSA Foundation Inc.  All Rights Reserved.
 *
 * HSAF is granting you permission to use this software and documentation (if
 * any) (collectively, the "Materials") pursuant to the terms and conditions
 * of the Software License Agreement included with the Materials.  If you do
 * not have a copy of the Software License Agreement, contact the  HSA Foundation for a copy.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
 */
```

<A NAME="Help">
Help Text
--------- 

```
   cloc: Convert an CL (Kernel C Language) file to brig, hsail, or
         object file using the LLVM to HSAIL backend compiler.
         Header .h files will also be created with .o files. 

   Usage: cloc [ options ] filename.cl

   Options without values:
    -c      Create .o object file for SNACK 
    -str    Create .o file with string for brig or hsail. e.g. to use with okra
    -hsail  Generate dissassembled hsail from brig 
    -ll     Generate dissassembled ll from bc, for info only
    -v      Display version of cloc then exit
    -q      Run quietly, no messages 
    -n      Dryrun, do nothing, show commands that would execute
    -h      Print this help message
    -k      Keep temporary files
    -nq     Shortcut for -n -q, Show commands without messages. 
    -fort   Generate fortran function names for -c option

   Options with values:
    -clopts  <compiler opts>  Default="-cl-std=CL2.0"
    -lkopts  <linker opts>    Read cloc script for defaults
    -s       <symbolname>     Default=filename (only with -str option)
    -t       <tdir>           Default=/tmp/cloc$$, Temp dir for files
    -o       <outfilename>    Default=<filename>.<ft> ft=brig, hsail, or o
    -p1     <path>            Default=$HSA_LLVM_PATH or /usr/local/HSAIL_LLVM_Backend/bin
    -p2     <path>            Default=$HSA_LIBHSAIL_PATH or /usr/local/HSAIL-Tools/libHSAIL/build
    -p3     <path>            Default=$HSA_RUNTIME_PATH or /usr/local/HSA-Runtime-AMD

   Examples:
    cloc mykernel.cl              /* create mykernel.brig            */
    cloc -c mykernel.cl           /* create mykernel.o for SNACK     */
    cloc -str mykernel.cl         /* create mykernel.o for okra      */
    cloc -hsail mykernel.cl       /* create mykernel.hsail           */
    cloc -str -hsail mykernel.cl  /* create mykernel.o for okra      */
    cloc -t /tmp/foo mykernel.cl  /* will automatically set -k       */

   You may set environment variables HSA_LLVM_PATH, HSA_LIBHSAIL_PATH, 
   HSA_RUNTIME_PATH, CLOPTS, or LKOPTS instead of providing options 
   -p1, -p2, -p3, -clopts, or -lkopts respectively.  
   Command line options will take precedence over environment variables. 

   (C) Copyright 2014 ADVANCED MICRO DEVICES, INC.

```
