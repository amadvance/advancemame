/*
 * This file is part of the Advance project
 *
 * Copyright (C) 2003 SVGALIB Team
 * Copyright (C) 2003 Andrea Mazzoleni
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

/*
 * This file is an adaption of the SVGALIB mouse driver.
 */

/*****************************************************************************/
/* Advance glue code */

/* Mouse types */
#define MOUSE_MICROSOFT 0
#define MOUSE_MOUSESYSTEMS 1
#define MOUSE_MMSERIES 2
#define MOUSE_LOGITECH 3
#define MOUSE_BUSMOUSE 4
#define MOUSE_PS2 5
#define MOUSE_LOGIMAN 6
#define MOUSE_GPM 7
#define MOUSE_SPACEBALL 8
#define MOUSE_NONE 9 /* Some special number for a non supported/existing mouse */
#define MOUSE_INTELLIMOUSE 10
#define MOUSE_IMPS2 11
#define MOUSE_PNP 12
#define MOUSE_WACOM_GRAPHIRE 13
#define MOUSE_DRMOUSE4DS 14
#define MOUSE_EXPPS2 15

/* MS IntelliMouse has 18 steps, Logitech FirstMouse+ has 24 */
#define DEFAULT_WHEEL_STEPS 18
#define DEFAULT_WHEEL_DELTA (360 / DEFAULT_WHEEL_STEPS)

#define MOUSE_ORIENTATION_VERTICAL 0
#define MOUSE_ORIENTATION_HORIZONTAL 1

#define MOUSE_LEFTBUTTON 4
#define MOUSE_MIDDLEBUTTON 2
#define MOUSE_RIGHTBUTTON 1
#define MOUSE_FOURTHBUTTON 8
#define MOUSE_FIFTHBUTTON 16
#define MOUSE_SIXTHBUTTON 32
#define MOUSE_RESETBUTTON 64

#define MOUSE_XDIM 1
#define MOUSE_YDIM 2
#define MOUSE_ZDIM 4
#define MOUSE_RXDIM 8
#define MOUSE_RYDIM 16
#define MOUSE_RZDIM 32
#define MOUSE_2DIM 3
#define MOUSE_3DIM 7
#define MOUSE_6DIM 63

#define MOUSE_DEFAULTSAMPLERATE 150

#define MOUSE_HAS_WHEEL 1 /* The mouse has a wheel */

#define MOUSEBUFFERSIZE 256
#define MOUSEDEVSIZE 256

struct raw_mouse_context {
	int button;
	int x;
	int y;
	int z;
	int rx;
	int ry;
	int rz;

	int type;
	char dev[MOUSEDEVSIZE];

	int info_cap;
	int info_button;
	int info_dim;

	int m_baud; /* Should be 1200. */
	int m_sample;
	int m_fd;
	int m_fdmode; /* 0 means don't wait (NDELAY) */
	int m_modem_ctl;
	int m_wheel_steps; /* Number of steps that make up a full turn of the wheel (for IntelliMouse & co) */
	int m_wheel_delta; /* Amount to change rotation about the X axis when wheel is turned */

	unsigned char e_buf[MOUSEBUFFERSIZE];
	int e_nu_bytes;
	int e_but;
	int e_mouse_orientation;
	unsigned int e_prev;
	int e_oldax;
	int e_olday;
	int e_nodev;
};

static void raw_mouse_handler(struct raw_mouse_context* context, int button, int dx, int dy, int dz, int drx, int dry, int drz)
{
    context->button = button;
    context->x += dx;
    context->y += dy;
    context->z += dz;
    context->rx += drx;
    context->ry += dry;
    context->rz += drz;
}

/*****************************************************************************/
/* ms.c from the VGALIB source */

/* Microsoft mouse with three button */
#define MS3B

/* Based on:
 * simple driver for serial mouse
 * Andrew Haylett, 14th December 1992
 * and on the driver in XFree86.
 * Edited for svgalib (hhanemaa@cs.ruu.nl).
 * This probably doesn't work with all types of bus mouse.
 * HH: Added PS/2 mouse support.
 * Fixed Logitech support thanks to Daniel Jackson.
 * MouseSystems movement overflow fixed by Steve VanDevender.
 * Logitech fixed again.
 * Michael: Added support for controlling DTR and RTS.
 * Added mouse acceleration 3/97 - Mike Chapman mike@paranoia.com
 * Added Intellimouse support 5/98 - Brion Vibber brion@pobox.com
 * Totally customisable mouse behaviour 28 Mar 1998 - by 101; 101@kempelen.inf.bme.hu
 * Added rx-axis support for IntelliMouse wheel and improved fake keyboard
 *  event scancode setting 7/98 - Brion
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_TERMIOS_H
#include <termios.h>
#endif

/* #define DEBUG */
/* #define DEBUG_ACCEL */
/* #define DEBUG_WHEEL */

