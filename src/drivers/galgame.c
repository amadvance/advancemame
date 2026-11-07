// license:BSD-3-Clause
// copyright-holders:Mariusz Wojcieszek
/*

    Galaxy Game

    Driver by Mariusz Wojcieszek

*/

#include "driver.h"
#include "cpu/t11/t11.h"

#define MAX_POINTS 2048

static UINT16 m_clk, m_x, m_y;
static INT16 m_mq, m_ac;
static UINT16 *galgame_ram;
static struct galgame_point { UINT16 x, y; }
	m_point_work_list[MAX_POINTS], m_point_display_list[MAX_POINTS];
static int m_point_work_list_index, m_point_display_list_index, m_interrupt;

static READ16_HANDLER(ke_r)
{
	UINT16 ret;

	switch( offset )
	{
		case 0:
			ret = 0;
			break;
		case 1:
			ret = m_ac;
			break;
		case 2:
			ret = m_mq;
			break;
		case 7:
			ret = 0;
			break;
		default:
			ret = 0;
			logerror("Unhandled KE read (%d)\n", offset );
			break;
	}
	return ret;
}

static WRITE16_HANDLER(ke_w)
{
	switch( offset )
	{
		case 0:
			{
				if ( data != 0 )
				{
					INT64 dividend = (INT32)(((UINT32)(UINT16)m_ac << 16) | (UINT16)(m_mq));
					m_mq = dividend / (INT16)data;
					m_ac = dividend % (INT16)data;
				}
				else
				{
					m_mq = 0;
					m_ac = 0;
				}
			}
			break;
		case 1:
			m_ac = (INT16)data;
			break;
		case 2:
			m_mq = (INT16)data;
			if (m_mq < 0)
			{
				m_ac = -1;
			}
			else
			{
				m_ac = 0;
			}
			break;
		case 3:
			{
				INT32 mulres = (INT32)m_mq*(INT32)(INT16)data;
				m_ac = mulres >> 16;
				m_mq = mulres & 0xffff;
			}
			break;
		case 6:
			{
				data &= 63;
				INT32 val = (INT32)(((UINT32)(UINT16)m_ac << 16) | (UINT16)(m_mq));
				if ( data < 32 )
				{
					val = (UINT32)val << data;
				}
				else
				{
					val = data == 32 ? (val < 0 ? -1 : 0) : val >> (64 - data);
				}
				m_mq = val & 0xffff;
				m_ac = (val >> 16) & 0xffff;
			}
			break;
		case 7:
			{
				data &= 63;
				INT32 val = (INT32)(((UINT32)(UINT16)m_ac << 16) | (UINT16)(m_mq));
				if ( data < 32 )
				{
					val = (UINT32)val << data;
				}
				else
				{
					val = data == 32 ? (val < 0 ? -1 : 0) : val >> (64 - data);
				}
				m_mq = val & 0xffff;
				m_ac = (val >> 16) & 0xffff;
			}
			break;
		default:
			logerror("Unhandled KE write (%d)\n", offset);
			break;
	}

}

static VIDEO_UPDATE( galaxygame )
{
	int i;
	fillbitmap(bitmap, get_black_pen(), cliprect);
	for (i = 0; i < m_point_display_list_index; i++ )
	{
		plot_pixel(bitmap, m_point_display_list[i].y >> 7, m_point_display_list[i].x >> 7, Machine->pens[1]);
	}
}

static READ16_HANDLER(x_r)
{
	return m_x;
}

static WRITE16_HANDLER(x_w)
{
	m_x = data;
}

static READ16_HANDLER(y_r)
{
	return m_y;
}

static WRITE16_HANDLER(y_w)
{
	int i;
	m_y = data;
	if ( data == 0x0101 )
	{
		/* Publish a complete point list once per clock interrupt to avoid partial frames. */

		if ( m_interrupt )
		{
			for ( i = 0; i < m_point_work_list_index ; i++ )
			{
				m_point_display_list[i].x = m_point_work_list[i].x;
				m_point_display_list[i].y = m_point_work_list[i].y;
			}
			m_point_display_list_index = m_point_work_list_index;
			m_point_work_list_index = 0;
			m_interrupt = 0;
		}
	}
	else
	{
		if ( m_point_work_list_index >= MAX_POINTS )
		{
			logerror("Work list overflow\n");
		}
		else
		{
			m_point_work_list[m_point_work_list_index].x = m_x;
			m_point_work_list[m_point_work_list_index].y = m_y;
			m_point_work_list_index++;
		}
	}
}

static INPUT_PORTS_START( galaxygame )
	PORT_START
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_COIN4 )
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x8000, 0x0000, "Gravity" )
	PORT_DIPSETTING(      0x0000, "Positive Gravity" )
	PORT_DIPSETTING(      0x8000, "Negative Gravity" )
	PORT_DIPNAME( 0x4000, 0x0000, "Sun" )
	PORT_DIPSETTING(      0x0000, "Sun (& Gravity)" )
	PORT_DIPSETTING(      0x4000, "No Sun (& No Gravity)" )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(1)

	PORT_DIPNAME( 0x0080, 0x0000, "Speed" )
	PORT_DIPSETTING(      0x0000, "Slow Speed" )
	PORT_DIPSETTING(      0x0080, "Fast Speed" )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Players ) )
	PORT_DIPSETTING(      0x0000, "Two Players" )
	PORT_DIPSETTING(      0x0040, "One Player" )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
