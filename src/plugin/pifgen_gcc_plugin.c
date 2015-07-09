
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
#include <c-family/c-pragma.h>

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

static std::vector<std::string> g_cl_modules;

static std::map<std::string, int> pif_table;
typedef struct pif_printers_s {
   pretty_printer *pifdefs; 
   pretty_printer *fn_table; 
} pif_printers_t;
static std::vector<pif_printers_t> pif_printers;

static pretty_printer g_kerneldecls;

static int initialized = 0;

static std::string g_output_pifdefs_filename;

void write_headers(FILE *fp) {
    fprintf(fp, "\
#include \"atmi.h\"\n\
#include \"atmi_rt.h\"\n\n");
}

void write_cpp_warning_header(FILE *fp) {
fprintf(fp, "#ifdef __cplusplus \n\
#define _CPPSTRING_ \"C\" \n\
#endif \n\
#ifndef __cplusplus \n\
#define _CPPSTRING_ \n\
#endif \n\n\
static hsa_executable_t g_executable;\n\
static int klist_initalized = 0;\n\
static int gpu_initalized = 0;\n\
static int cpu_initalized = 0;\n\n\
atmi_klist_t *atmi_klist = NULL;\n\n\
");
}

std::string exec(const char* cmd) {
    /* borrowed from the SO solution: 
     * http://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c
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

void brig2brigh(const char *brigfilename, const char *cl_module_name) {
    std::string cmd = exec("which hexdump");
    
    if(cmd == "" || cmd == "ERROR") {
        fprintf(stderr, "hexdump command is needed for converting brig to a string array, and it is not found in the PATH.\n");
        exit(-1);
    }
    std::string outfile(cl_module_name);
    outfile += "_brig.h"; 
    FILE *fp_brigh = fopen(outfile.c_str(), "w");
    fprintf(fp_brigh, "char %s_HSA_BrigMem[] = {\n", cl_module_name); 

    std::string hex_cmd("hexdump -v -e '\"0x\" 1/1 \"%02X\" \",\"' ");
    hex_cmd += std::string(brigfilename);
    std::string hex_str = exec(hex_cmd.c_str());
    if(hex_str == "" || hex_str == "ERROR") {
        fprintf(stderr, "hexdump on brig file %s failed.\n", brigfilename);
        exit(-1);
    }
    
    fprintf(fp_brigh, "%s};\n", hex_str.c_str());
    fprintf(fp_brigh, "size_t %s_HSA_BrigMemSz = sizeof(%s_HSA_BrigMem);\n", cl_module_name, cl_module_name); 
    fclose(fp_brigh);
}

void cl2brigh(const char *clfilename) {
    std::string cmd;
    cmd.clear();
    cmd += exec("which cl2brigh.sh");
    
    if(cmd == "" || cmd == "ERROR") {
        fprintf(stderr, "cl2brigh.sh script not found in the PATH.\n");
        exit(-1);
    }
    char cmd_c[2048] = {0};
    strcpy(cmd_c, cmd.c_str());
    // popen returns a newline. remove it.
    strtok(cmd_c, "\n");
    strcat(cmd_c, " ");
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
    if(num_params == 1) {
        strcat(fn_decl, "(atmi_task_t **var0)");
    }
    else if(num_params > 1) {
        strcat(fn_decl, "(atmi_task_t **var0, ");
    }
    
    int var_idx = 0;
    pch = strtok (NULL, ",)");
    //DEBUG_PRINT("Parsing but ignoring this string now: %s\n", pch);
    for(var_idx = 1; var_idx < num_params; var_idx++) {
        pch = strtok (NULL, ",)");
    //    DEBUG_PRINT("Parsing this string now: %s\n", pch);
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
    if(num_params == 1) {
        strcat(fn_decl, "(atmi_task_t *var0)");
    }
    else if(num_params > 1) {
        strcat(fn_decl, "(atmi_task_t *var0, ");
    }
    
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
   
    if(num_params == 1) {
        strcat(fn_decl, "(atmi_lparm_t *lparm)");
    }
    else if(num_params > 1) {
        strcat(fn_decl, "(atmi_lparm_t *lparm, ");
    }

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
        pp_printf((pif_printers[pif_index].pifdefs), "extern _CPPSTRING_ %s { \n", pif_decl);
        pp_printf((pif_printers[pif_index].pifdefs), "    /* launcher code (PIF definition) */\n");
        pp_printf((pif_printers[pif_index].pifdefs), "\
    if (%s_FK == 0 ) { \n\
        snk_pif_init(%s_pif_fn_table, sizeof(%s_pif_fn_table)/sizeof(%s_pif_fn_table[0]));\n\
        %s_FK = 1; \n\
    }\n",
                pif_name,
                pif_name, pif_name, pif_name,
                pif_name);
        pp_printf((pif_printers[pif_index].pifdefs), "\
    int k_id = lparm->kernel_id; \n\
    if(k_id < 0 || k_id >= sizeof(%s_pif_fn_table)/sizeof(%s_pif_fn_table[0])) { \n\
        fprintf(stderr, \"Kernel_id out of bounds for PIF %s.\\n\"); \n\
        return NULL; \n\
    } \n\
    atmi_devtype_t devtype = %s_pif_fn_table[k_id].devtype; \n\
    if(devtype == ATMI_DEVTYPE_CPU) {\n\
        if(cpu_initalized == 0) { \n\
            snk_init_cpu_context();\n\
            cpu_initalized = 1;\n\
        }\n",
        pif_name, pif_name,
        pif_name,
        pif_name);
        int arg_idx;
#if 1
        pp_printf((pif_printers[pif_index].pifdefs), "\
        typedef struct cpu_args_struct_s {\n\
            size_t arg0_size;\n\
            atmi_task_t **arg0;\n");
        for(arg_idx = 1; arg_idx < num_params; arg_idx++) {
            pp_printf((pif_printers[pif_index].pifdefs), "\
            size_t arg%d_size;\n\
            %s* arg%d;\n",
            arg_idx,
            arg_list[arg_idx].c_str(), arg_idx);
        }
        pp_printf((pif_printers[pif_index].pifdefs), "\
        } cpu_args_struct_t; \n\
        void *thisKernargAddress = malloc(sizeof(cpu_args_struct_t));\n\
        cpu_args_struct_t *args = (cpu_args_struct_t *)thisKernargAddress; \n\
        args->arg0_size = sizeof(atmi_task_t **);\n\
        args->arg0 = NULL; \
        ");
        for(arg_idx = 1; arg_idx < num_params; arg_idx++) {
            pp_printf((pif_printers[pif_index].pifdefs), "\n\
        args->arg%d_size = sizeof(%s*); \n\
        args->arg%d = (%s*)malloc(sizeof(%s));\n\
        memcpy(args->arg%d, &var%d, sizeof(%s));\
        ",
            arg_idx, arg_list[arg_idx].c_str(),
            arg_idx, arg_list[arg_idx].c_str(), arg_list[arg_idx].c_str(),
            arg_idx, arg_idx, arg_list[arg_idx].c_str()
            );
        }
#else
        pp_printf((pif_printers[pif_index].pifdefs), "\n\n\
        snk_kernel_args_t *cpu_kernel_arg_list = (snk_kernel_args_t *)malloc(sizeof(snk_kernel_args_t)); \n\
        cpu_kernel_arg_list->args[0] = (uint64_t)NULL; \
                ");
        for(arg_idx = 1; arg_idx < num_params; arg_idx++) {
            pp_printf((pif_printers[pif_index].pifdefs), "\n\
        cpu_kernel_arg_list->args[%d] = (uint64_t)var%d;", arg_idx, arg_idx);
        }
#endif
        pp_printf((pif_printers[pif_index].pifdefs), "\n\
        return snk_cpu_kernel(lparm, \n\
                    \"%s\", \n\
                    thisKernargAddress); \
                ", pif_name);
        pp_printf((pif_printers[pif_index].pifdefs), "\n\
    } \n\
    else if(devtype == ATMI_DEVTYPE_GPU) {\n\
        if(gpu_initalized == 0) {\n\
            snk_init_gpu_context();\n\
            snk_gpu_create_program();\n");
        
        for(std::vector<std::string>::iterator it = g_cl_modules.begin(); 
                it != g_cl_modules.end(); it++) {
            pp_printf((pif_printers[pif_index].pifdefs), "\
            snk_gpu_add_brig_module(%s_HSA_BrigMem); \n", it->c_str());
        }
        pp_printf((pif_printers[pif_index].pifdefs), "\
            snk_gpu_build_executable(&g_executable);\n\
            gpu_initalized = 1;\n\
        }\n\
        /* Allocate the kernel argument buffer from the correct region. */\n\
        void* thisKernargAddress;\n\
        snk_gpu_memory_allocate(lparm, g_executable, \"%s\", &thisKernargAddress);\n", pif_name);
        pp_printf((pif_printers[pif_index].pifdefs), "\
        struct gpu_args_struct {\n\
            uint64_t arg0;\n\
            uint64_t arg1;\n\
            uint64_t arg2;\n\
            uint64_t arg3;\n\
            uint64_t arg4;\n\
            uint64_t arg5;\n\
            atmi_task_t* arg6;\n");
        for(arg_idx = 1; arg_idx < num_params; arg_idx++) {
            pp_printf((pif_printers[pif_index].pifdefs), "\
            uint64_t arg%d;\n", arg_idx+6);
        }
        pp_printf((pif_printers[pif_index].pifdefs), "\
        } __attribute__ ((aligned (16))); \n\
        struct gpu_args_struct* gpu_args;\n\
        /* Setup kernel args */\n\
        gpu_args = (struct gpu_args_struct*) thisKernargAddress;\n\
        gpu_args->arg0=0;\n\
        gpu_args->arg1=0;\n\
        gpu_args->arg2=0;\n\
        gpu_args->arg3=0;\n\
        gpu_args->arg4=0;\n\
        gpu_args->arg5=0;\n\
        gpu_args->arg6=(uint64_t)NULL;\n");
        for(arg_idx = 1; arg_idx < num_params; arg_idx++) {
            pp_printf((pif_printers[pif_index].pifdefs), "\
        gpu_args->arg%d=(uint64_t)var%d;\n", arg_idx+6, arg_idx);
        }
        pp_printf((pif_printers[pif_index].pifdefs), "\
        return snk_gpu_kernel(lparm,\n\
                            g_executable,\n\
                            \"%s\",\n\
                            thisKernargAddress);\n", pif_name);

        pp_printf((pif_printers[pif_index].pifdefs), "\
    }\n");
        pp_printf((pif_printers[pif_index].pifdefs), "\
}\n\n");
        
        /* add PIF function table definition */
        pp_printf((pif_printers[pif_index].fn_table), "\nsnk_pif_kernel_table_t %s_pif_fn_table[] = {\n", pif_name);
    }
    if(devtype == ATMI_DEVTYPE_CPU) {
        pp_printf(&g_kerneldecls, "extern _CPPSTRING_ %s\n", fn_decl);
        pp_printf(&g_kerneldecls, "extern _CPPSTRING_ %s {\n", fn_cpu_wrapper_decl);
        pp_printf(&g_kerneldecls, "\
    %s(*var0",
        fn_name);
        int arg_idx;
        for(arg_idx = 1; arg_idx < num_params; arg_idx++) {
            pp_printf(&g_kerneldecls, ", *var%d", arg_idx);
        }
        pp_printf(&g_kerneldecls, ");\n}\n");
        pp_printf((pif_printers[pif_index].fn_table), "\
    {.pif_name=\"%s\",.devtype=ATMI_DEVTYPE_CPU,.num_params=%d,.cpu_kernel={.kernel_name=\"%s_wrapper\",.function=(snk_generic_fp)%s_wrapper},.gpu_kernel={.kernel_name=NULL}},\n",
            pif_name, num_params, fn_name, fn_name);
    } 
    else if(devtype == ATMI_DEVTYPE_GPU) {
        pp_printf((pif_printers[pif_index].fn_table), "\
    {.pif_name=\"%s\",.devtype=ATMI_DEVTYPE_GPU,.num_params=%d,.cpu_kernel={.kernel_name=NULL,.function=(snk_generic_fp)NULL},.gpu_kernel={.kernel_name=\"&__OpenCL_%s_kernel\"}},\n",
            pif_name, num_params, fn_name);
    }
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
register_headers (void *event_data, void *data)
{
    //warning (0, G_("Callback to register header files"));
    DEBUG_PRINT("Done registering header files: %s \n", (const char *)event_data);
    if(cpp_included(parse_in, (const char *)event_data)) {
        DEBUG_PRINT("Header file %s is included\n", (const char *)event_data);
    }
}

