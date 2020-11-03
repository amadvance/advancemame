/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002, 2003, 2004, 2005 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#ifndef __MSDOS__ /* nothing for dos */

#ifdef __WIN32__
#include <windows.h>
#else
#include <termios.h>
#endif
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>

#include "portable.h"
#include "advance.h"
#include "driver.h"
#include "vidhrdw/vector.h"
#include "osdepend.h"

#include "jsmn.h"


// 0-15
#define DVG_RELEASE             0
#define DVG_BUILD               0
#define ARRAY_SIZE(a)           (sizeof(a)/sizeof((a)[0]))
#define CMD_BUF_SIZE             0x20000
#define FLAG_COMPLETE            0x0
#define FLAG_RGB                 0x1
#define FLAG_XY                  0x2
#define FLAG_EXIT                0x7
#define FLAG_CMD                 0x5
#define FLAG_CMD_GET_DVG_INFO    0x1

#define FLAG_COMPLETE_MONOCHROME (1 << 28)

#define DVG_RES_MIN              0
#define DVG_RES_MAX              4095

#define SAVE_TO_FILE             0
#define SORT_VECTORS             0
#define MAX_VECTORS              0x10000

// Defining region codes 
#define LEFT                     0x1
#define RIGHT                    0x2
#define BOTTOM                   0x4
#define TOP                      0x8

#define GAME_NONE                0
#define GAME_ARMORA              1
#define GAME_WARRIOR             2

typedef struct vec_t {
    struct vec_t *next;
    int32_t       x0;
    int32_t       y0;
    int32_t       x1;
    int32_t       y1;
    uint8_t       r;
    uint8_t       g;
    uint8_t       b;
} vector_t;

typedef struct {
    char     *name;
    uint32_t exclude_blank_vectors;
    uint32_t artwork;
    uint32_t bw_game;
} game_info_t;

static uint32_t  s_exclude_blank_vectors;
static uint32_t  s_init;
static int       s_dual_display;
static int32_t   s_serial_fd = -1;
static int32_t   s_xmin, s_xmax, s_ymin, s_ymax;
static float     s_xscale, s_yscale;
static uint32    s_cmd_offs; 
static uint8_t   *s_cmd_buf;
static char      s_serial_dev[128];
static uint32_t  s_swap_xy = 0;
static uint32_t  s_flip_x = 0;
static uint32_t  s_flip_y = 1;
static int32_t   s_clipx_min = DVG_RES_MIN;
static int32_t   s_clipx_max = DVG_RES_MAX;
static int32_t   s_clipy_min = DVG_RES_MIN;
static int32_t   s_clipy_max = DVG_RES_MAX;
static uint32_t  s_first_call;
static int32_t   s_last_r;
static int32_t   s_last_g;
static int32_t   s_last_b;
static int32_t   s_intensity_table[256];
static uint8_t   s_gamma_table[256]; 
static uint32_t  s_artwork;
static uint32_t  s_bw_game;
static vector_t  *s_in_vec_list;
static uint32_t  s_in_vec_cnt;
static uint32_t  s_in_vec_last_x;
static uint32_t  s_in_vec_last_y;
static vector_t  *s_out_vec_list;
static uint32_t  s_out_vec_cnt;
static char      s_json_buf[512];
static int       s_json_length;
static uint32_t  s_vertical_display;


static game_info_t s_games[] = {
    {"armora",   0, GAME_ARMORA, 1},
    {"armorap",  0, GAME_ARMORA, 1},
    {"armorar",  0, GAME_ARMORA, 1},        
    {"asteroid", 0, GAME_NONE, 1},  
    {"asteroi1", 0, GAME_NONE, 1},    
    {"astdelux", 0, GAME_NONE, 1},   
    {"astdelu1", 0, GAME_NONE, 1},  
    {"llander",  0, GAME_NONE, 1},
    {"llander1", 0, GAME_NONE, 1},
    {"barrier",  0, GAME_NONE, 1},
    {"bzone",    0, GAME_NONE, 1},   
    {"bzone2",   0, GAME_NONE, 1},  
    {"bzonec",   0, GAME_NONE, 1},  
    {"redbaron", 0, GAME_NONE, 1},   
    {"ripoff",   0, GAME_NONE, 1},   
    {"solarq",   0, GAME_NONE, 1},
    {"speedfrk", 0, GAME_NONE, 1},  
    {"starhawk", 0, GAME_NONE, 1},   
    {"sundance", 0, GAME_NONE, 1},   
    {"tailg",    0, GAME_NONE, 1},  
    {"warrior",  0, GAME_WARRIOR, 1}, 
    {"wotw",     0, GAME_NONE, 1},   
    {"starwars", 1, GAME_NONE, 0},
    {"starwar1", 1, GAME_NONE, 0},
    {"esb",      1, GAME_NONE, 0}
};


static void transform_final(int *px, int *py);
static int dvg_get_option(char *option, char *val_buf, uint32_t val_buf_size);



//
// Function to compute region code for a point(x, y) 
//
uint32_t compute_code(int32_t x, int32_t y)
{
    // initialized as being inside 
    uint32_t code = 0;

    if (x < s_clipx_min)      // to the left of rectangle 
        code |= LEFT;
    else if (x > s_clipx_max) // to the right of rectangle 
        code |= RIGHT;
    if (y < s_clipy_min)      // below the rectangle 
        code |= BOTTOM;
    else if (y > s_clipy_max) // above the rectangle 
        code |= TOP;

    return code;
}

