/*
MIT License 

Copyright © 2016 Advanced Micro Devices, Inc.  

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef __ATMI_PIFGEN_PLUGIN__
#define __ATMI_PIFGEN_PLUGIN__
struct cl_decoded_option
{
    /* The index of this option, or an OPT_SPECIAL_* value for
     *      non-options and unknown options.  */
    size_t opt_index;

    /* Any warning to give for use of this option, or NULL if none.  */
    const char *warn_message;

    /* The string argument, or NULL if none.  For OPT_SPECIAL_* cases,
     *      the option or non-option command-line argument.  */
    const char *arg;

    /* The original text of option plus arguments, with separate argv
     *      elements concatenated into one string with spaces separating
     *           them.  This is for such uses as diagnostics and
     *                -frecord-gcc-switches.  */
    const char *orig_option_with_args_text;

    /* The canonical form of the option and its argument, for when it is
     *      necessary to reconstruct argv elements (in particular, for
     *           processing specs and passing options to subprocesses from the
     *                driver).  */
    const char *canonical_option[4];

    /* The number of elements in the canonical form of the option and
     *      arguments; always at least 1.  */
    size_t canonical_option_num_elements;

    /* For a boolean option, 1 for the true case and 0 for the "no-"
     *      case.  For an unsigned integer option, the value of the
     *           argument.  1 in all other cases.  */
    int value;

    /* Any flags describing errors detected in this option.  */
    int errors;
};

/* Decoded options, and number of such options.  */
extern struct cl_decoded_option *save_decoded_options;
extern unsigned int save_decoded_options_count;

#endif // __ATMI_PIFGEN_PLUGIN__
