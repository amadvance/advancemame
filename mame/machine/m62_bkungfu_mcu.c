// license:BSD-3-Clause
// copyright-holders:David Haywood, Andrea Bogazzi

/*	MCU simulation code for IREM's Beyond Kung Fu

	The MCU is fully in charge of the tilemap layer, copying text strings
	and background data from a partially encrypted external ROM

	The exact MCU type is not confirmed

	TODO:

	- determine the purpose of the optional fifth/sixth level composition pointers
	- background draw timing isn't 100% correct, although this has no impact on overall game timing
	- does the hardware really have twice the VRAM as the other boards?
	- test mode doesn't work (there are strings for it in the MCU data ROM, is the MCU involved, or is it incomplete)

	NOTES:

	Command 0x01 at offset 0x00 uses the table at 0x200

	The Z80 parameter is the direct table index and advances modulo eight in the
	order 1,2,3,4,5,6,7,0.  Consequently slot 0 below is gameplay stage 8, slots
	1-7 are gameplay stages 1-7, and redraw slots 8-15 have the same ordering.
	Older notes below use "Stage 1-8" as shorthand for table slots 0-7.

	Table for levels
	These point to tilemap structures in their initial state, before objects at
	the start of each stage have been animated.

	0200  4D 18 | 184d
	0202  DD 17 | 17dd
	0204  6D 17 | 176d
	0206  FD 16 | 16fd
	0208  8D 16 | 168d
	020A  1D 16 | 161d
	020C  AD 15 | 15ad
	020E  3D 15 | 153d

	2nd Tables for levels
	These point to tilemap structures with moving objects (doors, trapdoors) in
	their post-animated state, used to redraw the level when the level name text
	needs to be removed without resetting the moving pieces to their original state.

	0210  4D 18 | 184d
	0212  DD 17 | 17dd
	0214  6D 17 | 176d
	0216  7D 1A | 1a7d
	0218  0D 1A | 1a0d
	021A  9D 19 | 199d
	021C  2D 19 | 192d
	021E  BD 18 | 18bd

	This initial / redraw after animation table use can be confirmed by looking at the pairs.

	0200  4D 18 | 184d / 0210  4D 18 | 184d  - identical in both states (Stage 1 data)
	0202  DD 17 | 17dd / 0212  DD 17 | 17dd  - identical in both states (Stage 2 data)
	0204  6D 17 | 176d / 0214  6D 17 | 176d  - identical in both states (Stage 3 data)
	0206  FD 16 | 16fd / 0216  7D 1A | 1a7d  - different (Stage 4 data)
	0208  8D 16 | 168d / 0218  0D 1A | 1a0d  - different (Stage 5 data)
	020A  1D 16 | 161d / 021A  9D 19 | 199d  - different (Stage 6 data)
	020C  AD 15 | 15ad / 021C  2D 19 | 192d  - different (Stage 7 data)
	020E  3D 15 | 153d / 021E  BD 18 | 18bd  - different (Stage 8 data)

	The game has 8 stages, the first 3 stages do not contain animated objects.
	The remaining stages have animated objects (animated with different commands)
	that close behind the player when they first enter the stage.

	Stage 4 contains a door on the very right of the tilemap
	Stage 5 contains a trap door on the very left of the tilemap
	Stage 6 contains a trap door on the very right of the tilemap
	Stage 7 contains a trap door on the very left of the tilemap
	Stage 8 contains a trap door on the very right of the tilemap

	if you sort by address pointed to you get

	020E  3D 15 | 153d
	020C  AD 15 | 15ad
	020A  1D 16 | 161d
	0208  8D 16 | 168d
	0206  FD 16 | 16fd
	0204  6D 17 | 176d / 0214  6D 17 | 176d
	0202  DD 17 | 17dd / 0212  DD 17 | 17dd
	0200  4D 18 | 184d / 0210  4D 18 | 184d
	021E  BD 18 | 18bd
	021C  2D 19 | 192d
	021A  9D 19 | 199d
	0218  0D 1A | 1a0d
	0216  7D 1A | 1a7d

	so each of these blocks is 0x70 bytes long giving the following ranges

	153d - 15ac
	15ad - 161c
	161d - 168c
	168d - 16fc
	16fd - 176c
	176d - 17dc
	17dd - 184c
	184d - 18bc
	18bd - 192c
	192d - 199c
	199d - 1a0d
	1a0d - 1a7d
	1a7d - 1aec

	data from 1aed - 7fff is referenced by these tables, see further notes below

	When the levels are drawn they're drawn in 4 tile wide strips, from top to
	bottom, left to right each screen has 8 of these strips each level is 7 screens
	wide.  This drawing is not instant, the draw-in can be seen on hardware footage.


	All multi-byte values described below are little-endian.  Bytes at ROM offsets
	0000-153c are plaintext.  Starting at 153d, each byte is independently decoded
	with a 256-byte key table K.  For ROM address A:

		s = (A_low + A_high) & ff
		first = (s & 1) ? (K[s] - cipher) : (cipher ^ K[s])

	This transform is address-local: it has no feedback from preceding bytes and
	can be performed as data is read.  No shorter generator for K is known.

	Level/object payload bytes and the pointers stored inside those payloads use a
	second address-dependent transform after the first one:

		if (s & 1)
			value = 60 - first
		else
			value = ((first & 20) ? a0 : 60) - first
					- ((first & 1) ? 0 : 2)

	All arithmetic is modulo 256.  Structure tokens are recognised at the stage
	used by their format, rather than blindly applying the payload transform to
	every byte.

	The level directory at 0200 contains sixteen pointers.  Each points to a
	70-byte block of 56 two-byte entry pointers.  Consecutive entry pairs describe
	two adjacent four-tile-wide columns: the even entry supplies the upper ten
	tile rows and the odd entry supplies the lower sixteen rows.  Each entry points
	to a 0000-terminated list of payload-encoded pointers.  The first four select
	the tile streams; optional fifth and sixth pointers select unresolved seven-
	byte records associated with the six boundaries between the seven screens.
	Each main stream supplies two adjacent tiles per row; four streams therefore
	form the eight tiles across the column pair.  Upper streams contain 20 literal
	cells and lower streams contain 32.  In a level stream, 00 terminates the
	stream, 01 followed by a payload byte changes the current attribute, and all
	other payload bytes are tile codes.  The current attribute is written beside
	every tile code in tilemap RAM.

	Object IDs 80-90 select seventeen pointers in the table at 0100.  The selected
	five-byte record is:

		width, destination_low, destination_high, stream_low, stream_high

	The object stream is row-major and wraps after the record's width.  Token 5f
	is followed by a payload-encoded attribute; 5e or 60 terminates the stream;
	other bytes are payload-encoded tile codes.  This same format describes both
	title dragons, both flame animation frames and the later-stage moving objects.

	ROM text commands use a two-byte pointer table indexed by the command's text
	number.  Text streams use 00 as terminator, 01 followed by an attribute, 02
	followed by a little-endian tilemap position, and literal tile bytes otherwise.
	The fixed HUD uses the same grammar through the pointer at 0140.  Command 0c
	uses an equivalent stream prepared by the Z80 in shared RAM.  Text positions
	advance by two bytes per character and wrap within a 64-tile (0x80-byte) row.

	The data format contains the tile codes and palette attributes.  In tilemap
	RAM they are emitted as adjacent bytes, tile first and attribute second.  The
	tile byte supplies code bits 0-7; attribute bits 5-7 supply code bits 8-10 and
	attribute bits 0-4 select the palette.  The separate background-bank latch
	supplies code bit 11.  The solid-colour low tile codes 04-0b in each bank are
	ordinary tiles aligned with the palette colours; they do not require a special
	fill opcode.

*/

