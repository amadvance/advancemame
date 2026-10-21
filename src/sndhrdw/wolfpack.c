#include "driver.h"
#include "streams.h"
#include "sound/custom.h"
#include "sound/samples.h"

const char wolfpack_speech_credits[] =
	"S14001A: Copyright Jonathan Gevaryahu; Kevin Horton; R. Belmont; Zsolt Vasvari";

typedef struct
{
	int index;
	sound_stream * stream;

	UINT8 WordInput;
	UINT8 LatchedWord;
	UINT16 SyllableAddress;
	UINT16 PhoneAddress;
	UINT8 PlayParams;
	UINT8 PhoneOffset;
	UINT8 LengthCounter;
	UINT8 RepeatCounter;
	UINT8 OutputCounter;
	UINT8 machineState;
	UINT8 nextstate;
	UINT8 laststate;
	UINT8 resetState;
	UINT8 oddeven;
	UINT8 GlobalSilenceState;
	UINT8 OldDelta;
	UINT8 DACOutput;
	UINT8 audioout;
	UINT8 *SpeechRom;
	INT16 filtervals[8];
	UINT8 VSU1000_amp;
} S14001AChip;

#define SILENCE 0x7
#define ALTFLAG 0xFF

#define LASTSYLLABLE ((chip->PlayParams & 0x80)>>7)
#define MIRRORMODE ((chip->PlayParams & 0x40)>>6)
#define SILENCEFLAG ((chip->PlayParams & 0x20)>>5)
#define LENGTHCOUNT ((chip->PlayParams & 0x1C)>>1)
#define REPEATCOUNT ((chip->PlayParams<<1)&0x6)
#define LOCALSILENCESTATE ((chip->OutputCounter & 0x2) && (MIRRORMODE))

static const INT8 DeltaTable[4][4] =
{
	{ -3, -3, -1, -1, },
	{ -1, -1,  0,  0, },
	{  0,  0,  1,  1, },
	{  1,  1,  3,  3  },
};

static INT16 audiofilter(S14001AChip *chip)
{
	UINT8 temp1;
        INT16 temp2 = 0;

	for (temp1 = 0; temp1 < 8; temp1++) { temp2 += chip->filtervals[temp1]; }
	temp2 >>= 3;
	return temp2;
}

static void shiftIntoFilter(S14001AChip *chip, INT16 inputvalue)
{
	UINT8 temp1;
	for (temp1 = 7; temp1 > 0; temp1--)
	{
		chip->filtervals[temp1] = chip->filtervals[(temp1 - 1)];
	}
	chip->filtervals[0] = inputvalue;

}

static void PostPhoneme(S14001AChip *chip)
{

	chip->RepeatCounter++;
	chip->OutputCounter++;
	if (MIRRORMODE)
	{

		if (chip->RepeatCounter == 0x8)
		{

			chip->RepeatCounter = REPEATCOUNT;
			if (chip->LengthCounter & 0x1)
			{

				chip->PhoneAddress+=8;
			}
			chip->LengthCounter++;
			if (chip->LengthCounter == 0x10)
			{

				chip->SyllableAddress += 2;
				chip->nextstate = LASTSYLLABLE ? 13 : 3;
			}
			else
			{

				chip->PhoneOffset = (chip->OutputCounter&1) ? 7 : 0;
				chip->nextstate = (chip->OutputCounter&1) ? 9 : 5;
			}
		}
		else
		{

			chip->PhoneOffset = (chip->OutputCounter&1) ? 7 : 0;
			chip->nextstate = (chip->OutputCounter&1) ? 9 : 5;
		}
	}
	else
	{

		if (chip->RepeatCounter == 0x8)
		{

			chip->RepeatCounter = REPEATCOUNT;
			chip->LengthCounter++;
			if (chip->LengthCounter == 0x10)
			{

				chip->SyllableAddress += 2;
				chip->nextstate = LASTSYLLABLE ? 13 : 3;

				return;
			}
		}
		chip->PhoneAddress += 8;
		chip->PhoneOffset = 0;
		chip->nextstate = 5;
	}

}

