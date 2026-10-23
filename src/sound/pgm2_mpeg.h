// license:BSD-3-Clause
// copyright-holders:Olivier Galibert
/* Converted from MAME 0.287 to C. */
#ifndef PGM2_MPEG_H
#define PGM2_MPEG_H
#include <stdint.h>
#include <stdbool.h>
#include <setjmp.h>
	typedef struct pgm2_band_info {
		int modulo;
		double s1;
		int bits, cube_bits;
		int s4, s5;
		double range, s7, scale, offset;
	} pgm2_band_info;

typedef struct pgm2_mpeg {
	const uint8_t *base;
	int accepted, position_align;

	int sampling_rate, last_frame_number;
	int param_index, cbr_param_index;

	int channel_count, total_bands, joint_bands;

	int band_param[2][32];
	int scfsi[2][32];
	int scf[2][3][32];
	double amp_values[2][3][32];
	double bdata[2][3][32];
	double subbuffer[2][32];
	double audio_buffer[2][32*32];
	int audio_buffer_pos[2];
	double m_cos_cache[32][32];

	int current_pos, current_limit;

 jmp_buf limit_hit;
 int (*do_gb)(const unsigned char *, int *, int);
} pgm2_mpeg;
void pgm2_mpeg_init(pgm2_mpeg *ctx, const void *base);
void pgm2_mpeg_clear(pgm2_mpeg *ctx);
int pgm2_mpeg_decode(pgm2_mpeg *ctx, int *pos, int limit, short *output, int *samples, int *rate, int *channels, int atbl);
#endif
