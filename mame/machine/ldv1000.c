// license:BSD-3-Clause
// copyright-holders:Ryan Holtz, Aaron Giles
/* Pioneer LD-V1000 laserdisc player simulation. */

#include "driver.h"
#include "streams.h"
#include "libchdr/chd.h"
#include "sound/custom.h"
#include "machine/laserdsc.h"

enum player_command
{
	CMD_CLEAR                = 0xbf,
	CMD_0                    = 0x3f,
	CMD_1                    = 0x0f,
	CMD_2                    = 0x8f,
	CMD_3                    = 0x4f,
	CMD_4                    = 0x2f,
	CMD_5                    = 0xaf,
	CMD_6                    = 0x6f,
	CMD_7                    = 0x1f,
	CMD_8                    = 0x9f,
	CMD_9                    = 0x5f,
	CMD_STORE                = 0xf5,
	CMD_RECALL               = 0x7f,
	CMD_DISPLAY              = 0xf1,
	CMD_AUDIO1               = 0xf4,
	CMD_AUDIO2               = 0xfc,
	CMD_PLAY                 = 0xfd,
	CMD_STOP                 = 0xfb,
	CMD_AUTOSTOP             = 0xf3,
	CMD_SEARCH               = 0xf7,
	CMD_SCAN_FWD             = 0xf0,
	CMD_SCAN_REV             = 0xf8,
	CMD_STEP_FWD             = 0xf6,
	CMD_STEP_REV             = 0xfe,
	CMD_REJECT               = 0xf9,
	CMD_NO_ENTRY             = 0xff,
	CMD_LOAD                 = 0xcc,
	CMD_DISPLAY_DISABLE      = 0xcd,
	CMD_DISPLAY_ENABLE       = 0xce,
	CMD_GET_FRAME_NUM        = 0xc2,
	CMD_GET_2ND_DISPLAY      = 0xc3,
	CMD_GET_1ST_DISPLAY      = 0xc4,
	CMD_TRANSFER_MEMORY      = 0xc8,
	CMD_FWD_X0               = 0xa0,
	CMD_FWD_X1_4             = 0xa1,
	CMD_FWD_X1_2             = 0xa2,
	CMD_FWD_X1               = 0xa3,
	CMD_FWD_X2               = 0xa4,
	CMD_FWD_X3               = 0xa5,
	CMD_FWD_X4               = 0xa6,
	CMD_FWD_X5               = 0xa7,
	CMD_SKIP_FWD_10          = 0xb1,
	CMD_SKIP_FWD_20          = 0xb2,
	CMD_SKIP_FWD_30          = 0xb3,
	CMD_SKIP_FWD_40          = 0xb4,
	CMD_SKIP_FWD_50          = 0xb5,
	CMD_SKIP_FWD_60          = 0xb6,
	CMD_SKIP_FWD_70          = 0xb7,
	CMD_SKIP_FWD_80          = 0xb8,
	CMD_SKIP_FWD_90          = 0xb9,
	CMD_SKIP_FWD_100         = 0xba
};

enum player_status
{
	STATUS_PARK              = 0x7c,
	STATUS_PLAY              = 0x64,
	STATUS_STOP              = 0x65,
	STATUS_SEARCH            = 0x50,
	STATUS_SEARCH_FINISH     = 0xd0,
	STATUS_SEARCH_ERROR      = 0x90,
	STATUS_AUTOSTOP          = 0x54,
	STATUS_SCAN              = 0x4c,
	STATUS_FORWARD           = 0x2e,
	STATUS_LOAD              = 0x48,
	STATUS_LOAD_END          = 0xc8,
	STATUS_LOAD_ERROR        = 0xc4,
	STATUS_FOCUS_UNLOCK      = 0xbc,
	STATUS_LEADIN            = 0x58,
	STATUS_LEADOUT           = 0x5c,
	STATUS_REJECT            = 0x60,
	STATUS_READY             = 0x80
};

enum player_mode
{
	MODE_PARK,
	MODE_PLAY,
	MODE_SCAN_FORWARD,
	MODE_SCAN_REVERSE,
	MODE_SEARCH,
	MODE_STOP
};

