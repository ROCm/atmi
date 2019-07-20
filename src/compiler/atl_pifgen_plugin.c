/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#include "atmi.h"
#include <gcc-plugin.h>
#include <plugin-version.h>
#include <plugin.h>
#include <c-family/c-common.h>
#include <tree.h>
#include <tree-iterator.h>
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
#include <cpplib.h>
#include <c-tree.h>
#include <cgraph.h>
#include <c-family/c-pragma.h>
#include <gimple-expr.h>
#include <coretypes.h>

#include "atl_pifgen.h"

#include <cctype>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iterator>
#include <algorithm>

using namespace std;
//#define DEBUG_ATMI_RT_PLUGIN
#ifdef DEBUG_ATMI_RT_PLUGIN
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

int plugin_is_GPL_compatible;

static const char *plugin_name = "atmi_pifgen";

typedef struct string_table_s {
    const char *key;
    const char *value;
} string_table_t;

string_table_t res_keywords_table[] = {
{"__kernel",""},
{"__global",""},
};

std::string main_input_filename_str;
static std::vector<std::string> g_cl_modules;
static std::vector<std::string> g_all_pifdecls;
static std::vector<std::string> g_cl_files;
// format of the pif_table
// "PIF name", (pif_index_in_table, kernels_for_this_pif_count)
static std::map<std::string, std::pair<int,int> > pif_table;
typedef struct pif_printers_s {
   pretty_printer *pifdefs; 
   pretty_printer *fn_table; 
   pretty_printer *kid_table; 
} pif_printers_t;
static std::vector<pif_printers_t> pif_printers;

static pretty_printer pif_spawn;

static pretty_printer kl_init_funs;

static pretty_printer g_kerneldecls;

static int initialized = 0;
static int g_hsa_offline_finalize = 0;
static std::string g_output_pifdefs_filename;

void append_kl_init_funs(FILE *pifFile);
void write_kl_init(const char *pif_name, int pif_index, std::vector<std::string> arg_list, int num_params);
void write_kernel_dispatch_routine(FILE *fp);
void write_pif_kl(FILE *clFile);
void write_spawn_function(const char *pif_name, int pif_index, std::vector<std::string> arg_list, int num_params);

void write_headers(FILE *fp) {
    fprintf(fp, "\
#include \"atmi.h\"\n\
#include \"atmi_runtime.h\"\n\
#include <stdio.h>\n\
#include \"atmi_kl.h\"\n\n");
}

void write_cpp_warning_header(FILE *fp) {
fprintf(fp, "#ifdef __cplusplus \n\
#define _CPPSTRING_ \"C\" \n\
#endif \n\
#ifndef __cplusplus \n\
#define _CPPSTRING_ \n\
#endif \n\n");
}

void write_globals(FILE *fp) {
fprintf(fp, "\
static int g_initialized = 0;\n\n\
atmi_klist_t *atmi_klist;\n\
#define ErrorCheck(msg, status) \\\n\
if (status != ATMI_STATUS_SUCCESS) { \\\n\
   printf(\"%%s failed.\\n\", #msg); \\\n\
} \n\n\
extern _CPPSTRING_ void atl_kl_init(atmi_klist_t *atmi_klist, atmi_kernel_t kernel, const int pif_id); \n\
void kl_init(); \n\n\
");
fprintf(fp, "\
typedef struct %s_kernel_table_s { \n\
    atmi_devtype_t devtype; \n\
    atmi_generic_fp cpu_kernel; \n\
    const char *gpu_kernel; \n\
} %s_kernel_table_t; \n", 
main_input_filename_str.c_str(),
main_input_filename_str.c_str()
);
}

void write_cpp_get_gpu_agent(FILE *fp) {
fprintf(fp, "\
static hsa_status_t get_gpu_agent(hsa_agent_t agent, void *data) {\n\
    hsa_status_t status;\n\
    hsa_device_type_t device_type;\n\
    status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);\n\
    if (HSA_STATUS_SUCCESS == status && HSA_DEVICE_TYPE_GPU == device_type) {\n\
        hsa_agent_t* ret = (hsa_agent_t*)data;\n\
        *ret = agent;\n\
        return HSA_STATUS_INFO_BREAK;\n\
    }\n\
    return HSA_STATUS_SUCCESS;\n\
}\n\n\
");
}

void
print_struct (tree type)
{
    tree decl_before (TYPE_NAME (type));
    tree id_before (DECL_NAME (decl_before));
    const char* name_before (IDENTIFIER_POINTER (id_before));
    printf("%s at %s:%d\n", name_before, DECL_SOURCE_FILE (decl_before), DECL_SOURCE_LINE (decl_before));

    type = TYPE_MAIN_VARIANT (type);
    tree decl (TYPE_NAME (type));
    tree id (DECL_NAME (decl));
    const char* name (IDENTIFIER_POINTER (id));

    printf("is typedef of struct %s at %s:%d\n", name, DECL_SOURCE_FILE (decl), DECL_SOURCE_LINE (decl));
}

void
print_decl (tree decl)
{
    tree type (TREE_TYPE (decl));
    int tc;
    printf("--------------------------\n");
    if (type)
    {
        tc = TREE_CODE (type);

        if (tc == RECORD_TYPE)
        {
            // If DECL_ARTIFICIAL is true this is a class
            // declaration. Otherwise this is a typedef.
            //
            //if (DECL_ARTIFICIAL (decl))
            {
                print_struct (type);
                return;
            }
        }
        else
            printf("Not a type_decl and record_type\n");
    }
    else
        printf("Not a type\n");
}

std::string exec(const char* cmd) {
    /* SO: how to execute a command and get output of command within c
     */
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
}

void cloc_wrapper(const char *clfilename, const char *symbolname) {
    std::string cmd;
    cmd.clear();
    cmd += exec("which cloc_wrapper.sh");

    if(cmd == "" || cmd == "ERROR") {
        fprintf(stderr, "cloc_wrapper.sh script not found in the PATH.\n");
        exit(-1);
    }
    char cmd_c[2048] = {0};
    strcpy(cmd_c, cmd.c_str());
    // popen returns a newline. remove it.
    strtok(cmd_c, "\n");
    strcat(cmd_c, " ");
    if(symbolname != NULL && symbolname != "") {
        strcat(cmd_c, "-s ");
        strcat(cmd_c, symbolname);
        strcat(cmd_c, " ");
    }
    //if(some_condition) {
    //strcat(cmd_c, "-rp /opt/hsa.1_1T ");
    //}
    if(g_hsa_offline_finalize == 1) {
        strcat(cmd_c, " -hof ");
    }
    //keep the temp files?
    //strcat(cmd_c, " -k ");
    //strcat(cmd_c, " -vv ");
    strcat(cmd_c, clfilename);
    DEBUG_PRINT("Executing cmd: %s\n", cmd_c);
    int ret = system(cmd_c);
    if(WIFEXITED(ret) == 0 || WEXITSTATUS(ret) != 0) {
        fprintf(stderr, "\"%s\" returned with error %d\n", cmd_c, WEXITSTATUS(ret));
        exit(-1);
    }
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}


void generate_task_wrapper(char *text, const char *fn_name, const int num_params, char *fn_decl) {
    char *pch = strtok(text,"<");
    if(pch != NULL) strcpy(fn_decl, pch);
    pch = strtok (NULL, ">");
    strcat(fn_decl, fn_name);
    strcat(fn_decl, "_wrapper");
    pch = strtok (NULL, "(");
    if(pch != NULL) strcat(fn_decl, pch);
    if(num_params == 0) {
        strcat(fn_decl, "()");
    }
    else if(num_params > 0) {
        strcat(fn_decl, "(");
    }
    
    int var_idx = 0;
    //pch = strtok (NULL, ",)");
    //DEBUG_PRINT("Parsing but ignoring this string now: %s\n", pch);
    for(var_idx = 0; var_idx < num_params; var_idx++) {
        pch = strtok (NULL, ",)");
        DEBUG_PRINT("Parsing this string now: %s\n", pch);
        strcat(fn_decl, pch);
        char var_decl[64] = {0}; 
        sprintf(var_decl, "* var%d", var_idx);
        strcat(fn_decl, var_decl);
        if(var_idx == num_params - 1) // last param must end with )
            strcat(fn_decl, ")");
        else
            strcat(fn_decl, ",");
    }
}

