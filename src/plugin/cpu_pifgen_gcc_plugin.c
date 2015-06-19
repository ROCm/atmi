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

//#define DEBUG_SNK_PLUGIN
#ifdef DEBUG_SNK_PLUGIN
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

int plugin_is_GPL_compatible;

static const char *plugin_name = "snack";

static pretty_printer buffer;
static int initialized = 0;


void write_headers(FILE *fp, const char *context_name, const char *pif_name) {
    fprintf(fp, "\
#include \"snk.h\"\n\
#include \"atmi.h\"\n\
\n\
hsa_agent_t                      %s_CPU_Agent;\n\
hsa_region_t                     %s_CPU_KernargRegion;\n\
int                              %s_CPU_FC = 0; \n\n\
int                              %s_CPU_FK = 0; \n\n\
\n\
status_t %s_InitCPUContext(){ \n\
    return snk_init_cpu_context( \n\
      &%s_CPU_Agent,\n\
      &%s_CPU_KernargRegion\n\
    );\n\
} \n\n\
", context_name, 
context_name, 
context_name, 
pif_name,
   context_name, 
   context_name, 
   context_name);
}

void write_kernel_inits(FILE *fp, const char *fn_name, const char *context_name) {
    
    fprintf(fp, "\
extern status_t %s_cpu_init(const char *kernel_name, \n\
                            const int num_params, \n\
                            snk_generic_fp fn_ptr){ \n\
    if (%s_CPU_FC == 0 ) { \n\
        status_t status = %s_InitCPUContext();\n\
        if ( status  != STATUS_SUCCESS ) return;\n\
        %s_CPU_FC = 1;\n\
    }\n\
    return snk_init_cpu_kernel(kernel_name, num_params, fn_ptr);\n\
} /* end of init */\n\n\
", fn_name, context_name, context_name, context_name);
}

/* Attribute handler callback */
static tree
handle_user_attribute (tree *node, tree name, tree args,
        int flags, bool *no_add_attrs)
{
    return NULL_TREE;
}

void generate_task(char *text, const char *fn_name, const int num_params, char *fn_decl) {
    char *pch = strtok(text,"<");
    if(pch != NULL) strcpy(fn_decl, pch);
    pch = strtok (NULL, ">");
    strcat(fn_decl, fn_name);
    pch = strtok (NULL, "(");
    if(pch != NULL) strcat(fn_decl, pch);
    strcat(fn_decl, "(");
    
    int var_idx = 0;
    for(var_idx = 0; var_idx < num_params; var_idx++) {
        char var_decl[64] = {0}; 
        sprintf(var_decl, "uint64_t var%d", var_idx);
        strcat(fn_decl, var_decl);
        if(var_idx == num_params - 1) // last param must end with )
            strcat(fn_decl, ")");
        else
            strcat(fn_decl, ",");
    }
    strcat(fn_decl, ";");
}

void generate_task_pif(char *text, const char *fn_name, const int num_params, char *fn_decl) {
    char *pch = strtok(text,"<");
    if(pch != NULL) strcpy(fn_decl, pch);
    pch = strtok (NULL, ">");
    strcat(fn_decl, fn_name);
    strcat(fn_decl, "_pif");
    pch = strtok (NULL, "(");
    if(pch != NULL) strcat(fn_decl, pch);
    strcat(fn_decl, "(");
    
    int var_idx = 0;
    for(var_idx = 0; var_idx < num_params; var_idx++) {
        char var_decl[64] = {0}; 
        sprintf(var_decl, "uint64_t var%d", var_idx);
        strcat(fn_decl, var_decl);
        if(var_idx == num_params - 1) // last param must end with )
            strcat(fn_decl, ")");
        else
            strcat(fn_decl, ",");
    }
}

void generate_pif(char *text, const char *fn_name, const int num_params, char *fn_decl) {
    char *pch = strtok(text,"<");
    if(pch != NULL) strcpy(fn_decl, "atmi_task_t *");
    pch = strtok (NULL, ">");
    strcat(fn_decl, fn_name);
    pch = strtok (NULL, "(");
    if(pch != NULL) strcat(fn_decl, pch);
    strcat(fn_decl, "(atmi_lparm_t *lparm, ");
    
    int var_idx = 0;
    pch = strtok (NULL, ",)");
    DEBUG_PRINT("Parsing but ignoring this string now: %s\n", pch);
    for(var_idx = 1; var_idx < num_params; var_idx++) {
        pch = strtok (NULL, ",)");
        DEBUG_PRINT("Parsing this string now: %s\n", pch);
        strcat(fn_decl, pch);
        char var_decl[64] = {0}; 
        sprintf(var_decl, " var%d", var_idx);
        strcat(fn_decl, var_decl);
        if(var_idx == num_params - 1) // last param must end with )
            strcat(fn_decl, ")");
        else
            strcat(fn_decl, ",");
    }
}