#include "driver.h"
#include "includes/m62_bkungfu_mcu.h"

static UINT8 m_mailbox[0x800];
static UINT32 m_mailbox_mask;
static UINT32 m_data_rom_mask;
static UINT16 m_timer;
static UINT32 m_p1score;
static UINT32 m_topscore;
static UINT32 m_p2score;
static UINT8 m_lives;
static UINT8 m_player_energy;
static UINT8 m_boss_energy;
static UINT8 m_floorcount;
static UINT8 m_floorcount_state;
static UINT8 m_valid;
static int m_initialized;
static int m_running;
static UINT8 m_leveldraw_row;
static UINT8 m_leveldraw_column;
static UINT8 m_leveldraw_number;
static mame_timer *m_leveldraw_timer;
static UINT8 *m_data_rom;

static void clear();
static UINT8 read_data(UINT16 address);
static void vram_page_w(offs_t offset, UINT8 data);
static UINT8 decrypt_data(UINT16 address);
static void write_number(int x, int y, UINT8 number);
static void write_floor_dot(int which, int lit);
static void write_lifebar(int xbase, int ybase, UINT8 energy, int boss);
static void update_slot(UINT8 slot);
static void complete(UINT16 offset);
static void mailbox_out(UINT16 offset, UINT8 data);
static void draw_text(UINT16 table_offset, int use_mailbox);
static void draw_credits_continue();
static void clear_tilemap();
static void execute_slot(UINT8 slot);
static UINT8 mailbox_r(offs_t offset);
static void mailbox_from_main_w(offs_t offset, UINT8 data);
static void mailbox_w(offs_t offset, UINT8 data);
static UINT8 decode_payload(UINT16 address);
static UINT16 decode_payload_word(UINT16 address);
static void draw_object(UINT8 id);
static void draw_level_column_row(int column, int row, UINT8 tile, UINT8 attr);
static void draw_level_strip(int column, int row);
static void command_w(UINT8 command);
static void clear()
{
	m_initialized = 0;
}