//
// Cohen-Sutherland line-clipping algorithm.  Some games (such as starwars)
// generate coordinates outside the view window, so we need to clip them here.
//
uint32_t line_clip(int32_t *pX1, int32_t *pY1, int32_t *pX2, int32_t *pY2)
{
    int32_t x = 0, y = 0, x1, y1, x2, y2;
    uint32_t accept, code1, code2, code_out;

    x1 = *pX1;
    y1 = *pY1;
    x2 = *pX2;
    y2 = *pY2;

    accept = 0;
    // Compute region codes for P1, P2 
    code1 = compute_code(x1, y1);
    code2 = compute_code(x2, y2);

    while (1) {
        if ((code1 == 0) && (code2 == 0)) {
            // If both endpoints lie within rectangle 
            accept = 1;
            break;
        }
        else if (code1 & code2) {
            // If both endpoints are outside rectangle, 
            // in same region 
            break;
        }
        else {
            // Some segment of line lies within the 
            // rectangle 
            // At least one endpoint is outside the 
            // rectangle, pick it. 
            if (code1 != 0) {
                code_out = code1;
            }
            else {
                code_out = code2;
            }

            // Find intersection point; 
            // using formulas y = y1 + slope * (x - x1), 
            // x = x1 + (1 / slope) * (y - y1) 
            if (code_out & TOP) {
                // point is above the clip rectangle 
                x = x1 + (x2 - x1) * (s_clipy_max - y1) / (y2 - y1);
                y = s_clipy_max;
            }
            else if (code_out & BOTTOM) {
                // point is below the rectangle 
                x = x1 + (x2 - x1) * (s_clipy_min - y1) / (y2 - y1);
                y = s_clipy_min;
            }
            else if (code_out & RIGHT) {
                // point is to the right of rectangle 
                y = y1 + (y2 - y1) * (s_clipx_max - x1) / (x2 - x1);
                x = s_clipx_max;
            }
            else if (code_out & LEFT) {
                // point is to the left of rectangle 
                y = y1 + (y2 - y1) * (s_clipx_min - x1) / (x2 - x1);
                x = s_clipx_min;
            }

            // Now intersection point x, y is found 
            // We replace point outside rectangle 
            // by intersection point 
            if (code_out == code1) {
                x1 = x;
                y1 = y;
                code1 = compute_code(x1, y1);
            }
            else {
                x2 = x;
                y2 = y;
                code2 = compute_code(x2, y2);
            }
        }
    }
    *pX1 = x1;
    *pY1 = y1;
    *pX2 = x2;
    *pY2 = y2;
    return accept;
}  


//
// Sort, optimize and add vectors (and blank vectors) to the output vector list.
// 
void sort_and_reconnect_vectors()
{
    int32_t  dmin;
    uint32_t reverse;
    int32_t  last_x = -1;
    int32_t  last_y = -1;
    int32_t  x0, y0, x1, y1, dx0, dy0, dx1, dy1, d0, d1;
    vector_t **min_v, *head, *s;

    head = &s_in_vec_list[0];
    s_out_vec_cnt = 0;

    while (head) {
        min_v = &head;
        reverse = 0;
        dmin = INT32_MAX;
	vector_t **v;
        for (v = min_v; *v; v = &(*v)->next) {
            dx0 = (*v)->x0 - last_x;
            dy0 = (*v)->y0 - last_y;
            dx1 = (*v)->x1 - last_x;
            dy1 = (*v)->y1 - last_y;
            d0 = (int32_t)sqrt((dx0 * dx0) + (dy0 * dy0));
            d1 = (int32_t)sqrt((dx1 * dx1) + (dy1 * dy1));
            if (d0 < dmin) {
                min_v = v;
                dmin = d0;
                reverse = 0;
            }
            if (d1 < dmin) {
                min_v = v;
                dmin = d1;
                reverse = 1;
            }
            if (dmin == 0) {
                break;
            }
        }
        s = *min_v;
        if (!s) {
            break;
        }

        x0 = reverse ? s->x1 : s->x0;
        y0 = reverse ? s->y1 : s->y0;
        x1 = reverse ? s->x0 : s->x1;
        y1 = reverse ? s->y0 : s->y1;

        if (last_x != x0 || last_y != y0) {
            s_out_vec_list[s_out_vec_cnt].x0 = last_x;
            s_out_vec_list[s_out_vec_cnt].y0 = last_y;
            s_out_vec_list[s_out_vec_cnt].x1 = x0;
            s_out_vec_list[s_out_vec_cnt].y1 = y0;
            s_out_vec_list[s_out_vec_cnt].r = 0;
            s_out_vec_list[s_out_vec_cnt].g = 0;
            s_out_vec_list[s_out_vec_cnt].b = 0;
            s_out_vec_cnt++;
        }

        s_out_vec_list[s_out_vec_cnt].x0 = last_x;
        s_out_vec_list[s_out_vec_cnt].y0 = last_y;
        s_out_vec_list[s_out_vec_cnt].x1 = x1;
        s_out_vec_list[s_out_vec_cnt].y1 = y1;
        s_out_vec_list[s_out_vec_cnt].r = s->r;
        s_out_vec_list[s_out_vec_cnt].g = s->g;
        s_out_vec_list[s_out_vec_cnt].b = s->b;
        s_out_vec_cnt++;
        last_x = x1;
        last_y = y1;
        *min_v = s->next;
    }
}


