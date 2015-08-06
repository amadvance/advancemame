/*********************************************************************

	unicode.c

	Unicode related functions

*********************************************************************/

#include "unicode.h"
#include "utils.h"
#include "mamecore.h"

int uchar_isvalid(unicode_char_t uchar)
{
	return (uchar < 0x110000) && !((uchar >= 0xd800) && (uchar <= 0xdfff));
}



int uchar_from_utf8(unicode_char_t *uchar, const char *utf8char, size_t count)
{
	unicode_char_t c, minchar;
	int auxlen, i;
	char auxchar;

	if (!utf8char || !count)
		return 0;

	c = (unsigned char) *utf8char;
	count--;
	utf8char++;

	if (c < 0x80)
	{
		/* unicode char 0x00000000 - 0x0000007F */
		c &= 0x7f;
		auxlen = 0;
		minchar = 0x00000000;
	}
	else if ((c >= 0xc0) & (c < 0xe0))
	{
		/* unicode char 0x00000080 - 0x000007FF */
		c &= 0x1f;
		auxlen = 1;
		minchar = 0x00000080;
	}
	else if ((c >= 0xe0) & (c < 0xf0))
	{
		/* unicode char 0x00000800 - 0x0000FFFF */
		c &= 0x0f;
		auxlen = 2;
		minchar = 0x00000800;
	}
	else if ((c >= 0xf0) & (c < 0xf8))
	{
		/* unicode char 0x00010000 - 0x001FFFFF */
		c &= 0x07;
		auxlen = 3;
		minchar = 0x00010000;
	}
	else if ((c >= 0xf8) & (c < 0xfc))
	{
		/* unicode char 0x00200000 - 0x03FFFFFF */
		c &= 0x03;
		auxlen = 4;
		minchar = 0x00200000;
	}
	else if ((c >= 0xfc) & (c < 0xfe))
	{
		/* unicode char 0x04000000 - 0x7FFFFFFF */
		c &= 0x01;
		auxlen = 5;
		minchar = 0x04000000;
	}
	else
	{
		/* invalid */
		return -1;
	}

	/* exceeds the count? */
	if (auxlen > count)
		return -1;

	/* we now know how long the char is, now compute it */
	for (i = 0; i < auxlen; i++)
	{
		auxchar = utf8char[i];

		/* all auxillary chars must be between 0x80-0xbf */
		if ((auxchar & 0xc0) != 0x80)
			return -1;

		c = c << 6;
		c |= auxchar & 0x3f;
	}

	/* make sure that this char is above the minimum */
	if (c < minchar)
		return -1;

	*uchar = c;
	return auxlen + 1;
}



int uchar_from_utf16(unicode_char_t *uchar, const utf16_char_t *utf16char, size_t count)
{
	int rc;

	if ((utf16char[0] > 0xd800) && (utf16char[0] < 0xdbff))
	{
		if ((utf16char[1] > 0xdc00) && (utf16char[1] < 0xdfff))
		{
			*uchar = 0x10000 + (utf16char[0] & 0x3ff) + ((utf16char[1] & 0x3ff) * 0x400);
			rc = 2;
		}
		else
		{
			rc = -1;
		}
	}
	else if ((utf16char[0] > 0xdc00) && (utf16char[0] < 0xdfff))
	{
		rc = -1;
	}
	else
	{
		*uchar = utf16char[0];
		rc = 1;
	}
	return rc;
}



int uchar_from_utf16f(unicode_char_t *uchar, const utf16_char_t *utf16char, size_t count)
{
	utf16_char_t buf[2];
	if (count > 0)
		buf[0] = FLIPENDIAN_INT16(utf16char[0]);
	if (count > 1)
		buf[1] = FLIPENDIAN_INT16(utf16char[0]);
	return uchar_from_utf16(uchar, buf, count);
}