static UINT8 read_data(UINT16 address)
{
	return m_data_rom[address & m_data_rom_mask];
}

static void vram_page_w(offs_t offset, UINT8 data)
{
	// the tilemap needs to be 256 tiles wide for the backgrounds, which are copied in a single command
	// however the blitter commands seem to only have enough co-ordinates for the current 64 tile page
	// and the higher bits aren't communicated to the MCU, so assume they mirror across all pages for now
	//
	// It's also possible the tilemap is still 64 tiles wide, like kungfum and the MCU is loading in
	// backgrounds as needed, even if the command to draw the background is only sent at the start of
	// a level.  The draw-in time on the background might give clues to this.

	int xpart = offset & 0x7f;
	int ypart = offset & ~0x7f;

	for (int page = 0; page < 0x200; page += 0x80)
	{
		int realoffset = (ypart << 2) | xpart | page;
		bkungfu_tileram_w(realoffset, data);
	}
}


static UINT8 decrypt_data(UINT16 address)
{
	if (address >= 0x8000)
		return 0xff;

	UINT8 const cipher = read_data(address);
	UINT8 const index = (UINT8)((address & 0xff) + (address >> 8));

	if (index & 1)
		return (UINT8)(
			(index ^ ((index & 2) ? 0x7e : 0x12)) - 0x20 - cipher);

	return (UINT8)(cipher ^ index ^ ((index & 2) ? 0x5e : 0xae));
}

static void write_number(int x, int y, UINT8 number)
{
	vram_page_w(((y * 0x40 + x) << 1) & 0x0fff, (number & 0x0f) + 0x30);
}

static void write_floor_dot(int which, int lit)
{
	vram_page_w(((3 * 0x40 + 0x20 + which * 2) << 1) & 0x0fff, lit ? 0xd5 : 0xd6);
}