static tree
handle_cpu_task_attribute (tree *node, tree name, tree args,
        int flags, bool *no_add_attrs)
{
    //if(strcmp(IDENTIFIER_POINTER(name), "atmi_cpu_task") != 0) {
    //    return NULL_TREE;
    //}
    static int counter = 0;
    tree decl = *node;
    char f_genw_filename[1024];
    memset(f_genw_filename, 0, 1024);
    strcpy(f_genw_filename, DECL_SOURCE_FILE(decl));
    strcat(f_genw_filename, ".pif.def.c");
    FILE *f_genw = NULL;

    char context_name[1024];
    memset(context_name, 0, 1024);
    strcpy(context_name, DECL_SOURCE_FILE(decl));
    char *pch_context = strchr(context_name, '.');
    while(pch_context != NULL) {
        *pch_context = '_';
        pch_context = strchr(pch_context + 1, '.');
    }
    DEBUG_PRINT("Context name: %s\n", context_name);
    //
    // Print the arguments of the attribute
    DEBUG_PRINT("Fn Attribute: %s\nFn Attribute Params: ", IDENTIFIER_POINTER( name ));
    char pif_name[1024]; 
    int attrib_id = 0;
    for( tree itrArgument = args; itrArgument != NULL_TREE; itrArgument = TREE_CHAIN( itrArgument ) )
    {
        if(attrib_id == 0) { 
            const char *devtype_str = TREE_STRING_POINTER(TREE_VALUE(itrArgument));
            if(strcmp(devtype_str, "CPU") == 0 || strcmp(devtype_str, "cpu") == 0) {
                int devtype = 0;
            }
        //DEBUG_PRINT("DevType: %lu\n", TREE_INT_CST_LOW(TREE_VALUE(itrArgument)));
        //DEBUG_PRINT("DevType: %lu\n", TREE_INT_CST_HIGH(TREE_VALUE(itrArgument)));
        }
        else if(attrib_id == 1) {
            strcpy(pif_name, TREE_STRING_POINTER (TREE_VALUE ( itrArgument )));
        }
        DEBUG_PRINT("%s ", TREE_STRING_POINTER( TREE_VALUE ( itrArgument )));
        attrib_id++;
    }
    DEBUG_PRINT("\n"); 

    if(counter == 0) { // first time write, then append
        f_genw = fopen(f_genw_filename, "w");
        write_headers(f_genw, context_name, pif_name);
    } else {
        f_genw = fopen(f_genw_filename, "a");
    }

    FILE *f_tmp = fopen("tmp.pif.def.c", "w");
    if (!initialized) {
        //pp_construct (&buffer, /* prefix */NULL, /* line-width */0);
        pp_needs_newline (&buffer) = true;
        initialized = 1;
    }
    buffer.buffer->stream = f_tmp;

    const char* fn_name = IDENTIFIER_POINTER(DECL_NAME(decl));
    DEBUG_PRINT("Fn Name: %s\n", fn_name); 

    tree fn_type = TREE_TYPE(decl);
    //print_generic_stmt(f_genw, fn_type, TDF_RAW);
    dump_generic_node (&buffer, fn_type, 0, TDF_RAW, true);
    char *text = (char *) pp_formatted_text (&buffer);
    DEBUG_PRINT("Plugin Fn dump: %s\n", text);

    int text_sz = strlen(text); 
    char text_dup[2048]; 
    strcpy(text_dup, text);
    pp_flush (&buffer);
    
    int num_commas = 0;
    char *commas = strpbrk(text, ",");
    while (commas != NULL) {
        num_commas++;
        commas = strpbrk (commas+1, ",");
    }
    int num_params = num_commas + 1;
    DEBUG_PRINT("Number of arguments = %d\n", num_params);

    char pif_decl[2048];
    char fn_decl[2048];
    char fn_pif_decl[2048];

    generate_pif(text, pif_name, num_params, pif_decl); 
    DEBUG_PRINT("PIF Decl: %s\n", pif_decl);
    
    generate_task(text_dup, fn_name, num_params, fn_decl); 
    DEBUG_PRINT("FN Decl: %s\n", fn_decl);
    
    generate_task_pif(text_dup, fn_name, num_params, fn_pif_decl); 
    DEBUG_PRINT("FN specific PIF Decl: %s\n", fn_pif_decl);
    
    pp_printf(&buffer, "int %s_CPU_FK;\n", fn_name);
    pp_printf(&buffer, "%s { \n", pif_decl);
    pp_printf(&buffer, "    /* launcher code (PIF definition) */\n");
    pp_printf(&buffer, "    printf(\"In PIF Def\\n\"); \n");
    pp_printf(&buffer, "\n\
    /* Kernel initialization has to be done before kernel arguments are set/inspected */ \n\
    extern %s\n\
    if (%s_CPU_FK == 0 ) { \n\
        status_t status = %s_cpu_init(\"%s\", %d, (snk_generic_fp)%s); \n\
        if ( status  != STATUS_SUCCESS ) return; \n\
        %s_CPU_FK = 1; \n\
    } \n\
    snk_kernel_args_t *cpu_kernel_arg_list = (snk_kernel_args_t *)malloc(sizeof(snk_kernel_args_t)); \n\
    ", fn_decl,
    fn_name,  
    fn_name, fn_name, num_params, fn_name, 
    fn_name); 
    pp_printf(&buffer, "cpu_kernel_arg_list->args[0] = (uint64_t)NULL; \
        ");
    int arg_idx;
    for(arg_idx = 1; arg_idx < num_params; arg_idx++) {
        pp_printf(&buffer, "\n\
    cpu_kernel_arg_list->args[%d] = (uint64_t)var%d; \
        ", arg_idx, arg_idx);
    }
    pp_printf(&buffer, "\n\
    return snk_cpu_kernel(lparm, \n\
        \"%s\", \n\
        cpu_kernel_arg_list); \n\
    ", 
    fn_name);
    pp_printf(&buffer, "\n\
}\n");
    // FIXME: Output the PIF only once after aggregating all 
    // function declarations? Need to think of additional params
    // for alternate kernel execution
    char *buf_text = (char *)pp_formatted_text(&buffer);
    write_kernel_inits(f_genw, fn_name, context_name);
    fputs (buf_text, f_genw);
    pp_clear_output_area(&buffer);

    //int idx = 0;
    // TODO: Think about the below and verify the better approach for 
    // parameter parsing. Ignore below if above code works.
    //for(; idx < 32; idx++) {
    //   DEBUG_PRINT("%d ", idx);
    //   print_generic_stmt(stdout, fn_type, (1 << idx));
    //   print_generic_stmt(stdout, decl, (1 << idx));
    //}
    //tree arg;
    //function_args_iterator args_iter;
    //FOREACH_FUNCTION_ARGS(fn_type, arg, args_iter)
    //{
    //for(idx = 0; idx < 32; idx++) {
    // DEBUG_PRINT("%d ", idx);
    // print_generic_stmt(stdout, arg, (1 << idx));
    //}
    //debug_tree_chain(arg);
    //}
    fclose(f_genw);
    counter++;
    fclose(f_tmp);
    int ret_del = remove("tmp.pif.def.c");
    if(ret_del != 0) fprintf(stderr, "Unable to delete temp file: tmp.pif.def.c\n");
    return NULL_TREE;
}