int utf8_from_uchar(char *utf8string, size_t count, unicode_char_t uchar)
{
	int rc = 0;
	if (!uchar_isvalid(uchar))
		return -1;

	if (uchar < 0x80)
	{
		/* unicode char 0x00000000 - 0x0000007F */
		utf8string[rc++] = (char) uchar;
	}
	else if (uchar < 0x800)
	{
		/* unicode char 0x00000080 - 0x000007FF */
		utf8string[rc++] = ((char) (uchar >> 6)) | 0xC0;
		utf8string[rc++] = ((char) ((uchar >> 0) & 0x3F)) | 0x80;
	}
	else if (uchar < 0x10000)
	{
		/* unicode char 0x00000800 - 0x0000FFFF */
		utf8string[rc++] = ((char) (uchar >> 12)) | 0xE0;
		utf8string[rc++] = ((char) ((uchar >> 6) & 0x3F)) | 0x80;
		utf8string[rc++] = ((char) ((uchar >> 0) & 0x3F)) | 0x80;
	}
	else if (uchar < 0x00200000)
	{
		/* unicode char 0x00010000 - 0x001FFFFF */
		utf8string[rc++] = ((char) (uchar >> 18)) | 0xF0;
		utf8string[rc++] = ((char) ((uchar >> 12) & 0x3F)) | 0x80;
		utf8string[rc++] = ((char) ((uchar >> 6) & 0x3F)) | 0x80;
		utf8string[rc++] = ((char) ((uchar >> 0) & 0x3F)) | 0x80;
	}
	else if (uchar < 0x04000000)
	{
		/* unicode char 0x00200000 - 0x03FFFFFF */
		utf8string[rc++] = ((char) (uchar >> 24)) | 0xF8;
		utf8string[rc++] = ((char) ((uchar >> 18) & 0x3F)) | 0x80;
		utf8string[rc++] = ((char) ((uchar >> 12) & 0x3F)) | 0x80;
		utf8string[rc++] = ((char) ((uchar >> 6) & 0x3F)) | 0x80;
		utf8string[rc++] = ((char) ((uchar >> 0) & 0x3F)) | 0x80;
	}
	else if (uchar < 0x80000000)
	{
		/* unicode char 0x04000000 - 0x7FFFFFFF */
		utf8string[rc++] = ((char) (uchar >> 30)) | 0xFC;
		utf8string[rc++] = ((char) ((uchar >> 24) & 0x3F)) | 0x80;
		utf8string[rc++] = ((char) ((uchar >> 18) & 0x3F)) | 0x80;
		utf8string[rc++] = ((char) ((uchar >> 12) & 0x3F)) | 0x80;
		utf8string[rc++] = ((char) ((uchar >> 6) & 0x3F)) | 0x80;
		utf8string[rc++] = ((char) ((uchar >> 0) & 0x3F)) | 0x80;
	}
	else
	{
		rc = -1;
	}
	return rc;
}



int utf16_from_uchar(utf16_char_t *utf16string, size_t count, unicode_char_t uchar)
{
	int rc;
	if (!uchar_isvalid(uchar))
		return -1;

	if (uchar < 0x10000)
	{
		if (count < 1)
			return -1;
		utf16string[0] = (utf16_char_t) uchar;
		rc = 1;
	}
	else if (uchar < 0x100000)
	{
		if (count < 2)
			return -1;
		utf16string[0] = ((uchar >> 10) & 0x03ff) | 0xd800;
		utf16string[1] = ((uchar >>  0) & 0x03ff) | 0xdc00;
		rc = 2;
	}
	else
		return -1;
	return rc;
}



int utf16f_from_uchar(utf16_char_t *utf16string, size_t count, unicode_char_t uchar)
{
	int rc;
	utf16_char_t buf[2] = { 0, 0 };

	rc = utf16_from_uchar(buf, count, uchar);

	if (rc >= 1)
		utf16string[0] = FLIPENDIAN_INT16(buf[0]);
	if (rc >= 2)
		utf16string[1] = FLIPENDIAN_INT16(buf[1]);
	return rc;
}