static void write_lifebar(int xbase, int ybase, UINT8 energy, int boss)
{
	int const full_segments = (energy & 0x78) >> 3;
	for (int segment = 0; segment < 8; segment++)
	{
		UINT8 const part = (segment < full_segments) ? 8 : (segment == full_segments) ? (energy & 7) : 0;
		UINT8 const tile = part ? (UINT8)((boss ? 0xcc : 0xc4) + 8 - part) : 0xc2;
		vram_page_w(((ybase * 0x40 + xbase + segment) << 1) & 0x0fff, tile);
	}
}

static void update_slot(UINT8 slot)
{
	if (!m_initialized)
		return;

	switch (slot)
	{
	case 0x10:
		for (int i = 0; i < 6; i++) write_number(0x14 + i, 0, (m_p1score >> ((5 - i) * 4)) & 0x0f);
		break;
	case 0x14:
		for (int i = 0; i < 6; i++) write_number(0x29 + i, 0, (m_p2score >> ((5 - i) * 4)) & 0x0f);
		break;
	case 0x18:
		for (int i = 0; i < 6; i++) write_number(0x1f + i, 0, (m_topscore >> ((5 - i) * 4)) & 0x0f);
		break;
	case 0x1c:
		for (int i = 0; i < 4; i++) write_number(0x25 + i, 5, (m_timer >> ((3 - i) * 4)) & 0x0f);
		break;
	case 0x20:
		for (int i = 0; i < 8; i++) write_floor_dot(i, i <= ((int)(m_floorcount) - (m_floorcount_state == 0x02)));
		break;
	case 0x24:
		write_number(0x2d, 5, m_lives);
		break;
	case 0x28:
		if (m_player_energy <= 0x40) write_lifebar(0x17, 2, m_player_energy, 0);
		break;
	case 0x2c:
		if (m_boss_energy <= 0x40) write_lifebar(0x17, 4, m_boss_energy, 1);
		break;
	}
}

static void complete(UINT16 offset)
{
	mailbox_out(offset, 0xfe);
}

static void mailbox_out(UINT16 offset, UINT8 data)
{
	offset &= m_mailbox_mask;
	m_mailbox[offset] = data;
	
}

static void draw_text(UINT16 table_offset, int use_mailbox)
{
	if (!m_running)
		return;

	UINT16 data_address;
	const UINT8 *data;
	UINT32 data_mask;
	UINT8 position_low;
	UINT8 position_high;
	UINT8 attribute;
	if (use_mailbox)
	{
		data_address = 0x100;
		data = m_mailbox;
		data_mask = m_mailbox_mask;
		position_low = 3;
		position_high = 4;
		attribute = 5;
	}
	else
	{
		data_address = read_data(table_offset) | ((UINT16)(read_data(table_offset + 1)) << 8);
		data = m_data_rom;
		data_mask = m_data_rom_mask;
		position_low = 2;
		position_high = 3;
		attribute = 4;
	}

	// Allow at most one full pass through the source, including command operands.
	INT32 remaining = data_mask + 1;
	while (remaining > 0)
	{
		UINT8 const value = data[data_address++ & data_mask];
		remaining--;
		if (value == 0)
			return;

		if (value == 0x01)
		{
			if (remaining < 1)
				break;
			remaining--;
			mailbox_out(attribute, data[data_address++ & data_mask]);
		}
		else if (value == 0x02)
		{
			if (remaining < 2)
				break;
			remaining -= 2;
			mailbox_out(position_low, data[data_address++ & data_mask]);
			mailbox_out(position_high, data[data_address++ & data_mask]);
		}
		else
		{
			UINT16 position = ((UINT16)(m_mailbox[position_high]) << 8) | m_mailbox[position_low];
			vram_page_w(position & 0x0fff, value);
			vram_page_w((position + 1) & 0x0fff, m_mailbox[attribute]);
			position = (position & ~0x007f) | ((position + 2) & 0x007f);
			mailbox_out(position_low, position & 0xff);
			mailbox_out(position_high, position >> 8);
		}
	}
	logerror("%s: Invalid text stream (missing terminator or truncated command)\n", "bkungfu");
}

