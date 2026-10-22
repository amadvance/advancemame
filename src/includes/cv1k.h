/*
license:BSD-3-Clause
copyright-holders:David Haywood, Luca Elia, MetalliC
*/

#ifndef CV1K_H
#define CV1K_H
void cv1k_flash_init(int region);
void cv1k_flash_hard_reset(void);
void cv1k_flash_nvram(mame_file *file, int write);
UINT8 cv1k_flash_ready_r(void);
UINT8 cv1k_flash_io_r(void);
void cv1k_flash_enab_w(UINT8 data);
void cv1k_flash_cmd_w(UINT8 data);
void cv1k_flash_addr_w(UINT8 data);
void cv1k_flash_data_w(UINT8 data);
void cv1k_rtc_init(void);
void cv1k_rtc_nvram(mame_file *file, int write);
void cv1k_rtc_write_bit(int state);
int cv1k_rtc_read_bit(void);
void cv1k_rtc_set_cs_line(int state);
void cv1k_rtc_set_clock_line(int state);
void cv1k_video_init(UINT64 *ram, int size);
READ64_HANDLER(cv1k_blitter_r);
WRITE64_HANDLER(cv1k_blitter_w);
VIDEO_UPDATE(cv1k);
void cv1k_ymz_write(int offset, UINT8 data);
struct CustomSound_interface;
void *cv1k_ymz_start(int clock, const struct CustomSound_interface *config);
void cv1k_video_reset(void);
void cv1k_ymz_reset(void *token);
#endif