//
// Add vectors (and blank vectors) to the output vector list.
// 
void reconnect_vectors()
{
    int32_t  last_x = 0;
    int32_t  last_y = 0;
    int32_t  x0, y0, x1, y1;
    
    s_out_vec_cnt = 0;

    uint32_t i;
    for (i = 0; i < s_in_vec_cnt ; i++) {
        x0 = s_in_vec_list[i].x0;
        y0 = s_in_vec_list[i].y0;
        x1 = s_in_vec_list[i].x1;
        y1 = s_in_vec_list[i].y1;

        if (last_x != x0 || last_y != y0) {
            // Disconnect detected. Insert a blank vector.
            s_out_vec_list[s_out_vec_cnt].x0 = last_x;
            s_out_vec_list[s_out_vec_cnt].y0 = last_y;
            s_out_vec_list[s_out_vec_cnt].x1 = x0;
            s_out_vec_list[s_out_vec_cnt].y1 = y0;
            s_out_vec_list[s_out_vec_cnt].r = 0;
            s_out_vec_list[s_out_vec_cnt].g = 0;
            s_out_vec_list[s_out_vec_cnt].b = 0;
            s_out_vec_cnt++;
        }
        s_out_vec_list[s_out_vec_cnt].x0 = last_x;
        s_out_vec_list[s_out_vec_cnt].y0 = last_y;
        s_out_vec_list[s_out_vec_cnt].x1 = x1;
        s_out_vec_list[s_out_vec_cnt].y1 = y1;
        s_out_vec_list[s_out_vec_cnt].r = s_in_vec_list[i].r;
        s_out_vec_list[s_out_vec_cnt].g = s_in_vec_list[i].g;
        s_out_vec_list[s_out_vec_cnt].b = s_in_vec_list[i].b;
        s_out_vec_cnt++;
        last_x = x1;
        last_y = y1;
    }
}


//
// Needed to recompute the color gamma table.
//
static void recalc_gamma_table(float _gamma)
{
    int i, h;

    for (i = 0; i < 256; i++) {
        h = 255.0*pow(i/255.0, 1.0/_gamma);
        if( h > 255) h = 255;
        s_gamma_table[i]= h;
    }
}

//
// Reset the indexes to the vector list and command buffer.
//
static void cmd_reset(uint32_t initial)
{
    uint32_t i, cnt;
    s_in_vec_last_x  = 0;
    s_in_vec_last_y  = 0;
    s_in_vec_cnt     = 0;
    s_out_vec_cnt    = 0;
    s_cmd_offs         = 0;
   // Special sync pattern
   cnt = 8;
   if (initial) {
      cnt = 512;
   }
   for (i = 0 ; i < cnt ; i++) {
      s_cmd_buf[s_cmd_offs++] = 0xc0 | (i & 0x3);
   }
}

//
// Add a vector to the input vector list.  We don't keep
// blank vectors.  They will be added later.
//
static void cmd_add_vec(int x, int y, int r, int g, int b) 
{
    uint32_t   blank;
    uint32_t   add;
    int32_t    x0, y0, x1, y1;

    x0 = s_in_vec_last_x;
    y0 = s_in_vec_last_y;
    x1 = x;
    y1 = y;

    if (s_exclude_blank_vectors) {
        // Don't include blank vectors.  We will add them again (see reconnect_vectors()) before sending.
        blank = (r == 0) && (g == 0) && (b == 0);
    }
    else {
        blank = 0;
    }

    if (!blank) {
        if (s_in_vec_cnt < MAX_VECTORS) {
            add = line_clip(&x0, &y0, &x1, &y1);
            if (add) {
                if (s_in_vec_cnt) {
                    s_in_vec_list[s_in_vec_cnt - 1].next = &s_in_vec_list[s_in_vec_cnt];
                }
                s_in_vec_list[s_in_vec_cnt].x0 = x0;
                s_in_vec_list[s_in_vec_cnt].y0 = y0;
                s_in_vec_list[s_in_vec_cnt].x1 = x1;
                s_in_vec_list[s_in_vec_cnt].y1 = y1;
                s_in_vec_list[s_in_vec_cnt].r  = r;
                s_in_vec_list[s_in_vec_cnt].g  = g;
                s_in_vec_list[s_in_vec_cnt].b  = b;
                s_in_vec_cnt++;
            }
        }
    }
    s_in_vec_last_x = x;
    s_in_vec_last_y = y;
}