void kl_unit (FILE *fp_pifdefs_genw) {
    FILE *fp_kl_genw = fopen("kernel_wrapper.c", "rb");

    if(fp_pifdefs_genw == NULL || fp_kl_genw == NULL)
    {
        fprintf(stderr, "Err: cannot open file %s\n", main_input_filename);
        exit(-1);
    }

    char buffer[4097];


    int n;
    while((n = fread(buffer, sizeof(char), 4096, fp_kl_genw)))
    {
        buffer[n] = '\0';
        fprintf(fp_pifdefs_genw, "%s", buffer);
    }

    fclose(fp_kl_genw);
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
    /* 1) dump kernel impl declarations (fn pointers */
    for(std::vector<std::string>::iterator it = g_cl_modules.begin(); 
                it != g_cl_modules.end(); it++) {
        fprintf(fp_pifdefs_genw, "#include \"%s_brig.h\"\n", it->c_str());
    }
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

    kl_unit(fp_pifdefs_genw);
    fclose(fp_pifdefs_genw);
}

static void
register_start_unit (void *event_data, void *data) {
    DEBUG_PRINT("Callback at start of compilation unit\n");
    pp_needs_newline (&g_kerneldecls) = true;

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
#if 0
static void
register_finish_type(void *event_data, void *data) {
    tree type = (tree)event_data;
    if(TREE_CODE(type) == FUNCTION_DECL || 
       TREE_CODE(type) == VAR_DECL || 
       TREE_CODE(type) == TYPE_DECL) {
        DEBUG_PRINT("Callback at finish of type %p %p\n", event_data, data);
        debug_tree_chain(type);
    }
}

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
    if (t == 0)
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
            code            !=            ADDR_EXPR            && 
            code            !=            INDIRECT_REF            &&
            code            !=            COMPONENT_REF)
        parse_tree(TREE_OPERAND(t,                    1),               callback,                indent+1);

}

