/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2004 Andrea Mazzoleni
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

#include "portable.h"

#include "lcd.h"
#include "log.h"
#include "target.h"
#include "snstring.h"

#if HAVE_NETDB_H
#include <netdb.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h> /* On Darwin, `stdlib.h' is a prerequisite. */
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

static adv_error adv_lcd_sendstr_noblock(adv_lcd* context, char* data)
{
	int r;
	unsigned len = strlen(data);

	log_std(("lcd: send %s", data));

	r = write(context->f, data, len);

	if (r < 0) {
		if (errno == EAGAIN) {
			log_std(("WARNING:lcd: ignored data due blocking call\n"));
			return 0;
		} else {
			log_std(("ERROR:lcd: send() failed, with error %d, %s\n", errno, strerror(errno)));
			return -1;
		}
	}

	if (r != len) {
		log_std(("ERROR:lcd: incomplete send(), %d instead of %d\n", r, len));
		return -1;
	}

	return 0;
}

static adv_error adv_lcd_send_wait(adv_lcd* context, const char* buffer, unsigned size, struct timeval* tv)
{
	while (size > 0) {
		int r;

		r = write(context->f, buffer, size);

		if (r < 0) {
			fd_set ds;

			if (errno != EWOULDBLOCK) {
				log_std(("ERROR:lcd: write() failed, with error %d, %s\n", errno, strerror(errno)));
				return -1;
			}

			FD_ZERO(&ds);
			FD_SET(context->f, &ds);

			r = select(FD_SETSIZE, 0, &ds, 0, tv);
			if (r < 0) {
				log_std(("ERROR:lcd: select() failed, with error %d, %s\n", errno, strerror(errno)));
				return -1;
			}
			if (r == 0) {
				log_std(("ERROR:lcd: timeout in write\n"));
				return -1;
			}
		} else {
			buffer += r;
			size -= r;
		}
	}

	return 0;
}

static adv_error adv_lcd_sendstr_wait(adv_lcd* context, const char* buffer, struct timeval* tv)
{
	log_std(("lcd: send %s", buffer));

	return adv_lcd_send_wait(context, buffer, strlen(buffer), tv);
}

static adv_error adv_lcd_recv_wait(adv_lcd* context, char* buffer, unsigned size, int stop, struct timeval* tv)
{
	unsigned count;

	count = 0;

	while (size > 0) {
		int r;

		r = read(context->f, buffer, 1);

		if (r < 0) {
			fd_set ds;

			if (errno != EWOULDBLOCK) {
				log_std(("ERROR:lcd: read() failed, with error %d, %s\n", errno, strerror(errno)));
				return -1;
			}

			FD_ZERO(&ds);
			FD_SET(context->f, &ds);

			r = select(FD_SETSIZE, &ds, 0, 0, tv);
			if (r < 0) {
				log_std(("ERROR:lcd: select() failed, with error %d, %s\n", errno, strerror(errno)));
				return -1;
			}
			if (r == 0) {
				log_std(("ERROR:lcd: timeout in read\n"));
				return -1;
			}
		} else {
			if (*buffer == stop)
				break;
			buffer += 1;
			size -= 1;
			++count;
		}
	}

	return count;
}

static adv_error adv_lcd_recvstr_wait(adv_lcd* context, char* buffer, unsigned size, struct timeval* tv)
{
	int r;

	if (size == 0)
		return -1;

	r = adv_lcd_recv_wait(context, buffer, size - 1, '\n', tv);

	if (r < 0)
		return -1;

	buffer[r] = 0;

	log_std(("lcd: recv %s\n", buffer));

	return 0;
}

/**
 * Initialize the LCD support.
 * \note To prevent the program termination if the server is closed you
 * need to ignore or intercept the SIGPIPE signal.
 * \param address Server location in the form [address][:port]. The address may
 * be a name or a dot address. The default address is 127.0.0.1. The default
 * port is 13666.
 * \param timeou Connection timeout in milleseconds.
 * \return The LCD context to use on other calls or 0 on error.
 */