INPUT_PORTS_END

static WRITE16_HANDLER( clk_w ) { m_clk = data; }

static ADDRESS_MAP_START( galaxygame_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM AM_BASE(&galgame_ram)
	AM_RANGE(0xfec0, 0xfecf) AM_READWRITE(ke_r, ke_w)
	AM_RANGE(0xff52, 0xff53) AM_READWRITE(y_r, y_w)
	AM_RANGE(0xff54, 0xff55) AM_READ(input_port_0_word_r)
	AM_RANGE(0xff5a, 0xff5b) AM_READWRITE(x_r, x_w)
	AM_RANGE(0xff5c, 0xff5d) AM_READ(input_port_1_word_r)
	AM_RANGE(0xff66, 0xff67) AM_WRITE(clk_w)
ADDRESS_MAP_END

static PALETTE_INIT( galaxygame )
{
	palette_set_color(0, 0, 0, 0);
	palette_set_color(1, 255, 255, 255);
}

static int galaxygame_irq_callback(int irqline)
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);
	return 0x40;
}

static INTERRUPT_GEN( galaxygame_irq )
{
	if (m_clk & 0x40)
	{
		m_interrupt = 1;
		cpunum_set_input_line(0, 0, ASSERT_LINE);
	}
}

static MACHINE_RESET( galaxygame )
{
	m_clk = m_x = m_y = 0;
	m_mq = m_ac = 0;
	m_point_work_list_index = m_point_display_list_index = m_interrupt = 0;
	cpunum_set_irq_callback(0, galaxygame_irq_callback);
	cpunum_set_input_line(0, 0, CLEAR_LINE);
}

static struct t11_setup t11_data = { 5 << 13, 1 };

static MACHINE_DRIVER_START( galaxygame )
	MDRV_CPU_ADD(T11, 3000000)
	MDRV_CPU_CONFIG(t11_data)
	MDRV_CPU_PROGRAM_MAP(galaxygame_map, 0)
	MDRV_CPU_PERIODIC_INT(galaxygame_irq, TIME_IN_HZ(60))
	MDRV_MACHINE_RESET(galaxygame)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 511, 0, 511)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(galaxygame)
	MDRV_VIDEO_UPDATE(galaxygame)
MACHINE_DRIVER_END

ROM_START( galgame )
	ROM_REGION( 0x20000, REGION_USER1, ROMREGION_ERASE00 )
	ROM_LOAD( "sw97.lst", 0x00000, 0x1f062, CRC(838018a5) SHA1(e3c47c5cf78299650b031ec49fde7d9e4024a759) )
ROM_END

static int galgame_octal(const UINT8 *text, int digits)
{
	int value = 0;
	while (digits--)
	{
		if (*text < '0' || *text > '7')
			return -1;
		value = (value << 3) | (*text++ - '0');
	}
	return value;
}

static DRIVER_INIT( galaxygame )
{
	const UINT8 *code = memory_region(REGION_USER1);
	int pos = 0;
	/* Only object columns are loaded; source operands may contain octal text too. */
	while (pos < 0x1f062)
	{
		const UINT8 *line = code + pos;
		int length = 0, address, column;
		while (pos < 0x1f062 && code[pos] != 13 && code[pos] != 10)
		{
			++pos;
			++length;
		}
		while (pos < 0x1f062 && (code[pos] == 13 || code[pos] == 10))
			++pos;
		if (length < 21 || line[8] == ' ')
			continue;
		address = galgame_octal(line + 7, 6);
		if (address < 0 || address >= 0x2000)
			continue;
		if (line[15] != ' ')
		{
			for (column = 15; column <= 29 && column + 6 <= length; column += 7)
			{
				int value = galgame_octal(line + column, 6);
				if (value < 0 || address >= 0x2000)
					break;
				galgame_ram[address >> 1] = value;
				address += 2;
			}
		}
		else
		{
			int value = galgame_octal(line + 18, 3);
			if (value >= 0)
			{
				int shift = (address & 1) * 8;
				galgame_ram[address >> 1] = (galgame_ram[address >> 1] & ~(255 << shift)) | (value << shift);
			}
		}
	}
	/* Start with interrupts enabled before entering the program at octal 0500. */
	galgame_ram[0] = 012700;
	galgame_ram[1] = 0;
	galgame_ram[2] = 0x8d00;
	galgame_ram[3] = 000167;
	galgame_ram[4] = 000500 - 10;
}

GAME( 1971, galgame, 0, galaxygame, galaxygame, galaxygame, ROT270, "Computer Recreations, Inc", "Galaxy Game", 0 )