static void handle_pre_generic(void *gcc_data, void *user_data)
{
    // Print AST
    tree fndecl = (tree)gcc_data;

    if(
       //TREE_CODE(fndecl) == FUNCTION_DECL || 
       TREE_CODE(fndecl) == VAR_DECL || 
       TREE_CODE(fndecl) == TYPE_DECL) {
        tree id = DECL_NAME(fndecl);
        const char *fnname = id ? IDENTIFIER_POINTER(id) : "<unnamed>";
        printf("%s %s\n", get_tree_code_name(FUNCTION_DECL), fnname);

        // Print function body..
        tree fnbody = DECL_SAVED_TREE(fndecl);
        if (TREE_CODE(fnbody) == BIND_EXPR) {
            // second operand of BIND_EXPR
            tree t = TREE_OPERAND(fnbody, 1);

            // use the utility function "parse_tree" to parse
            // through the tree recursively  (../include/parse-tree.h)
            parse_tree(t, print_tree_node, 1);
        }
    }                                                                                                                
}
#endif

int plugin_init(struct plugin_name_args *plugin_info,
        struct plugin_gcc_version *version) {
    if (!plugin_default_version_check (version, &gcc_version))
        return 1;

    int i;
    g_output_pifdefs_filename.clear();
    g_cl_modules.clear();
    for(i = 0; i < plugin_info->argc; i++) { 
        if(strcmp(plugin_info->argv[i].key, "clfile") == 0) {
            DEBUG_PRINT("Plugin Arg %d: (%s, %s)\n", i, plugin_info->argv[i].key, plugin_info->argv[i].value);
            const char *clfilename = plugin_info->argv[i].value;
            /* compile CL to BRIG header file with char array */
            cl2brigh(clfilename);

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
        else if(strcmp(plugin_info->argv[i].key, "brigfile") == 0) {
            DEBUG_PRINT("Plugin Arg %d: (%s, %s)\n", i, plugin_info->argv[i].key, plugin_info->argv[i].value);
            const char *brigfilename = plugin_info->argv[i].value;

            /* remove ._brig.h */
            vector<string> tokens = split(brigfilename, '.');
            //DEBUG_PRINT("Plugin Help String %s\n", plugin_info->help);
            for(std::vector<std::string>::iterator it = tokens.begin(); 
                    it != tokens.end(); it++) {
                DEBUG_PRINT("Brig File token: %s\n", it->c_str());
            }
            /* saving just the first token to the left of a dot. 
             * Ideal solution should be to concatenate all tokens 
             * except the cl with _ as the glue insted of dot
             */
            g_cl_modules.push_back(tokens[0]);
            
            /* compile CL to BRIG header file with char array */
            brig2brigh(brigfilename, tokens[0].c_str());
        }        
        else if(strcmp(plugin_info->argv[i].key, "brighfile") == 0) {
            DEBUG_PRINT("Plugin Arg %d: (%s, %s)\n", i, plugin_info->argv[i].key, plugin_info->argv[i].value);
            const char *brighfilename = plugin_info->argv[i].value;

            /* remove ._brig.h */
            vector<string> tokens = split(brighfilename, '_');
            //DEBUG_PRINT("Plugin Help String %s\n", plugin_info->help);
            for(std::vector<std::string>::iterator it = tokens.begin(); 
                    it != tokens.end(); it++) {
                DEBUG_PRINT("Brig H File token: %s\n", it->c_str());
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
        else {
            fprintf(stderr, "Unknown plugin argument pair (%s %s).\nAllowed plugin arguments are clfile, brighfile and pifgenfile.\n", plugin_info->argv[i].key, plugin_info->argv[i].value);
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
#if 0
    register_callback (plugin_name, PLUGIN_FINISH_TYPE,
                  register_finish_type, NULL);
    register_callback (plugin_name, PLUGIN_PRE_GENERICIZE,
                          handle_pre_generic, NULL);
#endif
    return 0;
}