static struct
{
	UINT8 cmd_buffer[21], cmd_number[5];
	UINT32 cmd_length, cmd_number_length, curr_frame, search_frame, stop_frame;
	UINT32 curr_register, scan_speed, scan_speed_accum;
	UINT16 user_ram[1024];
	UINT8 status, pre_stop_status, mode;
	double play_speed;
	int audio_enable[2], audio_active[2], video_enabled;
	int status_strobe, command_strobe;
} player;

static mame_timer *park_timer, *stop_timer, *status_off_timer, *command_on_timer, *command_off_timer;
static chd_file *disc;
static UINT8 *hunk, *vbi;
static UINT32 total_hunks, hunk_bytes;
static int decoded_hunk;
static int width, height, sample_rate, field, track;
static double slider_speed, slider_fraction;
static mame_bitmap *video;
static sound_stream *audio_stream;
#define AUDIO_BUFFER_SIZE 16384
static INT16 audio_buffer[2][AUDIO_BUFFER_SIZE];
static unsigned audio_in, audio_out;

static void advance_slider(int delta)
{
	track += delta;
	if (track < 0) track = 0;
	if (track >= total_hunks / 2) track = total_hunks / 2 - 1;
}

static void set_slider_speed(double speed)
{
	slider_speed = speed;
}

static void process_command_buffer(void);
static int is_command_byte(const UINT8 data);
static int is_command_number(const UINT8 data);
static void process_command(size_t cmd_index);
static UINT32 bcd_to_literal(UINT32 bcd);
static UINT32 literal_to_bcd(UINT32 value);
static UINT32 cmd_number_to_bcd(void);
static UINT8 cmd_to_number(const UINT8 cmd);
static void set_mode(const UINT8 mode);
static void set_playing(const UINT8 new_status, const double fields_per_vsync);
static void set_stopped(const UINT8 new_status);
static void cmd_play(void);
static void cmd_stop(void);
static void cmd_step(const int direction);
static void cmd_play_forward(const double fields_per_vsync);
static void cmd_search(const UINT32 frame);
static void cmd_skip_forward(const INT32 amount);
static void update_video_enable(void);
static void update_audio_enable(void);

static void process_command_buffer(void)
{
	size_t cmd_index = 0;
	while (cmd_index < player.cmd_length)
	{
		process_command(cmd_index);
		cmd_index++;
	}

	player.cmd_length = 0;
}

static int is_command_byte(const UINT8 data)
{
	switch (data)
	{
		case CMD_CLEAR:
		case CMD_0:
		case CMD_1:
		case CMD_2:
		case CMD_3:
		case CMD_4:
		case CMD_5:
		case CMD_6:
		case CMD_7:
		case CMD_8:
		case CMD_9:
		case CMD_STORE:
		case CMD_RECALL:
		case CMD_DISPLAY:
		case CMD_AUDIO1:
		case CMD_AUDIO2:
		case CMD_PLAY:
		case CMD_STOP:
		case CMD_AUTOSTOP:
		case CMD_SEARCH:
		case CMD_SCAN_FWD:
		case CMD_SCAN_REV:
		case CMD_STEP_FWD:
		case CMD_STEP_REV:
		case CMD_REJECT:
		case CMD_NO_ENTRY:
		case CMD_LOAD:
		case CMD_DISPLAY_DISABLE:
		case CMD_DISPLAY_ENABLE:
		case CMD_GET_FRAME_NUM:
		case CMD_GET_2ND_DISPLAY:
		case CMD_GET_1ST_DISPLAY:
		case CMD_TRANSFER_MEMORY:
		case CMD_FWD_X0:
		case CMD_FWD_X1_4:
		case CMD_FWD_X1_2:
		case CMD_FWD_X1:
		case CMD_FWD_X2:
		case CMD_FWD_X3:
		case CMD_FWD_X4:
		case CMD_FWD_X5:
		case CMD_SKIP_FWD_10:
		case CMD_SKIP_FWD_20:
		case CMD_SKIP_FWD_30:
		case CMD_SKIP_FWD_40:
		case CMD_SKIP_FWD_50:
		case CMD_SKIP_FWD_60:
		case CMD_SKIP_FWD_70:
		case CMD_SKIP_FWD_80:
		case CMD_SKIP_FWD_90:
		case CMD_SKIP_FWD_100:
			return 1;
	}
	return 0;
}