static const unsigned short cflag[] = {
    (CS7 | CREAD | CLOCAL | HUPCL),			/* MicroSoft */
    (CS8 | CSTOPB | CREAD | CLOCAL | HUPCL),		/* MouseSystems */
    (CS8 | PARENB | PARODD | CREAD | CLOCAL | HUPCL),	/* MMSeries */
    (CS8 | CSTOPB | CREAD | CLOCAL | HUPCL),		/* Logitech */
    0,							/* BusMouse */
    0,							/* PS/2 */
    (CS7 | CREAD | CLOCAL | HUPCL),			/* MouseMan */
    (CS8 | CSTOPB | CREAD | CLOCAL | HUPCL),		/* GPM (MouseSystems) */
    (CS8 | CLOCAL | CREAD | IXON | IXOFF ),		/* Spaceball */
    0,							/* Dummy entry for MOUSE_NONE */
    (CS7 | CREAD | CLOCAL | HUPCL),			/* IntelliMouse (Serial) */
    CS7,       						/* IntelliMouse (PS/2) */
    (CS7 | CREAD | CLOCAL | HUPCL),			/* plug'n'pray */
    (CS8 | CREAD | CLOCAL | HUPCL),			/* Wacom Graphire tablet/mouse */
    0,							/* DRMOUSE4DS */
    0,							/* IntelliMouse Explorer (PS/2) */
};

static const unsigned char proto[][5] =
{
    /*  hd_mask hd_id   dp_mask dp_id   nobytes */
    {0x40, 0x40, 0x40, 0x00, 3},	/* MicroSoft */
    {0xf8, 0x80, 0x00, 0x00, 5},	/* MouseSystems */
    {0xe0, 0x80, 0x80, 0x00, 3},	/* MMSeries */
    {0xe0, 0x80, 0x00, 0x00, 3},	/* Logitech */
    {0xf8, 0x80, 0x00, 0x00, 5},	/* BusMouse */
    {0xc0, 0x00, 0x00, 0x00, 3},	/* PS/2 mouse */
    {0x40, 0x40, 0x40, 0x00, 3},	/* Mouseman */
    {0xf8, 0x80, 0x00, 0x00, 5},	/* gpm (MouseSystems) */
    {0xe0, 0x40, 0x00, 0x00, 6},	/* Spaceball */
    {0, 0, 0, 0, 0},			/* Dummy entry for MOUSE_NONE */
    {0xc0, 0x40, 0xc0, 0x00, 4},	/* IntelliMouse (Serial) */
    {0xc8, 0x08, 0x00, 0x00, 4},	/* IntelliMouse (PS/2) */
    {0x40, 0x40, 0x40, 0x00, 3},	/* pnp */
    {0x80, 0x80, 0x80, 0x00, 7},	/* Wacom Graphire */
    {0xc8, 0x08, 0x00, 0x00, 4},	/* DRMOUSE4DS (Digital Research 2-wheel PS/2) */
    {0xc8, 0x08, 0x00, 0x00, 4},        /* IntelliMouse Explorer (PS/2) */
};

static void raw_mouse_setspeed(struct raw_mouse_context* context, const int old, const int new, const unsigned short c_cflag)
{
    struct termios tty;
    char *c;

    tcgetattr(context->m_fd, &tty);

    tty.c_iflag = IGNBRK | IGNPAR;
    tty.c_oflag = 0;
    tty.c_lflag = 0;
#if 0 /* only present in Linux */
    tty.c_line  = 0;
#endif
    tty.c_cc[VTIME] = 0;
    tty.c_cc[VMIN]  = 1;

    switch (old) {
    case 9600:
	tty.c_cflag = c_cflag | B9600;
	break;
    case 4800:
	tty.c_cflag = c_cflag | B4800;
	break;
    case 2400:
	tty.c_cflag = c_cflag | B2400;
	break;
    case 1200:
    default:
	tty.c_cflag = c_cflag | B1200;
	break;
    }

    tcsetattr(context->m_fd, TCSAFLUSH, &tty);

    switch (new) {
    case 9600:
	c = "*q";
	tty.c_cflag = c_cflag | B9600;
	break;
    case 4800:
	c = "*p";
	tty.c_cflag = c_cflag | B4800;
	break;
    case 2400:
	c = "*o";
	tty.c_cflag = c_cflag | B2400;
	break;
    case 1200:
    default:
	c = "*n";
	tty.c_cflag = c_cflag | B1200;
	break;
    }

    write(context->m_fd, c, 2);
    usleep(10000);
    tcsetattr(context->m_fd, TCSAFLUSH, &tty);
}

