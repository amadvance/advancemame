/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#include "portable.h"

#include "wave.h"
#include "endianrw.h"
#include "log.h"

#define WAVE_FORMAT_PCM 0x0001 /* Microsoft Pulse Code Modulation (PCM) format */
#define IBM_FORMAT_MULAW 0x0101 /* IBM mu-law format */
#define IBM_FORMAT_ALAW 0x0102 /* IBM a-law format */
#define IBM_FORMAT_ADPCM 0x0103 /* IBM AVC Adaptive Differential Pulse Code Modulation format */

static adv_error le_uint16_fwrite(FILE* f, unsigned v)
{
	unsigned char p[2];

	le_uint16_write(p, v);
	if (fwrite(p, 2, 1, f) != 1)
		return -1;

	return 0;
}

static adv_error le_uint32_fwrite(FILE* f, unsigned v)
{
	unsigned char p[4];

	le_uint32_write(p, v);
	if (fwrite(p, 4, 1, f) != 1)
		return -1;

	return 0;
}

static adv_error wave_write_tag(FILE* f, const char* tag)
{
	if (fwrite(tag, 1, 4, f) != 4)
		return -1;
	return 0;
}

static adv_error wave_write_id(FILE* f, const char* tag, unsigned size)
{
	if (wave_write_tag(f, tag) != 0)
		return -1;
	if (le_uint32_fwrite(f, size) != 0)
		return -1;
	return 0;
}

static adv_error wave_write_fmt(FILE* f, unsigned format_tag, unsigned channels, unsigned samples_per_sec, unsigned avg_u8s_per_sec, unsigned block_align)
{
	if (le_uint16_fwrite(f, format_tag) != 0)
		return -1;
	if (le_uint16_fwrite(f, channels) != 0)
		return -1;
	if (le_uint32_fwrite(f, samples_per_sec) != 0)
		return -1;
	if (le_uint32_fwrite(f, avg_u8s_per_sec) != 0)
		return -1;
	if (le_uint16_fwrite(f, block_align) != 0)
		return -1;
	return 0;
}

static adv_error wave_write_fmt_PCM(FILE* f, unsigned bits_per_sample)
{
	if (le_uint16_fwrite(f, bits_per_sample) != 0)
		return -1;
	return 0;
}

/**
 * Write a WAVE header to a file.
 * \param f File to write.
 * \param channel Number of channels.
 * \param bit Bits per sample.
 * \param size Size of the data in bytes.
 * \param freq Frequency in Hz.
 */
adv_error wave_write(FILE* f, unsigned channel, unsigned bit, unsigned size, unsigned freq)
{
	unsigned size_byte;
	if (bit <= 8)
		size_byte = 1;
	else if (bit <= 16)
		size_byte = 2;
	else
		size_byte = 4;

	if (wave_write_id(f, "RIFF", 0x24 + size) != 0)
		return -1;

	if (wave_write_tag(f, "WAVE") != 0)
		return -1;

	if (wave_write_id(f, "fmt ", 0x10) != 0)
		return -1;

	if (wave_write_fmt(f, WAVE_FORMAT_PCM, channel, freq, size_byte * channel * freq, size_byte * channel) != 0)
		return -1;

	if (wave_write_fmt_PCM(f, bit) != 0)
		return -1;

	if (wave_write_id(f, "data", size) != 0)
		return -1;

	return 0;
}

/**
 * Adjust a WAVE header setting the correct size.
 * \param f File to write.
 * \param size Number of samples.
 */
adv_error wave_write_size(FILE* f, unsigned size)
{
	unsigned dsize;

	dsize = 0x24 + size;
	if (fseek(f, 4, SEEK_SET) != 0)
		return -1;
	if (le_uint32_fwrite(f, dsize) != 0)
		return -1;

	dsize = size;
	if (fseek(f, 0x28, SEEK_SET) != 0)
		return -1;
	if (le_uint32_fwrite(f, dsize) != 0)
		return -1;

	return 0;
}

