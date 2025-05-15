#!/bin/sh
#

# Reconfigure (with force) to get the latest revision from git
autoreconf -f

# Build for a generic architecture not using -march
if ! ./configure CFLAGS="-O2 -fno-strict-aliasing -fno-strict-overflow -fsigned-char"; then
	exit 1
fi

if ! make CONF_ARCH=deb deb -j4 ; then
	exit 1
fi
