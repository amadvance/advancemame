#!/bin/sh
#

# Reconfigure (with force) to get the latest revision from git
autoreconf -f

if ! ./configure; then
	exit 1
fi

if ! make whole -j4 ; then
	exit 1
fi

if ! make wholedist ; then
	exit 1
fi

