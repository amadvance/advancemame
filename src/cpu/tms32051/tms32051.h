#ifndef _TMS32051_H
#define _TMS32051_H

#if (HAS_TMS32051)
void tms32051_get_info(UINT32 state, union cpuinfo *info);
#endif

#ifdef MAME_DEBUG
extern int tms32051_dasm_one(char *buffer, offs_t pc);
#endif

#endif /* _TMS32051_H */
