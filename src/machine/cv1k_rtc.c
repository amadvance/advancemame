/*
license:BSD-3-Clause
copyright-holders:Angelo Salese, David Haywood

    rtc9701.c

    Epson RTC-9701-JE

    Serial Real Time Clock + EEPROM
*/
#include "driver.h"
#include "includes/cv1k.h"
#include <time.h>
typedef enum rtc9701_state_t
{
	RTC9701_CMD_WAIT = 0,
	RTC9701_RTC_READ,
	RTC9701_RTC_WRITE,
	RTC9701_EEPROM_READ,
	RTC9701_EEPROM_WRITE,
	RTC9701_AFTER_WRITE_ENABLE
} rtc9701_state_t;

typedef struct rtc_regs_t
{
	UINT8 sec, min, hour, day, wday, month, year;
} rtc_regs_t;

static int                     m_latch;
static int                     m_reset_line;
static int                     m_clock_line;
static rtc9701_state_t rtc_state;
static int cmd_stream_pos;
static int current_cmd;
static int rtc9701_address_pos;
static int rtc9701_current_address;
static UINT16 rtc9701_current_data;
static int rtc9701_data_pos;
static UINT16 rtc9701_data[0x100];
static rtc_regs_t m_rtc;
static void rtc_tick(int param)
{
	static const UINT8 dpm[12] = { 0x31, 0x28, 0x31, 0x30, 0x31, 0x30, 0x31, 0x31, 0x30, 0x31, 0x30, 0x31 };
	int dpm_count;

	m_rtc.sec++;

	if((m_rtc.sec & 0x0f) >= 0x0a)              { m_rtc.sec+=0x10; m_rtc.sec&=0xf0; }
	if((m_rtc.sec & 0xf0) >= 0x60)              { m_rtc.min++; m_rtc.sec = 0; }
	if((m_rtc.min & 0x0f) >= 0x0a)              { m_rtc.min+=0x10; m_rtc.min&=0xf0; }
	if((m_rtc.min & 0xf0) >= 0x60)              { m_rtc.hour++; m_rtc.min = 0; }
	if((m_rtc.hour & 0x0f) >= 0x0a)             { m_rtc.hour+=0x10; m_rtc.hour&=0xf0; }
	if((m_rtc.hour & 0xff) >= 0x24)             { m_rtc.day++; m_rtc.wday<<=1; m_rtc.hour = 0; }
	if(m_rtc.wday & 0x80)                       { m_rtc.wday = 1; }
	if((m_rtc.day & 0x0f) >= 0x0a)              { m_rtc.day+=0x10; m_rtc.day&=0xf0; }

	dpm_count = (m_rtc.month & 0xf) + (((m_rtc.month & 0x10) >> 4)*10)-1;

	if(((m_rtc.year % 4) == 0) && m_rtc.month == 2)
	{
		if((m_rtc.day & 0xff) >= dpm[dpm_count]+1+1)
			{ m_rtc.month++; m_rtc.day = 0x01; }
	}
	else if((m_rtc.day & 0xff) >= dpm[dpm_count]+1){ m_rtc.month++; m_rtc.day = 0x01; }
	if((m_rtc.month & 0x0f) >= 0x0a)            { m_rtc.month = 0x10; }
	if(m_rtc.month >= 0x13)                     { m_rtc.year++; m_rtc.month = 1; }
	if((m_rtc.year & 0x0f) >= 0x0a)             { m_rtc.year+=0x10; m_rtc.year&=0xf0; }
	if((m_rtc.year & 0xf0) >= 0xa0)             { m_rtc.year = 0; }
}

static UINT8 rtc_read(UINT8 offset)
{
	UINT8 res;

	res = 0;

	switch(offset)
	{
		case 0: res = m_rtc.sec; break;
		case 1: res = m_rtc.min; break;
		case 2: res = m_rtc.hour; break;
		case 3: res = m_rtc.wday; break;
		case 4: res = m_rtc.day; break;
		case 5: res = m_rtc.month; break;
		case 6: res = m_rtc.year & 0xff; break;
		case 7: res = 0x20; break;
	}

	return res;
}

