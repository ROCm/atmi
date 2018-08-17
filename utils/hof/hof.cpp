/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "hsa.h"
#include "hsa_ext_finalize.h"
/*  set NOTCOHERENT needs this include
#include "hsa_ext_amd.h"
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <argp.h>
#include <iostream>
#include <fstream>
/* -------------- Helper functions -------------------------- */
#define ErrorCheck(msg, status) \
if (status != HSA_STATUS_SUCCESS) { \
    printf("%s failed. 0x%x\n", #msg, status); \
    /*exit(1); */\
} else { \
 /*  printf("%s succeeded.\n", #msg);*/ \
}

const char *argp_program_version =
"HSA Offline Finalizer";
const char *argp_program_bug_address =
"<ashwin.aji@amd.com>";

/* Program documentation. */
static char doc[] =
"HSA Offline Finalizer -- a tool to finalize BRIG/HSAIL modules to machine binary.";

/* A description of the arguments we accept. */
static char args_doc[] = "";
//static char args_doc[] = "brig hsail output";
#define ARGS_COUNT 3
/* The options we understand. */
static struct argp_option options[] = {
    {"brig",  'b', "<BRIG File>", 0, "BRIG File" },
    {"hsail",    'h', "<HSAIL File>", 0, "HSAIL File" },
    {"output",   'o', "<Output File>", 0, "Output file (Default: a.o)" },
    { 0 }
};

/* Used by main to communicate with parse_opt. */
typedef struct arguments_s
{
    //char *args[ARGS_COUNT];                
    const char *brig_file;
    const char *hsail_file;
    const char *output_file;
}arguments_t;

/* Parse a single option. */
    static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
     *      know is a pointer to our arguments structure. */
    arguments_t *arguments = (arguments_t *)(state->input);

    //if(arg && state) 
    //    std::cout << "Arg Key: " << (char)key << " Arg: " << arg << " Arg_num: "<< state->arg_num << std::endl;
    switch (key)
    {
        case 'b': 
            arguments->brig_file = arg;
            break;
        case 'h':
            arguments->hsail_file = arg;
            break;
        case 'o':
            arguments->output_file = arg;
            break;

        case ARGP_KEY_ARG:
            //std::cout << "Arg Number: " << state->arg_num << std::endl;
            //if (state->arg_num >= 2)
                /* Too many arguments. */
            //    argp_usage (state);

            //arguments->args[state->arg_num] = arg;

            break;

        case ARGP_KEY_END:
            //std::cout << "End Arg Number: " << state->arg_num << std::endl;
            //if (state->arg_num < 2)
                /* Not enough arguments. */
            //    argp_usage (state);
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* Our argp parser.*/
static struct argp argp = { options, parse_opt, args_doc, doc };

/*
 * Loads a BRIG module from a specified file. This
 * function does not validate the module.
 */
int load_module_from_file(const char* file_name, hsa_ext_module_t* module) {
#if 0
    int rc = -1;

    FILE *fp = fopen(file_name, "rb");

    rc = fseek(fp, 0, SEEK_END);
    if(ferror(fp)) printf("Seek error\n");

    size_t file_size = (size_t) (ftell(fp) * sizeof(char));

    rc = fseek(fp, 0, SEEK_SET);
    if(ferror(fp)) printf("Seek error\n");

    char* buf = (char*) malloc(file_size);

    memset(buf,0,file_size);

    size_t read_size = fread(buf,sizeof(char),file_size,fp);
    if(ferror(fp)) printf("Fread error\n");

    if(read_size != file_size) {
        free(buf);
    } else {
        rc = 0;
        *module = (hsa_ext_module_t) buf;
    }

    fclose(fp);

    return rc;
#else
    // Open file.
    std::ifstream file(file_name, std::ios::in | std::ios::binary);
    assert(file.is_open() && file.good());

    // Find out file size.
    file.seekg(0, file.end);
    size_t size = file.tellg();
    file.seekg(0, file.beg);

    // Allocate memory for raw code object.
    void *raw_code_object = malloc(size);
    assert(raw_code_object);

    // Read file contents.
    file.read((char*)raw_code_object, size);

    // Close file.
    file.close();
    *module = (hsa_ext_module_t) raw_code_object;
    return 0;
#endif
}

/*
 * Determines if the given agent is of type HSA_DEVICE_TYPE_GPU
 * and sets the value of data to the agent handle if it is.
 */
static hsa_status_t get_gpu_agent(hsa_agent_t agent, void *data) {
    hsa_status_t status;
    hsa_device_type_t device_type;
    status = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
    if (HSA_STATUS_SUCCESS == status && HSA_DEVICE_TYPE_GPU == device_type) {
        hsa_agent_t* ret = (hsa_agent_t*)data;
        *ret = agent;
        return HSA_STATUS_INFO_BREAK;
    }
    return HSA_STATUS_SUCCESS;
}

hsa_status_t serialize_alloc(size_t size, hsa_callback_data_t data, void **address) {
    *address = malloc(size);
    return HSA_STATUS_SUCCESS;
}

int
main (int argc, char **argv)
{
    arguments_t arguments;

    /* Default values.*/
    arguments.brig_file = "-";
    arguments.hsail_file = "-";
    arguments.output_file = "-";

    /* Parse our arguments; every option seen by parse_opt will be
     *      reflected in arguments. */
    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    hsa_status_t err;
    err = hsa_init();
    ErrorCheck(HSA Init, err);
    /*
    printf ("Brig File = %s\nHSAIL File = %s\nOUTPUT_FILE = %s\n",
            arguments.brig_file,
            arguments.hsail_file,
            arguments.output_file);
    */
    /* 
     * Iterate over the agents and pick the gpu agent using 
     * the get_gpu_agent callback.
     */
    hsa_agent_t agent;
    err = hsa_iterate_agents(get_gpu_agent, &agent);
    if(err == HSA_STATUS_INFO_BREAK) { err = HSA_STATUS_SUCCESS; }
    ErrorCheck(Getting a gpu agent, err);

    /*
     * Query the name of the agent.
     */
    char name[64] = { 0 };
    err = hsa_agent_get_info(agent, HSA_AGENT_INFO_NAME, name);
    ErrorCheck(Querying the agent name, err);

        
    hsa_profile_t device_profile;
    err = hsa_agent_get_info(agent, HSA_AGENT_INFO_PROFILE, &device_profile);
    ErrorCheck(Querying the agent profile, err);
    /*
     * Create hsa program.
     */
    hsa_ext_program_t program;
    memset(&program,0,sizeof(hsa_ext_program_t));
    err = hsa_ext_program_create(HSA_MACHINE_MODEL_LARGE, device_profile, HSA_DEFAULT_FLOAT_ROUNDING_MODE_DEFAULT, NULL, &program);
    ErrorCheck(Create the program, err);

    /*
     * Load the BRIG binary.
     */
    hsa_ext_module_t module;
    int rc = load_module_from_file(arguments.brig_file, &module);

    /*
     * Add the BRIG module to hsa program.
     */
    err = hsa_ext_program_add_module(program, module);
    ErrorCheck(Adding the brig module to the program, err);

    /*
     * Determine the agents ISA.
     */
    hsa_isa_t isa;
    err = hsa_agent_get_info(agent, HSA_AGENT_INFO_ISA, &isa);
    ErrorCheck(Query the agents isa, err);

    /*
     * Finalize the program and extract the code object.
     */
    hsa_ext_control_directives_t control_directives;
    memset(&control_directives, 0, sizeof(hsa_ext_control_directives_t));
    hsa_code_object_t code_object;
    err = hsa_ext_program_finalize(program, isa, 0, control_directives, "", HSA_CODE_OBJECT_TYPE_PROGRAM, &code_object);
    ErrorCheck(Finalizing the program, err);

    /* Serialize code object */
    void *serialized_code_object = NULL;
    size_t sco_size;
    hsa_callback_data_t data;
    err = hsa_code_object_serialize(code_object, serialize_alloc, data, NULL, &serialized_code_object, &sco_size);
    ErrorCheck(Code Object Serialization, err);

    const char *output = (arguments.output_file == "") ? "a.o" : arguments.output_file;
    std::ofstream ofile(output, std::ios::out | std::ios::binary);
    assert(ofile.is_open() && ofile.good());
    ofile.write((char *)serialized_code_object, sco_size);
    ofile.close();

    /*
     * Destroy the program, it is no longer needed.
     */
    err=hsa_ext_program_destroy(program);
    ErrorCheck(Destroying the program, err);

    err=hsa_code_object_destroy(code_object);
    ErrorCheck(Destroying the code object, err);

    err = hsa_shut_down();
    ErrorCheck(HSA Finalize, err);

    free(serialized_code_object);
    free((char *)module);

    exit(0);
}
