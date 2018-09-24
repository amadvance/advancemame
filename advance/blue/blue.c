/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2018 Andrea Mazzoleni
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
 */

#include "portable.h"

size_t snprintfcat(char* buf, size_t size, char const* fmt, ...)
{
	size_t result;
	va_list args;

	size_t len = strnlen(buf, size);

	va_start(args, fmt);
	result = vsnprintf(buf + len, size - len, fmt, args);
	va_end(args);

	return result + len;
}

int opt_foreground; /** Run in foreground */
int opt_stat; /** Print stats */
int opt_msg; /** Save message */

/**
 * Backlog
 */
#define BACKLOG_LINES 16
#define BACKLOG_MAX 128

char backlog[BACKLOG_LINES][BACKLOG_MAX];
int backlog_begin;
int backlog_end;

void backlog_insert(const char* line)
{
	int last = backlog_end;

	backlog_end = (backlog_end + 1) % BACKLOG_LINES;

	if (backlog_end == backlog_begin)
		backlog_begin = (backlog_begin + 1) % BACKLOG_LINES;

	snprintf(backlog[last], BACKLOG_MAX, "%s", line);
}

void backlog_print(void)
{
	int i;

	printf("---\n");

	for (i = backlog_begin; i != backlog_end; i = (i + 1) % BACKLOG_LINES) {
		printf("%s\n", backlog[i]);
	}
}

/**
 * Output buffer
 */
#define OUTPUT_MAX 8192
static char output_base[OUTPUT_MAX];
char* output_begin = output_base;
char* output_end = output_base;

/**
 * Receive output data
 */
void blue_recv(int f)
{
	ssize_t ret;

	ret = fcntl(f, F_SETFL, O_NONBLOCK);
	if (ret != 0) {
		fprintf(stderr, "Failed to set not blocking\n");
		exit(EXIT_FAILURE);
	}

	ret = read(f, output_end, OUTPUT_MAX - (output_end - output_base));
	if (ret == -1 && errno == EAGAIN) {
		/* nothing more to read */
		return;
	} else if (ret > 0) {
		output_end += ret;
	} else {
		fprintf(stderr, "Failed to read %d\n", (int)ret);
		exit(EXIT_FAILURE);
	}
}

/**
 * Get one line of output
 */
char* blue_line(int f)
{
	/* move the data at the begin */
	if (output_begin != output_base) {
		memmove(output_base, output_begin, output_end - output_begin);
		output_end -= (output_begin - output_base);
		output_begin = output_base;
	}

	do {
		char* sep;

		/* get new data */
		blue_recv(f);

		/* check if we have a full line */
		sep = output_begin;
		while (sep < output_end && *sep != '\n')
			++sep;
		if (sep < output_end) {
			/* return the line */
			char* line;
			line = output_begin;
			*sep = 0;
			output_begin = sep + 1;
			backlog_insert(line);
			return line;
		}

		/* check if we have the prompt */
		if (output_begin + 2 < output_end && output_end[-1] == ' ' && output_end[-2] == '#') {
			/* no more input */
			output_end[-2] = '@'; /* avoid double detection */
			return 0;
		}
	} while (1);
}

/**
 * Discard all the output data
 */
void blue_discard(int f)
{
	char* line = blue_line(f);
	while (line)
		line = blue_line(f);
}

/**
 * Send input data
 */
void blue_send(int f, const char* command)
{
	ssize_t ret;
	size_t len;

	len = strlen(command);

	ret = write(f, command, len);

	if (ret != len) {
		fprintf(stderr, "Failed to write command\n");
		exit(EXIT_FAILURE);
	}

	/* give some time to process the command */
	usleep(500);
}

struct blue_device {
	char* id;
	char name[128];
	int is_game;
	int is_paired;
	int is_trusted;
	int is_connected;
	int need_connect;

	struct blue_device* next;
};

struct blue_device* devs;

struct blue_device* dev_find(const char* id)
{
	struct blue_device* i = devs;

	while (i != 0) {
		if (strcmp(id, i->id) == 0)
			return i;
		i = i->next;
	}

	/* if not found, allocate */
	i = calloc(1, sizeof(struct blue_device));

	i->id = strdup(id);
	i->next = devs;
	devs = i;

	return i;
}

/**
 * Process a device line
 *
 * Like: 'Device E4:17:D8:B8:0B:7E 8Bitdo SFC30 GamePad'
 */