static void rtc_write(UINT8 offset,UINT8 data)
{
	switch(offset)
	{
		case 0: m_rtc.sec = data; break;
		case 1: m_rtc.min = data; break;
		case 2: m_rtc.hour = data; break;
		case 3: m_rtc.wday = data; break;
		case 4: m_rtc.day = data; break;
		case 5: m_rtc.month = data; break;
		case 6: m_rtc.year = data; break;
		case 7: break;
	}
}

void cv1k_rtc_write_bit(int state)
{
	m_latch = state;
}

int cv1k_rtc_read_bit(void)
{
	if (rtc_state == RTC9701_RTC_READ)
	{

		return ((rtc9701_current_data) >> (rtc9701_data_pos-1))&1;

	}
	else if (rtc_state == RTC9701_EEPROM_READ)
	{

		return ((rtc9701_current_data) >> (rtc9701_data_pos-1))&1;

	}
	else
	{

	}

	return 0;
}

void cv1k_rtc_set_cs_line(int state)
{

	m_reset_line = state;

	if (m_reset_line != CLEAR_LINE)
	{
		rtc_state = RTC9701_CMD_WAIT;
		cmd_stream_pos = 0;
		current_cmd = 0;
		rtc9701_address_pos = 0;
		rtc9701_current_address = 0;
		rtc9701_current_data = 0;
		rtc9701_data_pos = 0;

	}
}

