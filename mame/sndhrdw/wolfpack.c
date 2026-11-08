// license:BSD-3-Clause
// copyright-holders:Stefan Jokisch
#include "driver.h"
#include "sound/s14001a.h"
#include "sound/samples.h"

static UINT8 effects[7];

WRITE8_HANDLER( wolfpack_word_w )
{
	s14001a_word_w(0, data & 0x1f);
}

static const char *wolfpack_sample_names[] =
{
	"*wolfpack", "high_explosion.wav", "sonar.wav", "siren.wav",
	"pt_boat.wav", "torpedo.wav", "low_explosion.wav", "screw.wav", NULL
};

static void wolfpack_samples_start(void)
{
	memset(effects, 0, sizeof(effects));
}

struct Samplesinterface wolfpack_samples_interface =
{
	7, wolfpack_sample_names, wolfpack_samples_start
};

WRITE8_HANDLER( wolfpack_effect_w )
{
	int active = data & 1;
	int loop = offset == 2 || offset == 3 || offset == 6;
	if (active && !effects[offset]) sample_start(offset, offset, loop);
	if (!active && loop) sample_stop(offset);
	effects[offset] = active;
}

void wolfpack_effect_reset(void)
{
	int i;
	for (i = 0; i < 7; i++) sample_stop(i);
	memset(effects, 0, sizeof(effects));
}
