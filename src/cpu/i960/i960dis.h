#ifndef _I960DISASSEMBLER_H
#define _I960DISASSEMBLER_H

typedef struct
{
	char		*buffer;	// output buffer
	UINT32	IP;
	UINT32	IPinc;
} disassemble_t;

char *i960_disassemble(disassemble_t *diss);

#endif // _I960DISASSEMBLER_H