static int raw_mouse_init(struct raw_mouse_context* context)
{
	context->button = 0;
	context->x = 0;
	context->y = 0;
	context->z = 0;
	context->rx = 0;
	context->ry = 0;
	context->rz = 0;
	context->m_sample = MOUSE_DEFAULTSAMPLERATE;
	context->m_baud = 1200;
	context->m_fd = -1;
	context->m_fdmode = 0;
	context->m_modem_ctl = 0;
	context->m_wheel_steps = DEFAULT_WHEEL_STEPS;
	context->m_wheel_delta = DEFAULT_WHEEL_DELTA;
	context->e_nu_bytes = 0;
	context->e_but = 0;
	context->e_mouse_orientation = 0;
	context->e_prev = 0;
	context->e_oldax = 0;
	context->e_olday = 0;
	context->e_nodev = 0;

#ifdef DEBUG_ACCEL
    fprintf(stderr,"m_accel_type: %ld\n",	(long)m_accel_type);
    fprintf(stderr,"m_force: %ld\n",		(long)m_force);
    fprintf(stderr,"m_accel_thresh: %ld\n",	(long)m_accel_thresh);
    fprintf(stderr,"m_accel_offset: %ld\n",	(long)m_accel_offset);
    fprintf(stderr,"m_accel_mult: %f\n",	(double)m_accel_mult);
    fprintf(stderr,"m_accel_power: %f\n",	(double)m_accel_power);
    fprintf(stderr,"m_maxdelta: %ld\n",		(long)m_maxdelta);
    fprintf(stderr,"m_accel_maxdelta: %ld\n",	(long)m_accel_maxdelta);
#endif

    /* Set the proper wheel delta */
    if(context->m_wheel_steps)
        context->m_wheel_delta = (360 / context->m_wheel_steps);
    else
        context->m_wheel_delta = 0;

    if ((context->m_fd = open(context->dev, O_RDWR | O_NDELAY )) < 0) {
	return -1;
    }

    if (context->type == MOUSE_BUSMOUSE || context->type == MOUSE_PS2 
    		|| context->type == MOUSE_IMPS2 || context->type == MOUSE_GPM
		|| context->type == MOUSE_DRMOUSE4DS || context->type == MOUSE_EXPPS2)
	context->m_modem_ctl = 0;

    if (context->m_modem_ctl) {
	/* the modem configuration is removed */
	return -1;
    }

    if (context->type == MOUSE_SPACEBALL) {
      context->m_baud = 9600;
      raw_mouse_setspeed(context, 1200, context->m_baud, cflag[context->type]);
    } else if (context->type == MOUSE_LOGIMAN) {
	raw_mouse_setspeed(context, 9600, 1200, cflag[context->type]);
	raw_mouse_setspeed(context, 4800, 1200, cflag[context->type]);
	raw_mouse_setspeed(context, 2400, 1200, cflag[context->type]);
	raw_mouse_setspeed(context, 1200, 1200, cflag[context->type]);
	write(context->m_fd, "*X", 2);
	raw_mouse_setspeed(context, 1200, context->m_baud, cflag[context->type]);
    } else if (context->type == MOUSE_WACOM_GRAPHIRE) {
    	context->m_baud = 9600;
    	raw_mouse_setspeed(context, 1200, context->m_baud, cflag[context->type]);
    	/* Reset baud rate */
    	write(context->m_fd, "\r$", 2);
    	usleep(250000);
    	/* Reset tablet */
    	write(context->m_fd, "\r#", 2);
    	usleep(75000);
    	/* Set hardware filtering */
    	write(context->m_fd, "\rSU3", 4);
    	usleep(75000);
    	/* Start sending coordinates */
    	write(context->m_fd, "\rST\r", 4);
    } else if (context->type == MOUSE_IMPS2 || context->type == MOUSE_DRMOUSE4DS) {
	/* Initialize the mouse into wheel mode */
	write(context->m_fd, "\363\310\363\144\363\120", 6);
    } else if (context->type == MOUSE_EXPPS2) {
	write(context->m_fd, "\363\310\363\310\363\120", 6);
    } else if (context->type == MOUSE_PNP) {
    	/* Need to do this termios stuff here, by hand, raw_mouse_setspeed won't 
	   work with pnp */
	struct termios tty;
 	context->m_baud = 1200;
	tcgetattr(context->m_fd, &tty);   
	tty.c_iflag = IGNBRK | IGNPAR;
	tty.c_oflag = 0;
	tty.c_lflag = 0;
#if 0 /* only present in Linux */
	tty.c_line = 0;
#endif
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 1;
	tty.c_cflag = cflag[context->type] | B1200;
	tcsetattr(context->m_fd, TCSAFLUSH, &tty);
    } else if (context->type != MOUSE_BUSMOUSE && context->type != MOUSE_PS2) {
	raw_mouse_setspeed(context, 9600, context->m_baud, cflag[context->type]);
	raw_mouse_setspeed(context, 4800, context->m_baud, cflag[context->type]);
	raw_mouse_setspeed(context, 2400, context->m_baud, cflag[context->type]);
	raw_mouse_setspeed(context, 1200, context->m_baud, cflag[context->type]);

	if (context->type == MOUSE_LOGITECH) {
	    write(context->m_fd, "S", 1);
	    raw_mouse_setspeed(context, context->m_baud, context->m_baud, cflag[MOUSE_MMSERIES]);
	}
	if (context->m_sample <= 0)
	    write(context->m_fd, "O", 1);
	else if (context->m_sample <= 15)
	    write(context->m_fd, "J", 1);
	else if (context->m_sample <= 27)
	    write(context->m_fd, "K", 1);
	else if (context->m_sample <= 42)
	    write(context->m_fd, "L", 1);
	else if (context->m_sample <= 60)
	    write(context->m_fd, "R", 1);
	else if (context->m_sample <= 85)
	    write(context->m_fd, "M", 1);
	else if (context->m_sample <= 125)
	    write(context->m_fd, "Q", 1);
	else
	    write(context->m_fd, "N", 1);
    }

    context->info_button = 0;
    context->info_dim = 0;
    context->info_cap = 0;

    switch(context->type) {
    case MOUSE_SPACEBALL:
        /* 6 axes, many buttons */
        context->info_button =
            MOUSE_LEFTBUTTON | MOUSE_MIDDLEBUTTON | MOUSE_RIGHTBUTTON |
            MOUSE_FOURTHBUTTON | MOUSE_FIFTHBUTTON | MOUSE_SIXTHBUTTON |
            MOUSE_RESETBUTTON;
        context->info_dim = MOUSE_6DIM;
        break;
    
    case MOUSE_DRMOUSE4DS:
    	/* X, Y, RX and RY (two wheels), 3 buttons */
	context->info_dim = MOUSE_2DIM | MOUSE_RXDIM | MOUSE_RYDIM;
	context->info_button = MOUSE_LEFTBUTTON | MOUSE_RIGHTBUTTON | MOUSE_MIDDLEBUTTON;
	context->info_cap = MOUSE_HAS_WHEEL;
	break;

    case MOUSE_EXPPS2:
	context->info_dim = MOUSE_2DIM | MOUSE_RXDIM;
        context->info_button =
            MOUSE_LEFTBUTTON | MOUSE_MIDDLEBUTTON | MOUSE_RIGHTBUTTON |
            MOUSE_FOURTHBUTTON | MOUSE_FIFTHBUTTON;
        context->info_cap = MOUSE_HAS_WHEEL;
	break;

    case MOUSE_INTELLIMOUSE:
    case MOUSE_IMPS2:
    case MOUSE_WACOM_GRAPHIRE:
	/* X, Y, RX (wheel), 3 buttons, wheel */
	context->info_dim = MOUSE_2DIM | MOUSE_RXDIM;
	context->info_button = MOUSE_LEFTBUTTON | MOUSE_MIDDLEBUTTON | MOUSE_RIGHTBUTTON;
	context->info_cap = MOUSE_HAS_WHEEL;
	break;

    case MOUSE_LOGIMAN:		/* Some TrackMen have 4 buttons */
	context->info_button = MOUSE_FOURTHBUTTON;

    case MOUSE_MOUSESYSTEMS:
    case MOUSE_MMSERIES:
    case MOUSE_LOGITECH:
    case MOUSE_BUSMOUSE:
    case MOUSE_PS2:
    case MOUSE_GPM:
    case MOUSE_PNP:
#ifdef MS3B
    case MOUSE_MICROSOFT: /* Two buttons only */
#endif
        /* Any of these _can_ have 3 buttons, but may not */
        context->info_button |= MOUSE_MIDDLEBUTTON;

#ifndef MS3B
    case MOUSE_MICROSOFT: /* Two buttons only */
#endif
        context->info_button |= MOUSE_LEFTBUTTON | MOUSE_RIGHTBUTTON;
        context->info_dim |= MOUSE_2DIM;
        break;
    }

    return 0;
}