static int is_command_number(const UINT8 data)
{
	switch (data)
	{
		case CMD_0:
		case CMD_1:
		case CMD_2:
		case CMD_3:
		case CMD_4:
		case CMD_5:
		case CMD_6:
		case CMD_7:
		case CMD_8:
		case CMD_9:
			return 1;
	}
	return 0;
}

static void process_command(size_t cmd_index)
{
	const UINT8 command = player.cmd_buffer[cmd_index];

	if (BIT(player.status, 7))
	{
		player.status &= ~STATUS_READY;

		switch (command)
		{
			case CMD_CLEAR:

				break;

			case CMD_0:
			case CMD_1:
			case CMD_2:
			case CMD_3:
			case CMD_4:
			case CMD_5:
			case CMD_6:
			case CMD_7:
			case CMD_8:
			case CMD_9:
				if (player.cmd_number_length == 5)
					player.cmd_number_length = 0;
				player.cmd_number[player.cmd_number_length] = cmd_to_number(command);

				player.cmd_number_length++;
				break;

			case CMD_STORE:
				if (player.cmd_number_length == 0)
				{

					player.user_ram[player.curr_register] = literal_to_bcd(player.curr_frame);
				}
				else
				{
					player.user_ram[player.curr_register] = cmd_number_to_bcd();

				}
				break;

			case CMD_RECALL:

				break;

			case CMD_DISPLAY:

				break;

			case CMD_AUDIO1:
				if (player.cmd_number_length == 0)
				{
					player.audio_enable[0] = !player.audio_enable[0];

				}
				else
				{
					const UINT32 cmd_number = cmd_number_to_bcd();
					player.audio_enable[0] = BIT(cmd_number, 0);

				}
				break;

			case CMD_AUDIO2:
				if (player.cmd_number_length == 0)
				{
					player.audio_enable[1] = !player.audio_enable[1];

				}
				else
				{
					const UINT32 cmd_number = cmd_number_to_bcd();
					player.audio_enable[1] = BIT(cmd_number, 0);

				}
				break;

			case CMD_PLAY:

				cmd_play();
				break;

			case CMD_STOP:
				cmd_stop();
				break;

			case CMD_AUTOSTOP:
			{
				const UINT32 value = bcd_to_literal(cmd_number_to_bcd());

				if (value < player.curr_frame)
				{

					cmd_search(value);
					break;
				}
				player.status = STATUS_AUTOSTOP;
				break;
			}

			case CMD_SEARCH:
			{
				const UINT32 value = bcd_to_literal(cmd_number_to_bcd());

				cmd_search(value);
				break;
			}

			case CMD_SCAN_FWD:
			case CMD_SCAN_REV:

				set_mode(command == CMD_SCAN_FWD ? MODE_SCAN_FORWARD : MODE_SCAN_REVERSE);
				player.status = STATUS_SCAN;
				player.scan_speed = 4000;
				player.scan_speed_accum = 0;
				player.stop_frame = ~0U;
				player.search_frame = ~0U;
				break;

			case CMD_STEP_FWD:

				cmd_step(1);
				break;

			case CMD_STEP_REV:

				cmd_step(-1);
				break;

			case CMD_REJECT:

				set_mode(MODE_PARK);
				player.status = STATUS_PARK;
				player.stop_frame = ~0U;
				player.search_frame = ~0U;
				break;

			case CMD_NO_ENTRY:

				player.status |= STATUS_READY;
				break;

			case CMD_LOAD:

				break;

			case CMD_DISPLAY_DISABLE:

				break;

			case CMD_DISPLAY_ENABLE:

				break;

			case CMD_GET_FRAME_NUM:

				break;

			case CMD_GET_2ND_DISPLAY:

				break;

			case CMD_GET_1ST_DISPLAY:

				break;

			case CMD_TRANSFER_MEMORY:

				break;

			case CMD_FWD_X0:
				cmd_play_forward(0.0);
				break;

			case CMD_FWD_X1_4:
				cmd_play_forward(0.25);
				break;

			case CMD_FWD_X1_2:
				cmd_play_forward(0.5);
				break;

			case CMD_FWD_X1:
				cmd_play_forward(1.0);
				break;

			case CMD_FWD_X2:
				cmd_play_forward(2.0);
				break;

			case CMD_FWD_X3:
				cmd_play_forward(3.0);
				break;

			case CMD_FWD_X4:
				cmd_play_forward(4.0);
				break;

			case CMD_FWD_X5:
				cmd_play_forward(5.0);
				break;

			case CMD_SKIP_FWD_10:
				cmd_skip_forward(10);
				break;

			case CMD_SKIP_FWD_20:
				cmd_skip_forward(20);
				break;

			case CMD_SKIP_FWD_30:
				cmd_skip_forward(30);
				break;

			case CMD_SKIP_FWD_40:
				cmd_skip_forward(40);
				break;

			case CMD_SKIP_FWD_50:
				cmd_skip_forward(50);
				break;

			case CMD_SKIP_FWD_60:
				cmd_skip_forward(60);
				break;

			case CMD_SKIP_FWD_70:
				cmd_skip_forward(70);
				break;

			case CMD_SKIP_FWD_80:
				cmd_skip_forward(80);
				break;

			case CMD_SKIP_FWD_90:
				cmd_skip_forward(90);
				break;

			case CMD_SKIP_FWD_100:
				cmd_skip_forward(100);
				break;
		}

		if (!is_command_number(command))
		{
			player.cmd_number_length = 0;
		}

	}
	else if (command == CMD_NO_ENTRY)
	{
		player.status |= STATUS_READY;

	}
	else
	{
		player.status &= ~STATUS_READY;

	}
}