//
// Add commands to the serial buffer to send.  When we detect
// color changes, we add a command to update it.
// As an optimization there is a blank flag in the XY coord which
// allows USB-DVG to blank the beam without updating the RGB color DACs.
//
static void cmd_add_point(int x, int y, int r, int g, int b)
{
    uint32_t   cmd;
    uint32_t   color_change;
    uint32_t   blank;

    blank = (r == 0) && (g == 0) && (b == 0);
    if (!blank) {
        color_change = ((s_last_r != r) || (s_last_g != g) || (s_last_b != b));        
        if (color_change) {
            s_last_r = r;
            s_last_g = g;
            s_last_b = b;
            cmd = (FLAG_RGB << 29) | ((r & 0xff) << 16) | ((g & 0xff) << 8)| (b & 0xff);
            if (s_cmd_offs <= (CMD_BUF_SIZE - 8)) {
                s_cmd_buf[s_cmd_offs++] = cmd >> 24;               
                s_cmd_buf[s_cmd_offs++] = cmd >> 16;
                s_cmd_buf[s_cmd_offs++] = cmd >>  8;
                s_cmd_buf[s_cmd_offs++] = cmd >>  0;
            }
        }
    }
    transform_final(&x, &y);
    cmd = (FLAG_XY << 29) | ((blank & 0x1) << 28) | ((x & 0x3fff) << 14) | (y & 0x3fff); 
    if (s_cmd_offs <= (CMD_BUF_SIZE - 8)) {
        s_cmd_buf[s_cmd_offs++] = cmd >> 24;   
        s_cmd_buf[s_cmd_offs++] = cmd >> 16;
        s_cmd_buf[s_cmd_offs++] = cmd >>  8;
        s_cmd_buf[s_cmd_offs++] = cmd >>  0;
    }
}

//
// Open the virtual serial port. We support Linux and Windows platforms.
//
static int serial_open()
{
    int            result = -1;
#ifdef __WIN32__

#if SAVE_TO_FILE
    s_serial_fd = (int32_t)CreateFile("dvg.dat",  GENERIC_WRITE, 0, NULL, CREATE_NEW,  0, NULL);
#else
    DCB dcb;
    BOOL res;
    s_serial_fd = (int32_t)CreateFile(s_serial_dev, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,  0, NULL);
    if (s_serial_fd < 0) {
        log_std(("dvg: CreateFile(%s) failed. (%ld) \n", s_serial_dev, GetLastError()));
        goto END;
    }
    memset(&dcb, 0, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);
    res = GetCommState((HANDLE)s_serial_fd, &dcb);
    if (res == FALSE) {
        log_std(("dvg: GetCommState(%s) failed. (%ld) \n", s_serial_dev, GetLastError()));
        goto END;       
    }
    dcb.BaudRate        = 2000000;       //  Bit rate. Don't care for serial over USB.
    dcb.ByteSize        = 8;             //  Data size, xmit and rcv
    dcb.Parity          = NOPARITY;      //  Parity bit
    dcb.StopBits        = ONESTOPBIT;    //  Stop bit
    dcb.fOutX           = FALSE;
    dcb.fInX            = FALSE;
    dcb.fOutxCtsFlow    = FALSE;
    dcb.fOutxDsrFlow    = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fRtsControl     = RTS_CONTROL_ENABLE;
    dcb.fDtrControl     = DTR_CONTROL_ENABLE;
    res = SetCommState((HANDLE)s_serial_fd, &dcb);
    if (res == FALSE) {
        log_std(("dvg: SetCommState(%s) failed. (%ld) \n", s_serial_dev, GetLastError()));
        goto END;       
    }
#endif

#else
    struct termios attr;

    if (s_serial_fd >= 0) {
        log_std(("dvg: device already opened.\n"));        
        result = 0;
        goto END;
    }
    s_serial_fd = open(s_serial_dev, O_RDWR | O_NOCTTY);
    if (s_serial_fd < 0) {
        log_std(("dvg: open(%s) failed. (%d) \n", s_serial_dev, errno));    
        goto END;
    }
    // No modem signals 
    cfmakeraw(&attr);
    attr.c_cflag |= (CLOCAL | CREAD);
    attr.c_oflag &= ~OPOST;
    attr.c_cc[VMIN] = 0;
    attr.c_cc[VTIME] = 30;
    tcsetattr(s_serial_fd, TCSAFLUSH, &attr);
    sleep(2); //required to make flush work, for some reason
    tcflush(s_serial_fd, TCIOFLUSH); 
    result = 0;
#endif
END:
    cmd_reset(1);
    s_last_r = s_last_g = s_last_b = -1;
    return result;
}

//
//  Read responses from USB-DVG via the virtual serial port over USB.
// 
static int serial_read(void *buf, uint32_t size)
{
    int result = -1;
#ifdef __WIN32__
    DWORD read;
    if (ReadFile(s_serial_fd, buf, size, &read, NULL))
    {
        result = read;
    }
#else
    result = read(s_serial_fd, buf, size);
    if (result != (int)size) {
        log_std(("dvg: read error %d, expected %d\n", result, size));
        result = -1;
    }
#endif
    return result;
}

//
//  Send commands to USB-DVG via the virtual serial port over USB.
// 
static int serial_write(void *buf, uint32_t size) 
{
   int result = -1, written;
   uint32_t chunk, total;

    total = size;
    while (size) {
        chunk = MIN(size, 512);
#ifdef __WIN32__
        WriteFile(s_serial_fd, buf,  chunk, (DWORD *)&written, NULL);
#else
        written = write(s_serial_fd, buf, chunk);
#endif
        if (written != (int)chunk) {
            goto END;
        }
        buf  += chunk;
        size -= chunk;
    }
    result = total;
END:
   return result;
}