/* Scooped from X driver. */
static void raw_mouse_close(struct raw_mouse_context* context)
{
    if (context->m_fd == -1)
	return;
    if (context->type == MOUSE_LOGITECH) {
	write(context->m_fd, "U", 1);
	raw_mouse_setspeed(context, context->m_baud, 1200, cflag[MOUSE_LOGITECH]);
    }

    close(context->m_fd);
    context->m_fd = -1;
}

static int raw_mouse_poll(struct raw_mouse_context* context, int wait) {
/*
   Changed to process multiple packets.
   wait value:
   0    Process any mouse events, and return status.
   1    Wait for mouse event, then return.

   Status indicates whether an event was processed.
 */
    int nu_packets = 0;
    char event_handled = 0;
    int bytesread;
    int i, wheel;
/*  int but; */ /* static is hack for MouseMan */
    int dx=0, dy=0, dz=0, drx=0, dry=0, drz=0;
    int ax=0, ay=0;
    int j;
    char SpaceWare[] = "SpaceWare!";

    if (context->m_fd == -1)
	return -1;

  again:

    if (context->m_fdmode == 1) {
	/* We don't want to wait, set NDELAY mode. */
	fcntl(context->m_fd, F_SETFL, O_RDONLY | O_NDELAY );
	context->m_fdmode = 0;
    }
        bytesread = read(context->m_fd,
		     &context->e_buf[context->e_nu_bytes], MOUSEBUFFERSIZE - context->e_nu_bytes);
    i = 0;

    if (bytesread >= 1)
	context->e_nu_bytes += bytesread;

#ifdef DEBUG
    fprintf(stderr,"#bytes in buffer: %d\n", context->e_nu_bytes);
#endif

  handle_packets:

    /* Handle packets in buffer. */

#ifdef DEBUG
    fprintf(stderr,"Bytes left in buffer: %d at %d, packet is %d bytes\n",
	   context->e_nu_bytes - i, i, proto[context->type][4]);
    if (context->e_nu_bytes - i > 0 )
      fprintf(stderr,"Header byte: %c %d\n", (context->e_buf[i] & 0177), context->e_buf[i]);

#endif

    if ((context->type == MOUSE_LOGIMAN) &&
	((context->e_nu_bytes - i) >= 1) &&
	((context->e_buf[i] & proto[context->type][0]) != proto[context->type][1]) &&
	((char) (context->e_buf[i] & ~0x33) == 0)) { /* s/23/33/, for 4-but trackman */
	/* Hack-o-matic, stolen from xf86_Mouse.c */
	context->e_but = ((context->e_buf[i] & 0x20) >> 4) | ((context->e_buf[i] & 0x10) >> 1) | (context->e_but & 0x05);
	raw_mouse_handler(context, context->e_but, 0, 0, 0, 0, 0, 0);
	event_handled++;
	i++;
    }
    if ((context->type == MOUSE_SPACEBALL)) {
      j=i;
      while ((context->e_nu_bytes - j > 0) && (context->e_buf[j]!=13))
        j++;
      nu_packets=(context->e_buf[j]==13);
    } else {
      nu_packets=1;
    }
    if ((nu_packets==0)||(context->e_nu_bytes - i < proto[context->type][4])) {
	/* No full packet available. */
	if (wait == 0 || (wait == 1 && event_handled)) {
	    if (i >= context->e_nu_bytes) {
		context->e_nu_bytes = 0;
		i = 0;
	    } else {
		/* Move partial packet to front of buffer. */
		for (j = i; j < context->e_nu_bytes; j++)
		    context->e_buf[j - i] = context->e_buf[j];
		context->e_nu_bytes -= i;
	    }
	    return event_handled;
	} else {		/* (wait == 1 && !event_handled) */
	    if (i >= context->e_nu_bytes) {
		context->e_nu_bytes = 0;
		i = 0;
	    }
	    /* Wait mode, we'll sleep on reads. */
	    fcntl(context->m_fd, F_SETFL, O_RDONLY);
	    context->m_fdmode = 1;
	    read(context->m_fd, &context->e_buf[context->e_nu_bytes], 1);
            if ((context->type == MOUSE_SPACEBALL)) {
              nu_packets=(context->e_buf[context->e_nu_bytes]==13);
            } else {
              nu_packets=1;
            }
	    context->e_nu_bytes++;
	    if ((nu_packets==0)||(context->e_nu_bytes - i < proto[context->type][4]))
		/* Not a complete packet. */
		goto again;
	}
    }

    /* Check header byte. */
    if ((context->e_buf[i] & proto[context->type][0]) != proto[context->type][1]) {
	/* Not a header byte. */
#ifdef DEBUG
    fprintf(stderr,"Bad header byte: %c %d\n", (context->e_buf[i] & 0177), context->e_buf[i]);
#endif
	i++;
	goto handle_packets;
    }
    /* Check whether it's a valid data packet. */
    if ((context->type != MOUSE_PS2)&&(context->type != MOUSE_IMPS2)
	&&(context->type != MOUSE_EXPPS2)&&(context->type != MOUSE_SPACEBALL)
    	&&(context->type != MOUSE_WACOM_GRAPHIRE) && (context->type != MOUSE_DRMOUSE4DS) )
	for (j = 1; j < proto[context->type][4]; j++)
	    if ((context->e_buf[i + j] & proto[context->type][2]) != proto[context->type][3]
		|| context->e_buf[i + j] == 0x80) {
		i = i + j + 1;
		goto handle_packets;
	    }
    /* Construct the event. */
    switch (context->type) {
#ifdef MS3B
    case MOUSE_MICROSOFT:	/* Microsoft */
        context->e_but = (context->e_but & 8) | ((context->e_buf[i] & 0x20) >> 3) | ((context->e_buf[i] & 0x10) >> 4);
	dx = (char) (((context->e_buf[i] & 0x03) << 6) | (context->e_buf[i + 1] & 0x3F));
	dy = (char) (((context->e_buf[i] & 0x0C) << 4) | (context->e_buf[i + 2] & 0x3F));
        if((dx==0)&&(dy==0)&&(context->e_but==(context->e_prev&~MOUSE_MIDDLEBUTTON)))
            context->e_but=context->e_prev^MOUSE_MIDDLEBUTTON; else
            context->e_but |= context->e_prev&MOUSE_MIDDLEBUTTON;
        context->e_prev=context->e_but;
        break;
#else
    case MOUSE_MICROSOFT:	/* Microsoft */
#endif
    case MOUSE_LOGIMAN:	/* MouseMan / TrackMan */
    case MOUSE_PNP:
    default:
	context->e_but = (context->e_but & 0x0A) | ((context->e_buf[i] & 0x20) >> 3) | ((context->e_buf[i] & 0x10) >> 4);
	dx = (char) (((context->e_buf[i] & 0x03) << 6) | (context->e_buf[i + 1] & 0x3F));
	dy = (char) (((context->e_buf[i] & 0x0C) << 4) | (context->e_buf[i + 2] & 0x3F));
        break;
    case MOUSE_WACOM_GRAPHIRE: /* Wacom Graphire Tablet */
        if (!(context->e_buf[i] & 0x40)) { /* no device on tablet */
        	context->e_nodev = 1;
        	break;
        }
        context->e_but =   (context->e_buf[i+3] & 0x08) ? MOUSE_LEFTBUTTON : 0 |
        	(context->e_buf[i+3] & 0x10) ? MOUSE_RIGHTBUTTON : 0 |
        	(context->e_buf[i+3] & 0x20) ? MOUSE_MIDDLEBUTTON : 0;

        /* The absolute position is returned, not the change in position, so
           we convert it. */
        ax = ((context->e_buf[i+0] & 0x03) << 14) | (context->e_buf[i+1] << 7) | context->e_buf[i+2];
        ay = ((context->e_buf[i+3] & 0x03) << 14) | (context->e_buf[i+4] << 7) | context->e_buf[i+5];

        if (context->e_nodev) {
        	context->e_oldax = ax;
        	context->e_olday = ay;
        	context->e_nodev = 0;
        }

	dx = ax - context->e_oldax;
	dy = ay - context->e_olday;

	dz = (((context->e_buf[i+6] & 0x3f) << 1) | ((context->e_buf[i+3] & 0x04) >> 2));

	if (context->e_buf[i+6] & 0x40)
		dz = -dz;

	/* The tablet has *very* high resolution, so we normalize
	   that a bit. */
	dx /= 2;
	dy /= 2;
	
	context->e_oldax = ax;
	context->e_olday = ay;

	if (context->e_buf[i] & 0x20 && dz) /* stylus has pressure */
		context->e_but |= MOUSE_LEFTBUTTON;
	else if (context->e_buf[i+6] & 0x30) { /* roller is being turned */
		wheel = (context->e_buf[i+6] & 0x30) >> 3;
		if (context->e_buf[i+6] & 0x40)
			wheel = -wheel;
#ifdef DEBUG_WHEEL
            fprintf(stderr, " Wheel turned (0x%02x)", wheel);
#endif
            /* RX-axis */
            if(context->m_wheel_delta) {
                drx = ((wheel < 0) ? (-context->m_wheel_delta) : context->m_wheel_delta);
#ifdef DEBUG_WHEEL
                fprintf(stderr, "; RX axis delta = %d", drx);
#endif
            }
        }
	break;

    case MOUSE_INTELLIMOUSE:    /* Serial IntelliMouse */
        /* This bit modified from gpm 1.13 */
        context->e_but = ((context->e_buf[i] & 0x20) >> 3)         /* left */
               | ((context->e_buf[i + 3] & 0x10) >> 3)  /* middle */
               | ((context->e_buf[i] & 0x10) >> 4);     /* right */
        dx = (char) (((context->e_buf[i] & 0x03) << 6) | (context->e_buf[i + 1] & 0x3F));
        dy = (char) (((context->e_buf[i] & 0x0C) << 4) | (context->e_buf[i + 2] & 0x3F));

        /* Did we turn the wheel? */
        if((wheel = context->e_buf[i + 3] & 0x0f) != 0) {
#ifdef DEBUG_WHEEL
            fprintf(stderr, " Wheel turned (0x%02x)", wheel);
#endif
            /* RX-axis */
            if(context->m_wheel_delta) {
                drx = ((wheel > 7) ? (-context->m_wheel_delta) : context->m_wheel_delta);
#ifdef DEBUG_WHEEL
                fprintf(stderr, "; RX axis delta = %d", drx);
#endif
            }
        }
        break;
    case MOUSE_MOUSESYSTEMS:	/* Mouse Systems Corp */
    case MOUSE_GPM:
	context->e_but = (~context->e_buf[i]) & 0x07;
	dx = (char) (context->e_buf[i + 1]);
	dx += (char) (context->e_buf[i + 3]);
	dy = -((char) (context->e_buf[i + 2]));
	dy -= (char) (context->e_buf[i + 4]);
	break;
    case MOUSE_MMSERIES:	/* MM Series */
    case MOUSE_LOGITECH:	/* Logitech */
	context->e_but = context->e_buf[i] & 0x07;
	dx = (context->e_buf[i] & 0x10) ? context->e_buf[i + 1] : -context->e_buf[i + 1];
	dy = (context->e_buf[i] & 0x08) ? -context->e_buf[i + 2] : context->e_buf[i + 2];
	break;
    case MOUSE_BUSMOUSE:	/* BusMouse */
	context->e_but = (~context->e_buf[i]) & 0x07;
	dx = (char) context->e_buf[i + 1];
	dy = -(char) context->e_buf[i + 2];
	break;
    case MOUSE_PS2:		/* PS/2 mouse */
	context->e_but = (context->e_buf[i] & 0x04) >> 1 |	/* Middle */
	    (context->e_buf[i] & 0x02) >> 1 |	/* Right */
	    (context->e_buf[i] & 0x01) << 2;	/* Left */
	dx = (context->e_buf[i] & 0x10) ? context->e_buf[i + 1] - 256 : context->e_buf[i + 1];
	dy = (context->e_buf[i] & 0x20) ? -(context->e_buf[i + 2] - 256) : -context->e_buf[i + 2];
	break;
    case MOUSE_DRMOUSE4DS:
    /* Digital Research 4-Axis mouse - like the PS/2 IntelliMouse, context->e_but 
       has two wheels.  */
        /* This bit modified from the gpm 1.13 imps2 patch by Tim Goodwin */
        context->e_but = ((context->e_buf[i] & 1) << 2) /* left */
            | ((context->e_buf[i] & 6) >> 1); /* middle and right */
        dx = (context->e_buf[i] & 0x10) ? context->e_buf[i + 1] - 256 : context->e_buf[i + 1];
        dy = (context->e_buf[i] & 0x20) ? -(context->e_buf[i + 2] - 256) : -context->e_buf[i + 2];

        /* Did we turn the wheel? */
        if((wheel = context->e_buf[i + 3]) != 0) {
#ifdef DEBUG_WHEEL
            fprintf(stderr, " Wheel turned (0x%02x)", wheel);
#endif
	/* RX reports as 1 or F, RY reports as 2 or E */
	/* both never report at the same time, which makes life easier */
	if (context->m_wheel_delta) 
	{
		switch (wheel & 0x0F)
		{
		case 1:		drx = context->m_wheel_delta; break;
		case 15:	drx = -context->m_wheel_delta; break;
		case 2:		dry = -context->m_wheel_delta; break;
		case 14:	dry = context->m_wheel_delta; break;
		};
#ifdef DEBUG_WHEEL
                fprintf(stderr, "; RX axis delta = %d : RY axis delta = %d", drx, dry);
#endif

#ifdef DEBUG_WHEEL
            fprintf(stderr, ".\n");
#endif
	}
    }
    break;
    
    case MOUSE_IMPS2:           /* PS/2 IntelliMouse */
        /* This bit modified from the gpm 1.13 imps2 patch by Tim Goodwin */
        context->e_but = ((context->e_buf[i] & 1) << 2) /* left */
            | ((context->e_buf[i] & 6) >> 1); /* middle and right */
        dx = (context->e_buf[i] & 0x10) ? context->e_buf[i + 1] - 256 : context->e_buf[i + 1];
        dy = (context->e_buf[i] & 0x20) ? -(context->e_buf[i + 2] - 256) : -context->e_buf[i + 2];

        /* Did we turn the wheel? */
        if((wheel = context->e_buf[i + 3]) != 0) {
#ifdef DEBUG_WHEEL
            fprintf(stderr, " Wheel turned (0x%02x)", wheel);
#endif
            /* RX-axis */
            if(context->m_wheel_delta) {
                drx = ((wheel > 0x7f) ? (-context->m_wheel_delta) : context->m_wheel_delta);
#ifdef DEBUG_WHEEL
                fprintf(stderr, "; RX axis delta = %d", drx);
#endif
            }
#ifdef DEBUG_WHEEL
            fprintf(stderr, ".\n");
#endif
        }
        break;
    
    case MOUSE_EXPPS2:           /* PS/2 IntelliMouse Explorer */
        /* This bit modified from the gpm 1.13 imps2 patch by Tim Goodwin */
        context->e_but = ((context->e_buf[i] & 1) << 2) /* left */
            | ((context->e_buf[i] & 6) >> 1) /* middle and right */
	    | ((context->e_buf[i+3] & 48) >> 1);
        dx = (context->e_buf[i] & 0x10) ? context->e_buf[i + 1] - 256 : context->e_buf[i + 1];
        dy = (context->e_buf[i] & 0x20) ? -(context->e_buf[i + 2] - 256) : -context->e_buf[i + 2];

        /* Did we turn the wheel? */
        if(((wheel = context->e_buf[i + 3]) & 15) != 0) {
#ifdef DEBUG_WHEEL
            fprintf(stderr, " Wheel turned (0x%02x)", wheel);
#endif
            /* RX-axis */
            if(context->m_wheel_delta) {
                drx = ((wheel > 7) ? (-context->m_wheel_delta) : context->m_wheel_delta);
#ifdef DEBUG_WHEEL
                fprintf(stderr, "; RX axis delta = %d", drx);
#endif
            }
#ifdef DEBUG_WHEEL
            fprintf(stderr, ".\n");
#endif
        }
        break;

    case MOUSE_SPACEBALL:

	switch (context->e_buf[i]) {
	  case 'D':
	
	    context->e_but=0177 & context->e_buf[i+1];

	    /* Strip the MSB, which is a parity bit */
	    for (j = 2; j < 11; ++j) {
	        context->e_buf[i+j] &= 0177;           /* Make sure everything is 7bit */
	        context->e_buf[i+j] ^= SpaceWare[j-2]; /* What's this doing in the data? */
	    }

	    /* Turn chars into 10 bit integers */
	    if (context->e_mouse_orientation == MOUSE_ORIENTATION_VERTICAL) {
	      dx = ((context->e_buf[i+2] & 0177)<<3)|((context->e_buf[i+3] & 0160)>>4);
	      dy = ((context->e_buf[i+3] & 0017)<<6)|((context->e_buf[i+4] & 0176)>>1);
	      dz = ((context->e_buf[i+4] & 0001)<<9)|((context->e_buf[i+5] & 0177)<<2)|
                    ((context->e_buf[i+6] & 0140)>>5);
	      drx = ((context->e_buf[i+6] & 0037)<<5)|((context->e_buf[i+7] & 0174)>>2);
	      dry = ((context->e_buf[i+7] & 0003)<<8)|((context->e_buf[i+8] & 0177)<<1)|
                     ((context->e_buf[i+9] & 0100)>>6);
	      drz = ((context->e_buf[i+9] & 0077)<<4)|((context->e_buf[i+10] & 0170)>>3);
	    } else {
	      dx = ((context->e_buf[i+2] & 0177)<<3)|((context->e_buf[i+3] & 0160)>>4);
	      dz = ((context->e_buf[i+3] & 0017)<<6)|((context->e_buf[i+4] & 0176)>>1);
	      dy = ((context->e_buf[i+4] & 0001)<<9)|((context->e_buf[i+5] & 0177)<<2)|
                    ((context->e_buf[i+6] & 0140)>>5);
	      drx = ((context->e_buf[i+6] & 0037)<<5)|((context->e_buf[i+7] & 0174)>>2);
	      drz = ((context->e_buf[i+7] & 0003)<<8)|((context->e_buf[i+8] & 0177)<<1)|
                     ((context->e_buf[i+9] & 0100)>>6);
	      dry = ((context->e_buf[i+9] & 0077)<<4)|((context->e_buf[i+10] & 0170)>>3);
	    }

	    /* Get the sign right. */
	    if (dx > 511) dx -= 1024;
	    if (dy > 511) dy -= 1024;
	    if (dz > 511) dz -= 1024;
	    if (drx > 511) drx -= 1024;
	    if (dry > 511) dry -= 1024;
	    if (drz > 511) drz -= 1024;
            if (context->e_mouse_orientation == MOUSE_ORIENTATION_HORIZONTAL)
              { dz *= -1;
                drz *= -1;
              }
	    /*    if (fabs(dx) < sorb_trans_thresh[1]) dx = 0; */
            i+=13;
#ifdef DEBUG
    fprintf(stderr,"Got D packet! context->e_but=%d, x=%d y=%d z=%d rx=%d ry=%d rz=%d\n",
            context->e_but,dx,dy,dz,drx,dry,drz);
#endif
            break;
	  case 'K':
	/* Button press/release w/out motion */
	    context->e_but=0177 & context->e_buf[i+2];
	    if (context->e_but==MOUSE_RESETBUTTON)
	      context->e_mouse_orientation=1-context->e_mouse_orientation;
#ifdef DEBUG
    fprintf(stderr,"Got K packet! context->e_but=%d, x=%d y=%d z=%d rx=%d ry=%d rz=%d\n",
            context->e_but,dx,dy,dz,drx,dry,drz);
#endif
	    i+=6;
	    break;
	  case 'R':
#ifdef DEBUG
    fprintf(stderr,"Got init string!\n");
#endif
	    for (j=i;((context->e_buf[j] !=13)&&(j<context->e_nu_bytes));j++)
              fprintf(stderr,"%c",(context->e_buf[j] & 0177));
            fprintf(stderr,"\n\n");
            i=j+1;
            break;
	  default:
#ifdef DEBUG
    fprintf(stderr,"Got unknown packet!\n");
#endif
            i++;
            break;
	}

	break;
    }

    if (context->type != MOUSE_SPACEBALL)
      i += proto[context->type][4];

    /* Try to snag that optional mouseman fourth byte, if present */
    if ((context->type == MOUSE_LOGIMAN) &&
	((context->e_nu_bytes - i) >= 1) &&
	((context->e_buf[i] & proto[context->type][0]) != proto[context->type][1]) &&
	((char) (context->e_buf[i] & ~0x23) == 0)) {
	/* Hack-o-matic, stolen from xf86_Mouse.c */
	context->e_but = ((context->e_buf[i] & 0x20) >> 4) | (context->e_but & 0x05);
	i++;
    }

    raw_mouse_handler(context, context->e_but, dx, dy, dz, drx, dry, drz);

    event_handled = 1;

    goto handle_packets;
}

