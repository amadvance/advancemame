/***************************************************************************

    CDRDAO TOC parser for CHD compression frontend

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "chd.h"
#include "md5.h"
#include "sha1.h"
#include "cdrom.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#endif

/***************************************************************************
    CONSTANTS & DEFINES
***************************************************************************/

#define EATWHITESPACE	\
	while ((linebuffer[i] == ' ' || linebuffer[i] == 0x09 || linebuffer[i] == '\r') && (i < 512))	\
	{	\
		i++;	\
	}

#define EATQUOTE	\
	while ((linebuffer[i] == '"' || linebuffer[i] == '\'') && (i < 512))	\
	{	\
		i++;	\
	}

#define TOKENIZE	\
	j = 0; \
	while (!isspace(linebuffer[i]) && (i < 512) && (j < 128))	\
	{	\
		token[j] = linebuffer[i];	\
		i++;	\
		j++;	\
	}	\
	token[j] = '\0';

#define TOKENIZETOCOLON	\
	j = 0; \
	while ((linebuffer[i] != ':') && (i < 512) && (j < 128))	\
	{	\
		token[j] = linebuffer[i];	\
		i++;	\
		j++;	\
	}	\
	token[j] = '\0';

#define TOKENIZETOCOLONINC	\
	j = 0; \
	while ((linebuffer[i] != ':') && (i < 512) && (j < 128))	\
	{	\
		token[j] = linebuffer[i];	\
		i++;	\
		j++;	\
	}	\
	token[j++] = ':';	\
	token[j] = '\0';

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static char linebuffer[512];

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    get_file_size - returns the 64-bit file size
    for a file
-------------------------------------------------*/

