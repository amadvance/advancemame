#include <stdlib.h>
#include <string.h>

/*
The expat documentation claims that without XML_DTD and
XML_CONTEXT_BYTES the library may be faster. But the
speed difference is very small.
*/

#define XML_NS 1
#define XML_DTD 1
#define XML_CONTEXT_BYTES 1024

#if defined(USE_LSB) || defined(LSB_FIRST)
#define BYTEORDER 1234
#else
#define BYTEORDER 4321
#endif

#define HAVE_MEMMOVE