static void draw_credits_continue()
{
	if (!m_running)
		return;

	UINT16 const position = ((UINT16)(m_mailbox[3]) << 8) | m_mailbox[2];
	UINT8 const attribute = m_mailbox[4];
	vram_page_w(position & 0x0fff, (m_mailbox[1] >> 4) | 0x30);
	vram_page_w((position + 1) & 0x0fff, attribute);
	vram_page_w((position + 2) & 0x0fff, (m_mailbox[1] & 0x0f) | 0x30);
	vram_page_w((position + 3) & 0x0fff, attribute);
}

static void clear_tilemap()
{
	if (!m_running)
		return;

	for (UINT16 position = 0; position < 0x1000; position += 2)
	{
		vram_page_w(position, m_mailbox[2]);
		vram_page_w(position + 1, m_mailbox[1]);
	}
}

static void execute_slot(UINT8 slot)
{
	UINT8 const trigger = m_mailbox[slot];
	UINT8 const p1 = m_mailbox[slot + 1];
	UINT8 const p2 = m_mailbox[slot + 2];
	UINT8 const p3 = m_mailbox[slot + 3];
	switch (slot)
	{
	case 0x10:
		m_p1score = ((UINT32)(p1) << 16) | ((UINT32)(p2) << 8) | p3;
		break;
	case 0x14:
		m_p2score = ((UINT32)(p1) << 16) | ((UINT32)(p2) << 8) | p3;
		break;
	case 0x18:
		m_topscore = ((UINT32)(p1) << 16) | ((UINT32)(p2) << 8) | p3;
		break;
	case 0x1c:
		m_timer = ((UINT16)(p1) << 8) | p2;
		break;
	case 0x20:
		m_floorcount = p1;
		m_floorcount_state = trigger;
		break;
	case 0x24:
		m_lives = p1;
		break;
	case 0x28:
		m_player_energy = p1;
		break;
	case 0x2c:
		m_boss_energy = p1;
		break;
	default:
		return;
	}
	m_valid |= (UINT8)(1U << ((slot - 0x10) >> 2));
	update_slot(slot);
	complete(slot);
}

static UINT8 mailbox_r(offs_t offset)
{

	return m_mailbox[offset & m_mailbox_mask];
}

static void mailbox_from_main_w(offs_t offset, UINT8 data)
{
	if (offset < 0x800)
		mailbox_w(offset, data);

	if (offset == 0x00)
		command_w(data);
}

static void mailbox_w(offs_t offset, UINT8 data)
{
	if (offset >= sizeof(m_mailbox))
		return;

	m_mailbox[offset] = data;
	if (m_running && offset >= 0x10 && offset < 0x30 && !(offset & 3))
		execute_slot(offset);
}

static UINT8 decode_payload(UINT16 address)
{
	UINT8 const value = decrypt_data(address);
	UINT8 const sum = (UINT8)((address & 0xff) + (address >> 8));
	if (sum & 1)
		return (UINT8)(0x60 - value);

	UINT8 const base = (value & 0x20) ? 0xa0 : 0x60;
	return (UINT8)(base - value - ((value & 1) ? 0 : 2));
}

static UINT16 decode_payload_word(UINT16 address)
{
	return decode_payload(address) | ((UINT16)(decode_payload(address + 1)) << 8);
}

