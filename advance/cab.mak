############################################################################
# CAB dist

TSR_SRC = \
	$(wildcard advance/tsr/*.asm) \
	$(wildcard advance/tsr/*.c) \
	$(wildcard advance/tsr/*.h)

VBE_SRC = \
	advance/vbe/makefile \
	$(wildcard advance/vbe/*.asm) \
	$(wildcard advance/vbe/*.c) \
	$(wildcard advance/vbe/*.h)

VGA_SRC = \
	advance/vga/makefile \
	$(wildcard advance/vga/*.asm) \
	$(wildcard advance/vga/*.c) \
	$(wildcard advance/vga/*.h)

OFF_SRC = \
	advance/off/makefile \
	$(wildcard advance/off/*.c) \
	$(wildcard advance/off/*.h)

PORTIO_SRC = \
	advance/portio/makefile \
	$(wildcard advance/portio/*.c) \
	$(wildcard advance/portio/*.h)

VIDEO_SRC = \
	advance/video/makefile \
	$(wildcard advance/video/*.c) \
	$(wildcard advance/video/*.h)

CAB_ROOT_SRC = \
	makefile

CAB_ADVANCE_SRC = \
	advance/advance.mak \
	advance/cab.mak \
	advance/d2.mak

CAB_DOC_SRC = \
	doc/copying \
	doc/license.d \
	doc/vbe.d \
	doc/vga.d \
	doc/video.d \
	doc/off.d \
	doc/portio.d \
	doc/histcab.d \
	doc/readcab.txt \
	doc/relecab.txt

CAB_DOC_BIN = \
	doc/copying \
	$(DOCOBJ)/license.txt \
	$(DOCOBJ)/vbe.txt \
	$(DOCOBJ)/vga.txt \
	$(DOCOBJ)/video.txt \
	$(DOCOBJ)/off.txt \
	$(DOCOBJ)/portio.txt \
	$(DOCOBJ)/advv.txt \
	$(DOCOBJ)/license.html \
	$(DOCOBJ)/vbe.html \
	$(DOCOBJ)/vga.html \
	$(DOCOBJ)/video.html \
	$(DOCOBJ)/off.html \
	$(DOCOBJ)/portio.html \
	$(DOCOBJ)/advv.html \
	$(RCSRC) \

CAB_SUPPORT_SRC = \
	$(RCSRC) \
	support/video.pcx \
	support/videobis.pcx \
	support/vbev.bat \
	support/vgav.bat

CAB_CONTRIB_SRC = \
	$(wildcard contrib/cab/*)

CAB_ROOT_BIN = \
	advance/vbe/vbe.com \
	advance/vga/vga.exe \
	advance/video/video.exe \
	advance/off/off.com \
	advance/portio/portio.exe \
	support/video.pcx \
	support/videobis.pcx \
	support/vbev.bat \
	support/vgav.bat \
	$(VOBJ)/advv$(EXE)

CAB_DIST_DIR_SRC = tmpcab
CAB_DIST_DIR_BIN = tmpcabbin

distcab: $(RCSRC)
	mkdir $(CAB_DIST_DIR_SRC)
	cp $(CAB_ROOT_SRC) $(CAB_DIST_DIR_SRC)
	mkdir $(CAB_DIST_DIR_SRC)/doc
	cp $(CAB_DOC_SRC) $(CAB_DIST_DIR_SRC)/doc
	mkdir $(CAB_DIST_DIR_SRC)/support
	cp $(CAB_SUPPORT_SRC) $(CAB_DIST_DIR_SRC)/support
	mkdir $(CAB_DIST_DIR_SRC)/advance
	cp $(CAB_ADVANCE_SRC) $(CAB_DIST_DIR_SRC)/advance
	mkdir $(CAB_DIST_DIR_SRC)/advance/common
	cp $(COMMON_SRC) $(CAB_DIST_DIR_SRC)/advance/common
	mkdir $(CAB_DIST_DIR_SRC)/advance/card
	cp $(CARD_SRC) $(CAB_DIST_DIR_SRC)/advance/card
	mkdir $(CAB_DIST_DIR_SRC)/advance/tsr
	cp $(TSR_SRC) $(CAB_DIST_DIR_SRC)/advance/tsr
	mkdir $(CAB_DIST_DIR_SRC)/advance/vbe
	cp $(VBE_SRC) $(CAB_DIST_DIR_SRC)/advance/vbe
	mkdir $(CAB_DIST_DIR_SRC)/advance/vga
	cp $(VGA_SRC) $(CAB_DIST_DIR_SRC)/advance/vga
	mkdir $(CAB_DIST_DIR_SRC)/advance/video
	cp $(VIDEO_SRC) $(CAB_DIST_DIR_SRC)/advance/video
	mkdir $(CAB_DIST_DIR_SRC)/advance/off
	cp $(OFF_SRC) $(CAB_DIST_DIR_SRC)/advance/off
	mkdir $(CAB_DIST_DIR_SRC)/advance/portio
	cp $(PORTIO_SRC) $(CAB_DIST_DIR_SRC)/advance/portio
	mkdir $(CAB_DIST_DIR_SRC)/advance/d2
	cp $(D2_SRC) $(CAB_DIST_DIR_SRC)/advance/d2
	mkdir $(CAB_DIST_DIR_SRC)/contrib
	mkdir $(CAB_DIST_DIR_SRC)/contrib/cab
	cp -R $(CAB_CONTRIB_SRC) $(CAB_DIST_DIR_SRC)/contrib/cab
	rm -f advancecab-$(CABVERSION).zip
	cd $(CAB_DIST_DIR_SRC) && zip -r ../advancecab-$(CABVERSION).zip *
	rm -r $(CAB_DIST_DIR_SRC)

distcabbin: $(CAB_ROOT_BIN) $(CAB_DOC_BIN)
	mkdir $(CAB_DIST_DIR_BIN)
	cp doc/readcab.txt $(CAB_DIST_DIR_BIN)/readme.txt
	cp doc/relecab.txt $(CAB_DIST_DIR_BIN)/release.txt
	cp $(CAB_ROOT_BIN) $(CAB_DIST_DIR_BIN)
	mkdir $(CAB_DIST_DIR_BIN)/doc
	cp $(CAB_DOC_BIN) $(CAB_DIST_DIR_BIN)/doc
	mkdir $(CAB_DIST_DIR_BIN)/contrib
	cp -r $(CAB_CONTRIB_SRC) $(CAB_DIST_DIR_BIN)/contrib
	rm -f advancecab-$(CABVERSION)-$(BINARYTAG).zip
	find $(CAB_DIST_DIR_BIN) \( -name "*.txt" \) -type f -exec utod {} \;
	cd $(CAB_DIST_DIR_BIN) && zip -r ../advancecab-$(CABVERSION)-$(BINARYTAG).zip *
	rm -r $(CAB_DIST_DIR_BIN)
