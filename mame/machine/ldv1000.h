// license:BSD-3-Clause
// copyright-holders:Ryan Holtz, Aaron Giles
#ifndef LDV1000_H
#define LDV1000_H

int ldv1000_init(void);
void ldv1000_reset(void);
void ldv1000_write(UINT8 data);
UINT8 ldv1000_read(void);
int ldv1000_status_strobe(void);
int ldv1000_ready(void);
void ldv1000_field(void);
void ldv1000_video(mame_bitmap *bitmap, const rectangle *cliprect);
extern const struct CustomSound_interface ldv1000_sound;

#endif
