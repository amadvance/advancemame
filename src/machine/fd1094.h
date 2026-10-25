// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria, Andreas Naive, Charles MacDonald
#define FD1094_STATE_RESET	0x0100
#define FD1094_STATE_IRQ	0x0200
#define FD1094_STATE_RTE	0x0300

int fd1094_set_state(unsigned char *key,int state);
int fd1094_decode(int address,int val,unsigned char *key,int vector_fetch);
