#!/bin/sh
#
echo "Generating build information using autoconf"
echo "This may take a while ..."

# Touch the timestamps on all the files since CVS messes them up
touch configure.ac

# Regenerate configuration files
aclocal
autoconf
autoheader && touch advance/lib/config.hin

# Run configure for this platform
echo "Now you are ready to run ./configure"
