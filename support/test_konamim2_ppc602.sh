#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p /tmp/advancemame
work=$(mktemp -d /tmp/advancemame/ppc602-test.XXXXXX)
trap 'rm -rf -- "$work"' EXIT

cd "$root"
"${CC:-cc}" -std=gnu99 -O2 '-DINLINE=static inline' -DHAS_PPC602=1 -DSLJIT_DEBUG=1 \
	-I. -Imame -Iadvance/osd -Ialien/sljit -ffunction-sections -fdata-sections -Wl,--gc-sections -pthread \
	support/test_konamim2_ppc602.c alien/sljit/sljit_mame.c -o "$work/test"
"$work/test"
