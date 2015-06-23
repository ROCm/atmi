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

#define DEBUG_SNK_PLUGIN
#ifdef DEBUG_SNK_PLUGIN
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

int plugin_is_GPL_compatible;

static const char *plugin_name = "snack";

static pretty_printer buffer;
static int initialized = 0;


void write_headers(FILE *fp, const char *filename) {
    fprintf(fp, "\
#include \"atmi.h\"\n\
#include \"snk.h\"\n\
\n\
#include \"%s.kerneldecls.h\"\n\
", filename);
}

void write_kerneldecls(FILE *fp) {
fprintf(fp, "#ifdef __cplusplus \n\
#define _CPPSTRING_ \"C\" \n\
#endif \n\
#ifndef __cplusplus \n\
#define _CPPSTRING_ \n\
#endif \n");
}

void generate_task(char *text, const char *fn_name, const int num_params, char *fn_decl) {
    char *pch = strtok(text,"<");
    if(pch != NULL) strcpy(fn_decl, pch);
    pch = strtok (NULL, ">");
    strcat(fn_decl, fn_name);
    pch = strtok (NULL, "(");
    if(pch != NULL) strcat(fn_decl, pch);
    strcat(fn_decl, "(atmi_task_t *var0, ");
    
    int var_idx = 0;
    pch = strtok (NULL, ",)");
    //DEBUG_PRINT("Parsing but ignoring this string now: %s\n", pch);
    for(var_idx = 1; var_idx < num_params; var_idx++) {
        pch = strtok (NULL, ",)");
    //    DEBUG_PRINT("Parsing this string now: %s\n", pch);
        strcat(fn_decl, pch);
        char var_decl[64] = {0}; 
        sprintf(var_decl, " var%d", var_idx);
        strcat(fn_decl, var_decl);
        if(var_idx == num_params - 1) // last param must end with )
            strcat(fn_decl, ")");
        else
            strcat(fn_decl, ",");
    }
    strcat(fn_decl, ";");
}

