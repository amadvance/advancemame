############################################################################
# Lib dist

LIB_VERSION = 0.1

doclib:
	doxygen advance/lib.cfg

LIB_ADVANCE_SRC = \
	advance/lib.mak \
	advance/lib.cfg

LIB_DISTFILE_SRC = advancelib-$(LIB_VERSION)
LIB_DIST_DIR_SRC = $(LIB_DISTFILE_SRC)

distlib:
	mkdir $(LIB_DIST_DIR_SRC)
	cp -r reference $(LIB_DIST_DIR_SRC)/reference
	mkdir $(LIB_DIST_DIR_SRC)/advance
	cp $(LIB_ADVANCE_SRC) $(LIB_DIST_DIR_SRC)/advance
	mkdir $(LIB_DIST_DIR_SRC)/advance/linux
	cp $(LINUX_SRC) $(LIB_DIST_DIR_SRC)/advance/linux
	mkdir $(LIB_DIST_DIR_SRC)/advance/dos
	cp $(DOS_SRC) $(LIB_DIST_DIR_SRC)/advance/dos
	mkdir $(LIB_DIST_DIR_SRC)/advance/windows
	cp $(WINDOWS_SRC) $(LIB_DIST_DIR_SRC)/advance/windows
	mkdir $(LIB_DIST_DIR_SRC)/advance/sdl
	cp $(SDL_SRC) $(LIB_DIST_DIR_SRC)/advance/sdl
	mkdir $(LIB_DIST_DIR_SRC)/advance/lib
	cp $(LIB_SRC) $(LIB_DIST_DIR_SRC)/advance/lib
	mkdir $(LIB_DIST_DIR_SRC)/advance/blit
	cp $(BLIT_SRC) $(LIB_DIST_DIR_SRC)/advance/blit
	mkdir $(LIB_DIST_DIR_SRC)/advance/card
	cp $(CARD_SRC) $(LIB_DIST_DIR_SRC)/advance/card
	mkdir $(LIB_DIST_DIR_SRC)/advance/svgalib
	cp $(SVGALIB_SRC) $(LIB_DIST_DIR_SRC)/advance/svgalib
	mkdir $(LIB_DIST_DIR_SRC)/advance/svgalib/clockchi
	cp $(SVGALIBCLOCKCHI_SRC) $(LIB_DIST_DIR_SRC)/advance/svgalib/clockchi
	mkdir $(LIB_DIST_DIR_SRC)/advance/svgalib/ramdac
	cp $(SVGALIBRAMDAC_SRC) $(LIB_DIST_DIR_SRC)/advance/svgalib/ramdac
	mkdir $(LIB_DIST_DIR_SRC)/advance/svgalib/drivers
	cp $(SVGALIBDRIVERS_SRC) $(LIB_DIST_DIR_SRC)/advance/svgalib/drivers
	mkdir $(LIB_DIST_DIR_SRC)/advance/k
	cp $(K_SRC) $(LIB_DIST_DIR_SRC)/advance/k
	mkdir $(LIB_DIST_DIR_SRC)/advance/j
	cp $(J_SRC) $(LIB_DIST_DIR_SRC)/advance/j
	mkdir $(LIB_DIST_DIR_SRC)/advance/m
	cp $(M_SRC) $(LIB_DIST_DIR_SRC)/advance/m
	mkdir $(LIB_DIST_DIR_SRC)/advance/s
	cp $(S_SRC) $(LIB_DIST_DIR_SRC)/advance/s
	mkdir $(LIB_DIST_DIR_SRC)/advance/i
	cp $(I_SRC) $(LIB_DIST_DIR_SRC)/advance/i
	rm -f $(LIB_DISTFILE_SRC).tar.gz
	tar cfzo $(LIB_DISTFILE_SRC).tar.gz $(LIB_DIST_DIR_SRC)
	rm -r $(LIB_DIST_DIR_SRC)

