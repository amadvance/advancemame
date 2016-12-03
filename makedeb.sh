#!/bin/sh
#

# Reconfigure (with force) to get the latest revision from git
autoreconf -f

if ! ./configure CFLAGS="-O2 -fno-strict-aliasing -fno-strict-overflow -fsigned-char"; then
	exit 1
fi

if ! make deb -j3 ; then
	exit 1
fi

