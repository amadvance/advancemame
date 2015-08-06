#ifndef _I960DISASSEMBLER_H
#define _I960DISASSEMBLER_H

typedef struct
{
	char		*buffer;	// output buffer
	unsigned long	IP;
	unsigned long	IPinc;
} disassemble_t;

char *i960_disassemble(disassemble_t *diss);

#endif // _I960DISASSEMBLER_H
