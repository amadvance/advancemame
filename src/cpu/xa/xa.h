// license:BSD-3-Clause
// copyright-holders:David Haywood
#ifndef XA_H
#define XA_H

struct xa_config {
	UINT8 (*port_r)(int port);
	void (*port_w)(int port, UINT8 data);
};
void xa_get_info(UINT32 state, union cpuinfo *info);

#endif
