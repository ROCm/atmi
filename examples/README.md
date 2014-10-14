```
To build, clone the following githubs 

* git clone https://github.com/HSAFoundation/HSA-Runtime-AMD
* git clone https://github.com/HSAFoundation/HSA-Drivers-Linux-AMD
* git clone https://github.com/HSAFoundation/Okra-Interface-to-HSA-Device
* git clone https://github.com/HSAFoundation/HSAIL-HLC-Development
* git clone https://github.com/HSAFoundation/HSAIL-Tools and build HSAIL-Tools

Set the environment variables
* HSA_RUNTIME_PATH= Path to HSA-Runtime-AMD
* HSA_KMT_PATH= Path to HSA-Drivers-Linux-AMD/kfd-0.8/libhsakmt/
* HSA_OKRA_PATH= Path to Okra-Interface-to-HSA-Device/okra/
* HSA_LLVM_PATH= Path to HSAIL-HLC-Developement/bin
* HSA_LIBHSAIL_PATH= Path to HSAIL-Tool/libHSAIL/build_linux


Note- If HSAIL-HLC-Stable is used. The tests have to be compiled with CFLAGS=-DDUMMY_ARGS=1. Example make all -DDUMMY_ARGS=1

#For building HSA examples. 
cd hsa 
make all
make test

#For building OKRA examples
cd okra
make all
make test

You can also build and run individually each test case

----------

Copyright 2014 ADVANCED MICRO DEVICES, INC.  

AMD is granting you permission to use this software (the Materials) pursuant to the 
terms and conditions of the Software License Agreement included with the Materials.  
If you do not have a copy of the Software License Agreement, contact your AMD 
representative for a copy or refer to 

  http://github.com/HSAFoundation/CLOC/blob/master/LICENSE.TXT

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

```