//
// Close the serial port.
//
static int serial_close()
{
    int      result = -1;
    uint32_t cmd;
    if (s_serial_fd < 0) {
        log_std(("dvg: device already closed.\n"));                
        goto END;
    }
    // Be gentle and indicate to USB-DVG that it is game over!
    cmd = (FLAG_EXIT << 29); 
    s_cmd_offs = 0;
    s_cmd_buf[s_cmd_offs++] = cmd >> 24;    
    s_cmd_buf[s_cmd_offs++] = cmd >> 16;
    s_cmd_buf[s_cmd_offs++] = cmd >>  8;
    s_cmd_buf[s_cmd_offs++] = cmd >>  0;
    serial_write(s_cmd_buf, s_cmd_offs);
#ifdef __WIN32__
    CloseHandle((HANDLE)s_serial_fd);
#else
    close(s_serial_fd);
#endif
    result = 0;
END:
    s_serial_fd = -1;   
    return result;  
}


//
// Preprocess and send commands to USB-DVG over the virtual serial port.
//
static int serial_send()
{
    int      result = -1;
    uint32_t cmd;

    if (s_serial_fd < 0) {
        log_std(("dvg: device not opened.\n"));            
        goto END;
    }

    #if SORT_VECTORS
        // USB-DVG has difficulties rendering sorted vectors.  The screen (especially text) wobbles.
        // I have yet to know why it does that.  Otherwise the algorithm works fine.
        sort_and_reconnect_vectors();
    #else
        reconnect_vectors();
    #endif

    uint32_t i;
    for (i = 0 ; i < s_out_vec_cnt ; i++) {
        cmd_add_point(s_out_vec_list[i].x1, s_out_vec_list[i].y1, s_out_vec_list[i].r, s_out_vec_list[i].g, s_out_vec_list[i].b);
    }   
    cmd = (FLAG_COMPLETE << 29); 
    if (s_bw_game) {
        cmd |= FLAG_COMPLETE_MONOCHROME;
    }
    s_cmd_buf[s_cmd_offs++] = cmd >> 24;
    s_cmd_buf[s_cmd_offs++] = cmd >> 16;
    s_cmd_buf[s_cmd_offs++] = cmd >>  8;
    s_cmd_buf[s_cmd_offs++] = cmd >>  0;     

    result  = serial_write(s_cmd_buf, s_cmd_offs);
END:
    cmd_reset(0);
    return result;
}


//
//  Convert the MAME-supplied coordinates to USB-DVG-compatible coordinates.
// 
static void  transform_coords(int *px, int *py)
{
    float x, y;

    x = (*px >> 16) + (*px & 0xffff) / 65536.0;
    x *= s_xscale;
    y = (*py >> 16) + (*py & 0xffff) / 65536.0;
    y *= s_yscale;

    *px = x;
    *py = y;
}

//
// Determine game type, orientation
//
int determine_game_settings() 
{
    uint32_t i;
    char opt[64];

    memset(opt, 0, sizeof(opt));
    if (dvg_get_option("vertical", opt, sizeof(opt) - 1) >= 0) {
        s_vertical_display = strncmp(opt, "true", 4) == 0;
        log_std(("dvg: display is %s\n", s_vertical_display ? "vertical":"horizontal"));
    }

    if (Machine->gamedrv->flags & ORIENTATION_SWAP_XY) {
        s_swap_xy = 1;
    }
    if (Machine->gamedrv->flags & ORIENTATION_FLIP_Y) {
        s_flip_y = 0;
    }
    if (Machine->gamedrv->flags & ORIENTATION_FLIP_X) {
        s_flip_x = 1;
    }  

    s_bw_game               = 0;
    s_artwork               = GAME_NONE;
    s_exclude_blank_vectors = 0;
#ifdef MESS
    s_bw_game               = 1;
#else
    for (i = 0 ; i < ARRAY_SIZE(s_games); i++) {
        if (!strcmp(Machine->gamedrv->name, s_games[i].name)) {
            s_artwork               = s_games[i].artwork;
            s_exclude_blank_vectors = s_games[i].exclude_blank_vectors;
            s_bw_game               = s_games[i].bw_game;
            break;
        }
    }
#endif
    return 0;    
}


//
//  Compute a final transformation to coordinates (flip and swap).
//
static void transform_final(int *px, int *py)
{
    int x, y, tmp;
    x = *px;
    y = *py;

    if (s_swap_xy) {
        tmp = x;
        x = y;
        y = tmp;
    }
    if (s_flip_x) {
        x = DVG_RES_MAX - x;
    }
    if (s_flip_y) {
       y = DVG_RES_MAX - y;
    }
    if (x < 0) {
       x = 0;
    }
    else if (x > DVG_RES_MAX) {
        x = DVG_RES_MAX;
    }    
    if (y < 0) {
        y = 0;
    } 
    else if (y > DVG_RES_MAX) {
       y = DVG_RES_MAX;
    }


    if (s_vertical_display) {
        if (s_swap_xy) {
            // Vertical on vertical display
        }
        else {
            // Horizontal on vertical display
            y = 512 + (0.75 * y);
        }

    }
    else {
        if (s_swap_xy) {
            // Vertical on horizontal display
            x = 512 + (0.75 * x);
        }
        else {
            // Horizontal on horizontal display
        }
        
    }

    *px = x;
    *py = y;
}