static void draw_object(UINT8 id)
{
	if (id < 0x80 || id > 0x90)
		return;

	UINT16 const recaddr = read_data(0x100 + 2 * (id - 0x80)) | ((UINT16)(read_data(0x100 + 2 * (id - 0x80) + 1)) << 8);
	if (recaddr == 0 || recaddr >= 0x8000 - 5)
		return;

	UINT8 const width = read_data(recaddr);
	UINT16 const pos = (read_data(recaddr + 1) | ((UINT16)(read_data(recaddr + 2)) << 8)) & 0xfff;
	UINT16 dataptr = read_data(recaddr + 3) | ((UINT16)(read_data(recaddr + 4)) << 8);
	if (width == 0 || width > 0x20 || dataptr >= 0x8000)
		return;

	UINT8 attrs[512], tiles[512];
	int count = 0;
	UINT8 attr = 0;
	while (dataptr < 0x8000 && count < 512)
	{
		UINT16 const address = dataptr;
		UINT8 const value = decrypt_data(dataptr++);
		if (value == 0x5f)
		{
			if (dataptr >= 0x8000)
				break;
			attr = decode_payload(dataptr++);
			continue;
		}
		if (value == 0x5e || value == 0x60)
			break;
		tiles[count] = decode_payload(address);
		attrs[count] = attr;
		count++;
	}

	for (int cell = 0; cell < count; cell++)
	{
		int const column = cell % width;
		int const row = cell / width;
		UINT16 const destination = (pos + row * 0x80 + column * 2) & 0xfff;
		vram_page_w(destination, tiles[cell]);
		vram_page_w(destination + 1, attrs[cell]);
	}
}

static void draw_level_column_row(int column, int row, UINT8 tile, UINT8 attr)
{
	int const offset = ((row + 6) * 256 + column * 4) * 2;
	for (int x = 0; x < 4; x++)
	{
		bkungfu_tileram_w(offset + x * 2, tile);
		bkungfu_tileram_w(offset + x * 2 + 1, attr);
	}
}

static void draw_level_strip(int column, int row)
{
	draw_level_column_row(column, row, 0x05, 0x19);
	int const source_entry = (column & ~1) + ((row >= 10) ? 1 : 0);
	int const source_row = (row < 10)
		? row + ((column & 1) ? 10 : 0)
		: row - 10 + ((column & 1) ? 16 : 0);
	UINT16 const table = 0x200 + ((m_leveldraw_number & 0x0f) << 1);
	UINT16 const block = read_data(table) | ((UINT16)(read_data(table + 1)) << 8);
	UINT16 const entry = block + source_entry * 2;
	if (block < 0x153d || entry >= 0x8000 - 1)
		return;

	UINT16 const record = decode_payload_word(entry);
	if (record < 0x1aed || record >= 0x8000 - 8)
		return;

	UINT8 tiles[4][64];
	UINT8 attrs[4][64];
	int count = -1;
	for (int stream = 0; stream < 4; stream++)
	{
		UINT16 dataptr = decode_payload_word(record + stream * 2);
		if (dataptr < 0x2349 || dataptr >= 0x8000)
			return;

		UINT8 attr = 0;
		int streamcount = 0;
		while (dataptr < 0x8000 && streamcount < 64)
		{
			UINT8 const value = decode_payload(dataptr++);
			if (value == 0x00)
				break;
			if (value == 0x01)
			{
				if (dataptr >= 0x8000)
					return;
				attr = decode_payload(dataptr++);
				continue;
			}
			tiles[stream][streamcount] = value;
			attrs[stream][streamcount] = attr;
			streamcount++;
		}

		if (count < 0)
			count = streamcount;
		else if (count != streamcount)
			return;
	}

	int const expected_count = (source_entry & 1) ? 32 : 20;
	if (count != expected_count)
		return;

	int const halfheight = count / 2;
	int const band = source_row / halfheight;
	int const bandrow = source_row % halfheight;
	int const offset = ((row + 6) * 256 + column * 4) * 2;
	for (int x = 0; x < 4; x++)
	{
		int const stream = band * 2 + x / 2;
		int const index = bandrow * 2 + x % 2;
		bkungfu_tileram_w(offset + x * 2, tiles[stream][index]);
		bkungfu_tileram_w(offset + x * 2 + 1, attrs[stream][index]);
	}
}

static void leveldraw_next(int param)
{
	draw_level_strip(m_leveldraw_column, m_leveldraw_row);
	if (++m_leveldraw_row == 26)
	{
		m_leveldraw_row = 0;
		m_leveldraw_column++;
	}
	if (m_leveldraw_column != 0x38)
		timer_adjust(m_leveldraw_timer, TIME_IN_USEC(175), 0, 0);
	else
		complete(0);
}