static UINT32 bcd_to_literal(UINT32 bcd)
{
	UINT32 value = 0;
	UINT32 shift = 28;
	UINT32 multiplier = 10000000;
	for (UINT32 i = 0; i < 8; i++)
	{
		UINT32 digit = (bcd >> shift) & 0xf;
		bcd &= ~(0xf << shift);

		value += digit * multiplier;

		multiplier /= 10;
		shift -= 4;
	}
	return value;
}

static UINT32 literal_to_bcd(UINT32 value)
{

	UINT32 bcd_value = 0;
	UINT32 shift = 28;
	UINT32 multiplier = 10000000;
	for (UINT32 i = 0; i < 8; i++)
	{
		UINT32 digit = value / multiplier;
		bcd_value |= digit << shift;

		value -= digit * multiplier;
		multiplier /= 10;
		shift -= 4;
	}

	return bcd_value;
}

static UINT32 cmd_number_to_bcd(void)
{
	if (player.cmd_number_length == 0)
	{

		return 0;
	}
	UINT32 bcd_value = 0;
	size_t shift = 0;
	for (int i = (int)player.cmd_number_length - 1; i >= 0; i--)
	{
		bcd_value |= player.cmd_number[i] << shift;
		shift += 4;
	}

	return bcd_value;
}

static UINT8 cmd_to_number(const UINT8 cmd)
{
	switch (cmd)
	{
		case CMD_0: return 0;
		case CMD_1: return 1;
		case CMD_2: return 2;
		case CMD_3: return 3;
		case CMD_4: return 4;
		case CMD_5: return 5;
		case CMD_6: return 6;
		case CMD_7: return 7;
		case CMD_8: return 8;
		case CMD_9: return 9;
	}
	return 0;
}

static void set_mode(const UINT8 mode)
{
	if (player.mode == mode)
	{
		return;
	}

	player.mode = mode;
	if (mode != MODE_PARK)
	{
		timer_adjust(park_timer, TIME_NEVER, 0, 0);
	}
	else
	{
		timer_adjust(park_timer, TIME_IN_MSEC(21), 0, TIME_IN_MSEC(21));
	}
}

static void set_playing(const UINT8 new_status, const double fields_per_vsync)
{
	set_mode(MODE_PLAY);
	player.status = new_status;
	player.play_speed = fields_per_vsync;
	player.search_frame = ~0U;
	player.stop_frame = ~0U;
}

static void set_stopped(const UINT8 new_status)
{
	set_mode(MODE_STOP);
	player.status = new_status;
}

static void cmd_play(void)
{

	set_slider_speed(0);
	set_playing(STATUS_PLAY, 1.0);
}

