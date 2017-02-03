#!/bin/sh
#

# Reconfigure (with force) to get the latest revision from git
autoreconf -f

if grep ID=raspbian /etc/os-release >/dev/null 2>&1; then
	if ! ./configure CFLAGS="-O2 -fno-strict-aliasing -fno-strict-overflow -fsigned-char" --disable-sdl --disable-sdl2; then
		exit 1
	fi
else
	# Build for a generic architecture not using -march
	if ! ./configure CFLAGS="-O2 -fno-strict-aliasing -fno-strict-overflow -fsigned-char"; then
		exit 1
	fi
fi

if ! make deb -j3 ; then
	exit 1
fi

