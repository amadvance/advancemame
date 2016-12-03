#!/bin/sh
#

# Reconfigure (with force) to get the latest revision from git
autoreconf -f

if ! ./configure ; then
	exit 1
fi

if ! make wholedist; then
	exit 1
fi

if ! make deb; then
	exit 1
fi

