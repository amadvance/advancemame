#ifndef __MPG123_H
#define __MPG123_H

#include "mpglib.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <math.h>

#ifdef __WIN32__
#define M_PI       3.14159265358979323846
#define M_SQRT2    1.41421356237309504880
#define random rand
#define srandom srand
#endif

#define FALSE                   0
#define TRUE                    1

#define MPG_MD_STEREO           0
#define MPG_MD_JOINT_STEREO     1
#define MPG_MD_DUAL_CHANNEL     2
#define MPG_MD_MONO             3

extern unsigned int mp3internal_get1bit(struct mp3_decoder_state* state);
extern unsigned int mp3internal_getbits(struct mp3_decoder_state* state, int number_of_bits);
extern unsigned int mp3internal_getbits_fast(struct mp3_decoder_state* state, int number_of_bits);
extern void mp3internal_skipbits(struct mp3_decoder_state* state, int number_of_bits);
extern int mp3_set_pointer(void*, long);

extern void mp3internal_make_decode_tables(long scaleval);
extern int mp3internal_do_layer3(void* external_state, struct mp3_decoder_state* state, struct mp3_frame *fr,unsigned char *,int *);
extern int mp3internal_decode_header(struct mp3_frame *fr,unsigned long newhead);

struct gr_info_s {
      int scfsi;
      unsigned part2_3_length;
      unsigned big_values;
      unsigned scalefac_compress;
      unsigned block_type;
      unsigned mixed_block_flag;
      unsigned table_select[3];
      unsigned subblock_gain[3];
      unsigned maxband[3];
      unsigned maxbandl;
      unsigned maxb;
      unsigned region1start;
      unsigned region2start;
      unsigned preflag;
      unsigned scalefac_scale;
      unsigned count1table_select;
      const mp3internal_real *full_gain[3];
      const mp3internal_real *pow2gain;
};

struct III_sideinfo
{
  unsigned main_data_begin;
  unsigned private_bits;
  struct {
    struct gr_info_s gr[2];
  } ch[2];
};

extern int mp3internal_synth_1to1 (struct mp3_decoder_state* state, mp3internal_real *,int,unsigned char *,int *);
extern int mp3internal_synth_1to1_mono (struct mp3_decoder_state* state, mp3internal_real *,unsigned char *,int *);

extern void mp3internal_init_layer3(int);
extern void mp3internal_make_decode_tables(long scale);
extern void mp3internal_dct64(mp3internal_real *,mp3internal_real *,mp3internal_real *);

extern mp3internal_real mp3internal_decwin[512+32];
extern mp3internal_real *mp3internal_pnts[5];

#endif