static adv_error wave_read_tag(adv_fz* f, char* tag)
{
	if (fzread(tag, 4, 1, f) != 1)
		return -1;

	return 0;
}

static adv_error wave_skip(adv_fz* f, unsigned num)
{
	unsigned count = 0;
	while (count<num) {
		char ch;
		if (fzread(&ch, 1, 1, f) != 1)
			return -1;
		++count;
	}
	return 0;
}

static adv_error wave_read_id(adv_fz* f, char* tag, unsigned* size)
{
	if (wave_read_tag(f, tag) != 0)
		return -1;
	if (le_uint32_fzread(f, size) != 0)
		return -1;
	return 0;
}

static adv_error wave_read_fmt(adv_fz* f, unsigned* format_tag, unsigned* channels, unsigned* samples_per_sec, unsigned* avg_u8s_per_sec, unsigned* block_align)
{
	if (le_uint16_fzread(f, format_tag) != 0)
		return -1;
	if (le_uint16_fzread(f, channels) != 0)
		return -1;
	if (le_uint32_fzread(f, samples_per_sec) != 0)
		return -1;
	if (le_uint32_fzread(f, avg_u8s_per_sec) != 0)
		return -1;
	if (le_uint16_fzread(f, block_align) != 0)
		return -1;
	return 0;
}

static adv_error wave_read_fmt_PCM(adv_fz* f, unsigned* size)
{
	if (le_uint16_fzread(f, size) != 0)
		return -1;
	return 0;
}

/**
 * Read a WAVE header from a file.
 * \param f File to read.
 * \param channel Number of channels. 1 or 2.
 * \param bit Bit per samples. 8 or 16.
 * \param size Size of the data in bytes.
 * \param freq Frequency in Hz.
 */
adv_error wave_read(adv_fz* f, unsigned* channel, unsigned* bit, unsigned* size, unsigned* freq)
{
	char id[4];
	unsigned r_size;
	unsigned r_bit;
	unsigned r_format_tag;
	unsigned r_channel;
	unsigned r_freq;
	unsigned avg_u8s_per_sec;
	unsigned block_align;

	if (wave_read_id(f, id, &r_size) != 0)
		return -1;
	if (memcmp(id, "RIFF", 4) != 0)
		return -1;

	if (wave_read_tag(f, id) != 0)
		return -1;
	if (memcmp(id, "WAVE", 4) != 0)
		return -1;

	/* read until the fmt tag */
	if (wave_read_id(f, id, &r_size) != 0)
		return -1;
	while (memcmp(id, "fmt ", 4) != 0) {
		if (wave_skip(f, r_size)!=0)
			return -1;
		if (wave_read_id(f, id, &r_size) != 0)
			return -1;
	}
	if (memcmp(id, "fmt ", 4) != 0)
		return -1;
	if (r_size < 16)
		return -1;

	if (wave_read_fmt(f, &r_format_tag, &r_channel, &r_freq, &avg_u8s_per_sec, &block_align) != 0)
		return -1;
	if (r_format_tag != WAVE_FORMAT_PCM)
		return -1;
	if (r_channel != 1 && r_channel != 2)
		return -1;

	if (wave_read_fmt_PCM(f, &r_bit)!=0)
		return -1;
	if (r_bit != 8 && r_bit != 16)
		return -1;

	/* skip any unknown extension at the format */
	if (wave_skip(f, r_size - 16) != 0)
		return -1;

	/* read until the data tag */
	if (wave_read_id(f, id, &r_size) != 0)
		return -1;
	while (memcmp(id, "data", 4) != 0) {
		if (wave_skip(f, r_size) != 0)
			return -1;
		if (wave_read_id(f, id, &r_size) != 0)
			return -1;
	}
	if (memcmp(id, "data", 4) != 0)
		return -1;

	log_std(("wave: read channel:%d bit:%d size:%d freq:%d\n", r_channel, r_bit, r_size, r_freq));

	*channel = r_channel;
	*bit = r_bit;
	*size = r_size;
	*freq = r_freq;

	return 0;
}
