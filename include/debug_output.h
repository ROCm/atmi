#ifndef ATMI_DEBUG_OUTPUT
#define ATMI_DEBUG_OUTPUT

#include <stdint.h>

#define ATMI_HAVE_DEBUG
#define ATMI_DEBUG_LEVEL	0

extern uint8_t atmi_debug_enable;

void atmi_debug_output(int output_id, const char *format, ...);

#if defined (ATMI_HAVE_DEBUG)
#define ATMI_DEBUG( INST ) if (atmi_debug_enable) { INST }
#else
#define ATMI_DEBUG( INST )
#endif

#endif /* ATMI_DEBUG_OUTPUT */
