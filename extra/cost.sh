#!/bin/bash

EMU=advancemame-0.78.0-diff
MENU=advancemenu-2.2.17
CAB=advancecab-1.1.4
SCAN=advancescan-1.8
COMP=advancecomp-1.9

rm -r -f cost.tmp
mkdir cost.tmp

(cd cost.tmp && tar zxf ../../$EMU.tar.gz)
(cd cost.tmp && tar zxf ../../$MENU.tar.gz)
(cd cost.tmp && tar zxf ../../$CAB.tar.gz)
(cd cost.tmp && tar zxf ../../../advscan/$SCAN.tar.gz)
(cd cost.tmp && tar zxf ../../../advcomp/$COMP.tar.gz)

for FILE in \
	configure config.guess config.sub missing install-sh mkinstalldirs aclocal.m4 \
	contrib \
	advance/svgalib/8514a.h advance/svgalib/accel.c advance/svgalib/accel.h \
	advance/svgalib/driver.h advance/svgalib/endianes.h advance/svgalib/interfac.h \
	advance/svgalib/io.h advance/svgalib/libvga.h advance/svgalib/timing.h \
	advance/svgalib/vga.h advance/svgalib/vgaio.c advance/svgalib/vgaio.h \
	advance/svgalib/vgammvga.c advance/svgalib/vgammvga.h advance/svgalib/vgapci.h \
	advance/svgalib/vgaregs.c advance/svgalib/vgaregs.h advance/svgalib/vgarelvg.c \
	advance/svgalib/vgarelvg.h advance/svgalib/svgadriv.h advance/svgalib/memio.h \
	advance/svgalib/drivers advance/svgalib/clockchi advance/svgalib/ramdac \
	advance/mpglib \
	advance/expat \
	advance/osd/y_tab.c advance/osd/lexyy.c \
	; do
	rm -f -r cost.tmp/$EMU/$FILE
	rm -f -r cost.tmp/$MENU/$FILE
	rm -f -r cost.tmp/$CAB/$FILE
done

for FILE in \
	advance/lib advance/linux advance/sdl advance/dos advance/windows \
	advance/svgalib advance/blit advance/card advance/d2 advance/v \
	advance/cfg \
	; do
	rm -f -r cost.tmp/$MENU/$FILE
	rm -f -r cost.tmp/$CAB/$FILE
done

for FILE in \
	configure install-sh mkinstalldirs depcomp missing config.guess config.sub aclocal.m4 Makefile.in config.h.in \
	7z expat lib \
	; do
	rm -f -r cost.tmp/$SCAN/$FILE
	rm -f -r cost.tmp/$COMP/$FILE
done

sloccount --effort 1.47 1.05 cost.tmp > cost/cost.txt

mkdir cost.tmp/advance
mkdir cost.tmp/advance/scan
mkdir cost.tmp/advance/comp
cp -a cost.tmp/$EMU/advance/* cost.tmp/advance
cp -a cost.tmp/$MENU/advance/* cost.tmp/advance
cp -a cost.tmp/$CAB/advance/* cost.tmp/advance
cp -a cost.tmp/$SCAN/* cost.tmp/advance/scan
cp -a cost.tmp/$COMP/* cost.tmp/advance/comp

sloccount --effort 1.47 1.05 cost.tmp/advance > cost/cost-detailed.txt

