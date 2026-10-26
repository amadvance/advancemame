// license:GPL-2.0+
// copyright-holders:Kevin Thacker, David Haywood
/***************************************************************************

  Sinclair ZX Spectrum border rendering engine

***************************************************************************/

#ifndef __BORDER_H__
#define __BORDER_H__

void force_border_redraw(void);
void set_last_border_color(int NewColor);
void draw_border(mame_bitmap *bitmap, int full_refresh,
		int TopBorderLines, int ScreenLines, int BottomBorderLines,
		int LeftBorderPixels, int ScreenPixels, int RightBorderPixels,
		int LeftBorderCycles, int ScreenCycles, int RightBorderCycles,
		int HorizontalRetraceCycles, int VRetraceTime, int EventID);

#endif /* __BORDER_H__ */