//
//  Called by MAME with the latest vector list to send to our hardware.  
//  We do some preprocessing to lighten the load for USB-DVG. 
//
int dvg_update(point *p, int num_points)
{
    int i, col, intensity;
    int x, x0, x1;
    int y, y0, y1;
    int r, g, b; 

    if (s_first_call) {
        s_first_call = 0;
        s_xmin = Machine->visible_area.min_x;
        s_xmax = Machine->visible_area.max_x;
        s_ymin = Machine->visible_area.min_y;
        s_ymax = Machine->visible_area.max_y;
        if (s_xmax == 0) {
            s_xmax = 1;
        }
        if (s_ymax == 0) {
            s_ymax = 1;
        }
        s_xscale = (float)(DVG_RES_MAX + 1) / (s_xmax - s_xmin);
        s_yscale = (float)(DVG_RES_MAX + 1) / (s_ymax - s_ymin);
        log_std(("dvg: xmin %d xmax %d ymin %d ymax %d xscale %g yscale %g\n", (int)s_xmin, (int)s_xmax, (int)s_ymin, (int)s_ymax, s_xscale, s_yscale));
        determine_game_settings();     
        recalc_gamma_table(vector_get_gamma());  
        s_clipx_min = DVG_RES_MIN;
        s_clipy_min = DVG_RES_MIN;
        s_clipx_max = DVG_RES_MAX;
        s_clipy_max = DVG_RES_MAX;
    }

    for (i = 0; i < num_points; i++) {
        if (p->status == VCLIP) {        
            x0 = p->x;  
            y0 = p->y;
            x1 = p->arg1;
            y1 = p->arg2;
            transform_coords(&x0, &y0);
            transform_coords(&x1, &y1);
            // Make sure the clip coordinates fall within the display coordinates.
            if (x0 > DVG_RES_MAX) {
                x0 = DVG_RES_MAX;
            }     
            if (y0 > DVG_RES_MAX) {
                y0 = DVG_RES_MAX;
            }
            if (x1 > DVG_RES_MAX) {
                x1 = DVG_RES_MAX;
            }
            if (y1 > DVG_RES_MAX) {
                y1 = DVG_RES_MAX;
            }   
            s_clipx_min = x0;
            s_clipy_min = y0;
            s_clipx_max = x1;
            s_clipy_max = y1;
        }
        else {    
            x         = p->x;
            y         = p->y;
            intensity = p->intensity;

            r = g = b = 0;
            if (intensity) {
                if (p->callback)
                    col = Tinten(s_gamma_table[intensity], p->callback());
                else
                    col = Tinten(s_gamma_table[intensity], p->col);           
                r  = RGB_RED(col);
                g  = RGB_GREEN(col);
                b  = RGB_BLUE(col);    
            }
            transform_coords(&x, &y);
            cmd_add_vec(x, y, r, g, b);         
        }
        p++;
    }    
    if (num_points) {
        switch (s_artwork) {
            case GAME_ARMORA:
                r = 108;
                g = 108;
                b = 108;
                // Upper Right Quadrant
                // Outer structure
                cmd_add_vec(3446, 2048, 0, 0, 0);
                cmd_add_vec(3958, 2224, r, g, b);
                cmd_add_vec(3958, 3059, r, g, b);
                cmd_add_vec(3323, 3059, r, g, b);
                cmd_add_vec(3323, 3225, r, g, b);
                cmd_add_vec(3194, 3225, r, g, b);
                cmd_add_vec(3194, 3393, r, g, b);
                cmd_add_vec(3067, 3393, r, g, b);
                cmd_add_vec(3067, 3901, r, g, b);
                cmd_add_vec(2304, 3901, r, g, b);
                cmd_add_vec(2304, 3225, r, g, b);
                cmd_add_vec(2048, 3225, r, g, b);
                // Center structure
                cmd_add_vec(2048, 2373, 0, 0, 0);
                cmd_add_vec(2562, 2738, r, g, b);
                cmd_add_vec(2430, 2738, r, g, b);
                cmd_add_vec(2430, 2893, r, g, b);
                cmd_add_vec(2306, 2893, r, g, b);
                cmd_add_vec(2306, 3065, r, g, b);
                cmd_add_vec(2048, 3065, r, g, b);
                // Big structure
                cmd_add_vec(2938, 2209, 0, 0, 0);
                cmd_add_vec(3198, 2383, r, g, b);
                cmd_add_vec(3706, 2383, r, g, b);
                cmd_add_vec(3706, 2738, r, g, b);
                cmd_add_vec(2938, 2738, r, g, b);
                cmd_add_vec(2938, 2209, r, g, b);
                // Small structure
                cmd_add_vec(2551, 3055, 0, 0, 0);
                cmd_add_vec(2816, 3590, r, g, b);
                cmd_add_vec(2422, 3590, r, g, b);
                cmd_add_vec(2422, 3231, r, g, b);
                cmd_add_vec(2555, 3231, r, g, b);
                cmd_add_vec(2555, 3055, r, g, b);
                // Upper Left Quadrant
                // Outer structure
                cmd_add_vec(649, 2048, 0, 0, 0);
                cmd_add_vec(137, 2224, r, g, b);
                cmd_add_vec(137, 3059, r, g, b);
                cmd_add_vec(772, 3059, r, g, b);
                cmd_add_vec(772, 3225, r, g, b);
                cmd_add_vec(901, 3225, r, g, b);
                cmd_add_vec(901, 3393, r, g, b);
                cmd_add_vec(1028, 3393, r, g, b);
                cmd_add_vec(1028, 3901, r, g, b);
                cmd_add_vec(1792, 3901, r, g, b);
                cmd_add_vec(1792, 3225, r, g, b);
                cmd_add_vec(2048, 3225, r, g, b);
                // Center structure
                cmd_add_vec(2048, 2373, 0, 0, 0);
                cmd_add_vec(1533, 2738, r, g, b);
                cmd_add_vec(1665, 2738, r, g, b);
                cmd_add_vec(1665, 2893, r, g, b);
                cmd_add_vec(1789, 2893, r, g, b);
                cmd_add_vec(1789, 3065, r, g, b);
                cmd_add_vec(2048, 3065, r, g, b);
                // Big structure
                cmd_add_vec(1157, 2209, 0, 0, 0);
                cmd_add_vec(897, 2383, r, g, b);
                cmd_add_vec(389, 2383, r, g, b);
                cmd_add_vec(389, 2738, r, g, b);
                cmd_add_vec(1157, 2738, r, g, b);
                cmd_add_vec(1157, 2209, r, g, b);
                // Small structure
                cmd_add_vec(1544, 3055, 0, 0, 0);
                cmd_add_vec(1280, 3590, r, g, b);
                cmd_add_vec(1673, 3590, r, g, b);
                cmd_add_vec(1673, 3231, r, g, b);
                cmd_add_vec(1540, 3231, r, g, b);
                cmd_add_vec(1540, 3055, r, g, b);
                // Lower Right Quadrant
                // Outer structure
                cmd_add_vec(3446, 2048, 0, 0, 0);
                cmd_add_vec(3958, 1871, r, g, b);
                cmd_add_vec(3958, 1036, r, g, b);
                cmd_add_vec(3323, 1036, r, g, b);
                cmd_add_vec(3323, 870, r, g, b);
                cmd_add_vec(3194, 870, r, g, b);
                cmd_add_vec(3194, 702, r, g, b);
                cmd_add_vec(3067, 702, r, g, b);
                cmd_add_vec(3067, 194, r, g, b);
                cmd_add_vec(2304, 194, r, g, b);
                cmd_add_vec(2304, 870, r, g, b);
                cmd_add_vec(2048, 870, r, g, b);
                // Center structure
                cmd_add_vec(2048, 1722, 0, 0, 0);
                cmd_add_vec(2562, 1357, r, g, b);
                cmd_add_vec(2430, 1357, r, g, b);
                cmd_add_vec(2430, 1202, r, g, b);
                cmd_add_vec(2306, 1202, r, g, b);
                cmd_add_vec(2306, 1030, r, g, b);
                cmd_add_vec(2048, 1030, r, g, b);
                // Big structure
                cmd_add_vec(2938, 1886, 0, 0, 0);
                cmd_add_vec(3198, 1712, r, g, b);
                cmd_add_vec(3706, 1712, r, g, b);
                cmd_add_vec(3706, 1357, r, g, b);
                cmd_add_vec(2938, 1357, r, g, b);
                cmd_add_vec(2938, 1886, r, g, b);
                // Small structure
                cmd_add_vec(2551, 1040, 0, 0, 0);
                cmd_add_vec(2816, 505, r, g, b);
                cmd_add_vec(2422, 505, r, g, b);
                cmd_add_vec(2422, 864, r, g, b);
                cmd_add_vec(2555, 864, r, g, b);
                cmd_add_vec(2555, 1040, r, g, b);
                // Lower Left Quadrant
                // Outer structure
                cmd_add_vec(649, 2048, 0, 0, 0);
                cmd_add_vec(137, 1871, r, g, b);
                cmd_add_vec(137, 1036, r, g, b);
                cmd_add_vec(772, 1036, r, g, b);
                cmd_add_vec(772, 870, r, g, b);
                cmd_add_vec(901, 870, r, g, b);
                cmd_add_vec(901, 702, r, g, b);
                cmd_add_vec(1028, 702, r, g, b);
                cmd_add_vec(1028, 194, r, g, b);
                cmd_add_vec(1792, 194, r, g, b);
                cmd_add_vec(1792, 870, r, g, b);
                cmd_add_vec(2048, 870, r, g, b);
                // Center structure
                cmd_add_vec(2048, 1722, 0, 0, 0);
                cmd_add_vec(1533, 1357, r, g, b);
                cmd_add_vec(1665, 1357, r, g, b);
                cmd_add_vec(1665, 1202, r, g, b);
                cmd_add_vec(1789, 1202, r, g, b);
                cmd_add_vec(1789, 1030, r, g, b);
                cmd_add_vec(2048, 1030, r, g, b);
                // Big structure
                cmd_add_vec(1157, 1886, 0, 0, 0);
                cmd_add_vec(897, 1712, r, g, b);
                cmd_add_vec(389, 1712, r, g, b);
                cmd_add_vec(389, 1357, r, g, b);
                cmd_add_vec(1157, 1357, r, g, b);
                cmd_add_vec(1157, 1886, r, g, b);
                // Small structure
                cmd_add_vec(1544, 1040, 0, 0, 0);
                cmd_add_vec(1280, 505, r, g, b);
                cmd_add_vec(1673, 505, r, g, b);
                cmd_add_vec(1673, 864, r, g, b);
                cmd_add_vec(1540, 864, r, g, b);
                cmd_add_vec(1540, 1040, r, g, b);
                break;

            case GAME_WARRIOR:
                r = 108;
                g = 108;
                b = 108;
                cmd_add_vec(1187, 2232, 0, 0, 0);
                cmd_add_vec(1863, 2232, r, g, b);
                cmd_add_vec(1187, 1372, 0, 0, 0);
                cmd_add_vec(1863, 1372, r, g, b);
                cmd_add_vec(1187, 2232, 0, 0, 0);
                cmd_add_vec(1187, 1372, r, g, b);
                cmd_add_vec(1863, 2232, 0, 0, 0);
                cmd_add_vec(1863, 1372, r, g, b);
                cmd_add_vec(2273, 2498, 0, 0, 0);
                cmd_add_vec(2949, 2498, r, g, b);
                cmd_add_vec(2273, 1658, 0, 0, 0);
                cmd_add_vec(2949, 1658, r, g, b);
                cmd_add_vec(2273, 2498, 0, 0, 0);
                cmd_add_vec(2273, 1658, r, g, b);
                cmd_add_vec(2949, 2498, 0, 0, 0);
                cmd_add_vec(2949, 1658, r, g, b);
                break;
        }
        serial_send();
    }
END:    
    return s_dual_display;        
}



