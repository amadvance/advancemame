#!/bin/bash

SRC=$1
DST=.

if [[ ! $1 ]] ; then
	echo "Syntax: upgrade SVGALIBDIR"
	echo "Example: upgrade ../../../svgalib"
	exit 1
fi

if [[ ! -e readme.txt ]] ; then
	echo Wrong current dir
	exit 1
fi

if [[ ! -e $SRC/libvga.h ]] ; then
	echo Wrong source dir
	exit 1
fi

cp $SRC/8514a.h $DST
cp $SRC/accel.c $DST
cp $SRC/accel.h $DST
cp $SRC/driver.h $DST/svgadriv.h
cp $SRC/endianes.h $DST
cp $SRC/interfac.h $DST
cp $SRC/io.h $DST/memio.h
cp $SRC/libvga.h $DST
cp $SRC/timing.h $DST
cp $SRC/vga.h $DST
cp $SRC/vgaio.c $DST
cp $SRC/vgaio.h $DST
cp $SRC/vgammvga.c $DST
cp $SRC/vgammvga.h $DST
cp $SRC/vgapci.h $DST
cp $SRC/vgaregs.c $DST
cp $SRC/vgaregs.h $DST
cp $SRC/vgarelvg.c $DST
cp $SRC/vgarelvg.h $DST

cp $SRC/drivers/apm.c $DST/drivers
cp $SRC/drivers/ark.c $DST/drivers
cp $SRC/drivers/banshee.c $DST/drivers
cp $SRC/drivers/et6000.c $DST/drivers
cp $SRC/drivers/g400.c $DST/drivers
cp $SRC/drivers/glint_re.h $DST/drivers
cp $SRC/drivers/i740.c $DST/drivers
cp $SRC/drivers/i740_reg.h $DST/drivers
cp $SRC/drivers/i810.c $DST/drivers
cp $SRC/drivers/i810_reg.h $DST/drivers
cp $SRC/drivers/i810_wma.c $DST/drivers
cp $SRC/drivers/laguna.c $DST/drivers
cp $SRC/drivers/lagunaio.h $DST/drivers
cp $SRC/drivers/mga.h $DST/drivers
cp $SRC/drivers/mga_g450.c $DST/drivers
cp $SRC/drivers/millenni.c $DST/drivers
cp $SRC/drivers/mx.c $DST/drivers
cp $SRC/drivers/nv3.c $DST/drivers
cp $SRC/drivers/nv3io.c $DST/drivers
cp $SRC/drivers/nv3ref.h $DST/drivers
cp $SRC/drivers/nvreg.h $DST/drivers
cp $SRC/drivers/pm2.c $DST/drivers
cp $SRC/drivers/pm2io.h $DST/drivers
cp $SRC/drivers/r128.c $DST/drivers
cp $SRC/drivers/r128_reg.h $DST/drivers
cp $SRC/drivers/r128io.h $DST/drivers
cp $SRC/drivers/rage.c $DST/drivers
cp $SRC/drivers/rage.h $DST/drivers
cp $SRC/drivers/rageio.c $DST/drivers
cp $SRC/drivers/renditio.h $DST/drivers
cp $SRC/drivers/renditio.c $DST/drivers
cp $SRC/drivers/s3.c $DST/drivers
cp $SRC/drivers/savage.c $DST/drivers
cp $SRC/drivers/sis.c $DST/drivers
cp $SRC/drivers/trident.c $DST/drivers
cp $SRC/drivers/trident.h $DST/drivers
cp $SRC/drivers/v2kregs.h $DST/drivers

cp $SRC/clockchi/clockchi.h $DST/clockchi
cp $SRC/clockchi/icd2061a.c $DST/clockchi

cp $SRC/ramdac/ibmrgb52.h $DST/ramdac
cp $SRC/ramdac/attdacs.c $DST/ramdac
cp $SRC/ramdac/icw.c $DST/ramdac
cp $SRC/ramdac/normal.c $DST/ramdac
cp $SRC/ramdac/ramdac.c $DST/ramdac
cp $SRC/ramdac/ramdac.h $DST/ramdac
cp $SRC/ramdac/s3dacs.c $DST/ramdac
cp $SRC/ramdac/sierra.c $DST/ramdac
cp $SRC/ramdac/btdacs.c $DST/ramdac
cp $SRC/ramdac/ics_gend.c $DST/ramdac
cp $SRC/ramdac/ibmrgb52.c $DST/ramdac


