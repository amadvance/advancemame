// license:BSD-3-Clause
// copyright-holders:Curt Coder
/******************************************************************************
 PeT mess@utanet.at 2000
******************************************************************************/
#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"

#include "includes/studio2.h"

/*
  emulation of the rcata10171v1 (cdp1861) video controller
 */


static struct {
	UINT8 data[128][8];
	int byte;
	int line;
	int dma_activ;
	int state;
	int count;
} studio2_video={ { {0} } };

int studio2_get_vsync(void)
{
	return !studio2_video.dma_activ;
}

#define COLUMNS 14
#define LINES 256
void studio2_video_sc_w(int state)
{
	/* Each state code accounts for one eight-clock machine cycle, including
	   DMA and the extra execute cycle of long instructions. */
	if (state == CDP1802_STATE_CODE_S3_INTERRUPT)
		activecpu_set_input_line(CDP1802_INPUT_LINE_INT, INTERNAL_CLEAR_LINE);

	switch (studio2_video.state) {
	case 0: // deactivated
		break;
	case 1:
		studio2_video.count=29;
		studio2_video.line=0;
		studio2_video.state=10;
		activecpu_set_input_line(CDP1802_INPUT_LINE_INT, INTERNAL_ASSERT_LINE);
		studio2_video.dma_activ=1;
		break;
	case 2:
		--studio2_video.count;
		if (studio2_video.count<0) {
			studio2_video.line=0;
			studio2_video.state=10;
			studio2_video.count+=COLUMNS;
		}
		break;
	case 10:
		--studio2_video.count;
		if (studio2_video.count<=0) {
			/* These callbacks run inside the CPU: change lines immediately,
			   since queued changes would allow extra DMA transfers. The
			   shared core updates R0 and accounts for the stolen cycles. */
			studio2_video.byte=0;
			activecpu_set_input_line(CDP1802_INPUT_LINE_DMAOUT, INTERNAL_ASSERT_LINE);
			studio2_video.count+=COLUMNS;
		}
		break;
	case 11:
		--studio2_video.count;
		if (studio2_video.count<=0) {
// while dma_activ is high Register0 is corrected for doublescanning
// after is it waiting for it going high again
			studio2_video.dma_activ=1; 
			studio2_video.count+=COLUMNS;
			studio2_video.state++;
			if (++studio2_video.line>=256) studio2_video.state=1;
		}
		break;
	case 12:
		--studio2_video.count;
		if (studio2_video.count<=0) {
			studio2_video.dma_activ=0; 
			studio2_video.count+=COLUMNS;
			studio2_video.state=11;
			if (++studio2_video.line>=256) studio2_video.state=1;
		}
		break;
	}
}

void studio2_video_dma_w(UINT8 data)
{
	studio2_video.data[studio2_video.line][studio2_video.byte++]=data;
	if (studio2_video.byte==8) {
		activecpu_set_input_line(CDP1802_INPUT_LINE_DMAOUT, INTERNAL_CLEAR_LINE);
		if (++studio2_video.line>=128) {
			studio2_video.dma_activ=0;
			studio2_video.count+=2*COLUMNS;
			studio2_video.state=11;
		}
	}
}

void studio2_video_reset(void)
{
	/* A reset must cancel pending transfers before the CPU starts at R0. */
	memset(&studio2_video, 0, sizeof(studio2_video));
	cpunum_set_input_line(0, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
	cpunum_set_input_line(0, CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);
}

VIDEO_START( studio2 )
{
	/* video chip is initially disabled */
	studio2_video.state=0;
	return 0;
}

VIDEO_UPDATE( studio2 )
{
	int x, y, j;

	for (y=0; y<128;y++) {
		for (x=0, j=0; j<8;j++,x+=8*4)
			drawgfx(bitmap, Machine->gfx[0], studio2_video.data[y][j],0,
					0,0,x,y,
					0, TRANSPARENCY_NONE,0);
	}
}

 READ8_HANDLER( cdp1861_video_enable_r )
{
	studio2_video.state=1;
	return 0; //?
}