static void cmd_stop(void)
{
	if (player.cmd_number_length > 0)
	{
		const UINT32 command_value = cmd_number_to_bcd();
		timer_adjust(stop_timer, TIME_IN_MSEC(100 * command_value), 0, 0);
		player.pre_stop_status = player.status;

	}
	else
	{

	}
	set_stopped(STATUS_STOP);
}

static void cmd_step(const int direction)
{
	advance_slider(direction);
	set_stopped(STATUS_STOP);
	player.stop_frame = ~0U;
	player.search_frame = ~0U;
}

static void cmd_play_forward(const double fields_per_vsync)
{

	set_slider_speed(fields_per_vsync);
	set_playing(STATUS_FORWARD, fields_per_vsync);
}

static void cmd_search(const UINT32 frame)
{
	set_mode(MODE_SEARCH);
	player.status = STATUS_SEARCH;
	player.search_frame = frame;
	player.stop_frame = ~0U;
}

static void cmd_skip_forward(const INT32 amount)
{

	set_mode(MODE_SEARCH);
	player.search_frame = ~0U;
	player.stop_frame = player.curr_frame + amount;
	player.scan_speed = amount;
}

static void update_video_enable(void)
{

	player.video_enabled = player.mode == MODE_PLAY || player.mode == MODE_STOP;
}

static void update_audio_enable(void)
{
	if (player.mode == MODE_PLAY && player.play_speed == 1.0)
	{

		player.audio_active[0] = player.audio_enable[0];
		player.audio_active[1] = player.audio_enable[1];
	}
	else
	{

		player.audio_active[0] = player.audio_active[1] = 0;
	}
}

static void status_off(int param) { player.status_strobe = 1; }
static void command_off(int param) { player.command_strobe = 1; }
static void command_on(int param)
{
	player.command_strobe = 0;
	timer_adjust(command_off_timer, TIME_IN_USEC(25), 0, 0);
}
static void status_on(int param)
{
	player.status_strobe = 0;
	timer_adjust(status_off_timer, TIME_IN_USEC(26), 0, 0);
	timer_adjust(command_on_timer, TIME_IN_USEC(54), 0, 0);
}
static void resume_play(int param) { set_playing(player.pre_stop_status, player.play_speed); }

static void audio_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	int i;
	for (i = 0; i < samples; ++i)
	{
		int available = audio_out != audio_in;
		outputs[0][i] = available ? audio_buffer[0][audio_out] : 0;
		outputs[1][i] = available ? audio_buffer[1][audio_out] : 0;
		if (available) audio_out = (audio_out + 1) % AUDIO_BUFFER_SIZE;
	}
}

static void *audio_start(int clock, const struct CustomSound_interface *config)
{
	audio_stream = stream_create(0, 2, 48000, 0, audio_update);
	return &player;
}
const struct CustomSound_interface ldv1000_sound = { audio_start, 0, 0, 0 };

int ldv1000_init(void)
{
	char metadata[256];
	UINT32 size;
	int fps, fraction, interlaced, channels;
	const chd_header *header;
	disc = get_disk_handle(0);
	if (!disc) return 1;
	header = chd_get_header(disc);
	total_hunks = header->totalhunks;
	hunk_bytes = header->hunkbytes;
	if (chd_get_metadata(disc, AV_METADATA_TAG, 0, metadata, sizeof(metadata), &size, 0, 0) != CHDERR_NONE ||
		sscanf(metadata, AV_METADATA_FORMAT, &fps, &fraction, &width, &height, &interlaced, &channels, &sample_rate) != 7 ||
		width != 720 || height != 262 || !interlaced || channels != 2 || (sample_rate != 44100 && sample_rate != 48000))
	{
		fatalerror("Unsupported LD-V1000 disc format");
	}
	stream_set_sample_rate(audio_stream, sample_rate);
	vbi = auto_malloc(total_hunks * 16);
	if (chd_get_metadata(disc, CHD_MAKE_TAG('A','V','L','D'), 0, vbi, total_hunks * 16, &size, 0, 0) != CHDERR_NONE || size != total_hunks * 16)
		fatalerror("Laserdisc requires field VBI metadata");
	hunk = auto_malloc(hunk_bytes);
	video = auto_bitmap_alloc_depth(width, height * 2, 32);
	park_timer = timer_alloc(status_on);
	stop_timer = timer_alloc(resume_play);
	status_off_timer = timer_alloc(status_off);
	command_on_timer = timer_alloc(command_on);
	command_off_timer = timer_alloc(command_off);
	return 0;
}

