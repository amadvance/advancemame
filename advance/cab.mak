############################################################################
# CAB dist

CABVERSION = 1.1.4

TSR_SRC = \
	$(wildcard $(srcdir)/advance/tsr/*.asm) \
	$(wildcard $(srcdir)/advance/tsr/*.c) \
	$(wildcard $(srcdir)/advance/tsr/*.h)

DOS16_SRC = \
	$(wildcard $(srcdir)/advance/dos/pci.*) \
	$(wildcard $(srcdir)/advance/dos/map.*) \
	$(wildcard $(srcdir)/advance/dos/compil.h)

VBE_SRC = \
	$(srcdir)/advance/vbe/makefile \
	$(wildcard $(srcdir)/advance/vbe/*.asm) \
	$(wildcard $(srcdir)/advance/vbe/*.c) \
	$(wildcard $(srcdir)/advance/vbe/*.h)

VBE32_SRC = \
	$(srcdir)/advance/vbe32/makefile \
	$(wildcard $(srcdir)/advance/vbe32/*.c) \
	$(wildcard $(srcdir)/advance/vbe32/*.h)

VBEW_SRC = \
	$(srcdir)/advance/vbew/makefile \
	$(wildcard $(srcdir)/advance/vbew/*.c) \
	$(wildcard $(srcdir)/advance/vbew/*.h)

VGA_SRC = \
	$(srcdir)/advance/vga/makefile \
	$(wildcard $(srcdir)/advance/vga/*.asm) \
	$(wildcard $(srcdir)/advance/vga/*.c) \
	$(wildcard $(srcdir)/advance/vga/*.h)

OFF_SRC = \
	$(srcdir)/advance/off/makefile \
	$(wildcard $(srcdir)/advance/off/*.c) \
	$(wildcard $(srcdir)/advance/off/*.h)

PORTIO_SRC = \
	$(srcdir)/advance/portio/makefile \
	$(wildcard $(srcdir)/advance/portio/*.c) \
	$(wildcard $(srcdir)/advance/portio/*.h)

VIDEO_SRC = \
	$(srcdir)/advance/video/makefile \
	$(wildcard $(srcdir)/advance/video/*.c) \
	$(wildcard $(srcdir)/advance/video/*.h)

VIDEOWIN_SRC = \
	$(srcdir)/advance/videow/makefile \
	$(wildcard $(srcdir)/advance/videow/*.c) \
	$(wildcard $(srcdir)/advance/videow/*.h)

CAB_ROOT_SRC = \
	$(srcdir)/COPYING \
	$(CONF_SRC)

CAB_ADVANCE_SRC = \
	$(srcdir)/advance/advance.mak \
	$(srcdir)/advance/cab.mak \
	$(srcdir)/advance/d2.mak

CAB_DOC_SRC = \
	$(srcdir)/doc/advdev.d \
	$(srcdir)/doc/vbe.d \
	$(srcdir)/doc/vbe32.d \
	$(srcdir)/doc/vga.d \
	$(srcdir)/doc/video.d \
	$(srcdir)/doc/off.d \
	$(srcdir)/doc/portio.d \
	$(srcdir)/doc/videow.d \
	$(srcdir)/doc/svgawin.d \
	$(srcdir)/doc/carddos.d \
	$(srcdir)/doc/cardwin.d \
	$(srcdir)/doc/histcab.d \
	$(srcdir)/doc/readcab.d \
	$(srcdir)/doc/relecab.d

CAB_DOC_BIN += \
	$(DOCOBJ)/advdev.txt \
	$(DOCOBJ)/histcab.txt \
	$(DOCOBJ)/readcab.txt \
	$(DOCOBJ)/relecab.txt \
	$(DOCOBJ)/advv.txt \
	$(DOCOBJ)/advdev.html \
	$(DOCOBJ)/histcab.html \
	$(DOCOBJ)/readcab.html \
	$(DOCOBJ)/relecab.html \
	$(DOCOBJ)/advv.html
ifeq ($(CONF_SYSTEM),dos)
CAB_DOC_BIN += \
	$(DOCOBJ)/vbe.txt \
	$(DOCOBJ)/vbe32.txt \
	$(DOCOBJ)/vga.txt \
	$(DOCOBJ)/video.txt \
	$(DOCOBJ)/off.txt \
	$(DOCOBJ)/portio.txt \
	$(DOCOBJ)/carddos.txt \
	$(DOCOBJ)/vbe.html \
	$(DOCOBJ)/vbe32.html \
	$(DOCOBJ)/vga.html \
	$(DOCOBJ)/video.html \
	$(DOCOBJ)/off.html \
	$(DOCOBJ)/portio.html \
	$(DOCOBJ)/carddos.html
endif
ifeq ($(CONF_SYSTEM),windows)
CAB_DOC_BIN += \
	$(DOCOBJ)/videow.txt \
	$(DOCOBJ)/svgawin.txt \
	$(DOCOBJ)/cardwin.txt \
	$(DOCOBJ)/videow.html \
	$(DOCOBJ)/svgawin.html \
	$(DOCOBJ)/cardwin.html
endif

CAB_SUPPORT_SRC = \
	$(RCSRC) \
	$(srcdir)/support/video.pcx \
	$(srcdir)/support/videobis.pcx \
	$(srcdir)/support/vbev.bat \
	$(srcdir)/support/vgav.bat \
	$(srcdir)/support/videowv.bat

CAB_CONTRIB_SRC = \
	$(wildcard contrib/cab/*)

CAB_ROOT_BIN += \
	$(RCSRC)
ifeq ($(CONF_SYSTEM),dos)
CAB_ROOT_BIN += \
	$(srcdir)/advance/vbe/vbe.com \
	$(srcdir)/advance/vbe32/vbe32.exe \
	$(srcdir)/advance/vga/vga.exe \
	$(srcdir)/advance/video/video.exe \
	$(srcdir)/advance/off/off.com \
	$(srcdir)/advance/portio/portio.exe \
	$(srcdir)/support/vbev.bat \
	$(srcdir)/support/vgav.bat \
	$(srcdir)/support/cwsdpmi.exe \
	$(VOBJ)/advv$(EXE) \
	$(srcdir)/support/video.pcx \
	$(srcdir)/support/videobis.pcx
endif
ifeq ($(CONF_SYSTEM),windows)
CAB_ROOT_BIN += \
	$(srcdir)/support/sdl.dll \
	$(srcdir)/support/videowv.bat \
	$(VOBJ)/advv$(EXE) \
	$(srcdir)/advance/videow/videow.exe \
	$(srcdir)/advance/svgalib/svgawin/driver/svgawin.sys \
	$(srcdir)/advance/svgalib/svgawin/install/svgawin.exe
endif

CAB_DIST_FILE_SRC = advancecab-$(CABVERSION)
CAB_DIST_FILE_BIN = advancecab-$(CABVERSION)-$(BINARYTAG)
CAB_DIST_DIR_SRC = $(CAB_DIST_FILE_SRC)
CAB_DIST_DIR_BIN = $(CAB_DIST_FILE_BIN)

distcab: $(RCSRC)
	mkdir $(CAB_DIST_DIR_SRC)
	cp $(CAB_ROOT_SRC) $(CAB_DIST_DIR_SRC)
	mkdir $(CAB_DIST_DIR_SRC)/doc
	cp $(CAB_DOC_SRC) $(CAB_DIST_DIR_SRC)/doc
	mkdir $(CAB_DIST_DIR_SRC)/support
	cp $(CAB_SUPPORT_SRC) $(CAB_DIST_DIR_SRC)/support
	mkdir $(CAB_DIST_DIR_SRC)/advance
	cp $(CAB_ADVANCE_SRC) $(CAB_DIST_DIR_SRC)/advance
	mkdir $(CAB_DIST_DIR_SRC)/advance/card
	cp $(CARD_SRC) $(CAB_DIST_DIR_SRC)/advance/card
	mkdir $(CAB_DIST_DIR_SRC)/advance/tsr
	cp $(TSR_SRC) $(CAB_DIST_DIR_SRC)/advance/tsr
	mkdir $(CAB_DIST_DIR_SRC)/advance/vbe
	cp $(VBE_SRC) $(CAB_DIST_DIR_SRC)/advance/vbe
	mkdir $(CAB_DIST_DIR_SRC)/advance/vbe32
	cp $(VBE32_SRC) $(CAB_DIST_DIR_SRC)/advance/vbe32
	mkdir $(CAB_DIST_DIR_SRC)/advance/vbew
	cp $(VBEW_SRC) $(CAB_DIST_DIR_SRC)/advance/vbew
	mkdir $(CAB_DIST_DIR_SRC)/advance/vga
	cp $(VGA_SRC) $(CAB_DIST_DIR_SRC)/advance/vga
	mkdir $(CAB_DIST_DIR_SRC)/advance/video
	cp $(VIDEO_SRC) $(CAB_DIST_DIR_SRC)/advance/video
	mkdir $(CAB_DIST_DIR_SRC)/advance/off
	cp $(OFF_SRC) $(CAB_DIST_DIR_SRC)/advance/off
	mkdir $(CAB_DIST_DIR_SRC)/advance/portio
	cp $(PORTIO_SRC) $(CAB_DIST_DIR_SRC)/advance/portio
	mkdir $(CAB_DIST_DIR_SRC)/advance/videow
	cp $(VIDEOWIN_SRC) $(CAB_DIST_DIR_SRC)/advance/videow
	mkdir $(CAB_DIST_DIR_SRC)/advance/d2
	cp $(D2_SRC) $(CAB_DIST_DIR_SRC)/advance/d2
	mkdir $(CAB_DIST_DIR_SRC)/advance/dos
	cp $(DOS16_SRC) $(CAB_DIST_DIR_SRC)/advance/dos
	mkdir $(CAB_DIST_DIR_SRC)/advance/svgalib
	cp $(SVGALIB_SRC) $(CAB_DIST_DIR_SRC)/advance/svgalib
	mkdir $(CAB_DIST_DIR_SRC)/advance/svgalib/clockchi
	cp $(SVGALIBCLOCKCHI_SRC) $(CAB_DIST_DIR_SRC)/advance/svgalib/clockchi
	mkdir $(CAB_DIST_DIR_SRC)/advance/svgalib/ramdac
	cp $(SVGALIBRAMDAC_SRC) $(CAB_DIST_DIR_SRC)/advance/svgalib/ramdac
	mkdir $(CAB_DIST_DIR_SRC)/advance/svgalib/drivers
	cp $(SVGALIBDRIVERS_SRC) $(CAB_DIST_DIR_SRC)/advance/svgalib/drivers
	mkdir $(CAB_DIST_DIR_SRC)/advance/svgalib/svgados
	cp $(SVGALIBSVGADOS_SRC) $(CAB_DIST_DIR_SRC)/advance/svgalib/svgados
	mkdir $(CAB_DIST_DIR_SRC)/advance/svgalib/svgawin
	cp $(SVGALIBSVGAWIN_SRC) $(CAB_DIST_DIR_SRC)/advance/svgalib/svgawin
	mkdir $(CAB_DIST_DIR_SRC)/advance/svgalib/svgawin/sys
	cp $(SVGALIBSVGAWINSYS_SRC) $(CAB_DIST_DIR_SRC)/advance/svgalib/svgawin/sys
	mkdir $(CAB_DIST_DIR_SRC)/advance/svgalib/svgawin/install
	cp $(SVGALIBSVGAWININSTALL_SRC) $(CAB_DIST_DIR_SRC)/advance/svgalib/svgawin/install
	mkdir $(CAB_DIST_DIR_SRC)/advance/svgalib/svgawin/driver
	cp $(SVGALIBSVGAWINDRIVER_SRC) $(CAB_DIST_DIR_SRC)/advance/svgalib/svgawin/driver
	mkdir $(CAB_DIST_DIR_SRC)/advance/svgalib/svgavdd
	cp $(SVGALIBSVGAVDD_SRC) $(CAB_DIST_DIR_SRC)/advance/svgalib/svgavdd
	mkdir $(CAB_DIST_DIR_SRC)/advance/svgalib/svgavdd/vdd
	cp $(SVGALIBSVGAVDDVDD_SRC) $(CAB_DIST_DIR_SRC)/advance/svgalib/svgavdd/vdd
	mkdir $(CAB_DIST_DIR_SRC)/contrib
	mkdir $(CAB_DIST_DIR_SRC)/contrib/cab
	cp -R $(CAB_CONTRIB_SRC) $(CAB_DIST_DIR_SRC)/contrib/cab
	rm -f $(CAB_DIST_FILE_SRC).tar.gz
	tar cfzo $(CAB_DIST_FILE_SRC).tar.gz $(CAB_DIST_DIR_SRC)
	rm -r $(CAB_DIST_DIR_SRC)

distcabbin: $(CAB_ROOT_BIN) $(CAB_DOC_BIN)
	mkdir $(CAB_DIST_DIR_BIN)
	cp $(DOCOBJ)/readcab.txt $(CAB_DIST_DIR_BIN)/readme.txt
	cp $(DOCOBJ)/relecab.txt $(CAB_DIST_DIR_BIN)/release.txt
	cp $(DOCOBJ)/histcab.txt $(CAB_DIST_DIR_BIN)/history.txt
	cp $(srcdir)/COPYING $(CAB_DIST_DIR_BIN)/copying.txt
	cp $(CAB_ROOT_BIN) $(CAB_DIST_DIR_BIN)
	mkdir $(CAB_DIST_DIR_BIN)/doc
	cp $(CAB_DOC_BIN) $(CAB_DIST_DIR_BIN)/doc
ifeq ($(CONF_SYSTEM),dos)
	mkdir $(CAB_DIST_DIR_BIN)/contrib
	cp -r $(CAB_CONTRIB_SRC) $(CAB_DIST_DIR_BIN)/contrib
endif
	rm -f $(CAB_DIST_FILE_BIN).zip
	find $(CAB_DIST_DIR_BIN) \( -name "*.txt" \) -type f -exec unix2dos {} \;
	cd $(CAB_DIST_DIR_BIN) && zip -r ../$(CAB_DIST_FILE_BIN).zip *
	rm -r $(CAB_DIST_DIR_BIN)
