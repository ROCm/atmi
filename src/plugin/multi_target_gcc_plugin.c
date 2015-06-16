#include <gcc-plugin.h>
#include <plugin-version.h>
#include <plugin.h>
#include <tree.h>
#include <intl.h>
#include <diagnostic.h>
#include <print-tree.h>
#include <tree-core.h>
#include <pretty-print.h>
#include <tree-pretty-print.h>
#include <dumpfile.h>
#include <stdio.h>
#include <string.h>
int plugin_is_GPL_compatible;

static const char *plugin_name = "snack";

static pretty_printer buffer;
static int initialized = 0;

/* Attribute handler callback */
static tree
handle_user_attribute (tree *node, tree name, tree args,
        int flags, bool *no_add_attrs)
{
    static int counter = 0;
    tree decl = *node;
    char f_genw_filename[1024];
    memset(f_genw_filename, 0, 1024);
    strcpy(f_genw_filename, DECL_SOURCE_FILE(decl));
    strcat(f_genw_filename, ".pif.def.c");
    FILE *f_genw = NULL;
    if(counter == 0) // first time write, then append
        f_genw = fopen(f_genw_filename, "w");
    else
        f_genw = fopen(f_genw_filename, "a");

    FILE *f_tmp = fopen("tmp.pif.def.c", "w");
    if (!initialized) {
        //pp_construct (&buffer, /* prefix */NULL, /* line-width */0);
        pp_needs_newline (&buffer) = true;
        initialized = 1;
    }
    buffer.buffer->stream = f_tmp;

    const char* fn_name = IDENTIFIER_POINTER(DECL_NAME(decl));
    printf("Fn Name: %s\n", fn_name); 

    // Print the arguments of the attribute
    printf("Fn Attribute: %s ", IDENTIFIER_POINTER( name ));
    for( tree itrArgument = args; itrArgument != NULL_TREE; itrArgument = TREE_CHAIN( itrArgument ) )
    {
        printf("%s, ", TREE_STRING_POINTER( TREE_VALUE ( itrArgument )));
    }
    printf("\n"); 
    tree fn_type = TREE_TYPE(decl);
    //print_generic_stmt(f_genw, fn_type, TDF_RAW);
    dump_generic_node (&buffer, fn_type, 0, TDF_RAW, true);
    char *text = (char *) pp_formatted_text (&buffer);
    printf("After dump: %s\n", text);
    pp_flush (&buffer);
    
    char fn_decl[2048]; 
    char *pch = strtok(text,"<");
    if(pch != NULL) strcpy(fn_decl, pch);
    pch = strtok (NULL, ">");
    strcat(fn_decl, fn_name);
    pch = strtok (NULL, "(");
    if(pch != NULL) strcat(fn_decl, pch);
    strcat(fn_decl, "(atmi_lparm_t *lparm, ");
    
    pch = strtok (NULL, ",)");
    int var_idx = 0;
    while(pch != NULL) {
        printf("Parsing this string now: %s\n", pch);
        strcat(fn_decl, pch);
        char var_decl[16] = {0}; 
        sprintf(var_decl, " var%d", var_idx++);
        strcat(fn_decl, var_decl);
        strcat(fn_decl, ",");
        pch = strtok (NULL, ",)");
    }
    fn_decl[strlen(fn_decl) - 1] = ')';
    pp_printf(&buffer, "%s { \n", fn_decl);
    pp_printf(&buffer, "/* Rest of the SNACK launcher code goes here */\n");
    pp_printf(&buffer, "}\n\n");

    printf("New Fn Decl: %s\n", fn_decl);
    /*
    while (pch != NULL) {
        printf ("%s\n",pch);
        pch = strtok (NULL, "<>");
    }*/

    text = (char *)pp_formatted_text(&buffer);
    fputs (text, f_genw);
    pp_clear_output_area(&buffer);

    //int idx = 0;
    //for(; idx < 32; idx++) {
    //   printf("%d ", idx);
    //   print_generic_stmt(stdout, fn_type, (1 << idx));
    //   print_generic_stmt(stdout, decl, (1 << idx));
    //}
    //tree arg;
    //function_args_iterator args_iter;
    //FOREACH_FUNCTION_ARGS(fn_type, arg, args_iter)
    //{
    //for(idx = 0; idx < 32; idx++) {
    // printf("%d ", idx);
    // print_generic_stmt(stdout, arg, (1 << idx));
    //}
    //debug_tree_chain(arg);
    //}
    fclose(f_genw);
    counter++;
    return NULL_TREE;
}

/* Attribute definition */
static struct attribute_spec cpu_attr =
{ "cpu", 0, 1, false,  false, false, handle_user_attribute, false };

static struct attribute_spec gpu_attr =
{ "gpu", 0, 1, false,  false, false, handle_user_attribute, false };

/* Plugin callback called during attribute registration.
 * Registered with register_callback (plugin_name, PLUGIN_ATTRIBUTES,
 * register_attributes, NULL)
 */
static void
register_attributes (void *event_data, void *data)
{
    warning (0, G_("Callback to register attributes"));
    register_attribute (&cpu_attr);
    register_attribute (&gpu_attr);
    printf("Done registering attributes\n");
}

int plugin_init(struct plugin_name_args *plugin_info,
        struct plugin_gcc_version *version) {
    if (!plugin_default_version_check (version, &gcc_version))
        return 1;

    printf("In plugin init function\n");

    register_callback (plugin_name, PLUGIN_ATTRIBUTES,
                  register_attributes, NULL);
    return 0;
}