static UINT64 get_file_size(const char *file)
{
#ifdef _WIN32
	DWORD highSize = 0, lowSize;
	HANDLE handle;
	UINT64 filesize;

	/* attempt to open the file */
	handle = CreateFile(file, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return 0;

	/* get the file size */
	lowSize = GetFileSize(handle, &highSize);
	filesize = lowSize | ((UINT64)highSize << 32);
	if (lowSize == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
		filesize = 0;

	/* close the file and return */
	CloseHandle(handle);
	return filesize;
#else
	size_t filesize;
	FILE *f;

	/* attempt to open the file */
	f = fopen(file, "rb");
	if (!f)
		return 0;

	/* get the size */
	fseek(f, 0, SEEK_END);
	filesize = ftell(f);
	fclose(f);

	return filesize;
#endif
}

/*-------------------------------------------------
    cdrom_parse_toc - parse a CDRDAO format TOC file
-------------------------------------------------*/

int cdrom_parse_toc(char *tocfname, cdrom_toc *outtoc, cdrom_track_input_info *outinfo)
{
	FILE *infile;
	int i, j, k, trknum, m, s, f, foundcolon;
	static char token[128];

	infile = fopen(tocfname, "rt");

	if (infile == (FILE *)NULL)
	{
		return CHDERR_FILE_NOT_FOUND;
	}

	/* clear structures */
	memset(outtoc, 0, sizeof(cdrom_toc));
	memset(outinfo, 0, sizeof(cdrom_track_input_info));

	trknum = -1;

	while (!feof(infile))
	{
		/* get the next line */
		fgets(linebuffer, 511, infile);

		/* if EOF didn't hit, keep going */
		if (!feof(infile))
		{
			i = 0;
			EATWHITESPACE
			TOKENIZE

			if ((!strcmp(token, "DATAFILE")) || (!strcmp(token, "FILE")))
			{
				/* found the data file for a track */
				EATWHITESPACE
				EATQUOTE
				TOKENIZE

				/* remove trailing quote if any */
				if (token[strlen(token)-1] == '"')
				{
					token[strlen(token)-1] = '\0';
				}

				/* keep the filename */
				strncpy(&outinfo->fname[trknum][0], token, strlen(token));

				/* get either the offset or the length */
				EATWHITESPACE

				if (linebuffer[i] == '#')
				{
					/* it's a decimal offset, use it */
					TOKENIZE
					outinfo->offset[trknum] = strtoul(&token[1], NULL, 10);

					/* we're using this token, go on */
					EATWHITESPACE
					TOKENIZETOCOLONINC
				}
				else
				{
					/* no offset, just M:S:F */
					outinfo->offset[trknum] = 0;
					TOKENIZETOCOLONINC
				}

				/*
                   This is tricky: the next number can be either a raw
                   number or an M:S:F number.  Check which it is.
                   If a space or LF/CR/terminator occurs before a colon in the token,
                   it's a raw number.
                */

trycolonagain:
				foundcolon = 0;
				for (k = 0; k < strlen(token); k++)
				{
					if ((token[k] <= ' ') && (token[k] != ':'))
					{
						break;
					}

					if (token[k] == ':')
					{
						foundcolon = 1;
						break;
					}
				}

				if (!foundcolon)
				{
					// rewind to the start of the real MSF
					while (linebuffer[i] != ' ')
					{
						i--;
					}

					// check for spurious offset included by newer CDRDAOs
					if ((token[0] == '0') && (token[1] == ' '))
					{
						i++;
						EATWHITESPACE
						TOKENIZETOCOLONINC
						goto trycolonagain;
					}

					i++;
					TOKENIZE


					f = strtoul(token, NULL, 10);
				}
				else
				{
					/* now get the MSF format length (might be MSF offset too) */
					m = strtoul(token, NULL, 10);
					i++;	/* skip the colon */
					TOKENIZETOCOLON
					s = strtoul(token, NULL, 10);
					i++;	/* skip the colon */
					TOKENIZE
					f = strtoul(token, NULL, 10);

					/* convert to just frames */
					s += (m * 60);
					f += (s * 75);
				}

				EATWHITESPACE
				if (isdigit(linebuffer[i]))
				{
					f *= outtoc->tracks[trknum].datasize;

					outinfo->offset[trknum] += f;

					EATWHITESPACE
					TOKENIZETOCOLON

					m = strtoul(token, NULL, 10);
					i++;	/* skip the colon */
					TOKENIZETOCOLON
					s = strtoul(token, NULL, 10);
					i++;	/* skip the colon */
					TOKENIZE
					f = strtoul(token, NULL, 10);

					/* convert to just frames */
					s += (m * 60);
					f += (s * 75);
				}

				if (f)
				{
					outtoc->tracks[trknum].frames = f;
				}
				else	/* track can't be zero length, guesstimate it */
				{
					UINT64 tlen;

					printf("Warning: Estimating length of track %d.  If this is not the final or only track\n on the disc, the estimate may be wrong.\n", trknum+1);

					tlen = get_file_size(outinfo->fname[trknum]);

					tlen /= (outtoc->tracks[trknum].datasize + outtoc->tracks[trknum].subsize);

					outtoc->tracks[trknum].frames = tlen;
				}
			}
			else if (!strcmp(token, "TRACK"))
			{
				/* found a new track */
				trknum++;

				/* next token on the line is the track type */
				EATWHITESPACE
				TOKENIZE

				if (!strcmp(token, "MODE1"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_MODE1;
					outtoc->tracks[trknum].datasize = 2048;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "MODE1_RAW"))
				{
					printf("\nWarning: Track %d uses RAW format data.  Although such images may be created, MAME cannot read them at present.\n", trknum+1);
					printf("To get usable data, burn this image to a disc or CD emulator and read as to a normal image with CDRDAO.\n\n");

					outtoc->tracks[trknum].trktype = CD_TRACK_MODE1_RAW;
					outtoc->tracks[trknum].datasize = 2352;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "MODE2"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_MODE2;
					outtoc->tracks[trknum].datasize = 2336;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "MODE2_FORM1"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_MODE2_FORM1;
					outtoc->tracks[trknum].datasize = 2048;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "MODE2_FORM2"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_MODE2_FORM2;
					outtoc->tracks[trknum].datasize = 2324;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "MODE2_FORM_MIX"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_MODE2_FORM_MIX;
					outtoc->tracks[trknum].datasize = 2336;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "MODE2_RAW"))
				{
					printf("\nWarning: Track %d uses RAW format data.  Although such images may be created, MAME cannot read them at present.\n", trknum+1);
					printf("To get usable data, burn this image to a disc or CD emulator and read it as a normal image with CDRDAO.\n");

					outtoc->tracks[trknum].trktype = CD_TRACK_MODE2_RAW;
					outtoc->tracks[trknum].datasize = 2352;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else if (!strcmp(token, "AUDIO"))
				{
					outtoc->tracks[trknum].trktype = CD_TRACK_AUDIO;
					outtoc->tracks[trknum].datasize = 2352;
					outtoc->tracks[trknum].subtype = CD_SUB_NONE;
					outtoc->tracks[trknum].subsize = 0;
				}
				else
				{
					printf("ERROR: Unknown track type [%s].  Contact MAMEDEV.\n", token);
				}

				/* next (optional) token on the line is the subcode type */
				EATWHITESPACE
				TOKENIZE

				if (!strcmp(token, "RW"))
				{
					outtoc->tracks[trknum].subtype = CD_SUB_NORMAL;
					outtoc->tracks[trknum].subsize = 96;
				}
				else if (!strcmp(token, "RW_RAW"))
				{
					outtoc->tracks[trknum].subtype = CD_SUB_RAW;
					outtoc->tracks[trknum].subsize = 96;
				}
			}
		}
	}

	/* close the input TOC */
	fclose(infile);

	/* store the number of tracks found */
	outtoc->numtrks = trknum + 1;

	return CHDERR_NONE;
}