void ldv1000_reset(void)
{
	memset(&player, 0, sizeof(player));
	player.status = STATUS_PARK | STATUS_READY;
	player.search_frame = player.stop_frame = ~0U;
	player.audio_enable[0] = player.audio_enable[1] = 1;
	player.status_strobe = player.command_strobe = 1;
	track = field = 0;
	decoded_hunk = -1;
	slider_speed = slider_fraction = 0;
	audio_in = audio_out = 0;
	fillbitmap(video, 0, 0);
	timer_adjust(stop_timer, TIME_NEVER, 0, 0);
	timer_adjust(status_off_timer, TIME_NEVER, 0, 0);
	timer_adjust(command_on_timer, TIME_NEVER, 0, 0);
	timer_adjust(command_off_timer, TIME_NEVER, 0, 0);
	timer_adjust(park_timer, TIME_IN_MSEC(21), 0, TIME_IN_MSEC(21));
}

void ldv1000_write(UINT8 data)
{
	if (player.cmd_length < sizeof(player.cmd_buffer)) player.cmd_buffer[player.cmd_length++] = data;
	if (is_command_byte(data)) process_command_buffer();
}
UINT8 ldv1000_read(void) { return player.status; }
int ldv1000_status_strobe(void) { return player.status_strobe; }
int ldv1000_ready(void) { return player.command_strobe; }

static UINT32 field_code(int hunk_index)
{
	const UINT8 *p = vbi + hunk_index * 16 + 13;
	return (p[0] << 16) | (p[1] << 8) | p[2];
}

static void process_vbi(void)
{
	UINT32 code = field_code(track * 2 + field);
	UINT32 old_frame = player.curr_frame;
	if ((code & 0xf00000) != 0xf00000 && code != 0x88ffff && code != 0x80eeee) return;
	player.curr_frame = code == 0x88ffff ? 0 : code == 0x80eeee ? 54000 : bcd_to_literal(code & 0x7ffff);
	if (player.mode == MODE_STOP) return;
	if (player.stop_frame != ~0U && player.search_frame == ~0U)
	{
		INT32 old_delta = player.stop_frame - old_frame;
		INT32 delta = player.stop_frame - player.curr_frame;
		if (!delta || ((old_delta < 0) != (delta < 0)))
		{
			player.stop_frame = ~0U;
			set_stopped((player.status & STATUS_READY) | STATUS_STOP);
		}
	}
	if (player.search_frame != ~0U)
	{
		INT32 delta = player.search_frame - player.curr_frame;
		if (!delta)
		{
			player.search_frame = ~0U;
			set_stopped(STATUS_SEARCH_FINISH);
		}
		else if (delta < 0) advance_slider(MIN(-2, delta / 2));
		else if (delta > 2) advance_slider(MAX(1, delta / 2));
	}
}

static int clamp_color(int value) { return value < 0 ? 0 : value > 255 ? 255 : value; }

