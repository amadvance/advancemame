/*
license:BSD-3-Clause
copyright-holders:David Haywood, Luca Elia
Serial Flash Device

todo: cleanup, refactor etc.
ghosteo.c is similar?
*/

#include "driver.h"
#include "includes/cv1k.h"
#define FLASH_PAGE_SIZE 2112
typedef enum { STATE_IDLE, STATE_READ, STATE_READ_ID, STATE_READ_STATUS, STATE_BLOCK_ERASE, STATE_PAGE_PROGRAM } flash_state_t;
static size_t                      m_length;
static UINT8* m_region;
static flash_state_t m_flash_state;
static UINT8 m_flash_enab;
static UINT8 m_flash_cmd_seq;
static UINT32 m_flash_cmd_prev;
static UINT8 m_flash_addr_seq;
static UINT8 m_flash_read_seq;
static UINT16 m_flash_row, m_flash_col;
static int m_flash_page_addr;
static UINT16 m_flash_page_index;
static UINT8 *m_flashwritemap;
static UINT8 m_last_flash_cmd;
static UINT32 m_flash_addr;
static UINT8 m_flash_page_data[FLASH_PAGE_SIZE];


static void cv1k_flash_change_state(flash_state_t state);
void cv1k_flash_hard_reset(void)
{

	m_flash_state = STATE_READ;

	m_flash_cmd_prev = -1;
	m_flash_cmd_seq = 0;

	m_flash_addr_seq = 0;
	m_flash_read_seq = 0;

	m_flash_row = 0;
	m_flash_col = 0;

	memset(m_flash_page_data, 0, FLASH_PAGE_SIZE);
	m_flash_page_addr = 0;
	m_flash_page_index = 0;
}

void cv1k_flash_enab_w(UINT8 data)
{

	m_flash_enab = data;
}

void cv1k_flash_change_state(flash_state_t state)
{
	m_flash_state = state;

	m_flash_cmd_prev = -1;
	m_flash_cmd_seq = 0;

	m_flash_read_seq = 0;
	m_flash_addr_seq = 0;

}

void cv1k_flash_cmd_w(UINT8 data)
{
	if (!m_flash_enab)
		return;

	if (m_flash_cmd_prev == -1)
	{
		m_flash_cmd_prev = data;

		switch (data)
		{
			case 0x00:
				m_flash_addr_seq = 0;
				break;

			case 0x60:
				m_flash_addr_seq = 0;
				break;

			case 0x70:
				cv1k_flash_change_state( STATE_READ_STATUS );
				break;

			case 0x80:
				m_flash_addr_seq = 0;

				m_flash_page_addr = 0;
				break;

			case 0x90:
				cv1k_flash_change_state( STATE_READ_ID );
				break;

			case 0xff:
				cv1k_flash_change_state( STATE_IDLE );
				break;

			default:
			{

			}
		}
	}
	else
	{
		switch (m_flash_cmd_prev)
		{
			case 0x00:
				if (data == 0x30)
				{
					memcpy(m_flash_page_data, m_region + m_flash_row * FLASH_PAGE_SIZE, FLASH_PAGE_SIZE);
					m_flash_page_addr = m_flash_col;
					m_flash_page_index = m_flash_row;

					cv1k_flash_change_state( STATE_READ );

				}
				break;

			case 0x60:
				if (data==0xd0)
				{
					cv1k_flash_change_state( STATE_BLOCK_ERASE );
					m_flashwritemap[m_flash_col] |= 1;
					memset(m_region + m_flash_col * FLASH_PAGE_SIZE, 0xff, FLASH_PAGE_SIZE);

				}
				else
				{

				}
				break;
			case 0x80:
				if (data==0x10)
				{
					cv1k_flash_change_state( STATE_PAGE_PROGRAM );
					m_flashwritemap[m_flash_row] |= (memcmp(m_region + m_flash_row * FLASH_PAGE_SIZE, m_flash_page_data, FLASH_PAGE_SIZE) != 0);
					memcpy(m_region + m_flash_row * FLASH_PAGE_SIZE, m_flash_page_data, FLASH_PAGE_SIZE);

				}
				else
				{

				}
				break;

			default:
			{

			}
		}
	}
}

void cv1k_flash_data_w(UINT8 data)
{
	if (!m_flash_enab)
		return;

	m_flash_page_data[m_flash_page_addr] = data;
	m_flash_page_addr++;
}

void cv1k_flash_addr_w(UINT8 data)
{
	if (!m_flash_enab)
		return;

	switch( m_flash_addr_seq++ )
	{
		case 0:
			m_flash_col = (m_flash_col & 0xff00) | data;
			break;
		case 1:
			m_flash_col = (m_flash_col & 0x00ff) | (data << 8);
			break;
		case 2:
			m_flash_row = (m_flash_row & 0xff00) | data;
			break;
		case 3:
			m_flash_row = (m_flash_row & 0x00ff) | (data << 8);
			m_flash_addr_seq = 0;
			break;
	}
}

UINT8 cv1k_flash_io_r(void)
{
	UINT8 data = 0x00;

	if (!m_flash_enab)
		return 0xff;

	switch (m_flash_state)
	{
		case STATE_READ_ID:

			switch( m_flash_read_seq++ )
			{
				case 0:
					data = 0xEC;
					break;
				case 1:
					data = 0xF1;
					break;
				case 2:
					data = 0x00;
					break;
				case 3:
					data = 0x15;
					m_flash_read_seq = 0;
					break;
			}

			break;

		case STATE_READ:
			if (m_flash_page_addr > FLASH_PAGE_SIZE-1)
				m_flash_page_addr = FLASH_PAGE_SIZE-1;

			data = m_flash_page_data[m_flash_page_addr++];

			break;

		case STATE_READ_STATUS:

			data = 0xe0;

			break;

		default:
		{

		}
	}

	return data;
}

UINT8 cv1k_flash_ready_r(void)
{
	return 1;
}


void cv1k_flash_init(int region)
{
 m_length = memory_region_length(region);
 m_region = memory_region(region);
 m_flashwritemap = auto_malloc(m_length / FLASH_PAGE_SIZE);
 memset(m_flashwritemap, 0, m_length / FLASH_PAGE_SIZE);
 m_flash_enab = 0;
 cv1k_flash_hard_reset();
}
void cv1k_flash_nvram(mame_file *file, int write)
{
 UINT32 page, count = m_length / FLASH_PAGE_SIZE;
 if (!file) return;
 if (write) {
  for (page = 0; page < count; ++page) if (m_flashwritemap[page]) {
   mame_fwrite(file, &page, 4);
   mame_fwrite(file, m_region + page * FLASH_PAGE_SIZE, FLASH_PAGE_SIZE);
  }
  mame_fwrite(file, &count, 4);
 } else {
  while (mame_fread(file, &page, 4) == 4 && page < count) {
   m_flashwritemap[page] = 1;
   mame_fread(file, m_region + page * FLASH_PAGE_SIZE, FLASH_PAGE_SIZE);
  }
 }
}
