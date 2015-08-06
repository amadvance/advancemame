/***************************************************************************

	stream.c

	Code for implementing Imgtool streams

***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <assert.h>

#include "unzip.h"
#include "osdepend.h"
#include "imgtool.h"
#include "utils.h"
#include "osd_tool.h"

typedef enum
{
	IMG_FILE,
	IMG_MEM
} imgtype_t;

struct _imgtool_stream
{
	imgtype_t imgtype;
	int write_protect;
	const char *name; // needed for clear
	INT64 position;

	union
	{
		osd_tool_file *f;
		struct
		{
			char *buf;
			size_t bufsz;
		} m;
	} u;
};



static imgtool_stream *stream_open_zip(const char *zipname, const char *subname, int read_or_write)
{
	imgtool_stream *imgfile = NULL;
	zip_file *z = NULL;
	zip_entry *zipent;
	FILE *f;

	if (read_or_write)
		goto error;

	/* check to see if the file exists */
	f = fopen(zipname, "r");
	if (!f)
		goto error;
	fclose(f);

	imgfile = malloc(sizeof(struct _imgtool_stream));
	if (!imgfile)
		goto error;

	memset(imgfile, 0, sizeof(*imgfile));
	imgfile->imgtype = IMG_MEM;
	imgfile->write_protect = 1;
	imgfile->position = 0;

	z = openzip(0, 0, zipname);
	if (!z)
		goto error;

	do
	{
		zipent = readzip(z);
		if (!zipent)
			goto error;
	}
	while(subname && strcmp(subname, zipent->name));

	imgfile->u.m.bufsz = zipent->uncompressed_size;
	imgfile->u.m.buf = malloc(zipent->uncompressed_size);
	if (!imgfile->u.m.buf)
		goto error;

	if (readuncompresszip(z, zipent, imgfile->u.m.buf))
		goto error;

	closezip(z);
	return imgfile;

error:
	if (z)
		closezip(z);
	if (imgfile)
	{
		if (imgfile->u.m.buf)
			free(imgfile->u.m.buf);
		free(imgfile);
	}
	return NULL;
}



imgtool_stream *stream_open(const char *fname, int read_or_write)
{
	const char *ext;
	imgtool_stream *imgfile = NULL;
	static const char *write_modes[] = {"rb", "wb", "rb+", "wb+"};
	osd_tool_file *f = NULL;
	char *buf = NULL;
	int len, i;
	imgtool_stream *s = NULL;
	char c;

	/* maybe we are just a ZIP? */
	ext = strrchr(fname, '.');
	if (ext && !mame_stricmp(ext, ".zip"))
		return stream_open_zip(fname, NULL, read_or_write);

	f = osd_tool_fopen(fname, write_modes[read_or_write]);
	if (!f)
	{
		if (!read_or_write)
		{
			len = strlen(fname);

			/* can't open the file; try opening ZIP files with other names */
			buf = malloc(len + 1);
			if (!buf)
				goto error;
			strcpy(buf, fname);

			for(i = len-1; !s && (i >= 0); i--)
			{
				if ((buf[i] == '\\') || (buf[i] == '/'))
				{
					c = buf[i];
					buf[i] = '\0';
					s = stream_open_zip(buf, buf + i + 1, read_or_write);
					buf[i] = c;
				}
			}
			free(buf);
			buf = NULL;

			if (s)
				return s;
		}

		/* ah well, it was worth a shot */
		goto error;
	}

	imgfile = malloc(sizeof(struct _imgtool_stream));
	if (!imgfile)
		goto error;

	/* Normal file */
	memset(imgfile, 0, sizeof(*imgfile));
	imgfile->imgtype = IMG_FILE;
	imgfile->position = 0;
	imgfile->write_protect = read_or_write ? 0 : 1;
	imgfile->u.f = f;
	imgfile->name = fname;
	return imgfile;

error:
	if (imgfile)
		free((void *) imgfile);
	if (f)
		osd_tool_fclose(f);
	if (buf)
		free(buf);
	return (imgtool_stream *) NULL;
}



imgtool_stream *stream_open_write_stream(int size)
{
	imgtool_stream *imgfile;

	imgfile = malloc(sizeof(struct _imgtool_stream));
	if (!imgfile)
		return NULL;

	imgfile->imgtype = IMG_MEM;
	imgfile->write_protect = 0;
	imgfile->position = 0;

	imgfile->u.m.bufsz = size;
	imgfile->u.m.buf = malloc(size);

	if (!imgfile->u.m.buf)
	{
		free(imgfile);
		return NULL;
	}

	return imgfile;
}



imgtool_stream *stream_open_mem(void *buf, size_t sz)
{
	imgtool_stream *imgfile;

	imgfile = malloc(sizeof(struct _imgtool_stream));
	if (!imgfile)
		return NULL;

	memset(imgfile, 0, sizeof(*imgfile));
	imgfile->imgtype = IMG_MEM;
	imgfile->position = 0;
	imgfile->write_protect = 0;

	imgfile->u.m.bufsz = sz;
	imgfile->u.m.buf = buf;
	return imgfile;
}



void stream_close(imgtool_stream *s)
{
	assert(s);

	switch(s->imgtype)
	{
		case IMG_FILE:
			osd_tool_fclose(s->u.f);
			break;

		case IMG_MEM:
			free(s->u.m.buf);
			break;

		default:
			assert(0);
			break;
	}
	free((void *) s);
}



