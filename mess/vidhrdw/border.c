// license:GPL-2.0+
// copyright-holders:Kevin Thacker, David Haywood
/***************************************************************************

  Sinclair ZX Spectrum border rendering engine

***************************************************************************/

#include "driver.h"
#include "vidhrdw/border.h"
#include "eventlst.h"

static int CurrBorderColor = 0;
static int LastDisplayedBorderColor = -1;

void force_border_redraw(void)
{
	LastDisplayedBorderColor = -1;
}

void set_last_border_color(int NewColor)
{
	CurrBorderColor = NewColor & 0x07;
}

void draw_border(mame_bitmap *bitmap,
	int full_refresh,
	int TopBorderLines,
	int ScreenLines,
	int BottomBorderLines,
	int LeftBorderPixels,
	int ScreenPixels,
	int RightBorderPixels,
	int LeftBorderCycles,
	int ScreenCycles,
	int RightBorderCycles,
	int HorizontalRetraceCycles,
	int VRetraceTime,
	int EventID)
{
	EVENT_LIST_ITEM *pItem;
	int TotalScreenHeight = TopBorderLines + ScreenLines + BottomBorderLines;
	int TotalScreenWidth = LeftBorderPixels + ScreenPixels + RightBorderPixels;
	int DisplayCyclesPerLine = LeftBorderCycles + ScreenCycles + RightBorderCycles;
	int CyclesPerLine = DisplayCyclesPerLine + HorizontalRetraceCycles;
	int CyclesSoFar;
	int NumItems, CurrItem = 0, NextItem;
	int ScrY;
	rectangle r;

	if (DisplayCyclesPerLine <= 0)
		DisplayCyclesPerLine = 1;
	if (CyclesPerLine <= 0)
		CyclesPerLine = 1;

	pItem = EventList_GetFirstItem();
	NumItems = EventList_NumEvents();

	/* Find first and second event matching EventID */
	while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID))
		CurrItem++;
	NextItem = CurrItem + 1;
	while ((NextItem < NumItems) && (pItem[NextItem].Event_ID != EventID))
		NextItem++;

	/* If at most one matching event exists, update CurrBorderColor */
	if (CurrItem < NumItems && NextItem >= NumItems)
		CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;

	if ((NextItem >= NumItems) && (CurrBorderColor == LastDisplayedBorderColor) && !full_refresh)
	{
		/* Border color is unchanged and full refresh not requested */
	}
	else if (NextItem >= NumItems)
	{
		/* Single border color across entire frame */
		/* Top border */
		r.min_x = 0;
		r.max_x = TotalScreenWidth - 1;
		r.min_y = 0;
		r.max_y = TopBorderLines - 1;
		fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

		/* Left border */
		r.min_x = 0;
		r.max_x = LeftBorderPixels - 1;
		r.min_y = TopBorderLines;
		r.max_y = TopBorderLines + ScreenLines - 1;
		fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

		/* Right border */
		r.min_x = LeftBorderPixels + ScreenPixels;
		r.max_x = TotalScreenWidth - 1;
		r.min_y = TopBorderLines;
		r.max_y = TopBorderLines + ScreenLines - 1;
		fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

		/* Bottom border */
		r.min_x = 0;
		r.max_x = TotalScreenWidth - 1;
		r.min_y = TopBorderLines + ScreenLines;
		r.max_y = TotalScreenHeight - 1;
		fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

		LastDisplayedBorderColor = CurrBorderColor;
	}
	else
	{
		/* Multiple border colors in this frame */

		/* Process events occurring before first displayed scanline */
		while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= VRetraceTime))
		{
			CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
			do {
				CurrItem++;
			} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
		}

		CyclesSoFar = VRetraceTime;

		/* Draw top border lines */
		for (ScrY = 0; ScrY < TopBorderLines; ScrY++)
		{
			r.min_y = r.max_y = ScrY;
			if ((CurrItem >= NumItems) || (pItem[CurrItem].Event_Time >= (CyclesSoFar + DisplayCyclesPerLine)))
			{
				/* Uniform color on this line */
				r.min_x = 0;
				r.max_x = TotalScreenWidth - 1;
				fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
			}
			else
			{
				/* Multiple colors on this scanline */
				int ScrX = (int)((pItem[CurrItem].Event_Time - CyclesSoFar) * TotalScreenWidth / DisplayCyclesPerLine);
				if (ScrX > TotalScreenWidth)
					ScrX = TotalScreenWidth;

				r.min_x = 0;
				r.max_x = ScrX - 1;
				if (r.max_x >= r.min_x)
					fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

				CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
				do {
					CurrItem++;
				} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));

				while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time < (CyclesSoFar + DisplayCyclesPerLine)))
				{
					int NextScrX = (int)((pItem[CurrItem].Event_Time - CyclesSoFar) * TotalScreenWidth / DisplayCyclesPerLine);
					if (NextScrX > TotalScreenWidth)
						NextScrX = TotalScreenWidth;

					r.min_x = ScrX;
					r.max_x = NextScrX - 1;
					if (r.max_x >= r.min_x)
						fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

					ScrX = NextScrX;
					CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
					do {
						CurrItem++;
					} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
				}

				r.min_x = ScrX;
				r.max_x = TotalScreenWidth - 1;
				if (r.max_x >= r.min_x)
					fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
			}

			/* Process color changes during horizontal retrace */
			CyclesSoFar += CyclesPerLine;
			while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= CyclesSoFar))
			{
				CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
				do {
					CurrItem++;
				} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
			}
		}

		/* Draw left and right border adjacent to display lines */
		for (ScrY = TopBorderLines; ScrY < (TopBorderLines + ScreenLines); ScrY++)
		{
			int LineCyclesLeft = LeftBorderCycles > 0 ? LeftBorderCycles : 1;
			int LineCyclesRight = RightBorderCycles > 0 ? RightBorderCycles : 1;

			r.min_y = r.max_y = ScrY;

			/* Draw left border */
			if ((CurrItem >= NumItems) || (pItem[CurrItem].Event_Time >= (CyclesSoFar + LeftBorderCycles)))
			{
				r.min_x = 0;
				r.max_x = LeftBorderPixels - 1;
				fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
			}
			else
			{
				int ScrX = (int)((pItem[CurrItem].Event_Time - CyclesSoFar) * LeftBorderPixels / LineCyclesLeft);
				if (ScrX > LeftBorderPixels)
					ScrX = LeftBorderPixels;

				r.min_x = 0;
				r.max_x = ScrX - 1;
				if (r.max_x >= r.min_x)
					fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

				CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
				do {
					CurrItem++;
				} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));

				while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time < (CyclesSoFar + LeftBorderCycles)))
				{
					int NextScrX = (int)((pItem[CurrItem].Event_Time - CyclesSoFar) * LeftBorderPixels / LineCyclesLeft);
					if (NextScrX > LeftBorderPixels)
						NextScrX = LeftBorderPixels;

					r.min_x = ScrX;
					r.max_x = NextScrX - 1;
					if (r.max_x >= r.min_x)
						fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

					ScrX = NextScrX;
					CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
					do {
						CurrItem++;
					} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
				}

				r.min_x = ScrX;
				r.max_x = LeftBorderPixels - 1;
				if (r.max_x >= r.min_x)
					fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
			}

			/* Process color changes during screen draw area */
			while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= (CyclesSoFar + LeftBorderCycles + ScreenCycles)))
			{
				CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
				do {
					CurrItem++;
				} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
			}

			/* Draw right border */
			if ((CurrItem >= NumItems) || (pItem[CurrItem].Event_Time >= (CyclesSoFar + DisplayCyclesPerLine)))
			{
				r.min_x = LeftBorderPixels + ScreenPixels;
				r.max_x = TotalScreenWidth - 1;
				fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
			}
			else
			{
				int RightStartCycle = CyclesSoFar + LeftBorderCycles + ScreenCycles;
				int ScrX = LeftBorderPixels + ScreenPixels + (int)((pItem[CurrItem].Event_Time - RightStartCycle) * RightBorderPixels / LineCyclesRight);
				if (ScrX < LeftBorderPixels + ScreenPixels)
					ScrX = LeftBorderPixels + ScreenPixels;
				if (ScrX > TotalScreenWidth)
					ScrX = TotalScreenWidth;

				r.min_x = LeftBorderPixels + ScreenPixels;
				r.max_x = ScrX - 1;
				if (r.max_x >= r.min_x)
					fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

				CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
				do {
					CurrItem++;
				} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));

				while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time < (CyclesSoFar + DisplayCyclesPerLine)))
				{
					int NextScrX = LeftBorderPixels + ScreenPixels + (int)((pItem[CurrItem].Event_Time - RightStartCycle) * RightBorderPixels / LineCyclesRight);
					if (NextScrX > TotalScreenWidth)
						NextScrX = TotalScreenWidth;

					r.min_x = ScrX;
					r.max_x = NextScrX - 1;
					if (r.max_x >= r.min_x)
						fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

					ScrX = NextScrX;
					CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
					do {
						CurrItem++;
					} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
				}

				r.min_x = ScrX;
				r.max_x = TotalScreenWidth - 1;
				if (r.max_x >= r.min_x)
					fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
			}

			/* Process color changes during horizontal retrace */
			CyclesSoFar += CyclesPerLine;
			while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= CyclesSoFar))
			{
				CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
				do {
					CurrItem++;
				} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
			}
		}

		/* Draw bottom border lines */
		for (ScrY = TopBorderLines + ScreenLines; ScrY < TotalScreenHeight; ScrY++)
		{
			r.min_y = r.max_y = ScrY;
			if ((CurrItem >= NumItems) || (pItem[CurrItem].Event_Time >= (CyclesSoFar + DisplayCyclesPerLine)))
			{
				/* Uniform color on this line */
				r.min_x = 0;
				r.max_x = TotalScreenWidth - 1;
				fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
			}
			else
			{
				/* Multiple colors on this scanline */
				int ScrX = (int)((pItem[CurrItem].Event_Time - CyclesSoFar) * TotalScreenWidth / DisplayCyclesPerLine);
				if (ScrX > TotalScreenWidth)
					ScrX = TotalScreenWidth;

				r.min_x = 0;
				r.max_x = ScrX - 1;
				if (r.max_x >= r.min_x)
					fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

				CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
				do {
					CurrItem++;
				} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));

				while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time < (CyclesSoFar + DisplayCyclesPerLine)))
				{
					int NextScrX = (int)((pItem[CurrItem].Event_Time - CyclesSoFar) * TotalScreenWidth / DisplayCyclesPerLine);
					if (NextScrX > TotalScreenWidth)
						NextScrX = TotalScreenWidth;

					r.min_x = ScrX;
					r.max_x = NextScrX - 1;
					if (r.max_x >= r.min_x)
						fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);

					ScrX = NextScrX;
					CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
					do {
						CurrItem++;
					} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
				}

				r.min_x = ScrX;
				r.max_x = TotalScreenWidth - 1;
				if (r.max_x >= r.min_x)
					fillbitmap(bitmap, Machine->pens[CurrBorderColor], &r);
			}

			/* Process color changes during horizontal retrace */
			CyclesSoFar += CyclesPerLine;
			while ((CurrItem < NumItems) && (pItem[CurrItem].Event_Time <= CyclesSoFar))
			{
				CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
				do {
					CurrItem++;
				} while ((CurrItem < NumItems) && (pItem[CurrItem].Event_ID != EventID));
			}
		}

		/* Process any remaining events after the last scanline */
		while (CurrItem < NumItems)
		{
			if (pItem[CurrItem].Event_ID == EventID)
				CurrBorderColor = pItem[CurrItem].Event_Data & 0x07;
			CurrItem++;
		}

		LastDisplayedBorderColor = -1;
	}

	/* Reset event buffer for the next frame */
	EventList_Reset();
	EventList_SetOffsetStartTime(TIME_TO_CYCLES(0, cpu_getscanline() * cpu_getscanlineperiod()));
}
