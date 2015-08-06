/***************************************************************************

  Functions to emulate the video hardware of the Enterprise.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/nick.h"
#include "vidhrdw/epnick.h"
#include "includes/enterp.h"

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/
VIDEO_START( enterprise )
{
	Nick_vh_start();
	return 0;
}

/***************************************************************************
  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
VIDEO_UPDATE( enterprise )
{
	Nick_DoScreen(bitmap);
}