void cv1k_rtc_set_clock_line(int state)
{

	if (m_reset_line == CLEAR_LINE)
	{
		if (state==1)
		{

			switch (rtc_state)
			{
				case RTC9701_CMD_WAIT:

					current_cmd = (current_cmd << 1) | (m_latch&1);
					cmd_stream_pos++;

					if (cmd_stream_pos==4)
					{
						cmd_stream_pos = 0;

						if (current_cmd==0x00)
						{

							rtc_state = RTC9701_RTC_WRITE;
							cmd_stream_pos = 0;
							rtc9701_address_pos = 0;
							rtc9701_current_address = 0;
							rtc9701_data_pos = 0;
							rtc9701_current_data = 0;
						}
						else if (current_cmd==0x02)
						{

							rtc_state = RTC9701_EEPROM_WRITE;
							cmd_stream_pos = 0;
							rtc9701_address_pos = 0;
							rtc9701_current_address = 0;
							rtc9701_data_pos = 0;
							rtc9701_current_data = 0;

						}
						else if (current_cmd==0x06)
						{

							rtc_state = RTC9701_AFTER_WRITE_ENABLE;
							cmd_stream_pos = 0;
						}
						else if (current_cmd==0x08)
						{

							rtc_state = RTC9701_RTC_READ;
							cmd_stream_pos = 0;
							rtc9701_address_pos = 0;
							rtc9701_current_address = 0;
							rtc9701_data_pos = 0;
							rtc9701_current_data = 0;
						}
						else if (current_cmd==0x0a)
						{

							rtc_state = RTC9701_EEPROM_READ;
							cmd_stream_pos = 0;
							rtc9701_address_pos = 0;
							rtc9701_current_address = 0;
							rtc9701_data_pos = 0;
							rtc9701_current_data = 0;

						}
						else
						{

						}

						current_cmd = 0;
					}
					break;

				case RTC9701_AFTER_WRITE_ENABLE:
					cmd_stream_pos++;
					if (cmd_stream_pos==12)
					{
						cmd_stream_pos = 0;

						rtc_state = RTC9701_CMD_WAIT;
					}
					break;

				case RTC9701_RTC_WRITE:
					cmd_stream_pos++;
					if (cmd_stream_pos<=4)
					{
						rtc9701_address_pos++;
						rtc9701_current_address = (rtc9701_current_address << 1) | (m_latch&1);
						if (cmd_stream_pos==4)
						{

						}
					}

					if (cmd_stream_pos>4)
					{
						rtc9701_data_pos++;
						rtc9701_current_data = (rtc9701_current_data << 1) | (m_latch&1);;
					}

					if (cmd_stream_pos==12)
					{
						cmd_stream_pos = 0;
						rtc_write(rtc9701_current_address,rtc9701_current_data);

						rtc_state = RTC9701_CMD_WAIT;
					}
					break;

				case RTC9701_EEPROM_READ:
					cmd_stream_pos++;
					if (cmd_stream_pos<=12)
					{
						rtc9701_address_pos++;
						rtc9701_current_address = (rtc9701_current_address << 1) | (m_latch&1);
						if (cmd_stream_pos==12)
						{

							rtc9701_current_data = rtc9701_data[(rtc9701_current_address>>1)&0xff];

							rtc9701_data_pos = 16;
						}
					}

					if (cmd_stream_pos>12)
					{
						rtc9701_data_pos--;

					}

					if (cmd_stream_pos==28)
					{
						cmd_stream_pos = 0;

					}
					break;

				case RTC9701_EEPROM_WRITE:
					cmd_stream_pos++;

					if (cmd_stream_pos<=12)
					{
						rtc9701_address_pos++;
						rtc9701_current_address = (rtc9701_current_address << 1) | (m_latch&1);
						if (cmd_stream_pos==12)
						{

						}
					}

					if (cmd_stream_pos>12)
					{
						rtc9701_data_pos++;
						rtc9701_current_data = (rtc9701_current_data << 1) | (m_latch&1);;
					}

					if (cmd_stream_pos==28)
					{
						cmd_stream_pos = 0;

						rtc9701_data[(rtc9701_current_address>>1)&0xff] = rtc9701_current_data;
						rtc_state = RTC9701_CMD_WAIT;
					}
					break;

				case RTC9701_RTC_READ:
					cmd_stream_pos++;
					if (cmd_stream_pos<=4)
					{
						rtc9701_address_pos++;
						rtc9701_current_address = (rtc9701_current_address << 1) | (m_latch&1);
						if (cmd_stream_pos==4)
						{

							rtc9701_current_data = rtc_read(rtc9701_current_address);

							rtc9701_data_pos = 8;
						}
					}

					if (cmd_stream_pos>4)
					{
						rtc9701_data_pos--;
					}

					if (cmd_stream_pos==12)
					{
						cmd_stream_pos = 0;

					}
					break;

				default:
					break;

			}
		}
	}
}

void cv1k_rtc_init(void)
{
 time_t now = time(NULL);
 struct tm *t = localtime(&now);
 memset(rtc9701_data, 0xff, sizeof(rtc9701_data));
 m_rtc.day = ((t->tm_mday / 10) << 4) | t->tm_mday % 10;
 m_rtc.month = (((t->tm_mon + 1) / 10) << 4) | (t->tm_mon + 1) % 10;
 m_rtc.wday = 1 << t->tm_wday;
 m_rtc.year = (((t->tm_year % 100) / 10) << 4) | t->tm_year % 10;
 m_rtc.hour = ((t->tm_hour / 10) << 4) | t->tm_hour % 10;
 m_rtc.min = ((t->tm_min / 10) << 4) | t->tm_min % 10;
 m_rtc.sec = ((t->tm_sec / 10) << 4) | t->tm_sec % 10;
 rtc_state = RTC9701_CMD_WAIT;
 cmd_stream_pos = current_cmd = m_latch = m_reset_line = m_clock_line = 0;
 timer_pulse(TIME_IN_HZ(1), 0, rtc_tick);
}
void cv1k_rtc_nvram(mame_file *file, int write)
{
 if (!file) memset(rtc9701_data, 0xff, sizeof(rtc9701_data));
 else if (write) mame_fwrite(file, rtc9701_data, sizeof(rtc9701_data));
 else mame_fread(file, rtc9701_data, sizeof(rtc9701_data));
}