size_t stream_read(imgtool_stream *s, void *buf, size_t sz)
{
	size_t result = 0;

	switch(s->imgtype)
	{
		case IMG_FILE:
			result = osd_tool_fread(s->u.f, s->position, sz, buf);
			break;

		case IMG_MEM:
			if ((s->position + sz) > s->u.m.bufsz)
				result = s->u.m.bufsz - s->position;
			else
				result = sz;
			memcpy(buf, s->u.m.buf + s->position, result);
			break;

		default:
			assert(0);
			break;
	}
	s->position += result;
	return result;
}



size_t stream_write(imgtool_stream *s, const void *buf, size_t sz)
{
	size_t result = 0;

	switch(s->imgtype)
	{
		case IMG_MEM:
			if (!s->write_protect)
			{
				if (s->u.m.bufsz < s->position + sz)
				{
					s->u.m.buf = realloc(s->u.m.buf, s->position + sz);
					s->u.m.bufsz = s->position + sz;
				}
				memcpy(s->u.m.buf + s->position, buf, sz);
				result = sz;
			}
			break;

		case IMG_FILE:
			result = osd_tool_fwrite(s->u.f, s->position, sz, buf);
			break;

		default:
			assert(0);
			break;
	}
	s->position += result;
	return result;
}



UINT64 stream_size(imgtool_stream *s)
{
	UINT64 result = 0;

	switch(s->imgtype)
	{
		case IMG_FILE:
			result = osd_tool_flength(s->u.f);
			break;

		case IMG_MEM:
			result = s->u.m.bufsz;
			break;

		default:
			assert(0);
			break;
	}
	return result;
}



void *stream_getptr(imgtool_stream *f)
{
	void *ptr;

	switch(f->imgtype)
	{
		case IMG_MEM:
			ptr = f->u.m.buf;
			break;

		default:
			ptr = NULL;
			break;
	}
	return ptr;
}



int stream_seek(imgtool_stream *s, INT64 pos, int where)
{
	UINT64 size;

	size = stream_size(s);

	switch(where)
	{
		case SEEK_CUR:
			pos += s->position;
			break;
		case SEEK_END:
			pos += size;
			break;
	}

	if (pos < 0)
		s->position = 0;
	else
		s->position = MIN(size, pos);

	if (s->position < pos)
		stream_fill(s, '\0', pos - s->position);

	return 0;
}



size_t stream_tell(imgtool_stream *s)
{
	return (size_t) s->position;
}



size_t stream_transfer(imgtool_stream *dest, imgtool_stream *source, size_t sz)
{
	size_t result = 0;
	size_t readsz;
	char buf[1024];

	while(sz && (readsz = stream_read(source, buf, MIN(sz, sizeof(buf)))))
	{
		stream_write(dest, buf, readsz);
		sz -= readsz;
		result += readsz;
	}
	return result;
}



size_t stream_transfer_all(imgtool_stream *dest, imgtool_stream *source)
{
	return stream_transfer(dest, source, stream_size(source));
}



int stream_crc(imgtool_stream *s, unsigned long *result)
{
	size_t sz;
	void *ptr;

	switch(s->imgtype)
	{
		case IMG_MEM:
			*result = crc32(0, (unsigned char *) s->u.m.buf, s->u.m.bufsz);
			break;

		default:
			sz = stream_size(s);
			ptr = malloc(sz);
			if (!ptr)
				return IMGTOOLERR_OUTOFMEMORY;
			stream_seek(s, 0, SEEK_SET);
			if (stream_read(s, ptr, sz) != sz)
				return IMGTOOLERR_READERROR;
			*result = crc32(0, ptr, sz);
			free(ptr);
			break;
	}
	return 0;
}



int file_crc(const char *fname,  unsigned long *result)
{
	int err;
	imgtool_stream *f;

	f = stream_open(fname, OSD_FOPEN_READ);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND;

	err = stream_crc(f, result);
	stream_close(f);
	return err;
}

size_t stream_fill(imgtool_stream *f, unsigned char b, UINT64 sz)
{
	size_t outsz;
	char buf[1024];

	outsz = 0;
	memset(buf, b, MIN(sz, sizeof(buf)));

	while(sz)
	{
		outsz += stream_write(f, buf, MIN(sz, sizeof(buf)));
		sz -= MIN(sz, sizeof(buf));
	}
	return outsz;
}



void stream_clear(imgtool_stream *s)
{
	switch(s->imgtype)
	{
		case IMG_FILE:
			if (!s->write_protect)
			{
				osd_tool_fclose(s->u.f);
				s->u.f = osd_tool_fopen(s->name, "wb+");
				osd_tool_fclose(s->u.f);
				s->u.f = osd_tool_fopen(s->name, "wb");
			}
			break;

		default:
			/* Need to implement */
			assert(0);
			break;
	}
}



int stream_isreadonly(imgtool_stream *s)
{
	return s->write_protect;
}



size_t stream_putc(imgtool_stream *stream, char c)
{
	return stream_write(stream, &c, 1);
}



size_t stream_puts(imgtool_stream *stream, const char *s)
{
	return stream_write(stream, s, strlen(s));
}



size_t stream_printf(imgtool_stream *stream, const char *fmt, ...)
{
	va_list va;
	char buf[256];

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);

	return stream_puts(stream, buf);
}


