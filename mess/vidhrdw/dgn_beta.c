/*
	vidhrdw/dgn_beta.c
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/m6845.h"
#include "mscommon.h"

#include "includes/crtc6845.h"
#include "includes/dgn_beta.h"

struct crtc6845_config dgnbeta_crtc_config =
{
	1000000,
	0,
	M6845_PERSONALITY_GENUINE
};

void init_video(void)
{
	videoram_size = 0x2000;
	crtc6845 = crtc6845_init(&dgnbeta_crtc_config);
}

VIDEO_UPDATE(dgnbeta)
{
	int x, y, i;
	int w=crtc6845_get_char_columns(crtc6845);
	int h=crtc6845_get_char_lines(crtc6845);
	int height=crtc6845_get_char_height(crtc6845);
	int start=crtc6845_get_start(crtc6845)&0x7ff;
	int full_refresh = 1;

	if (full_refresh) 
	{
		memset(dirtybuffer, 1, videoram_size);
	}

logerror("DgnBeta Video update !\n");	
	for (y=0, i=start; y<h;y++) 
		{
		for (x=0;x<w;x++, i=(i+1)&0x7ff) 
		{
			if (dirtybuffer[i]) 
			{
				drawgfx(bitmap,Machine->gfx[dgnbeta_font],
						videoram[i*2], 0, 0, 0, 8*x,height*y,
						&Machine->visible_area,TRANSPARENCY_NONE,0);
				dirtybuffer[i]=0;
			}
		}
	}
}