static void s14001a_clock(S14001AChip *chip)
{
	UINT8 CurDelta;

	chip->oddeven = !(chip->oddeven);
	if (chip->oddeven == 0)
        {
		chip->audioout = ALTFLAG;

	}
	else
	{

		if (chip->GlobalSilenceState || LOCALSILENCESTATE)
		{
			chip->DACOutput = SILENCE;
			chip->OldDelta = 2;
		}
		chip->audioout = (chip->GlobalSilenceState || LOCALSILENCESTATE) ? SILENCE : chip->DACOutput;

		switch(chip->machineState)
		{
		case 0:
			chip->nextstate = 0;
			break;
		case 1:
			chip->SyllableAddress = 0;
			chip->SyllableAddress |= chip->SpeechRom[(chip->LatchedWord<<1)]<<4;
			chip->nextstate = chip->resetState ? 1 : 2;
			break;
		case 2:
			chip->SyllableAddress |= chip->SpeechRom[(chip->LatchedWord<<1)+1]>>4;
			chip->nextstate = 3;
			break;
		case 3:
			chip->PhoneAddress = chip->SpeechRom[chip->SyllableAddress & 0x7ff]<<4;
			chip->nextstate = 4;
			break;
		case 4:
			chip->PlayParams = chip->SpeechRom[(chip->SyllableAddress+1) & 0x7ff];
			chip->GlobalSilenceState = SILENCEFLAG;
			chip->LengthCounter = LENGTHCOUNT;
			chip->RepeatCounter = REPEATCOUNT;
			chip->OutputCounter = 0;
			chip->PhoneOffset = 0;
			chip->OldDelta = 0x2;
			chip->DACOutput = SILENCE ;
			chip->nextstate = 5;
			break;
		case 5:
			CurDelta = (chip->SpeechRom[(chip->PhoneAddress+chip->PhoneOffset) & 0x7ff]&0xc0)>>6;
			chip->DACOutput += DeltaTable[CurDelta][chip->OldDelta];
			chip->OldDelta = CurDelta;
			chip->nextstate = 6;
			break;
		case 6:
	   		CurDelta = (chip->SpeechRom[(chip->PhoneAddress+chip->PhoneOffset) & 0x7ff]&0x30)>>4;
			chip->DACOutput += DeltaTable[CurDelta][chip->OldDelta];
			chip->OldDelta = CurDelta;
			chip->nextstate = 7;
			break;
		case 7:
			CurDelta = (chip->SpeechRom[(chip->PhoneAddress+chip->PhoneOffset) & 0x7ff]&0xc)>>2;
			chip->DACOutput += DeltaTable[CurDelta][chip->OldDelta];
			chip->OldDelta = CurDelta;
			chip->nextstate = 8;
			break;
		case 8:
			CurDelta = chip->SpeechRom[(chip->PhoneAddress+chip->PhoneOffset) & 0x7ff]&0x3;
			chip->DACOutput += DeltaTable[CurDelta][chip->OldDelta];
			chip->OldDelta = CurDelta;
			chip->PhoneOffset++;
			if (chip->PhoneOffset == 0x8)
			{

				PostPhoneme(chip);
			}
			else
			{
				chip->nextstate = 5;
			}
			break;
		case 9:
			CurDelta = (chip->SpeechRom[(chip->PhoneAddress+chip->PhoneOffset) & 0x7ff]&0x3);
			if (chip->laststate != 8)
			{
				chip->DACOutput -= DeltaTable[chip->OldDelta][CurDelta];
			}
			chip->OldDelta = CurDelta;
			chip->nextstate = 10;
			break;
		case 10:
			CurDelta = (chip->SpeechRom[(chip->PhoneAddress+chip->PhoneOffset) & 0x7ff]&0xc)>>2;
			chip->DACOutput -= DeltaTable[chip->OldDelta][CurDelta];
			chip->OldDelta = CurDelta;
			chip->nextstate = 11;
			break;
		case 11:
			CurDelta = (chip->SpeechRom[(chip->PhoneAddress+chip->PhoneOffset) & 0x7ff]&0x30)>>4;
			chip->DACOutput -= DeltaTable[chip->OldDelta][CurDelta];
			chip->OldDelta = CurDelta;
			chip->nextstate = 12;
			break;
		case 12:
			CurDelta = (chip->SpeechRom[(chip->PhoneAddress+chip->PhoneOffset) & 0x7ff]&0xc0)>>6;
			chip->DACOutput -= DeltaTable[chip->OldDelta][CurDelta];
			chip->OldDelta = CurDelta;
			chip->PhoneOffset--;
			if (chip->PhoneOffset == 0xFF)
			{

				PostPhoneme(chip);
			}
			else
			{
				chip->nextstate = 9;
			}
			break;
		case 13:
			chip->nextstate = 0;
			break;
		}

		chip->laststate = chip->machineState;
		chip->machineState = chip->nextstate;

	        chip->DACOutput &= 0xF;
	}
}

static void s14001a_pcm_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{

	S14001AChip *chip = param;
	int i;

	for (i = 0; i < length; i++)
	{
		s14001a_clock(chip);
		if (chip->audioout == ALTFLAG)
		{
			shiftIntoFilter(chip, audiofilter(chip));
			outputs[0][i] = audiofilter(chip)*chip->VSU1000_amp;
		}
		else
		{
			shiftIntoFilter(chip, ((((INT16)chip->audioout)-8)*512));
			outputs[0][i] = ((((INT16)chip->audioout)-8)*1024)*chip->VSU1000_amp;
		}
	}
}


static S14001AChip *speech;
static UINT8 effects[7];

static void wolfpack_speech_reset(void *token)
{
	S14001AChip *chip = token;
	UINT8 *rom = chip->SpeechRom;
	sound_stream *stream = chip->stream;
	stream_update(stream, 0);
	memset(chip, 0, sizeof(*chip));
	chip->SpeechRom = rom;
	chip->stream = stream;
	chip->GlobalSilenceState = 1;
	chip->OldDelta = 2;
	chip->DACOutput = SILENCE;
	chip->VSU1000_amp = 2;
}

static void *wolfpack_speech_start(int clock, const struct CustomSound_interface *config)
{
	speech = auto_malloc(sizeof(*speech));
	memset(speech, 0, sizeof(*speech));
	speech->SpeechRom = memory_region(REGION_SOUND1);
	speech->stream = stream_create(0, 1, clock, speech, s14001a_pcm_update);
	wolfpack_speech_reset(speech);
	return speech;
}

struct CustomSound_interface wolfpack_speech_interface =
{
	wolfpack_speech_start, NULL, wolfpack_speech_reset, NULL
};

int wolfpack_speech_busy(void)
{
	stream_update(speech->stream, 0);
	return speech->machineState != 0;
}

WRITE8_HANDLER( wolfpack_word_w )
{
	stream_update(speech->stream, 0);
	speech->WordInput = data & 0x1f;
}

WRITE8_HANDLER( wolfpack_start_speech_w )
{
	stream_update(speech->stream, 0);
	speech->LatchedWord = speech->WordInput;
	speech->resetState = data & 1;
	if (speech->resetState) speech->machineState = 1;
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
