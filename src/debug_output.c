#include "debug_output.h"
#include <stdio.h>
#include <stdarg.h>

uint8_t atmi_debug_enable = 1;

void atmi_debug_output(int output_id, const char *format, ...)
{
    if (output_id >= 0 && output_id <= ATMI_DEBUG_LEVEL) {
        va_list arglist;
        fprintf( stderr, "[Debug %d]: ", output_id );
        va_start(arglist, format);
        vfprintf(stderr, format, arglist);
        va_end(arglist);
    }
}