void process_device_line(char* line)
{
	char* id;
	int i;

	if (strncmp(line, "Device ", 7) != 0)
		return;
	line += 7;

	id = line;
	for (i = 0; i < 6; ++i) {
		if (!isxdigit(line[0]) || !isxdigit(line[1]))
			return;
		line += 2;
		if (i < 5) {
			if (line[0] != ':')
				return;
			++line;
		}
	}
	*line = 0;

	/* allocate if missing */
	dev_find(id);
}

/**
 * List all devices
 */
void process_list(int in_f, int out_f)
{
	char* line;

	blue_send(in_f, "devices\n");

	line = blue_line(out_f);
	while (line) {
		process_device_line(line);
		line = blue_line(out_f);
	}
}

/**
 * Process a info line
 *
 * Like:
 * Name: 8Bitdo SFC30 GamePad
 * Alias: 8Bitdo SFC30 GamePad
 * Class: 0x002508
 * Icon: input-gaming
 * Paired: yes
 * Trusted: yes
 * Blocked: no
 * Connected: no
 * LegacyPairing: no
 * UUID: Human Interface Device... (00001124-0000-1000-8000-00805f9b34fb)
 * UUID: PnP Information           (00001200-0000-1000-8000-00805f9b34fb)
 * Modalias: usb:v2DC8p2830d0100
 */
void process_info_line(struct blue_device* dev, const char* line)
{
	char* name;

	name = strstr(line, "Name:");
	if (name) {
		name += 5;
		while (*name != 0 && isspace(*name))
			++name;
		snprintf(dev->name, sizeof(dev->name), name);
	}

	if (strstr(line, "Icon: input-gaming")) {
		dev->is_game = 1;
	}

	if (strstr(line, "Paired: yes")) {
		dev->is_paired = 1;
	}

	if (strstr(line, "Paired: no")) {
		dev->is_paired = 0;
	}

	if (strstr(line, "Trusted: yes")) {
		dev->is_trusted = 1;
	}

	if (strstr(line, "Trusted: no")) {
		dev->is_trusted = 0;
	}

	if (strstr(line, "Connected: yes")) {
		dev->is_connected = 1;
	}

	if (strstr(line, "Connected: no")) {
		dev->is_connected = 0;
	}
}

/**
 * Get info or all devices
 */
void process_info(int in_f, int out_f)
{
	struct blue_device* i = devs;

	while (i != 0) {
		char cmd[128];
		char* line;

		snprintf(cmd, sizeof(cmd), "info %s\n", i->id);

		blue_send(in_f, cmd);

		line = blue_line(out_f);
		while (line) {
			process_info_line(i, line);
			line = blue_line(out_f);
		}

		i = i->next;
	}
}

/**
 * Connect all devices
 */
void process_connect(int in_f, int out_f)
{
	struct blue_device* i = devs;
	char processing[4096];
	char connected[4096];
	unsigned count_processing;
	unsigned count_connected;

	count_processing = 0;
	count_connected = 0;
	processing[0] = 0;
	connected[0] = 0;

	for (i = devs; i != 0; i = i->next) {
		char cmd[128];
		const char* name = i->name[0] ? i->name : i->id;

		if (!i->is_game)
			continue;

		if (!i->is_trusted) {
			snprintf(cmd, sizeof(cmd), "trust %s\n", i->id);
			blue_send(in_f, cmd);
			blue_discard(out_f);
			snprintfcat(processing, sizeof(processing) - 1, "Trusting %s...\n", name);
			++count_processing;
			continue;
		}

		if (!i->is_paired) {
			/*
			 * If the device is paired now, we need a manual connect
			 * If instead the device was already paired, the bluetooth daemon
			 * will connect it automatically.
			 */
			i->need_connect = 1;

			snprintf(cmd, sizeof(cmd), "pair %s\n", i->id);
			blue_send(in_f, cmd);
			blue_discard(out_f);
			snprintfcat(processing, sizeof(processing) - 1, "Pairing %s...\n", name);
			++count_processing;
			continue;
		}

		if (!i->is_connected && i->need_connect) {
			snprintf(cmd, sizeof(cmd), "connect %s\n", i->id);
			blue_send(in_f, cmd);
			blue_discard(out_f);
			snprintfcat(processing, sizeof(processing), "Connecting %s...\n", name);
			++count_processing;
			continue;
		}

		if (i->is_connected) {
			snprintfcat(connected, sizeof(connected), "Connected %s", name);
			++count_connected;
		}
	}

	/* ensure to have the final terminator */
	processing[sizeof(processing) - 1] = 0;
	connected[sizeof(connected) - 1] = 0;

	if (count_processing != 0) {
		if (count_connected == 1)
			snprintfcat(processing, sizeof(processing), "%s", connected);
		else if (count_connected > 1)
			snprintfcat(processing, sizeof(processing), "Connected %u devices", count_connected);
	} else {
		if (count_connected == 0)
			strcpy(processing, "Press START for one second to power on");
		else if (count_connected > 1)
			snprintf(processing, sizeof(processing), "Connected %u devices", count_connected);
		else
			strcpy(processing, connected);
	}

	if (opt_msg) {
		int f = open("/tmp/blue.msg", O_CREAT | O_TRUNC | O_WRONLY, 0644);
		if (f >= 0) {
			write(f, processing, strlen(processing));
			close(f);
		}
	}
}