void generate_pif(char *text, const char *fn_name, const int num_params, char *fn_decl) {
    char *pch = strtok(text,"<");
    // return atmi_task_t *
    if(pch != NULL) strcpy(fn_decl, "atmi_task_t *");
    pch = strtok (NULL, ">");
    strcat(fn_decl, fn_name);
    pch = strtok (NULL, "(");
    if(pch != NULL) strcat(fn_decl, pch);
    strcat(fn_decl, "(atmi_lparm_t *lparm, ");
    
    int var_idx = 0;
    pch = strtok (NULL, ",)");
    //DEBUG_PRINT("Parsing but ignoring this string now: %s\n", pch);
    for(var_idx = 1; var_idx < num_params; var_idx++) {
        pch = strtok (NULL, ",)");
        //DEBUG_PRINT("Parsing this string now: %s\n", pch);
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

#define MAX_PIFS 100
char pif_call_table[MAX_PIFS][1024];
int get_pif_counter(const char *pif_name) {
    int i;
    int pif_counter = 0;
    for(i = 0; i < MAX_PIFS; i++) {
        if(pif_call_table[i] && strcmp(pif_name, pif_call_table[i]) == 0) {
            pif_counter++;
        }
    }
    return pif_counter;
}

void register_pif(const char *pif_name, int index) {
    strcpy(pif_call_table[index],pif_name);
}

static tree
handle_cpu_task_attribute (tree *node, tree name, tree args,
        int flags, bool *no_add_attrs)
{
    DEBUG_PRINT("Handling __attribute__ %s\n", IDENTIFIER_POINTER(name));
    //if(strcmp(IDENTIFIER_POINTER(name), "atmi_cpu_task") != 0) {
    //    return NULL_TREE;
    //}
    static int counter = 0;
    tree decl = *node;
    char pifdefs_filename[1024];
    memset(pifdefs_filename, 0, 1024);
    strcpy(pifdefs_filename, DECL_SOURCE_FILE(decl));
    strcat(pifdefs_filename, ".pifdefs.h");
    
    char kerneldecls_filename[1024];
    memset(kerneldecls_filename, 0, 1024);
    strcpy(kerneldecls_filename, DECL_SOURCE_FILE(decl));
    strcat(kerneldecls_filename, ".kerneldecls.h");

    FILE *fp_pifdefs_genw = NULL;
    FILE *fp_kerneldecls_genw = NULL;

    char this_filename[1024];
    memset(this_filename, 0, 1024);
    strcpy(this_filename, DECL_SOURCE_FILE(decl));
    DEBUG_PRINT("Translation Unit Filename: %s\n", this_filename);
    //
    // Print the arguments of the attribute
    DEBUG_PRINT("Task Attribute Params: ");
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

    char piftable_filename[1024];
    memset(piftable_filename, 0, 1024);
    strcpy(piftable_filename, pif_name);
    strcat(piftable_filename, ".piftable.h");
    FILE *fp_piftable_genw = NULL;

    if(counter == 0) { // first time write, then append
        fp_pifdefs_genw = fopen(pifdefs_filename, "w");
        fp_kerneldecls_genw = fopen(kerneldecls_filename, "w");
        write_headers(fp_pifdefs_genw, this_filename);
        write_kerneldecls(fp_kerneldecls_genw);
    } else {
        fp_pifdefs_genw = fopen(pifdefs_filename, "a");
        fp_kerneldecls_genw = fopen(kerneldecls_filename, "a");
    }

    int pif_counter = get_pif_counter(pif_name); 
    if(pif_counter == 0) {
        fp_piftable_genw = fopen(piftable_filename, "w");
    } else {
        fp_piftable_genw = fopen(piftable_filename, "a");
    }
    register_pif(pif_name, counter);
    
    FILE *f_tmp = fopen("tmp.pif.def.c", "w");
    if (!initialized) {
        //pp_construct (&buffer, /* prefix */NULL, /* line-width */0);
        pp_needs_newline (&buffer) = true;
        initialized = 1;
    }
    buffer.buffer->stream = f_tmp;

    const char* fn_name = IDENTIFIER_POINTER(DECL_NAME(decl));
    DEBUG_PRINT("Task Name: %s\n", fn_name); 

    tree fn_type = TREE_TYPE(decl);
    //print_generic_stmt(fp_pifdefs_genw, fn_type, TDF_RAW);
    dump_generic_node (&buffer, fn_type, 0, TDF_RAW, true);
    char *text = (char *) pp_formatted_text (&buffer);
    DEBUG_PRINT("Plugin Task dump: %s\n", text);

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

    generate_pif(text, pif_name, num_params, pif_decl); 
    DEBUG_PRINT("PIF Decl: %s\n", pif_decl);
    
    generate_task(text_dup, fn_name, num_params, fn_decl); 
    DEBUG_PRINT("Task Decl: %s\n", fn_decl);
   
    if(pif_counter == 0) { //first time PIF is called 
        pp_printf(&buffer, "snk_pif_kernel_table_t %s_pif_fn_table[] = {\n", pif_name);
        pp_printf(&buffer, "    #include \"%s.piftable.h\"\n", pif_name);
        pp_printf(&buffer, "};\n");

        pp_printf(&buffer, "int %s_FK;\n", pif_name);
        pp_printf(&buffer, "%s { \n", pif_decl);
        pp_printf(&buffer, "    /* launcher code (PIF definition) */\n");
        pp_printf(&buffer, "\
    if (%s_FK == 0 ) { \n\
        snk_pif_init(%s_pif_fn_table, sizeof(%s_pif_fn_table)/sizeof(%s_pif_fn_table[0]));\n\
        %s_FK = 1; \n\
    } \n\
    snk_kernel_args_t *cpu_kernel_arg_list = (snk_kernel_args_t *)malloc(sizeof(snk_kernel_args_t)); \n", 
                pif_name,
                pif_name, pif_name, pif_name,
                pif_name);
        pp_printf(&buffer, "    cpu_kernel_arg_list->args[0] = (uint64_t)NULL; \
                ");
        int arg_idx;
        for(arg_idx = 1; arg_idx < num_params; arg_idx++) {
            pp_printf(&buffer, "\n\
    cpu_kernel_arg_list->args[%d] = (uint64_t)var%d;", arg_idx, arg_idx);
        }
        pp_printf(&buffer, "\n\
    return snk_cpu_kernel(lparm, \n\
                    \"%s\", \n\
                    cpu_kernel_arg_list); \n\
                ", 
                pif_name);
        pp_printf(&buffer, "\n\
}\n");
        // FIXME: Output the PIF only once after aggregating all 
        // function declarations? Need to think of additional params
        // for alternate kernel execution
        char *buf_text = (char *)pp_formatted_text(&buffer);
        fputs (buf_text, fp_pifdefs_genw);
        pp_clear_output_area(&buffer);
    }
    fprintf(fp_kerneldecls_genw, "extern _CPPSTRING_ %s\n", fn_decl);
    fprintf(fp_piftable_genw, "{.pif_name=\"%s\",.num_params=%d,.cpu_kernel={.kernel_name=\"%s\",.function=(snk_generic_fp)%s},.gpu_kernel={.kernel_name=NULL}},\n",
            pif_name, num_params, fn_name, fn_name);
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
    fclose(fp_pifdefs_genw);
    fclose(fp_kerneldecls_genw);
    fclose(fp_piftable_genw);

    fclose(f_tmp);
    int ret_del = remove("tmp.pif.def.c");
    if(ret_del != 0) fprintf(stderr, "Unable to delete temp file: tmp.pif.def.c\n");
    
    counter++;
    return NULL_TREE;
}

void
handle_pre_generic (void *event_data, void *data)
{
    tree fndecl = (tree) event_data;
    tree arg;
    for (arg = DECL_ARGUMENTS(fndecl); arg; arg = TREE_CHAIN (arg)) {
        tree attr;
        for (attr = DECL_ATTRIBUTES (arg); attr; attr = TREE_CHAIN (attr)) {
            tree attrname = TREE_PURPOSE (attr);
            tree attrargs = TREE_VALUE (attr);
            warning (0, G_("attribute '%s' on param '%s' of function %s"),
                    IDENTIFIER_POINTER (attrname),
                    IDENTIFIER_POINTER (DECL_NAME (arg)),
                    IDENTIFIER_POINTER (DECL_NAME (fndecl))
                    );
        }
    }
}
/* Attribute definition */
static struct attribute_spec launch_info_attr =
{ "launch_info", 0, 2, false,  false, false, handle_cpu_task_attribute, false };

/* Plugin callback called during attribute registration.
 * Registered with register_callback (plugin_name, PLUGIN_ATTRIBUTES,
 * register_attributes, NULL)
 */
static void
register_attributes (void *event_data, void *data)
{
    warning (0, G_("Callback to register attributes"));
    register_attribute (&launch_info_attr);
}/* Plugin callback called during attribute registration.
 * Registered with register_callback (plugin_name, PLUGIN_ATTRIBUTES,
 * register_attributes, NULL)
 */

static void
register_headers (void *event_data, void *data)
{
    //warning (0, G_("Callback to register header files"));
    DEBUG_PRINT("Done registering header files: %s for input file: %s\n", (const char *)event_data, main_input_filename);
}

static void
register_start_unit (void *event_data, void *data)
{
    warning (0, G_("Callback at start of compilation unit"));
    #if 0
    char pifdefs_filename[1024];
    memset(pifdefs_filename, 0, 1024);
    strcpy(pifdefs_filename, main_input_filename);
    strcat(pifdefs_filename, ".pifdefs.h");

    FILE *fp = fopen(pifdefs_filename, "w");
    fprintf(fp, "");
    fclose(fp);
    
   /* FILE *fp1 = fopen(main_input_filename, "a");
    fprintf(fp1, "#include \"%s\"", pifdefs_filename);
    fclose(fp1);*/
    #endif
}

int plugin_init(struct plugin_name_args *plugin_info,
        struct plugin_gcc_version *version) {
    if (!plugin_default_version_check (version, &gcc_version))
        return 1;

    DEBUG_PRINT("In plugin init function\n");

    register_callback (plugin_name, 
                    //PLUGIN_PASS_EXECUTION,
                    PLUGIN_START_UNIT,
                    //PLUGIN_OVERRIDE_GATE,
                    //PLUGIN_PRAGMAS,
                    //PLUGIN_NEW_PASS,
                    register_start_unit, NULL);
    //register_callback (plugin_name, PLUGIN_PRE_GENERICIZE,
    //                      handle_pre_generic, NULL);
    register_callback (plugin_name, PLUGIN_ATTRIBUTES,
                  register_attributes, NULL);
    register_callback (plugin_name, PLUGIN_INCLUDE_FILE,
                  register_headers, NULL);
    return 0;
}

