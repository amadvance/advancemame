#!/bin/bash

SRC=$1
DST=.

if [[ ! $1 ]] ; then
	echo "Syntax: upgrade SVGALIBDIR"
	echo "Example: upgrade ../svgalib.ori"
	exit 1
fi

if [[ ! -e $SRC/libvga.h ]] ; then
	echo Wrong source dir
	exit 1
fi

mkdir $DST/ramdac
mkdir $DST/clockchi
mkdir $DST/drivers

cp $SRC/8514a.h $DST
cp $SRC/accel.c $DST
cp $SRC/accel.h $DST
cp $SRC/driver.h $DST/svgadriv.h
cp $SRC/endianess.h $DST/endianes.h
cp $SRC/interface.h $DST/interfac.h
cp $SRC/io.h $DST/memio.h
cp $SRC/libvga.h $DST
cp $SRC/timing.h $DST
cp $SRC/vga.h $DST
cp $SRC/vgaio.c $DST
cp $SRC/vgaio.h $DST
cp $SRC/vgammvgaio.c $DST/vgammvga.c
cp $SRC/vgammvgaio.h $DST/vgammvga.h
cp $SRC/vgapci.h $DST
cp $SRC/vgaregs.c $DST
cp $SRC/vgaregs.h $DST
cp $SRC/vgarelvgaio.c $DST/vgarelvg.c
cp $SRC/vgarelvgaio.h $DST/vgarelvg.h

cp $SRC/drivers/apm.c $DST/drivers
cp $SRC/drivers/ark.c $DST/drivers
cp $SRC/drivers/banshee.c $DST/drivers
cp $SRC/drivers/et6000.c $DST/drivers
cp $SRC/drivers/g400.c $DST/drivers
cp $SRC/drivers/glint_regs.h $DST/drivers/glint_re.h
cp $SRC/drivers/i740.c $DST/drivers
cp $SRC/drivers/i740_reg.h $DST/drivers
cp $SRC/drivers/i810.c $DST/drivers
cp $SRC/drivers/i810_reg.h $DST/drivers
cp $SRC/drivers/i810_wmark.c $DST/drivers/i810_wma.c
cp $SRC/drivers/laguna.c $DST/drivers
cp $SRC/drivers/lagunaio.h $DST/drivers
cp $SRC/drivers/mga.h $DST/drivers
cp $SRC/drivers/mga_g450pll.c $DST/drivers/mga_g450.c
cp $SRC/drivers/millennium.c $DST/drivers/millenni.c
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
cp $SRC/drivers/renditionio.h $DST/drivers/renditio.h
cp $SRC/drivers/rendition.c $DST/drivers/renditio.c
cp $SRC/drivers/s3.c $DST/drivers
cp $SRC/drivers/savage.c $DST/drivers
cp $SRC/drivers/sis.c $DST/drivers
cp $SRC/drivers/trident.c $DST/drivers
cp $SRC/drivers/trident.h $DST/drivers
cp $SRC/drivers/v2kregs.h $DST/drivers

cp $SRC/clockchip/clockchip.h $DST/clockchi/clockchi.h
cp $SRC/clockchip/icd2061a.c $DST/clockchi

cp $SRC/ramdac/attdacs.c $DST/ramdac
cp $SRC/ramdac/btdacs.c $DST/ramdac
cp $SRC/ramdac/IBMRGB52x.c $DST/ramdac/ibmrgb52.c
cp $SRC/ramdac/IBMRGB52x.h $DST/ramdac/ibmrgb52.h
cp $SRC/ramdac/icw.c $DST/ramdac
cp $SRC/ramdac/ics_gendac.c $DST/ramdac/ics_gend.c
cp $SRC/ramdac/normal.c $DST/ramdac
cp $SRC/ramdac/ramdac.c $DST/ramdac
cp $SRC/ramdac/ramdac.h $DST/ramdac
cp $SRC/ramdac/s3dacs.c $DST/ramdac
cp $SRC/ramdac/sierra.c $DST/ramdac


