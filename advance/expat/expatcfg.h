#include <stdlib.h>
#include <string.h>

#define XML_NS 1
#define XML_DTD 1
#define XML_CONTEXT_BYTES 1024

#if defined(USE_LSB) || defined(LSB_FIRST)
#define BYTEORDER 1234
#else
#define BYTEORDER 4321
#endif

#define HAVE_MEMMOVE