void print_list(void)
{
	struct blue_device* i = devs;

	printf("\nDevices:\n");

	for (i = devs; i != 0; i = i->next) {
		printf("\t\"%s\" %s %s %s %s %s %s\n", i->name, i->id,
			i->is_game ? "game" : "",
			i->is_trusted ? "trusted" : "",
			i->is_paired ? "paired" : "",
			i->is_connected ? "connected" : "",
			i->need_connect ? "[need_connect]" : "[auto_connect]"
		);
	}
}

void process(int in_f, int out_f)
{
	blue_discard(out_f);

	blue_send(in_f, "power on\n");

	blue_discard(out_f);

	blue_send(in_f, "agent NoInputNoOutput\n");

	blue_discard(out_f);

	blue_send(in_f, "default-agent\n");

	blue_discard(out_f);

	blue_send(in_f, "scan on\n");

	blue_discard(out_f);

	while (1) {
		sleep(1);

		process_list(in_f, out_f);

		process_info(in_f, out_f);

		process_connect(in_f, out_f);

		if (opt_stat) {
			backlog_print();
			print_list();
		}
	}
}

int start(int* in_f, int* out_f)
{
	int p1[2];
	int p2[2];
	int pid;

	if (pipe(p1) == -1) {
		fprintf(stderr, "Failed to create pipes\n");
		exit(EXIT_FAILURE);
	}

	if (pipe(p2) == -1) {
		fprintf(stderr, "Failed to create pipes\n");
		exit(EXIT_FAILURE);
	}

	pid = fork();
	if (pid == -1) {
		fprintf(stderr, "Failed to fork\n");
		exit(EXIT_FAILURE);
	}

	if (pid) {
		*in_f = p1[1];
		*out_f = p2[0];

		close(p1[0]);
		close(p2[1]);
		return 0;
	} else {
		dup2(p1[0], 0);
		dup2(p2[1], 1);
		close(p1[0]);
		close(p1[1]);
		close(p2[0]);
		close(p2[1]);

		execl("/usr/bin/bluetoothctl", "/usr/bin/bluetoothctl", NULL);

		fprintf(stderr, "Failed to run bluetoothctl\n");
		exit(EXIT_FAILURE);
	}
}

void blue_run(void)
{
	int in_f;
	int out_f;

	start(&in_f, &out_f);

	process(in_f, out_f);
}

void blue_daemon(void)
{
	pid_t pid, sid;
	int in_f;
	int out_f;

	/* fork parent */
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Failed to fork daemon\n");
		exit(EXIT_FAILURE);
	}

	/* exit parent */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	umask(0);

	/* child sid */
	sid = setsid();
	if (sid < 0) {
		fprintf(stderr, "Failed to set sid\n");
		exit(EXIT_FAILURE);
	}

	if ((chdir("/")) < 0) {
		fprintf(stderr, "Failed to change dir\n");
		exit(EXIT_FAILURE);
	}

	start(&in_f, &out_f);

	close(0);
	close(1);
	close(2);

	process(in_f, out_f);
}

int main(int argc, char* argv[])
{
	int i;

	opt_msg = 1;

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-f") == 0) {
			opt_foreground = 1;
			opt_stat = 1;
		} else {
			printf("Syntax: advblue [-f]\n");
			exit(EXIT_FAILURE);
		}
	}

	if (opt_foreground)
		blue_run();
	else
		blue_daemon();

	return 0;
}