static void command_w(UINT8 command)
{
	switch (command)
	{
	case 0x01:
		m_leveldraw_number = m_mailbox[1];
		complete(0);
		break;

	case 0x02:
		m_leveldraw_row = 0;
		m_leveldraw_column = 0;
		timer_adjust(m_leveldraw_timer, TIME_IN_USEC(175), 0, 0);
		break;

	case 0x05:
		// Observed after command 0x0f for later-stage animation objects.
		// Its additional effect, if any, is not known yet.
		complete(0);
		break;

	case 0x08:
		clear_tilemap();
		clear();
		complete(0);
		break;

	case 0x0a:
	{
		UINT16 stream = m_data_rom[0x140] | ((UINT16)(m_data_rom[0x141]) << 8);
		UINT16 position = 0;
		UINT8 attribute = 0;
		INT32 remaining = m_data_rom_mask + 1;
		int terminated = 0;
		while (remaining > 0)
		{
			UINT8 const value = read_data(stream++);
			remaining--;
			if (value == 0x00)
			{
				terminated = 1;
				break;
			}
			if (value == 0x01)
			{
				if (remaining < 1)
					break;
				remaining--;
				attribute = read_data(stream++);
				continue;
			}
			if (value == 0x02)
			{
				if (remaining < 2)
					break;
				remaining -= 2;
				UINT8 const low = read_data(stream++);
				UINT8 const high = read_data(stream++);
				position = low | ((UINT16)(high) << 8);
				continue;
			}
			vram_page_w(position & 0x0fff, value);
			vram_page_w((position + 1) & 0x0fff, attribute);
			position = (position & ~0x007f) | ((position + 2) & 0x007f);
		}
		if (!terminated)
		{
			logerror("%s: Invalid initialization stream (missing terminator or truncated command)\n", "bkungfu");
			complete(0);
			break;
		}

		m_initialized = 1;
		for (UINT8 slot = 0x10; slot <= 0x2c; slot += 4)
		{
			if (m_valid & (1U << ((slot - 0x10) >> 2)))
				update_slot(slot);
		}
		complete(0);
		break;
	}

	case 0x0c:
		draw_text(0, 1);
		complete(0);
		break;

	case 0x0d:
	case 0x14:
		draw_text((UINT16)(m_mailbox[1]) << 1, 0);
		complete(0);
		break;

	case 0x0f:
		draw_object(m_mailbox[1]);
		complete(0);
		break;

	case 0x10:
		draw_credits_continue();
		complete(0);
		break;

	case 0xfe:
		m_running = 1;
		for (UINT8 slot = 0x10; slot <= 0x2c; slot += 4)
			complete(slot);
		complete(0x102);
		complete(0x106);
		complete(0x118);
		complete(0x11c);
		complete(0);
		break;
	}
}

MACHINE_RESET( bkungfu )
{
	memset(m_mailbox, 0, sizeof(m_mailbox));
	m_data_rom = memory_region(REGION_USER1);
	m_mailbox_mask = 0x7ff;
	m_data_rom_mask = 0xffff;
	m_timer = m_p1score = m_topscore = m_p2score = 0;
	m_lives = m_player_energy = m_boss_energy = 0;
	m_floorcount = m_floorcount_state = m_valid = 0;
	m_initialized = m_running = 0;
	m_leveldraw_row = m_leveldraw_column = m_leveldraw_number = 0;
	if (!m_leveldraw_timer) m_leveldraw_timer = timer_alloc(leveldraw_next);
	timer_adjust(m_leveldraw_timer, TIME_NEVER, 0, 0);
}

READ8_HANDLER( bkungfu_mailbox_r )
{ 
	return mailbox_r(offset); 
}

WRITE8_HANDLER( bkungfu_mailbox_w ) 
{ 
	mailbox_from_main_w(offset, data);
}
