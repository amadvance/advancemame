#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p /tmp/advancemame
work=$(mktemp -d /tmp/advancemame/dspp-test.XXXXXX)
trap 'rm -rf -- "$work"' EXIT

cd "$root"
"${CC:-cc}" -std=gnu99 -O2 '-DINLINE=static inline' -DSLJIT_DEBUG=1 \
	-I. -Imame -Iadvance/osd -Ialien/sljit -Ialien/libchdr/include \
	-ffunction-sections -fdata-sections -Wl,--gc-sections -pthread \
	support/test_konamim2_dspp.c alien/sljit/sljit_mame.c -o "$work/test"
"$work/test"
