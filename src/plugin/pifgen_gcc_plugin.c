#include <gcc-plugin.h>
#include <plugin-version.h>
#include <plugin.h>
#include <c-family/c-common.h>
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
#include <stringpool.h>
#include <langhooks.h>

#include <map>
#include <set>
#include <string>
#include <vector>
using namespace std;
//#define DEBUG_ATMI_RT_PLUGIN
#ifdef DEBUG_ATMI_RT_PLUGIN
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

int plugin_is_GPL_compatible;

static const char *plugin_name = "atmi_pifgen";

static std::map<std::string, int> pif_table;
typedef struct pif_printers_s {
   pretty_printer *pifdefs; 
   pretty_printer *fn_table; 
} pif_printers_t;
static std::vector<pif_printers_t> pif_printers;

static pretty_printer g_kerneldecls;

static int initialized = 0;

void write_headers(FILE *fp) {
    fprintf(fp, "\
#include \"atmi.h\"\n\
#include \"snk.h\"\n\n");
}

void write_cpp_warning_header(FILE *fp) {
fprintf(fp, "#ifdef __cplusplus \n\
#define _CPPSTRING_ \"C\" \n\
#endif \n\
#ifndef __cplusplus \n\
#define _CPPSTRING_ \n\
#endif \n\n");
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

void push_declaration(const char *pif_name, tree fn_type, int num_params) {
    tree fndecl;
    tree pifdecl = get_identifier(pif_name);

    fndecl = build_decl(DECL_SOURCE_LOCATION(pifdecl),
                        FUNCTION_DECL, pifdecl, fn_type);

    //print_generic_stmt(stdout, fndecl, TDF_RAW);
    
    tree arg;
    function_args_iterator args_iter;
    tree new_fn_type = NULL_TREE;
    tree new_fn_arglist = NULL_TREE;
    int iter = 0;
    tree new_ret_type = NULL_TREE;
    FOREACH_FUNCTION_ARGS(fn_type, arg, args_iter)
    {
        //print_generic_stmt(stdout, arg, TDF_RAW);
        if(iter == 0) {
            // replace atmi_task_t of the task with atmi_lparm_t for the PIF
            //new_ret_type = arg;
            tree type_decl = lookup_name(get_identifier("atmi_lparm_t"));
            tree new_arg = build_pointer_type(TREE_TYPE(type_decl));
            //arg = ptr_type_node;
            new_fn_arglist = tree_cons(NULL_TREE, new_arg, new_fn_arglist);
        }
        else if(iter >= num_params)
            break;
        else
            new_fn_arglist = tree_cons(NULL_TREE, arg, new_fn_arglist);

        iter++;
        //debug_tree_chain(arg);
    }
    // return atmi_task_t *
    tree ret_type_decl = lookup_name(get_identifier("atmi_task_t"));
    new_ret_type = build_pointer_type(TREE_TYPE(ret_type_decl));
    new_fn_type = build_function_type(new_ret_type, nreverse(new_fn_arglist));
    
    // build the PIF declaration 
    tree new_fndecl = build_decl(DECL_SOURCE_LOCATION(pifdecl),
                        FUNCTION_DECL, pifdecl, new_fn_type);
    //print_generic_stmt(stdout, new_fn_type, TDF_RAW);

    // By now, new_fndecl should be a tree for the entire PIF declaration
#ifdef DEBUG_ATMI_RT_PLUGIN    
    DEBUG_PRINT("Now printing new fn_type\n");
    print_generic_stmt(stdout, new_fndecl, TDF_RAW);
    FOREACH_FUNCTION_ARGS(new_fn_type, arg, args_iter)
    {
        print_generic_stmt(stdout, arg, TDF_RAW);
        // debug_tree_chain(arg);
    }
#endif
    // more access specifiers to the declaration
    DECL_ARTIFICIAL (new_fndecl) = 1;
    TREE_PUBLIC (new_fndecl) = TREE_PUBLIC (pifdecl);
    TREE_STATIC (new_fndecl) = 0;
    TREE_USED (new_fndecl) = 1;
    // do NOT use the below flag because C programs fail. Some ANSI C nonsense
    // that I am still yet to understand. C++ programs work fine with or 
    // without them, so we are going without because these declarations are in 
    // global scope anyway.
    //DECL_EXTERNAL (new_fndecl) = 1;
    DECL_CONTEXT (new_fndecl) = current_function_decl;

    // finally, push the new function declaration to the same compilation unit
    pushdecl(new_fndecl);
}

int get_pif_index(const char *pif_name) {
    /*DEBUG_PRINT("Looking up PIF %s\n", pif_name);
    for(std::map<std::string, int>::iterator it = pif_table.begin();
        it != pif_table.end(); it++) {
        DEBUG_PRINT("%s, %d\n", it->first.c_str(), it->second);
    }
    */
    if(pif_table.find(std::string(pif_name)) != pif_table.end()) {
        return pif_table[pif_name];
    }
    return -1;
}

void register_pif(const char *pif_name) {
    std::string pif_name_str(pif_name);
    if(pif_table.find(pif_name_str) == pif_table.end()) {
        pif_table[pif_name_str] = pif_printers.size();
        //pif_table.insert(std::pair<std::string,int>(pif_name_str, pif_printers.size()));
        pif_printers_t pp;
        pp.pifdefs = new pretty_printer;
        pp.fn_table = new pretty_printer;
        pp_needs_newline ((pp.pifdefs)) = true;
        pp_needs_newline ((pp.fn_table)) = true;
        pif_printers.push_back(pp);
    }
}

static tree
handle_task_impl_attribute (tree *node, tree name, tree args,
        int flags, bool *no_add_attrs)
{
    DEBUG_PRINT("Handling __attribute__ %s\n", IDENTIFIER_POINTER(name));

    tree decl = *node;
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

    int is_new_pif = (get_pif_index(pif_name) == -1) ? 1 : 0;
    DEBUG_PRINT("New PIF? %d\n", is_new_pif);
    register_pif(pif_name);
    int pif_index = get_pif_index(pif_name); 
    
    FILE *f_tmp = fopen("tmp.pif.def.c", "w");
    pretty_printer tmp_buffer;
    if (!initialized) {
        //pp_construct (&buffer, /* prefix */NULL, /* line-width */0);
        pp_needs_newline (&tmp_buffer) = true;
        initialized = 1;
    }
    tmp_buffer.buffer->stream = f_tmp;

    const char* fn_name = IDENTIFIER_POINTER(DECL_NAME(decl));
    DEBUG_PRINT("Task Name: %s\n", fn_name); 

    tree fn_type = TREE_TYPE(decl);
    //print_generic_stmt(fp_pifdefs_genw, fn_type, TDF_RAW);
    dump_generic_node (&tmp_buffer, fn_type, 0, TDF_RAW, true);
    char *text = (char *) pp_formatted_text (&tmp_buffer);
    DEBUG_PRINT("Plugin Task dump: %s\n", text);

    int text_sz = strlen(text); 
    char text_dup[2048]; 
    strcpy(text_dup, text);
    pp_flush (&tmp_buffer);
    fclose(f_tmp);
    int ret_del = remove("tmp.pif.def.c");
    if(ret_del != 0) fprintf(stderr, "Unable to delete temp file: tmp.pif.def.c\n");
    
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
   
    if(is_new_pif == 1) { //first time PIF is called 
        pp_printf((pif_printers[pif_index].pifdefs), "\nint %s_FK;\n", pif_name);
        pp_printf((pif_printers[pif_index].pifdefs), "extern _CPPSTRING_ %s { \n", pif_decl);
        pp_printf((pif_printers[pif_index].pifdefs), "    /* launcher code (PIF definition) */\n");
        pp_printf((pif_printers[pif_index].pifdefs), "\
    if (%s_FK == 0 ) { \n\
        snk_pif_init(%s_pif_fn_table, sizeof(%s_pif_fn_table)/sizeof(%s_pif_fn_table[0]));\n\
        %s_FK = 1; \n\
    } \n\
    snk_kernel_args_t *cpu_kernel_arg_list = (snk_kernel_args_t *)malloc(sizeof(snk_kernel_args_t)); \n", 
                pif_name,
                pif_name, pif_name, pif_name,
                pif_name);
        pp_printf((pif_printers[pif_index].pifdefs), "    cpu_kernel_arg_list->args[0] = (uint64_t)NULL; \
                ");
        int arg_idx;
        for(arg_idx = 1; arg_idx < num_params; arg_idx++) {
            pp_printf((pif_printers[pif_index].pifdefs), "\n\
    cpu_kernel_arg_list->args[%d] = (uint64_t)var%d;", arg_idx, arg_idx);
        }
        pp_printf((pif_printers[pif_index].pifdefs), "\n\
    return snk_cpu_kernel(lparm, \n\
                    \"%s\", \n\
                    cpu_kernel_arg_list); \n\
                ", 
                pif_name);
        pp_printf((pif_printers[pif_index].pifdefs), "\n\
}\n\n");
        
        /* add PIF function table definition */
        pp_printf((pif_printers[pif_index].fn_table), "\nsnk_pif_kernel_table_t %s_pif_fn_table[] = {\n", pif_name);
    }
    pp_printf(&g_kerneldecls, "extern _CPPSTRING_ %s\n", fn_decl);
    pp_printf((pif_printers[pif_index].fn_table), "\
    {.pif_name=\"%s\",.num_params=%d,.cpu_kernel={.kernel_name=\"%s\",.function=(snk_generic_fp)%s},.gpu_kernel={.kernel_name=NULL}},\n",
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

    if(is_new_pif == 1) {
        push_declaration(pif_name, fn_type, num_params);
    }
    return NULL_TREE;
}

/* Attribute definition */
static struct attribute_spec atmi_task_impl_attr =
{ "atmi_task_impl", 0, 2, false,  false, false, handle_task_impl_attribute, false };

/* Plugin callback called during attribute registration.
 * Registered with register_callback (plugin_name, PLUGIN_ATTRIBUTES,
 * register_attributes, NULL)
 */
static void
register_attributes (void *event_data, void *data)
{
    DEBUG_PRINT("Callback to register attributes\n");
    register_attribute (&atmi_task_impl_attr);
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
register_finish_unit (void *event_data, void *data) {
    /* Dump all the generated code into one .c file */
    DEBUG_PRINT("Callback at end of compilation unit\n");
    char pifdefs_filename[1024];
    memset(pifdefs_filename, 0, 1024);
    strcpy(pifdefs_filename, main_input_filename);
    strcat(pifdefs_filename, ".pifdefs.c");
    FILE *fp_pifdefs_genw = fopen(pifdefs_filename, "w");
    write_headers(fp_pifdefs_genw);
    write_cpp_warning_header(fp_pifdefs_genw);
    /* 1) dump kernel impl declarations (fn pointers */
    char *decl_text = (char *)pp_formatted_text(&g_kerneldecls);
    fputs (decl_text, fp_pifdefs_genw);
    pp_clear_output_area(&g_kerneldecls);

    /* 2) dump piftable array and PIF definition */
    for(std::vector<pif_printers_t>::iterator it = pif_printers.begin(); 
        it != pif_printers.end(); it++) {
        /* create piftable */
        char *piftable_text = (char *)pp_formatted_text(it->fn_table);
        fputs (piftable_text, fp_pifdefs_genw);
        fprintf(fp_pifdefs_genw, "};\n");
        pp_clear_output_area((it->fn_table));

        /* PIF definition */
        char *pif_text = (char *)pp_formatted_text(it->pifdefs);
        fputs (pif_text, fp_pifdefs_genw);
        pp_clear_output_area((it->pifdefs));
    }
    fclose(fp_pifdefs_genw);
}

static void
register_start_unit (void *event_data, void *data) {
    DEBUG_PRINT("Callback at start of compilation unit\n");
    pp_needs_newline (&g_kerneldecls) = true;
    //tree kernel_type = get_identifier("__kernel");
    //kernel_type = TYPE_STUB_DECL(kernel_type);
    //print_generic_stmt(stdout, kernel_type, TDF_RAW);         
    //tree tdecl = add_builtin_type ("__kernel", access_public_node);
    //TYPE_NAME (access_public_node) = tdecl;
    //print_generic_stmt(stdout, tdecl, TDF_RAW);         
}

int plugin_init(struct plugin_name_args *plugin_info,
        struct plugin_gcc_version *version) {
    if (!plugin_default_version_check (version, &gcc_version))
        return 1;

    DEBUG_PRINT("In plugin init function\n");
    register_callback (plugin_name, PLUGIN_START_UNIT,
                    register_start_unit, NULL);
    register_callback (plugin_name, PLUGIN_FINISH_UNIT,
                  register_finish_unit, NULL);
    register_callback (plugin_name, PLUGIN_ATTRIBUTES,
                  register_attributes, NULL);
#if 0
    //register_callback (plugin_name, PLUGIN_PRE_GENERICIZE,
    //                      handle_pre_generic, NULL);
    register_callback (plugin_name, PLUGIN_INCLUDE_FILE,
                  register_headers, NULL);
#endif
    return 0;
}