/* Attribute definition */
static struct attribute_spec devtype_attr =
{ "launch_info", 0, 2, false,  false, false, handle_cpu_task_attribute, false };
//{ "atmi_cpu_task", 0, 1, false,  false, false, handle_cpu_task_attribute, false };

static struct attribute_spec launcher_attr =
{ "launcher", 0, 2, false,  false, false, handle_user_attribute, false };

/* Plugin callback called during attribute registration.
 * Registered with register_callback (plugin_name, PLUGIN_ATTRIBUTES,
 * register_attributes, NULL)
 */
static void
register_attributes (void *event_data, void *data)
{
    warning (0, G_("Callback to register attributes"));
    register_attribute (&devtype_attr);
//    register_attribute (&launcher_attr);
    DEBUG_PRINT("Done registering attributes\n");
}/* Plugin callback called during attribute registration.
 * Registered with register_callback (plugin_name, PLUGIN_ATTRIBUTES,
 * register_attributes, NULL)
 */

static void
register_headers (void *event_data, void *data)
{
//    warning (0, G_("Callback to register header files"));
//    DEBUG_PRINT("Done registering header files: %s\n", (const char *)event_data);
}

int plugin_init(struct plugin_name_args *plugin_info,
        struct plugin_gcc_version *version) {
    if (!plugin_default_version_check (version, &gcc_version))
        return 1;

    DEBUG_PRINT("In plugin init function\n");

    register_callback (plugin_name, PLUGIN_ATTRIBUTES,
                  register_attributes, NULL);
    register_callback (plugin_name, PLUGIN_INCLUDE_FILE,
                  register_headers, NULL);
    return 0;
}

