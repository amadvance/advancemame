#!/bin/sh
#

# Reconfigure (with force) to get the latest revision from git
autoreconf -f

if ! ./configure; then
	exit 1
fi

if ! make whole wholedist -j4 ; then
	exit 1
fi