static void get_dvg_info()
{
    uint32_t cmd;
    uint8_t cmd_buf[4];
    int result;
    uint32_t version, major, minor;

    if (s_json_length) {
        return;
    }
    cmd = (FLAG_CMD << 29) | FLAG_CMD_GET_DVG_INFO;
    sscanf(ADV_VERSION, "%u.%u", &major, &minor);
    version = ((major & 0xf) << 12) | ((minor & 0xf) << 8) | (DVG_RELEASE << 4) | (DVG_BUILD);
    cmd |= version << 8;
    cmd_buf[0] = cmd >> 24;
    cmd_buf[1] = cmd >> 16;
    cmd_buf[2] = cmd >> 8;
    cmd_buf[3] = cmd >> 0;
    serial_write(cmd_buf, 4);
    result = serial_read(&cmd, sizeof(cmd));
    if (result < 0) goto END;
    result = serial_read(&s_json_length, sizeof(s_json_length));
    if (result < 0) goto END;
    s_json_length = MIN(s_json_length, sizeof(s_json_buf) - 1);
    result = serial_read(s_json_buf, s_json_length);
    if (result < 0) goto END;
    log_std(("dvg: JSON data %s\n", s_json_buf));
END:;

}


static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}


static int dvg_get_option(char *option, char *val_buf, uint32_t val_buf_size)
{
    int result = -1;
    jsmntok_t t[128];
    jsmn_parser p;
    int         r, i;

    get_dvg_info();
    jsmn_init(&p);
    r = jsmn_parse(&p, s_json_buf, strlen(s_json_buf), t, ARRAY_SIZE(t));
    if (r < 0) {
        log_std(("dvg: failed to parse JSON: %d\n", r));
        goto END;
    }
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        log_std(("dvg: JSON object expected.\n"));
        goto END;
    }
    for (i = 1; i < r; i++) {
        if (jsoneq(s_json_buf, &t[i], option) == 0) {
            int  size;
            size = t[i + 1].end - t[i + 1].start + 1;
            size = MIN(size, val_buf_size);
            strncpy(val_buf, s_json_buf + t[i + 1].start, size);
            val_buf[size - 1] = 0;
            result = 0;
            break;
        }
    }
END:
    return result;
}


//
// Init function
//
int dvg_init(const char *dvg_port, int dual_display)
{
    s_init = 1;
    s_dual_display = dual_display;
    s_cmd_buf = (uint8_t *)malloc(CMD_BUF_SIZE * sizeof(uint8_t));
    s_in_vec_list = (vector_t *)malloc(MAX_VECTORS * sizeof(vector_t));
    s_out_vec_list = (vector_t *)malloc(MAX_VECTORS * sizeof(vector_t));
    strncpy(s_serial_dev, dvg_port, sizeof(s_serial_dev) - 1);
    s_serial_dev[sizeof(s_serial_dev) - 1] = 0;
    log_std(("dvg: port is %s\n", s_serial_dev));
    return 0;
}

//
// Open the virtual serial port and register as a vector renderer to MAME.
//
int dvg_open()
{
    if (s_init) {
        log_std(("dvg: dvg_open()\n"));    
        s_first_call = 1;
        serial_open();
        vector_register_aux_renderer(dvg_update);
    }
    return 0;
}

//
// Close the virtual serial port.
//
void dvg_close()
{
    if (s_init) {
        log_std(("dvg: dvg_close()\n"));        
        serial_close();   
    } 
}
#endif
