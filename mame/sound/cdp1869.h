// license:BSD-3-Clause
// copyright-holders:Curt Coder
/* RCA CDP1869/70/76 Video Interface System */

#ifndef __CDP1869_SOUND__
#define __CDP1869_SOUND__

void cdp1869_set_toneamp(int which, int value);
void cdp1869_set_tonefreq(int which, int value);
void cdp1869_set_toneoff(int which, int value);
void cdp1869_set_tonediv(int which, int value);
void cdp1869_set_wnamp(int which, int value);
void cdp1869_set_wnfreq(int which, int value);
void cdp1869_set_wnoff(int which, int value);

#endif
