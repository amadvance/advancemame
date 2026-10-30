#ifndef CYCLONE_MAME_FASTRAM_H
#define CYCLONE_MAME_FASTRAM_H

/* Optional driver-declared immutable direct-memory regions. These are only
 * used by Cyclone's 68000 data accessors, never its opcode-fetch mapping.
 * The driver must not register ranges containing device handlers, dynamic
 * overlays, or replaceable host pointers. mask implements memory mirroring.
 */
#ifdef USE_CYCLONE
void cyclone_mame_clear_fastram(int cpunum);
void cyclone_mame_set_fastram(int cpunum, unsigned int start,
    unsigned int end, unsigned int mask, int readonly, void *base);
#endif

#endif