static void read_field(void)
{
	int samples, x, y, channel, i;
	const UINT8 *sound, *pixels;
	int hunk_index = track * 2 + field;
	/* Searches need only VBI metadata; avoid decoding invisible, muted fields. */
	if (!player.video_enabled && !player.audio_active[0] && !player.audio_active[1]) return;
	if (decoded_hunk != hunk_index)
	{
		chd_error error = chd_read(disc, hunk_index, hunk);
		if (error != CHDERR_NONE) fatalerror("Laserdisc read failed: %s", chd_error_string(error));
		decoded_hunk = hunk_index;
	}
	samples = (hunk[6] << 8) | hunk[7];
	/* Validate before deriving payload pointers: a bad field must never overrun the hunk. */
	if (memcmp(hunk, "chav", 4) || hunk[5] != 2 || ((hunk[8] << 8) | hunk[9]) != width ||
		((hunk[10] << 8) | hunk[11]) != height || 12 + hunk[4] + samples * 4 + width * height * 2 > hunk_bytes)
		fatalerror("Invalid laserdisc field payload");
	sound = hunk + 12 + hunk[4];
	pixels = sound + samples * 4;
	/* Flush the previous field before changing the samples or channel squelch. */
	stream_update(audio_stream, 0);
	for (i = 0; i < samples; ++i)
	{
		unsigned next = (audio_in + 1) % AUDIO_BUFFER_SIZE;
		if (next == audio_out) break;
		for (channel = 0; channel < 2; ++channel)
		{
			const UINT8 *p = sound + channel * samples * 2 + i * 2;
			audio_buffer[channel][audio_in] = player.audio_active[channel] ? (INT16)((p[0] << 8) | p[1]) : 0;
		}
		audio_in = next;
	}
	for (y = 0; y < height; ++y)
	{
		UINT32 *dest = (UINT32 *)video->line[y * 2 + field];
		for (x = 0; x < width; x += 2)
		{
			int y0 = pixels[0] - 16, cb = pixels[1] - 128;
			int y1 = pixels[2] - 16, cr = pixels[3] - 128;
			int r = 409 * cr + 128, g = -100 * cb - 208 * cr + 128, b = 516 * cb + 128;
			dest[x] = MAKE_RGB(clamp_color((298*y0+r)>>8), clamp_color((298*y0+g)>>8), clamp_color((298*y0+b)>>8));
			dest[x+1] = MAKE_RGB(clamp_color((298*y1+r)>>8), clamp_color((298*y1+g)>>8), clamp_color((298*y1+b)>>8));
			pixels += 4;
		}
	}
}

void ldv1000_field(void)
{
	if (player.mode != MODE_PARK)
	{
		/* Audio/video gates change together on field zero, including during seeks. */
		if (!field) { update_video_enable(); update_audio_enable(); }
		read_field();
		process_vbi();
		timer_set(TIME_IN_USEC(500), 0, status_on);
		if (field)
		{
			if (player.mode == MODE_PLAY || player.mode == MODE_SEARCH) advance_slider(1);
			else if (player.mode == MODE_SCAN_FORWARD || player.mode == MODE_SCAN_REVERSE)
			{
				int elapsed;
				player.scan_speed_accum += player.scan_speed;
				elapsed = player.scan_speed_accum / 60;
				player.scan_speed_accum -= elapsed * 60;
				advance_slider(player.mode == MODE_SCAN_REVERSE ? -elapsed : elapsed);
			}
		}
		slider_fraction += slider_speed / 2;
		if (slider_fraction >= 1 || slider_fraction <= -1)
		{
			int delta = slider_fraction;
			advance_slider(delta);
			slider_fraction -= delta;
		}
	}
	field ^= 1;
}

void ldv1000_video(mame_bitmap *bitmap, const rectangle *cliprect)
{
	if (player.video_enabled) copybitmap(bitmap, video, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
	else fillbitmap(bitmap, 0, cliprect);
}

/* The parallel HLE player and the firmware-driven player share one AV transport. */
void laserdisc_raw_reset(void)
{
	ldv1000_reset();
	timer_adjust(park_timer, TIME_NEVER, 0, 0);
	track = -1000;
}
void laserdisc_raw_advance(int delta)
{
	int max_track = total_hunks / 2 - 1;
	track += delta;
	if (track < -1000) track = -1000;
	if (track > max_track) track = max_track;
}
int laserdisc_raw_track(void) { return track; }
UINT32 laserdisc_raw_code(int line)
{
	const UINT8 *p;
	if (track < 0) return line == 16 ? 0 : 0x88ffff;
	p = vbi + (track * 2 + field) * 16 + (line == 16 ? 4 : line == 17 ? 7 : line == 18 ? 10 : 13);
	return (p[0] << 16) | (p[1] << 8) | p[2];
}
void laserdisc_raw_field(int field_number, int video_enabled, int audio_enabled)
{
	field = field_number;
	player.video_enabled = video_enabled && track >= 0;
	player.audio_active[0] = player.audio_active[1] = audio_enabled && track >= 0;
	if (track >= 0) read_field();
}

int laserdisc_raw_at_limit(void)
{
	int max_track = total_hunks / 2 - 1;
	return track <= -1000 || track >= max_track;
}
