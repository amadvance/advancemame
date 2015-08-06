#include "cpuintrf.h"

enum {
	SE3208_PC=1, SE3208_SR, SE3208_ER, SE3208_SP,SE3208_PPC,
	SE3208_R0, SE3208_R1, SE3208_R2, SE3208_R3, SE3208_R4, SE3208_R5, SE3208_R6, SE3208_R7
};

#define SE3208_INT	0

extern void SE3208_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
int SE3208Dasm(UINT32 PC,char *Buffer);
#endif
