#!/bin/sh
#
echo "Generating build information using aclocal, automake and autoconf"
echo "This may take a while ..."

# Touch the timestamps on all the files since CVS messes them up
touch configure.ac

# Regenerate configuration files
aclocal
automake --foreign --add-missing --copy
autoconf

# Run configure for this platform
echo "Now you are ready to run ./configure"
