#!/bin/sh

find advance -type f \( -name '*.c' -o -name '*.h' \) | uncrustify -c uncrustify.cfg --replace --no-backup -F -
find mame -type f \( -name '*.c' -o -name '*.h' \) | uncrustify -c uncrustify.cfg --replace --no-backup -F -
find mess -type f \( -name '*.c' -o -name '*.h' \) | uncrustify -c uncrustify.cfg --replace --no-backup -F -
