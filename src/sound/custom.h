// license:BSD-3-Clause
// copyright-holders:Aaron Giles
#ifndef CUSTOM_H
#define CUSTOM_H

struct CustomSound_interface
{
	void *(*start)(int clock, const struct CustomSound_interface *config);
	void (*stop)(void *token);
	void (*reset)(void *token);
	void *extra_data;
};


#endif
