@echo off

if "%2" == "" goto help

set src=%1
set dst=%2

if not exist readme.txt goto help
if not exist %src%\libvga.h goto help

mkdir %dst%
mkdir %dst%\clockchi
mkdir %dst%\ramdac
mkdir %dst%\drivers

copy readme.txt %dst%
copy svgalib.dif %dst%
copy upgrade.bat %dst%
copy makedif.bat %dst%
copy makedif %dst%
copy svgalib.gdt %dst%
copy svgalib.gpr %dst%
copy libdos.c %dst%
copy libdos.h %dst%

copy %src%\8514a.h %dst%
copy %src%\accel.c %dst%
copy %src%\accel.h %dst%
copy %src%\driver.h %dst%
copy %src%\endianes.h %dst%
copy %src%\interfac.h %dst%
copy %src%\io.h %dst%
copy %src%\libvga.h %dst%
copy %src%\timing.h %dst%
copy %src%\vga.h %dst%
copy %src%\vgaio.c %dst%
copy %src%\vgaio.h %dst%
copy %src%\vgammvga.c %dst%
copy %src%\vgammvga.h %dst%
copy %src%\vgapci.h %dst%
copy %src%\vgaregs.c %dst%
copy %src%\vgaregs.h %dst%
copy %src%\vgarelvg.c %dst%
copy %src%\vgarelvg.h %dst%

copy %src%\drivers\apm.c %dst%\drivers
copy %src%\drivers\ark.c %dst%\drivers
copy %src%\drivers\banshee.c %dst%\drivers
copy %src%\drivers\et6000.c %dst%\drivers
copy %src%\drivers\g400.c %dst%\drivers
copy %src%\drivers\glint_re.h %dst%\drivers
copy %src%\drivers\i740.c %dst%\drivers
copy %src%\drivers\i740_reg.h %dst%\drivers
copy %src%\drivers\i810.c %dst%\drivers
copy %src%\drivers\i810_reg.h %dst%\drivers
copy %src%\drivers\i810_wma.c %dst%\drivers
copy %src%\drivers\laguna.c %dst%\drivers
copy %src%\drivers\lagunaio.h %dst%\drivers
copy %src%\drivers\mga.h %dst%\drivers
copy %src%\drivers\mga_g450.c %dst%\drivers
copy %src%\drivers\millenni.c %dst%\drivers
copy %src%\drivers\mx.c %dst%\drivers
copy %src%\drivers\nv3.c %dst%\drivers
copy %src%\drivers\nv3io.c %dst%\drivers
copy %src%\drivers\nv3ref.h %dst%\drivers
copy %src%\drivers\nvreg.h %dst%\drivers
copy %src%\drivers\pm2.c %dst%\drivers
copy %src%\drivers\pm2io.h %dst%\drivers
copy %src%\drivers\r128.c %dst%\drivers
copy %src%\drivers\r128_reg.h %dst%\drivers
copy %src%\drivers\r128io.h %dst%\drivers
copy %src%\drivers\rage.c %dst%\drivers
copy %src%\drivers\rage.h %dst%\drivers
copy %src%\drivers\rageio.c %dst%\drivers
copy %src%\drivers\renditio.h %dst%\drivers
copy %src%\drivers\renditio.c %dst%\drivers
copy %src%\drivers\s3.c %dst%\drivers
copy %src%\drivers\savage.c %dst%\drivers
copy %src%\drivers\sis.c %dst%\drivers
copy %src%\drivers\trident.c %dst%\drivers
copy %src%\drivers\trident.h %dst%\drivers
copy %src%\drivers\v2kregs.h %dst%\drivers

copy %src%\clockchi\clockchi.h %dst%\clockchi
copy %src%\clockchi\icd2061a.c %dst%\clockchi

copy %src%\ramdac\ibmrgb52.h %dst%\ramdac
copy %src%\ramdac\attdacs.c %dst%\ramdac
copy %src%\ramdac\icw.c %dst%\ramdac
copy %src%\ramdac\normal.c %dst%\ramdac
copy %src%\ramdac\ramdac.c %dst%\ramdac
copy %src%\ramdac\ramdac.h %dst%\ramdac
copy %src%\ramdac\s3dacs.c %dst%\ramdac
copy %src%\ramdac\sierra.c %dst%\ramdac
copy %src%\ramdac\btdacs.c %dst%\ramdac
copy %src%\ramdac\ics_gend.c %dst%\ramdac
copy %src%\ramdac\ibmrgb52.c %dst%\ramdac

set src=
set dst=

goto end
:help
echo Syntax: upgrade ORIDIR NEWDIR
echo Example: upgrade ..\..\..\svgalib ..\svgalib
goto end
:end
