#ifndef CV1K_MPEG_H
#define CV1K_MPEG_H
#include "driver.h"
#include <setjmp.h>
enum { L1=1, L2=2, L2_5=4, L3=8, AMM=16 };
typedef struct band_info {
		int modulo;
		double s1;
		int bits, cube_bits;
		int s4, s5;
		double range, s7, scale, offset;
	} band_info;

	typedef struct mpeg_audio {
const UINT8 *base;
	int accepted, position_align;

	int sampling_rate, last_frame_number;
	int param_index;

	int channel_count, total_bands, joint_bands;

	int band_param[2][32];
	int scfsi[2][32];
	int scf[2][3][32];
	double amp_values[2][3][32];
	double bdata[2][3][32];
	double subbuffer[2][32];
	double audio_buffer[2][32*32];
	int audio_buffer_pos[2];

	int master_pos;

	int current_pos, current_limit;

	int (*do_gb)(const unsigned char *data, int *pos, int count);
jmp_buf limit_hit;
} mpeg_audio;
void cv1k_mpeg_init(mpeg_audio *ctx, const void *base);
void cv1k_mpeg_clear(mpeg_audio *ctx);
int cv1k_mpeg_decode(mpeg_audio *ctx, int *pos, int limit, short *output, int *output_samples, int *sample_rate, int *channels);

#endif
