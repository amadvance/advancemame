#!/bin/sh
#
echo "Generating build information using autoconf"
echo "This may take a while ..."

# Touch the timestamps on all the files since CVS messes them up
touch configure.ac

# Regenerate configuration files
# Note that we cannot use autoreconf because it won't call automake
# to add missing files, because we don't use automake
aclocal
automake --add-missing --force-missing 2>/dev/null
autoconf
autoheader && touch advance/lib/config.hin

# Run configure for this platform
echo "Now you are ready to run ./configure"
