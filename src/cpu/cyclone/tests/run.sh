#!/bin/sh
# Run from the repository root with a 32-bit little-endian ARM compiler.
# Example: CC=arm-linux-gnueabi-gcc RUNNER=qemu-arm sh src/cpu/cyclone/tests/run.sh
set -eu
mkdir -p /tmp/advancemame
work=$(mktemp -d /tmp/advancemame/cyclone-test.XXXXXX)
trap 'rm -rf "$work"' EXIT HUP INT TERM
cc=${CC:-cc}
stats=-DCYCLONE_TEST_STATS
if test "${TEST_ARGS:-}" = --bench; then
	stats=
fi
${BUILD_CC:-cc} -O2 src/cpu/m68000/m68kmake.c -o "$work/m68kmake"
"$work/m68kmake" "$work" src/cpu/m68000/m68k_in.c > "$work/generate.log"
for source in src/cpu/cyclone/tests/adapter.c src/cpu/cyclone/cyclone_imp.c \
	src/cpu/m68000/m68kcpu.c "$work/m68kops.c" "$work/m68kopac.c" \
	"$work/m68kopdm.c" "$work/m68kopnz.c"; do
	name=$(basename "$source" .c)
	$cc ${CFLAGS:-} -marm -O2 -fno-strict-aliasing -ffunction-sections -fdata-sections \
		-UNDEBUG -DMAME -DUSE_CYCLONE $stats -DLSB_FIRST '-DINLINE=static __inline__' \
		-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -Dasm=__asm__ \
		-Isrc -Iadvance/osd -Isrc/cpu/m68000 -I"$work" \
		-c "$source" -o "$work/$name.o"
done
$cc ${CFLAGS:-} -marm -c src/cpu/cyclone/cyclone.s -o "$work/core.o"
$cc ${LDFLAGS:--static} -Wl,--gc-sections "$work/"*.o -lm -o "$work/test"
${RUNNER:-} "$work/test" ${TEST_ARGS:-}
