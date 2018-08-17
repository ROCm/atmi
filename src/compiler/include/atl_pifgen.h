/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
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