adv_lcd* adv_lcd_init(const char* address, unsigned timeout)
{
	struct sockaddr_in server_addr;
	struct hostent* host;
	adv_lcd* context;
	int r;
	struct timeval tv;
	char buffer[128];
	const char* s_wid;
	const char* s_hgt;
	char server_buffer[256];
	int p;
	char c;
	const char* s_addr;
	const char* s_port;
	unsigned port;
	unsigned i;

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	sncpy(server_buffer, sizeof(server_buffer), address);

	p = 0;
	s_addr = stoken(&c, &p, server_buffer, ":", " ");
	if (c != 0 && c != ':') {
		log_std(("ERROR:lcd: invalid server specification '%s'\n", address));
		return 0;
	}
	s_port = stoken(&c, &p, server_buffer, "", " ");

	if (*s_addr == 0) {
		s_addr = "127.0.0.1";
	}
	if (*s_port == 0) {
		s_port = "13666";
	}

	port = atoi(s_port);

	host = gethostbyname(s_addr);
	if (!host) {
		log_std(("ERROR:lcd: gethostbyname(%s) failed, with error %d\n", address, h_errno));
		goto err;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr = *(struct in_addr *)host->h_addr;

	context = malloc(sizeof(adv_lcd));
	if (!context)
		goto err;

	context->mute_flag = 0;

	context->f = socket(PF_INET, SOCK_STREAM, 0);
	if (context->f < 0) {
		log_std(("ERROR:lcd: socket(PF_INET, SOCK_STREAM, 0) failed, with error %d, %s\n", errno, strerror(errno)));
		goto err_context;
	}

	r = connect(context->f, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (r < 0) {
		log_std(("ERROR:lcd: connect(%s:%d) failed, with error %d, %s\n", address, port, errno, strerror(errno)));
		goto err_close;
	}

	r = fcntl(context->f, F_SETFL, O_NONBLOCK);
	if (r < 0) {
		log_std(("ERROR:lcd: fcntl(%s:%d) failed, with error %d, %s\n", address, port, errno, strerror(errno)));
		goto err_shutdown;
	}

	r = adv_lcd_sendstr_wait(context, "hello\n", &tv);
	if (r != 0) {
		goto err_shutdown;
	}

	r = adv_lcd_recvstr_wait(context, buffer, sizeof(buffer), &tv);
	if (r != 0) {
		goto err_shutdown;
	}

	log_std(("lcd: server answer '%s'\n", buffer));

	/* example of server answer: */
	/* connect LCDproc 0.4.3 protocol 0.3 lcd wid 20 hgt 4 cellwid 5 cellhgt 8 */

	if (strncmp(buffer, "connect", 7) != 0) {
		log_std(("ERROR:lcd: invalid server answer '%s'\n", buffer));
		goto err_shutdown;
	}

	s_wid = strstr(buffer, " wid ");
	s_hgt = strstr(buffer, " hgt ");
	if (s_wid == 0 || s_hgt == 0) {
		log_std(("ERROR:lcd: invalid server answer '%s'\n", buffer));
		goto err_shutdown;
	}

	context->width = atoi(s_wid + 4);
	context->height = atoi(s_hgt + 4);
	if (context->width < 1 || context->height < 1) {
		log_std(("ERROR:lcd: invalid size in server answer '%s'\n", buffer));
		goto err_shutdown;
	}

	r = adv_lcd_sendstr_wait(context, "screen_add advance\n", &tv);
	if (r != 0) {
		goto err_shutdown;
	}

	for(i=0;i<context->height;++i) {
		char buffer[256];
		snprintf(buffer, sizeof(buffer), "widget_add advance message%d scroller\n", i);
		r = adv_lcd_sendstr_wait(context, buffer, &tv);
		if (r != 0) {
			goto err_shutdown;
		}
	}

	return context;

err_shutdown:
	shutdown(context->f, 2);
err_close:
	close(context->f);
err_context:
	free(context);
err:
	return 0;
}

/**
 * Deinitialize the LCD support.
 * \param context LCD context to use.
 */
void adv_lcd_done(adv_lcd* context)
{
	int r;

	log_std(("lcd: closing connection\n"));

	r = shutdown(context->f, 2);
	if (r < 0) {
		log_std(("ERROR:lcd: shutdown() failed, with error %d, %s\n", errno, strerror(errno)));
		goto err_close;
	}

	r = close(context->f);
	if (r < 0) {
		log_std(("ERROR:lcd: close() failed, with error %d, %s\n", errno, strerror(errno)));
		goto err_free;
	}

	free(context);
	return;

err_close:
	close(context->f);
err_free:
	free(context);
}

/**
 * Display a string on the LCD.
 * The string is displayed with a vertical scrolling it the LCD is too small.
 * \param context LCD context to use.
 * \param text Text to display.
 * \param speed Scrolling speed in 1/8th of seconds.
 */
adv_error adv_lcd_display(adv_lcd* context, unsigned row, const char* text, int speed)
{
	adv_error r;
	char* buffer;
	unsigned size;
	char* s;
	unsigned i;

	if (context->mute_flag) {
		return -1;
	}

	if (speed < -16)
		speed = -16;
	if (speed > 16)
		speed = 16;
	if (speed == 0)
		speed = 1;

	if (row >= context->height) {
		log_std(("WARNING:lcd: output out of screen\n"));
		return -1;
	}

	size = 256 + strlen(text);
	buffer = malloc(size);

	snprintf(buffer, size, "widget_set advance message%d %d %d %d %d h %d \"", row, 1, row+1, context->width, row+1, speed);

	s = buffer + strlen(buffer);
	for(i=0;text[i];++i) {
		if (text[i] != '"' && (text[i]>=' ' && text[i]<='~'))
			*s++ = text[i];
	}
	*s = 0;

	sncat(buffer, size, "\"\n");

	r = adv_lcd_sendstr_noblock(context, buffer);

	free(buffer);

	if (r != 0) {
		log_std(("WARNING:lcd: output disabled\n"));
		context->mute_flag = 1;
		return -1;
	}

	return 0;
}