void generate_task(char *text, const char *fn_name, const int num_params, char *fn_decl) {
    char *pch = strtok(text,"<");
    if(pch != NULL) strcpy(fn_decl, pch);
    pch = strtok (NULL, ">");
    strcat(fn_decl, fn_name);
    pch = strtok (NULL, "(");
    if(pch != NULL) strcat(fn_decl, pch);
    if(num_params == 0) {
        strcat(fn_decl, "()");
    }
    else if(num_params > 0) {
        strcat(fn_decl, "(");
    }
    
    int var_idx = 0;
    //pch = strtok (NULL, ",)");
    //DEBUG_PRINT("Parsing but ignoring this string now: %s\n", pch);
    for(var_idx = 0; var_idx < num_params; var_idx++) {
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
    strcat(fn_decl, ";");
}

void generate_pif(char *text, const char *fn_name, const int num_params, char *fn_decl) {
    char *pch = strtok(text,"<");
    // return atmi_task_handle_t
    if(pch != NULL) strcpy(fn_decl, "atmi_task_handle_t ");
    pch = strtok (NULL, ">");
    strcat(fn_decl, fn_name);
    pch = strtok (NULL, "(");
    if(pch != NULL) strcat(fn_decl, pch);
   
    if(num_params == 0) {
        strcat(fn_decl, "(atmi_lparm_t *lparm)");
    }
    else if(num_params > 0) {
        strcat(fn_decl, "(atmi_lparm_t *lparm, ");
    }

    int var_idx = 0;
    //pch = strtok (NULL, ",)");
    //do not ignore the first argument because we got rid of the atmi_task_hande_t 
    //requirement as the first argument
    //DEBUG_PRINT("Parsing but ignoring this string now: %s\n", pch);
    for(var_idx = 0; var_idx < num_params; var_idx++) {
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
    tree new_ret_type = NULL_TREE;
    // ADD a new arg atmi_lparm_t for the PIF
    tree type_decl = lookup_name(get_identifier("atmi_lparm_t"));
    tree new_arg = build_pointer_type(TREE_TYPE(type_decl));
    new_fn_arglist = tree_cons(NULL_TREE, new_arg, new_fn_arglist);
    int iter = 0;
    FOREACH_FUNCTION_ARGS(fn_type, arg, args_iter)
    {
        if(iter < num_params) {
            //print_generic_stmt(stdout, arg, TDF_RAW);
            new_fn_arglist = tree_cons(NULL_TREE, arg, new_fn_arglist);
            //debug_tree_chain(arg);
            iter++;
        }
    }
    // return atmi_task_t *
    tree ret_type_decl = lookup_name(get_identifier("atmi_task_handle_t"));
    //new_ret_type = build_pointer_type(TREE_TYPE(ret_type_decl));
    new_fn_type = build_function_type(TREE_TYPE(ret_type_decl), nreverse(new_fn_arglist));
    
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
    // do NOT use the below flag because ANSI C programs fail for some reason. 
    // C++ programs seem to work fine with or 
    // without them, so we are going without because these declarations are in 
    // global scope anyway.
    //DECL_EXTERNAL (new_fndecl) = 1;
    DECL_CONTEXT (new_fndecl) = current_function_decl;

    // finally, push the new function declaration to the same compilation unit
    pushdecl(new_fndecl);
}

int get_pif_index(const char *pif_name) {
    /*DEBUG_PRINT("Looking up PIF %s\n", pif_name);
    */
    if(pif_table.find(std::string(pif_name)) != pif_table.end()) {
        return pif_table[pif_name].first;
    }
    return -1;
}

int get_pif_count(const char *pif_name) {
    /*DEBUG_PRINT("Looking up PIF %s\n", pif_name);
    */
    if(pif_table.find(std::string(pif_name)) != pif_table.end()) {
        return pif_table[pif_name].second;
    }
    return -1;
}


void register_pif(const char *pif_name) {
    std::string pif_name_str(pif_name);
    if(pif_table.find(pif_name_str) == pif_table.end()) {
        pif_table[pif_name_str] = std::make_pair(pif_printers.size(), 1);
        //pif_table.insert(std::pair<std::string,int>(pif_name_str, pif_printers.size()));
        pif_printers_t pp;
        pp.pifdefs = new pretty_printer;
        pp.fn_table = new pretty_printer;
        pp.kid_table = new pretty_printer;
        pp_needs_newline ((pp.pifdefs)) = true;
        pp_needs_newline ((pp.fn_table)) = true;
        pp_needs_newline ((pp.kid_table)) = true;
        pif_printers.push_back(pp);
    }
    else {
        pif_table[pif_name_str].second++;
    }
}

void push_global_int_decl(const char *var_name, const int value) {
    /* Inspired from SO solution
     * http://stackoverflow.com/questions/25998225/insert-global-variable-declaration-whit-a-gcc-plugin?rq=1
     */
    tree global_var = build_decl(input_location, 
                        VAR_DECL, get_identifier(var_name), integer_type_node);
    DECL_INITIAL(global_var) = build_int_cst (integer_type_node, value);
    TREE_READONLY(global_var) = 1;
    TREE_STATIC (global_var) = 1;
    //DECL_CONTEXT (global_var) = ???;
    varpool_add_new_variable(global_var);
    /*AND if you have thought to use in another subsequent compilation, you 
      will need to give it an assembler name like this*/
    //change_decl_assembler_name(global_var, g_name);
    pushdecl(global_var);
}

static tree
handle_task_impl_attribute (tree *node, tree name, tree args,
        int flags, bool *no_add_attrs)
{
    DEBUG_PRINT("Handling __attribute__ %s\n", IDENTIFIER_POINTER(name));
    atmi_devtype_t devtype;
    tree decl = *node;
    // Print the arguments of the attribute
    DEBUG_PRINT("Task Attribute Params: ");
    char pif_name[1024]; 
    int attrib_id = 0;
    for( tree itrArgument = args; itrArgument != NULL_TREE; itrArgument = TREE_CHAIN( itrArgument ) )
    {
        if(attrib_id == 0) {
            strcpy(pif_name, TREE_STRING_POINTER (TREE_VALUE ( itrArgument )));
        }
        else if(attrib_id == 1) { 
            std::string devtype_str(TREE_STRING_POINTER(TREE_VALUE(itrArgument)));
            std::transform(devtype_str.begin(), devtype_str.end(), devtype_str.begin(), ::tolower);

            if(strcmp(devtype_str.c_str(), "cpu") == 0) {
                devtype = ATMI_DEVTYPE_CPU;
            }
            else if(strcmp(devtype_str.c_str(), "gpu") == 0) {
                devtype = ATMI_DEVTYPE_GPU;
            }
            else {
                fprintf(stderr, "Unsupported device type: %s at %s:%d\n", devtype_str.c_str(), 
                            DECL_SOURCE_FILE(decl), DECL_SOURCE_LINE(decl));
                exit(-1);
            }
        //DEBUG_PRINT("DevType: %lu\n", TREE_INT_CST_LOW(TREE_VALUE(itrArgument)));
        //DEBUG_PRINT("DevType: %lu\n", TREE_INT_CST_HIGH(TREE_VALUE(itrArgument)));
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
//    tmp_buffer.buffer->stream = f_tmp;

    const char* fn_name = IDENTIFIER_POINTER(DECL_NAME(decl));
    DEBUG_PRINT("Task Name: %s\n", fn_name); 

    tree fn_type = TREE_TYPE(decl);
    int idx = 0;
    // TODO: Think about the below and verify the better approach for 
    // parameter parsing. Ignore below if above code works.
    //for(; idx < 32; idx++) {
    //   DEBUG_PRINT("%d ", idx);
    //   print_generic_stmt(stdout, fn_type, (1 << idx));
    //   print_generic_stmt(stdout, decl, (1 << idx));
    //}
    tree arg;
    function_args_iterator args_iter;
    std::vector<std::string> arg_list;
    arg_list.clear();
    FOREACH_FUNCTION_ARGS(fn_type, arg, args_iter)
    {
        // TODO FIXME to debug more into the issue of typedefs being preceded
        // by a "struct" keyword for some reason. Strange bug!
        //print_decl(arg); 
        TREE_READONLY(arg) = 0;
    //for(idx = 0; idx < 32; idx++) {
     //DEBUG_PRINT("%s ", IDENTIFIER_POINTER(DECL_NAME(TYPE_IDENTIFIER(arg))));
     //tree arg_type_name = DECL_ARG_TYPE(arg);
     //if (TREE_CODE (arg_type_name) == TYPE_DECL) {
     //arg_type_name = DECL_NAME(arg_type_name);
        dump_generic_node (&tmp_buffer, arg, 0, TDF_RAW, true);
        char *text = (char *) pp_formatted_text (&tmp_buffer);
        arg_list.push_back(text);
        DEBUG_PRINT("ArgType: %s\n", text);
        pp_clear_output_area(&tmp_buffer);
     //print_generic_stmt(stdout, arg, TDF_RAW);
     //}
    //}
    //debug_tree_chain(arg);
    }

    //print_generic_stmt(fp_pifdefs_genw, fn_type, TDF_RAW);
    dump_generic_node (&tmp_buffer, fn_type, 0, TDF_RAW, true);
    char *text = (char *) pp_formatted_text (&tmp_buffer);
    DEBUG_PRINT("Plugin Task dump: %s\n", text);

    int text_sz = strlen(text); 
    char text_dup[2048]; 
    strcpy(text_dup, text);
    char text_dup_2[2048]; 
    strcpy(text_dup_2, text);
    pp_clear_output_area(&tmp_buffer);
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
    char fn_cpu_wrapper_decl[2048];

    generate_pif(text, pif_name, num_params, pif_decl); 
    DEBUG_PRINT("PIF Decl: %s\n", pif_decl);
    
    generate_task(text_dup, fn_name, num_params, fn_decl); 
    DEBUG_PRINT("Task Decl: %s\n", fn_decl);
   
    generate_task_wrapper(text_dup_2, fn_name, num_params, fn_cpu_wrapper_decl); 
    DEBUG_PRINT("Fn CPU Wrapper Decl: %s\n", fn_cpu_wrapper_decl);
    
    if(is_new_pif == 1) { //first time PIF is called 
        pp_printf((pif_printers[pif_index].pifdefs), "\nstatic int %s_FK = 0;\n", pif_name);
        pp_printf((pif_printers[pif_index].pifdefs), "\nstatic atmi_kernel_t %s_kernel;\n", pif_name);
        pp_printf((pif_printers[pif_index].pifdefs), "extern _CPPSTRING_ %s { \n", pif_decl);
        pp_printf((pif_printers[pif_index].pifdefs), "    /* launcher code (PIF definition) */\n");
        pp_printf((pif_printers[pif_index].pifdefs), "\
    const int num_args = %d; \n",
        num_params
        );
        pp_printf((pif_printers[pif_index].pifdefs), "\
    int k_id = lparm->kernel_id; \n\
    if(k_id < 0 || k_id >= sizeof(%s_fn_table)/sizeof(%s_fn_table[0])) { \n\
        fprintf(stderr, \"Kernel_id out of bounds for PIF %s.\\n\"); \n\
        atmi_task_handle_t null_task = 0; \n\
        return null_task; \n\
    } \n\
    atmi_devtype_t devtype = %s_fn_table[k_id].devtype; \n\
    if(g_initialized == 0) {\n",
        pif_name, pif_name,
        pif_name,
        pif_name);
        if(g_cl_modules.empty()) {
            pp_printf((pif_printers[pif_index].pifdefs), "\n\
        atmi_status_t err = atmi_init(ATMI_DEVTYPE_CPU);\n\
        ErrorCheck(ATMI Initializing, err); \n");
        }
        else {
            pp_printf((pif_printers[pif_index].pifdefs), "\n\
        atmi_status_t err = atmi_init(ATMI_DEVTYPE_ALL);\n\
        ErrorCheck(ATMI Initializing, err); \n");
            if(g_hsa_offline_finalize == 1) {
                pp_printf((pif_printers[pif_index].pifdefs), "\n\
        const char *modules[%lu];\n\
        atmi_platform_type_t module_types[%lu]; \n",
                g_cl_modules.size(),
                g_cl_modules.size());
        
                for(int idx = 0; idx < g_cl_modules.size(); idx++) {
                    pp_printf((pif_printers[pif_index].pifdefs), "\
        modules[%d] = \"%s.hof\"; \n\
        module_types[%d] = AMDGCN; \n",
                    idx, g_cl_modules[idx].c_str(),
                    idx);
                }
                pp_printf((pif_printers[pif_index].pifdefs), "\
        err = atmi_module_register(modules, module_types, %lu);\n\
        ErrorCheck(Building Executable, err); \n\
        kl_init(); \n",
                g_cl_modules.size());
            }
        }
        pp_printf((pif_printers[pif_index].pifdefs), "\
        g_initialized = 1;\n\
    }\n");
        pp_printf((pif_printers[pif_index].pifdefs), "\
    if (%s_FK == 0 ) { \n\
        size_t arg_sizes[num_args]; \n",
        pif_name
        );
        int arg_idx;
        for(arg_idx = 0; arg_idx < num_params; arg_idx++) {
            pp_printf((pif_printers[pif_index].pifdefs), "\
        arg_sizes[%d] = sizeof(%s);\n",
            arg_idx, arg_list[arg_idx].c_str()
        );
        }
        pp_printf((pif_printers[pif_index].pifdefs), "\
        atmi_kernel_create_empty(&%s_kernel, num_args, arg_sizes); \n\
        int num_impls = sizeof(%s_fn_table)/sizeof(%s_fn_table[0]);\n\
        int idx; \n\
        for(idx = 0; idx < num_impls; idx++) { \n\
            if(%s_fn_table[idx].devtype == ATMI_DEVTYPE_GPU) { \n\
                atmi_kernel_add_gpu_impl(%s_kernel, %s_fn_table[idx].gpu_kernel, idx); \n\
            }\n\
            else if(%s_fn_table[idx].devtype == ATMI_DEVTYPE_CPU) {\n\
                atmi_kernel_add_cpu_impl(%s_kernel, (atmi_generic_fp)(%s_fn_table[idx].cpu_kernel), idx);\n\
            }\n\
        } \n\
        %s_FK = 1; \n\
    }\n",
                pif_name, 
                pif_name, pif_name,
                pif_name,
                pif_name, pif_name,
                pif_name,
                pif_name, pif_name,
                pif_name);
    pp_printf((pif_printers[pif_index].pifdefs), "\
    void *args[num_args]; \n");
        for(arg_idx = 0; arg_idx < num_params; arg_idx++) {
            pp_printf((pif_printers[pif_index].pifdefs), "\
    args[%d] = &var%d; \n",
            arg_idx, arg_idx
        );
        }
        pp_printf((pif_printers[pif_index].pifdefs), "\n\
    return atmi_task_launch(lparm, %s_kernel, \n\
            args);\n",
            pif_name);
        pp_printf((pif_printers[pif_index].pifdefs), "\
}\n\n");

        /* add init function for dynamic kernel */
        write_kl_init(pif_name, pif_index, arg_list, num_params);


        /* add args struct of pif for dynamic dispatch */
        write_spawn_function(pif_name, pif_index, arg_list, num_params);

       
        /* add PIF function table definition */
        pp_printf((pif_printers[pif_index].fn_table), "\n%s_kernel_table_t %s_fn_table[] = {\n", main_input_filename_str.c_str(), pif_name);
        pp_printf((pif_printers[pif_index].kid_table), "K_ID_%s = 0, ", fn_name);
    }
    else {
        pp_printf((pif_printers[pif_index].kid_table), "K_ID_%s, ", fn_name);
    }
    if(devtype == ATMI_DEVTYPE_CPU) {
        pp_printf(&g_kerneldecls, "extern _CPPSTRING_ %s\n", fn_decl);
        pp_printf(&g_kerneldecls, "extern _CPPSTRING_ %s {\n", fn_cpu_wrapper_decl);
        int arg_idx;
        pp_printf(&g_kerneldecls, "\
    %s(",
        fn_name);
        for(arg_idx = 0; arg_idx < num_params; arg_idx++) {
            pp_printf(&g_kerneldecls, "*var%d", arg_idx);
            if(arg_idx < num_params - 1) pp_printf(&g_kerneldecls, ", ");
        }
        pp_printf(&g_kerneldecls, ");\n}\n");
        pp_printf((pif_printers[pif_index].fn_table), "\
    {.devtype=ATMI_DEVTYPE_CPU,.cpu_kernel=(atmi_generic_fp)%s_wrapper,.gpu_kernel=NULL},\n",
            fn_name);
    } 
    else if(devtype == ATMI_DEVTYPE_GPU) {
        pp_printf((pif_printers[pif_index].fn_table), "\
    {.devtype=ATMI_DEVTYPE_GPU,.cpu_kernel=(atmi_generic_fp)NULL,.gpu_kernel=\"%s\"},\n",
            fn_name);
    }
    // Push helper IDs for programmers to index into their kernels
    // Helper IDs have prefix K_ID_
    std::string fn_index_enum = std::string("K_ID_") + fn_name;
    push_global_int_decl(fn_index_enum.c_str(), get_pif_count(pif_name) - 1);
   
    //
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
        g_all_pifdecls.push_back(std::string(pif_decl));
    }
    return NULL_TREE;
}

/* Attribute definition */
static struct attribute_spec atmi_task_impl_attr =
{ "atmi_kernel", 0, 2, false,  false, false, handle_task_impl_attribute, false };

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
register_start_parse_function (void *event_data, void *data)
{
    //warning (0, G_("Callback to register header files"));
    DEBUG_PRINT("Starting to parse function: %s \n", (const char *)event_data);
}

static void
register_headers (void *event_data, void *data)
{
    //warning (0, G_("Callback to register header files"));
    DEBUG_PRINT("Done registering header files: %s \n", (const char *)event_data);
    if(cpp_included(parse_in, (const char *)event_data)) {
        DEBUG_PRINT("Header file %s is included\n", (const char *)event_data);
    }
}

static void
register_finish_unit (void *event_data, void *data) {
    /* Dump all the generated code into one .c file */
    DEBUG_PRINT("Callback at end of compilation unit\n");
    FILE *fp_pifdefs_genw = NULL;
    if(g_output_pifdefs_filename.empty()) {
        char pifdefs_filename[1024];
        memset(pifdefs_filename, 0, 1024);
        strcpy(pifdefs_filename, main_input_filename);
        strcat(pifdefs_filename, ".pifdefs.c");
        fp_pifdefs_genw = fopen(pifdefs_filename, "w");
    }
    else {
        fp_pifdefs_genw = fopen(g_output_pifdefs_filename.c_str(), "w");
    }
    write_headers(fp_pifdefs_genw);
    std::string header_str = "`which cat` " + std::string(main_input_filename) + " | `which grep` \\#include";
    std::string headers = exec(header_str.c_str());
    if(headers == "" || headers == "ERROR") {
        fprintf(stderr, "cat or grep not found in the PATH.\n");
        exit(-1);
    }
    fprintf(fp_pifdefs_genw, "/*Headers carried over from %s*/\n%s\n", main_input_filename, headers.c_str());
    write_cpp_warning_header(fp_pifdefs_genw);
    //write_cpp_get_gpu_agent(fp_pifdefs_genw);
    write_globals(fp_pifdefs_genw);
#if 0
    if(g_hsa_offline_finalize == 1) {
        for(std::vector<std::string>::iterator it = g_cl_modules.begin(); 
                it != g_cl_modules.end(); it++) {
            fprintf(fp_pifdefs_genw, "#include \"%s_hof.h\"\n", it->c_str());
        }
    }
#endif
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

    append_kl_init_funs(fp_pifdefs_genw);
    fclose(fp_pifdefs_genw);

    /* Inject all the PIF declarations into the CL file and compile (cloc) */
    for(std::vector<std::string>::iterator it = g_cl_files.begin(); 
            it != g_cl_files.end(); it++) {
        FILE *tmp_cl = fopen("tmp.cl", "w");
        //fprintf(tmp_cl, "#include \"atmi.h\"\n");
        write_cpp_warning_header(tmp_cl);
        //for(std::vector<std::string>::iterator it_pif = g_all_pifdecls.begin(); 
                //it_pif != g_all_pifdecls.end(); it_pif++) {
            //fprintf(tmp_cl, "extern _CPPSTRING_ %s;\n", it_pif->c_str());
        //}
        write_pif_kl(tmp_cl);
        fclose(tmp_cl);
        char cmd_c[2048] = {0};
        sprintf(cmd_c, "cat %s >> tmp.cl", it->c_str());

        DEBUG_PRINT("Executing cmd: %s\n", cmd_c);
        int ret = system(cmd_c);
        if(WIFEXITED(ret) == 0 || WEXITSTATUS(ret) != 0) {
            fprintf(stderr, "\"%s\" returned with error %d\n", cmd_c, WEXITSTATUS(ret));
            exit(-1);
        }
        vector<string> tokens = split(it->c_str(), '.');
        cloc_wrapper("tmp.cl", tokens[0].c_str());
        //int ret_del = remove("tmp.cl");
        //if(ret_del != 0) fprintf(stderr, "Unable to delete temp file: tmp.cl\n");
    }
}

static void
register_start_unit (void *event_data, void *data) {
    DEBUG_PRINT("Callback at start of compilation unit\n");
    g_all_pifdecls.clear();
    pp_needs_newline (&g_kerneldecls) = true;
    pp_needs_newline (&pif_spawn) = true;
    pp_needs_newline (&kl_init_funs) = true;

    /* Replace CL or other kernel-specific keywords with 
     * regular C/C++ keywords. 
     */
    int i;
    int num_elems = sizeof(res_keywords_table)/sizeof(res_keywords_table[0]);
    for(i = 0; i < num_elems; i++) { 
        string s(res_keywords_table[i].key);
        s += "=";
        s += res_keywords_table[i].value;
        cpp_define(parse_in, s.c_str());
    }
    //cpp_define(parse_in, "cl_long_long=char*");
}
#if 1
static void
register_finish_type(void *event_data, void *data) {
    tree type = (tree)event_data;
    if(TREE_CODE(type) == FUNCTION_DECL  
       //|| TREE_CODE(type) == VAR_DECL  
       //|| TREE_CODE(type) == TYPE_DECL
       ) {
        DEBUG_PRINT("Callback at finish of type %p %p\n", event_data, data);
        //debug_tree_chain(type);
    }
}

static void
register_finish_decl(void *event_data, void *data) {
    tree type = (tree)event_data;
    if(TREE_CODE(type) == FUNCTION_DECL 
       // || TREE_CODE(type) == VAR_DECL 
       // || TREE_CODE(type) == TYPE_DECL
       ) {
        DEBUG_PRINT("Callback at finish of decl %p %p\n", event_data, data);
        //debug_tree_chain(type);
    }
}
#endif
static void print_tree_node(tree t, int indent)
{
    // indentation..
    int i;
    for (i = 1; i <= indent; ++i)
        printf("  ");

    enum tree_code code = TREE_CODE(t);

    // Declarations..
    if (code == RESULT_DECL || 
            code == PARM_DECL || 
            code == LABEL_DECL || 
            code == VAR_DECL ||
            code == FUNCTION_DECL) {

        // Get DECL_NAME for this declaration
        tree id = DECL_NAME(t);

        // print name of declaration..
        const char *name = id ? IDENTIFIER_POINTER(id) : "<unnamed>";
        printf("%s : %s\n", get_tree_code_name(code), name);
    }

    // Integer constant..
    else if (code == INTEGER_CST) {
        // value of integer constant is:
        // (HIGH << HOST_BITS_PER_WIDE_INT) + LOW

        if (TREE_INT_CST_HIGH(t)) {
            printf("%s : high=0x%ld low=0x%ld\n", 
                    get_tree_code_name(code),
                    TREE_INT_CST_HIGH(t),
                    TREE_INT_CST_LOW(t));
        }
        else
        {
            printf("%s:%ld\n", 
                    get_tree_code_name(code),
                    TREE_INT_CST_LOW(t));
        }
        return;
    }

    else
    {
        //        print            tree_code_name            for                this                    tree                    node..
        printf("%s\n", get_tree_code_name(code));
    }

}

static void parse_tree(tree t, void (*callback)(tree t, int indent), int indent)
{
    // null => return
    if (t == 0 || indent > 3)
        return;

    (*callback)(t, indent);

    // Statement list..
    if (TREE_CODE(t) == STATEMENT_LIST) {
        tree_stmt_iterator it;
        for (it = tsi_start(t); !tsi_end_p(it); tsi_next(&it)) {
            parse_tree(tsi_stmt(it), callback, indent+1);
        }
        return;
    }

    // Don't parse into declarations/exceptions/constants..
    if (DECL_P(t) || EXCEPTIONAL_CLASS_P(t) || CONSTANT_CLASS_P(t)) {
        return;
    }

    // parse into first operand
    parse_tree(TREE_OPERAND(t, 0), callback, indent+1);

    if (UNARY_CLASS_P(t))
        return;

    // parse into second operand
    enum tree_code code = TREE_CODE(t);
    if (code != RETURN_EXPR && 
            code != LABEL_EXPR &&
            code != GOTO_EXPR &&
            code != NOP_EXPR &&
            code != DECL_EXPR &&
            code != ADDR_EXPR && 
            code != INDIRECT_REF &&
            code != COMPONENT_REF)
        parse_tree(TREE_OPERAND(t, 1), callback, indent+1);
}

static void handle_pre_generic(void *gcc_data, void *user_data)
{
    // Print AST
    tree fndecl = (tree)gcc_data;

    if(
       TREE_CODE(fndecl) == FUNCTION_DECL || 
       TREE_CODE(fndecl) == VAR_DECL || 
       TREE_CODE(fndecl) == TYPE_DECL) {
        tree id = DECL_NAME(fndecl);
        const char *fnname = id ? IDENTIFIER_POINTER(id) : "<unnamed>";
        printf("[Pregenericize] %s %s\n", get_tree_code_name(FUNCTION_DECL), fnname);

        if(strcmp(fnname, "main") == 0) {
        DEBUG_PRINT("In main function\n");
        // Print function body..
        tree fnbody = DECL_SAVED_TREE(fndecl);
        //debug_tree_chain(fnbody);
        print_generic_stmt(stdout, fnbody, 1);
        if (TREE_CODE(fnbody) == BIND_EXPR) {
            // second operand of BIND_EXPR
            tree t = TREE_OPERAND(fnbody, 1);

            // use the utility function "parse_tree" to parse
            // through the tree recursively  (../include/parse-tree.h)
            parse_tree(t, print_tree_node, 1);
        }
        }
    }                                                                                                                
}

int plugin_init(struct plugin_name_args *plugin_info,
        struct plugin_gcc_version *version) {

    if (!plugin_default_version_check (version, &gcc_version))
        return 1;

    int i;

    for(i = 0; i < save_decoded_options_count; i++) {
        DEBUG_PRINT("Cmd line option[%d]: %s\n", i, save_decoded_options[i].orig_option_with_args_text);
    }

    g_output_pifdefs_filename.clear();
    g_cl_modules.clear();

    vector<string> main_tokens = split(main_input_filename, '.');
    main_input_filename_str = main_tokens[0];

    /* Do this loop once before just to set HOF flag */
    for(i = 0; i < plugin_info->argc; i++) { 
        if(strcmp(plugin_info->argv[i].key, "jitcompile") == 0) {
            DEBUG_PRINT("Plugin Arg %d: (%s, %s)\n", i, plugin_info->argv[i].key, plugin_info->argv[i].value);
            const char *should_finalize = plugin_info->argv[i].value;
            std::string finalize_str(should_finalize);
            std::transform(finalize_str.begin(), finalize_str.end(), finalize_str.begin(), ::tolower);
            if(strcmp(finalize_str.c_str(), "false") == 0) {
                g_hsa_offline_finalize = 1;
            }
        }
    }

    for(i = 0; i < plugin_info->argc; i++) { 
        if(strcmp(plugin_info->argv[i].key, "clfile") == 0) {
            DEBUG_PRINT("Plugin Arg %d: (%s, %s)\n", i, plugin_info->argv[i].key, plugin_info->argv[i].value);
            const char *clfilename = plugin_info->argv[i].value;

            g_cl_files.push_back(std::string(clfilename));

            /* remove .cl extension */
            vector<string> tokens = split(clfilename, '.');
            //DEBUG_PRINT("Plugin Help String %s\n", plugin_info->help);
            for(std::vector<std::string>::iterator it = tokens.begin(); 
                    it != tokens.end(); it++) {
                DEBUG_PRINT("CL File token: %s\n", it->c_str());
            }
            /* saving just the first token to the left of a dot. 
             * Ideal solution should be to concatenate all tokens 
             * except the cl with _ as the glue insted of dot
             */
            g_cl_modules.push_back(tokens[0]);
        }
        else if(strcmp(plugin_info->argv[i].key, "pifgenfile") == 0) {
            DEBUG_PRINT("Plugin Arg %d: (%s, %s)\n", i, plugin_info->argv[i].key, plugin_info->argv[i].value);
            g_output_pifdefs_filename = std::string(plugin_info->argv[i].value);
        }
        else if(strcmp(plugin_info->argv[i].key, "jitcompile") == 0) {
            /* This case already taken care of before the start of this loop */
        }
        else {
            fprintf(stderr, "Unknown plugin argument pair (%s %s).\nAllowed plugin arguments are clfile, jitcompile and pifgenfile.\n", plugin_info->argv[i].key, plugin_info->argv[i].value);
            exit(-1);
        }
    }

    DEBUG_PRINT("In plugin init function\n");
    register_callback (plugin_name, PLUGIN_START_UNIT,
            register_start_unit, NULL);
    register_callback (plugin_name, PLUGIN_FINISH_UNIT,
            register_finish_unit, NULL);
    register_callback (plugin_name, PLUGIN_ATTRIBUTES,
            register_attributes, NULL);
    register_callback (plugin_name, PLUGIN_INCLUDE_FILE,
                  register_headers, NULL);

    register_callback (plugin_name, PLUGIN_FINISH_TYPE,
                  register_finish_type, NULL);
    register_callback (plugin_name, PLUGIN_FINISH_DECL,
                  register_finish_decl, NULL);
#if 0
    register_callback (plugin_name, PLUGIN_PRE_GENERICIZE,
                          handle_pre_generic, NULL);
    register_callback (plugin_name, PLUGIN_START_PARSE_FUNCTION,
                  register_start_parse_function, NULL);
#endif
    return 0;
}


void write_kl_init(const char *pif_name, int pif_index, std::vector<std::string> arg_list, int num_params)
{
        pp_printf((pif_printers[pif_index].pifdefs), "\
static int %s_kl_FK = 0;\n", pif_name);

        /* add init function for dynamic kernel */
        pp_printf((pif_printers[pif_index].pifdefs), "\
extern _CPPSTRING_ void %s_kl_init() {\n\n", pif_name);

        /* add init function for dynamic kernel into kl_init() */
        pp_printf(&kl_init_funs, "\
    %s_kl_init();\n", pif_name);

        pp_printf((pif_printers[pif_index].pifdefs), "\
    if(%s_kl_FK == 0) { \n\
        %s_kl_FK = 1; \n\
    } \n\
    else { \n\
        return; \n\
    }\n\n", pif_name, pif_name);

           pp_printf((pif_printers[pif_index].pifdefs), "\
    typedef struct cpu_args_struct_s {\n");
        int arg_idx;
        for(arg_idx = 0; arg_idx < num_params; arg_idx++) {
            pp_printf((pif_printers[pif_index].pifdefs), "\
        %s arg%d;\n",
            arg_list[arg_idx].c_str(), arg_idx);
        }
        pp_printf((pif_printers[pif_index].pifdefs), "\
    } cpu_args_struct_t; \n\
    void *cpuKernargAddress;\n\
    atmi_mem_place_t cpu_place = ATMI_MEM_PLACE_CPU_MEM(0, 0, 0); \n\
    atmi_malloc(&cpuKernargAddress, sizeof(cpu_args_struct_t) * MAX_NUM_KERNELS, cpu_place); \n\n");


        pp_printf((pif_printers[pif_index].pifdefs), "\
    typedef struct gpu_args_struct_s {\n\
        uint64_t arg0; \n\
        uint64_t arg1; \n\
        uint64_t arg2; \n\
        uint64_t arg3; \n\
        uint64_t arg4; \n\
        uint64_t arg5; \n");
        for(arg_idx = 0; arg_idx < num_params; arg_idx++) {
            pp_printf((pif_printers[pif_index].pifdefs), "\
        %s arg%d;\n", 
            arg_list[arg_idx].c_str(), arg_idx + 6);
        }

        pp_printf((pif_printers[pif_index].pifdefs), "\
    } gpu_args_struct_t __attribute__ ((aligned (16))) ;\n\
    void *gpuKernargAddress; \n\
    atmi_mem_place_t gpu_place = ATMI_MEM_PLACE_CPU_MEM(0, 0, 0); \n\
    atmi_malloc(&gpuKernargAddress, sizeof(gpu_args_struct_t) * MAX_NUM_KERNELS, gpu_place); \n\n");


        pp_printf((pif_printers[pif_index].pifdefs), "\
    const int num_args = %d; \n\
    if (%s_FK == 0 ) { \n\
        size_t arg_sizes[num_args]; \n",
        num_params,
        pif_name
        );
        for(arg_idx = 0; arg_idx < num_params; arg_idx++) {
            pp_printf((pif_printers[pif_index].pifdefs), "\
        arg_sizes[%d] = sizeof(%s);\n",
            arg_idx, arg_list[arg_idx].c_str()
        );
        }
        pp_printf((pif_printers[pif_index].pifdefs), "\
        atmi_kernel_create_empty(&%s_kernel, num_args, arg_sizes); \n\
        int num_impls = sizeof(%s_fn_table)/sizeof(%s_fn_table[0]);\n\
        int idx; \n\
        for(idx = 0; idx < num_impls; idx++) { \n\
            if(%s_fn_table[idx].devtype == ATMI_DEVTYPE_GPU) { \n\
                atmi_kernel_add_gpu_impl(%s_kernel, %s_fn_table[idx].gpu_kernel, idx);\n\
            }\n\
            else if(%s_fn_table[idx].devtype == ATMI_DEVTYPE_CPU) {\n\
                atmi_kernel_add_cpu_impl(%s_kernel, (atmi_generic_fp)(%s_fn_table[idx].cpu_kernel), idx);\n\
            }\n\
        } \n\
        %s_FK = 1;\n\
    }\n\n",
                pif_name, 
                pif_name, pif_name,
                pif_name,
                pif_name, pif_name,
                pif_name,
                pif_name, pif_name,
                pif_name);

        pp_printf((pif_printers[pif_index].pifdefs), "\
    int pif_id = %d;\n\n", pif_index);

        pp_printf((pif_printers[pif_index].pifdefs), "\
    atmi_klist[pif_id].kernel_packets = NULL; \n\
    atmi_klist[pif_id].num_kernel_packets = 0; \n\
    atmi_klist[pif_id].cpu_kernarg_heap = cpuKernargAddress; \n\
    atmi_klist[pif_id].cpu_kernarg_offset = 0;\n\
    atmi_klist[pif_id].gpu_kernarg_heap = gpuKernargAddress; \n\
    atmi_klist[pif_id].gpu_kernarg_offset = 0;\n\
    atmi_klist[pif_id].kernel_packets_heap; \n\
    atmi_mem_place_t packets_place = ATMI_MEM_PLACE_CPU_MEM(0, 0, 0); \n\
    atmi_malloc((void **)&(atmi_klist[pif_id].kernel_packets_heap), sizeof(atmi_kernel_packet_t) * MAX_NUM_KERNELS, packets_place); \n\
    atmi_klist[pif_id].kernel_packets_offset = 0;\n\n");

#if 0
    pp_printf((pif_printers[pif_index].pifdefs), "\
    if(g_cpu_initialized == 0) {\n\
        atmi_init(ATMI_DEVTYPE_CPU); \n\
        g_cpu_initialized = 1;\n\
    }\n\n");
#endif
#if 0
        pp_printf((pif_printers[pif_index].pifdefs), "\
    int i; \n\
    for(i = 0; i < MAX_NUM_KERNELS; i++) { \n\
        cpu_args_struct_t *args = (cpu_args_struct_t *)cpuKernargAddress + i; \n\
        // args->arg0 = {0}; \n");
        for(arg_idx = 1; arg_idx < num_params; arg_idx++) {
            pp_printf((pif_printers[pif_index].pifdefs), "\
        //args->arg%d = (%s*)malloc(sizeof(%s));\n\
        //if(i < 16)printf(\"[%s] %%p\\n\", args->arg%d);\n",
            arg_idx, arg_list[arg_idx].c_str(), arg_list[arg_idx].c_str(),
            arg_list[arg_idx].c_str(), arg_idx
            );
        }

        pp_printf((pif_printers[pif_index].pifdefs), "\
    }\n\n");
        //pp_printf((pif_printers[pif_index].pifdefs), "\
        //printf(\"%%d %%d\\n\", args->arg1, args->arg2);\n");

#endif

        pp_printf((pif_printers[pif_index].pifdefs), "\
    atl_kl_init(atmi_klist, %s_kernel, pif_id);\n\n", pif_name);

        pp_printf((pif_printers[pif_index].pifdefs), "}\n\n");
        
}


void write_kernel_dispatch_routine(FILE *fp) {

fprintf(fp, "\
#include \"hsa_cl.h\" \n\
#include \"atmi.h\" \n\
#include \"atmi_kl.h\" \n\
\n\
enum queue_type{device_queue = 0, soft_queue}; \n\
\n\
uint64_t get_atmi_context();\n\n\
uint16_t create_header(hsa_packet_type_t type, int barrier) {\n\
   uint16_t header = type << HSA_PACKET_HEADER_TYPE;\n\
   header |= barrier << HSA_PACKET_HEADER_BARRIER;\n\
   header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE;\n\
   header |= HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE;\n\
   return header;\n\
}\n\n");

//fprintf(fp, "\
//void memcpy(void *dest, void *source, unsigned long num) \n\
//{\n\
    //int i; \n\
    //for(i = 0; i < num; i++) \n\
    //{ \n\
        //((unsigned char *)dest)[i] = ((unsigned char *)source)[i]; \n\
    //} \n\
//}\n\n");

fprintf(fp, "\
void kernel_dispatch(const atmi_klparm_t *lparm, hsa_kernel_dispatch_packet_t *kernel_packet, const int pif_id) { \n\
 \n\
    atmi_klist_t *atmi_klist = (atmi_klist_t *)get_atmi_context();\n\
    //hsa_kernel_dispatch_packet_t *kernel_packet = (hsa_kernel_dispatch_packet_t *)(atmi_klist[pif_id].kernel_packets + k_id); \n\
 \n\
    //int q_offset = hsa_atomic_add_system((__global int *)(&(atmi_klist[pif_id].gpu_queue_offset)), 1); \n\
    //hsa_queue_t* this_Q = (hsa_queue_t *)atmi_klist[pif_id].gpu_queues[q_offset %% atmi_klist[pif_id].num_gpu_queues]; \n\
    hsa_queue_t* this_Q = (hsa_queue_t *)atmi_klist[pif_id].queues[device_queue]; \n\
 \n\
    /* Find the queue index address to write the packet info into.  */ \n\
    const uint32_t queueMask = this_Q->size - 1; \n\
    uint64_t index = hsa_queue_add_write_index_relaxed(this_Q, 1); \n\
    hsa_kernel_dispatch_packet_t *this_aql = &(((hsa_kernel_dispatch_packet_t *)(this_Q->base_address))[index&queueMask]); \n\
 \n\n\
    int ndim = -1;\n\
    if(lparm->gridDim[2] > 1)\n\
        ndim = 3;\n\
    else if(lparm->gridDim[1] > 1)\n\
        ndim = 2;\n\
    else\n\
        ndim = 1;\n\
\n\
    /*  Process lparm values */ \n\
    this_aql->setup  |= (uint16_t) ndim << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS; \n\
    this_aql->grid_size_x=lparm->gridDim[0]; \n\
    this_aql->workgroup_size_x=lparm->groupDim[0]; \n\
    if (ndim>1) { \n\
        this_aql->grid_size_y=lparm->gridDim[1]; \n\
        this_aql->workgroup_size_y=lparm->groupDim[1]; \n\
    } else { \n\
        this_aql->grid_size_y=1; \n\
        this_aql->workgroup_size_y=1; \n\
    } \n\
 \n\
    if (ndim>2) { \n\
        this_aql->grid_size_z=lparm->gridDim[2]; \n\
        this_aql->workgroup_size_z=lparm->groupDim[2]; \n\
    } \n\
    else \n\
    { \n\
        this_aql->grid_size_z=1; \n\
        this_aql->workgroup_size_z=1; \n\
    } \n\
 \n\
    /* thisKernargAddress has already been set up in the beginning of this routine */ \n\
    /*  Bind kernel argument buffer to the aql packet.  */ \n\
    this_aql->kernarg_address = kernel_packet->kernarg_address; \n\
    this_aql->kernel_object = kernel_packet->kernel_object; \n\
    this_aql->private_segment_size = kernel_packet->private_segment_size; \n\
    this_aql->group_segment_size = kernel_packet->group_segment_size; \n\
    this_aql->completion_signal = kernel_packet->completion_signal; \n\n\
    hsa_signal_add_relaxed(this_aql->completion_signal, 1); \n\
 \n\
    /*  Prepare and set the packet header */  \n\
    /* Only set barrier bit if asynchrnous execution */ \n\
    this_aql->header = create_header(HSA_PACKET_TYPE_KERNEL_DISPATCH, ATMI_FALSE); \n\
 \n\
    /* Increment write index and ring doorbell to dispatch the kernel.  */ \n\
    hsa_signal_store_relaxed(this_Q->doorbell_signal, atmi_klist[pif_id].gpu_kernarg_offset); \n\
    //hsa_signal_add_relaxed(this_Q->doorbell_signal, 1); \n\
}\n\n\n");

fprintf(fp,"\
void agent_dispatch(const atmi_klparm_t *lparm, hsa_agent_dispatch_packet_t *kernel_packet, const int pif_id) { \n\
 \n\
    atmi_klist_t *atmi_klist = (atmi_klist_t *)get_atmi_context();\n\
    //hsa_agent_dispatch_packet_t *kernel_packet = (hsa_agent_dispatch_packet_t *)(atmi_klist[pif_id].kernel_packets + k_id); \n\
    //hsa_agent_dispatch_packet_t *kernel_packet = this_packet; \n\
 \n\
    //int q_offset = hsa_atomic_add_system((__global int *)(&(atmi_klist[pif_id].cpu_queue_offset)), 1); \n\
    //hsa_queue_t* this_Q = (hsa_queue_t *)atmi_klist[pif_id].cpu_queues[q_offset %% atmi_klist[pif_id].num_cpu_queues]; \n\
    hsa_queue_t* this_Q = (hsa_queue_t *)atmi_klist[pif_id].queues[soft_queue]; \n\
    hsa_signal_t worker_sig = *((hsa_signal_t *)atmi_klist[pif_id].worker_sig); \n\
 \n\
    /* Find the queue index address to write the packet info into.  */ \n\
    const uint32_t queueMask = this_Q->size - 1; \n\
    uint64_t index = hsa_queue_add_write_index_relaxed(this_Q, 1); \n\
    hsa_agent_dispatch_packet_t *this_aql = \n\
        &(((hsa_agent_dispatch_packet_t *)(this_Q->base_address))[index&queueMask]); \n\
    \n\
    this_aql->type   = kernel_packet->type; \n\
    this_aql->arg[0] = kernel_packet->arg[0]; \n\
    this_aql->arg[1] = kernel_packet->arg[1]; \n\
    this_aql->arg[2] = kernel_packet->arg[2]; \n\
    this_aql->arg[3] = kernel_packet->arg[3]; \n\
    this_aql->completion_signal = kernel_packet->completion_signal; \n\n\
    hsa_signal_add_relaxed(this_aql->completion_signal, 1); \n\
 \n\
    /*  Prepare and set the packet header */  \n\
    /* Only set barrier bit if asynchrnous execution */ \n\
    this_aql->header = create_header(HSA_PACKET_TYPE_AGENT_DISPATCH, ATMI_FALSE); \n\
 \n\
    /* Increment write index and ring doorbell to dispatch the kernel.  */ \n\
    //hsa_signal_store_relaxed(this_Q->doorbell_signal, atmi_klist[pif_id].cpu_kernarg_offset); \n\
    hsa_signal_add_relaxed(this_Q->doorbell_signal, 1); \n\
 \n\
    hsa_signal_store_relaxed(worker_sig, 0); \n\
}\n\n\n");

}

/* add spawn function of pif for dynamic dispatch */
void write_spawn_function(const char *pif_name, int pif_index, std::vector<std::string> arg_list, int num_params)
{
    int arg_idx; 


    pp_printf(&pif_spawn, "\
atmi_task_handle_t %s(atmi_klparm_t *lparm ", pif_name);
    
    for(arg_idx = 0; arg_idx < num_params; arg_idx++) {
        pp_printf(&pif_spawn, ", %s var%d", arg_list[arg_idx].c_str(), arg_idx);
    }

    pp_printf(&pif_spawn, ") {\n\n");

    pp_printf(&pif_spawn, "\
    int pif_id = %d;\n", pif_index); 

    pp_printf(&pif_spawn, "\
    atmi_klist_t *atmi_klist = (atmi_klist_t *)get_atmi_context();\n\
    uint16_t *atmi_packet_header = (uint16_t *)(atmi_klist[pif_id].kernel_packets + lparm->kernel_id);\n\
    atmi_task_impl_t **task_ptr = (atmi_task_impl_t **)(atmi_klist->tasks); \n\
    int id = 0xFFFFFFFF & lparm->prevTask; \n\
    atmi_task_impl_t *task = task_ptr[id]; \n\
    if(atmi_packet_header[0] == 0){\n\
        hsa_kernel_dispatch_packet_t * kernel_packet = (hsa_kernel_dispatch_packet_t *)(atmi_klist[pif_id].kernel_packets + lparm->kernel_id); \n\n");

    pp_printf(&pif_spawn, "\
        struct gpu_args_struct {\n\
            uint64_t arg0; \n\
            uint64_t arg1; \n\
            uint64_t arg2; \n\
            uint64_t arg3; \n\
            uint64_t arg4; \n\
            uint64_t arg5; \n");
        for(arg_idx = 0; arg_idx < num_params; arg_idx++) {
            pp_printf(&pif_spawn, "\
            %s arg%d;\n", 
            arg_list[arg_idx].c_str(), arg_idx + 6);
        }
            
    pp_printf(&pif_spawn, "\
        } __attribute__ ((aligned (16))) ;\n\n");

    pp_printf(&pif_spawn, "\
        hsa_kernel_dispatch_packet_t this_packet; \n\
        this_packet.header = 0;\n\
        this_packet.kernel_object = kernel_packet->kernel_object;\n\
        this_packet.private_segment_size = kernel_packet->private_segment_size;\n\
        this_packet.group_segment_size = kernel_packet->group_segment_size;\n\
        int kernarg_offset = hsa_atomic_add_system((__global int *)(&(atmi_klist[pif_id].gpu_kernarg_offset)), 1);\n\
        this_packet.kernarg_address =  (void *)((struct gpu_args_struct *)atmi_klist[pif_id].gpu_kernarg_heap + kernarg_offset);\n\
        struct gpu_args_struct * gpu_args = (struct gpu_args_struct *)this_packet.kernarg_address; \n\
        this_packet.completion_signal = task->signal; \n\n");


    pp_printf(&pif_spawn, "\
        gpu_args->arg0=0;\n\
        gpu_args->arg1=0;\n\
        gpu_args->arg2=0;\n\
        gpu_args->arg3=(uint64_t)atmi_klist;\n\
        gpu_args->arg4=0;\n\
        gpu_args->arg5=0;\n");

    for(arg_idx = 0; arg_idx < num_params; arg_idx++) {
        pp_printf((&pif_spawn), "\
        gpu_args->arg%d = var%d;\n", arg_idx + 6, arg_idx);
    }

    pp_printf(&pif_spawn, "\
        kernel_dispatch(lparm, &this_packet, pif_id);\n\
    }\n");

    pp_printf(&pif_spawn, "\
    else{\n\n");
    pp_printf(&pif_spawn, "\
        hsa_agent_dispatch_packet_t * kernel_packet = (hsa_agent_dispatch_packet_t *)(atmi_klist[pif_id].kernel_packets + lparm->kernel_id); \n\n");

    pp_printf(&pif_spawn, "\
        struct cpu_args_struct {\n\
            ");
    for(arg_idx = 0; arg_idx < num_params; arg_idx++) {
        if(arg_list[arg_idx].c_str()[arg_list[arg_idx].size() - 1] == '*')
        {
            pp_printf(&pif_spawn, "\
            uint64_t arg%d;\n",
            arg_idx);

        }
        else
        {
            pp_printf(&pif_spawn, "\
            %s arg%d;\n",
            arg_list[arg_idx].c_str(), arg_idx);
        }

    }
    pp_printf(&pif_spawn, "\
        }; \n\n");


    pp_printf(&pif_spawn, "\
        int kernarg_offset = hsa_atomic_add_system((__global int *)(&(atmi_klist[pif_id].cpu_kernarg_offset)), 1);\n\
        hsa_agent_dispatch_packet_t this_packet; \n\
        this_packet.header = 1;\n\
        this_packet.type   = kernel_packet->type;\n\
        this_packet.arg[0] = (uint64_t)task;\n\
        this_packet.arg[1] = (uint64_t)((struct cpu_args_struct *)atmi_klist[pif_id].cpu_kernarg_heap + kernarg_offset);\n\
        this_packet.arg[2] = kernel_packet->arg[2]; \n\
        this_packet.arg[3] = kernel_packet->arg[3];\n\
        struct cpu_args_struct *cpu_args = (struct cpu_args_struct *)(this_packet.arg[1]);\n"); 

    //pp_printf(&pif_spawn, "\
    //    cpu_args->arg0 = NULL;\n");

    for(arg_idx = 1; arg_idx < num_params; arg_idx++) {
        if(arg_list[arg_idx].c_str()[arg_list[arg_idx].size() - 1] == '*')
        {
            pp_printf((&pif_spawn), "\
        cpu_args->arg%d = (uint64_t)var%d;\n", 
                    arg_idx, arg_idx);

        }
        else
        {
            pp_printf((&pif_spawn), "\
        cpu_args->arg%d = var%d;\n", 
                    arg_idx, arg_idx);

        }
    }

    pp_printf(&pif_spawn, "\
        this_packet.completion_signal = task->signal;\n");

    pp_printf(&pif_spawn, "\
        agent_dispatch(lparm, &this_packet, pif_id);\n\
    }\n");

    pp_printf(&pif_spawn, "\
    return lparm->prevTask; \n\
}\n\n");

}

void write_pif_kl(FILE *clFile) {
    write_kernel_dispatch_routine(clFile);
    char *cl_text = (char *)pp_formatted_text(&pif_spawn);
    typedef std::map<std::string, std::pair<int,int> > map_type;
    for(map_type::iterator it = pif_table.begin(); it != pif_table.end(); it++) {
        std::string pif_name = it->first;
        int i = it->second.first;
        char *enum_kid_text = (char *)pp_formatted_text(pif_printers[i].kid_table);
        fprintf(clFile, "enum %s_kid_klist{%s};\n", pif_name.c_str(), enum_kid_text);
        pp_clear_output_area(pif_printers[i].kid_table);
    }
    fprintf(clFile, "%s", cl_text);
    pp_clear_output_area(&pif_spawn);
}

void append_kl_init_funs(FILE *pifFile)
{
    char *kl_init_funs_text = (char *)pp_formatted_text(&kl_init_funs);

    fprintf(pifFile, "%s", "\
void kl_init() {\n");
    fprintf(pifFile, "\
    atmi_mem_place_t place = ATMI_MEM_PLACE_CPU_MEM(0, 0, 0); \n\
    atmi_malloc((void **)&atmi_klist, (sizeof(atmi_klist_t) * 1000), place);\n");
    fprintf(pifFile, "%s", kl_init_funs_text);

    fprintf(pifFile, "%s", "\
}\n\n");

    pp_clear_output_area(&kl_init_funs);

}
