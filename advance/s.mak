############################################################################
# system

SCFLAGS += \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/blit \
	-I$(srcdir)/advance/mpglib
SOBJS += \
	$(SOBJ)/lib/portable.o \
	$(SOBJ)/lib/snstring.o \
	$(SOBJ)/lib/log.o \
	$(SOBJ)/lib/measure.o \
	$(SOBJ)/lib/conf.o \
	$(SOBJ)/lib/incstr.o \
	$(SOBJ)/lib/sounddrv.o \
	$(SOBJ)/lib/device.o \
	$(SOBJ)/lib/mixer.o \
	$(SOBJ)/lib/wave.o \
	$(SOBJ)/lib/fz.o \
	$(SOBJ)/lib/soundall.o \
	$(SOBJ)/lib/error.o \
	$(SOBJ)/mpglib/dct64.o \
	$(SOBJ)/mpglib/decode.o \
	$(SOBJ)/mpglib/interfac.o \
	$(SOBJ)/mpglib/internal.o \
	$(SOBJ)/mpglib/layer3.o \
	$(SOBJ)/mpglib/tabinit.o \
	$(SOBJ)/s/s.o
SOBJDIRS += \
	$(SOBJ)/s \
	$(SOBJ)/lib \
	$(SOBJ)/mpglib

ifeq ($(CONF_SYSTEM),unix)
SCFLAGS += \
	-DADV_DATADIR=\"$(datadir)\" \
	-DADV_SYSCONFDIR=\"$(sysconfdir)\" \
	-I$(srcdir)/advance/linux
SLIBS += -lm
SOBJDIRS += \
	$(SOBJ)/linux
SOBJS += \
	$(SOBJ)/linux/file.o \
	$(SOBJ)/linux/target.o \
	$(SOBJ)/linux/os.o
ifeq ($(CONF_LIB_ALSA),yes)
SCFLAGS += \
	-DUSE_SOUND_ALSA
SOBJS += \
	$(SOBJ)/linux/salsa.o
SLIBS += -lasound
endif
ifeq ($(CONF_LIB_OSS),yes)
SCFLAGS += \
	-DUSE_SOUND_OSS
SOBJS += \
	$(SOBJ)/linux/soss.o
endif
ifeq ($(CONF_LIB_SDL),yes)
SCFLAGS += \
	$(SDLCFLAGS) \
	-I$(srcdir)/advance/sdl \
	-DUSE_SOUND_SDL
SLIBS += $(SDLLIBS)
SOBJDIRS += \
	$(SOBJ)/sdl
SOBJS += \
	$(SOBJ)/sdl/ssdl.o
endif
endif

ifeq ($(CONF_SYSTEM),dos)
SCFLAGS += \
	-I$(srcdir)/advance/dos \
	-DUSE_SOUND_SEAL -DUSE_SOUND_ALLEGRO -DUSE_SOUND_VSYNC
SLIBS += -laudio -lalleg -lm
SLDFLAGS += -Xlinker --wrap -Xlinker _mixer_init
SOBJDIRS += \
	$(SOBJ)/dos
SOBJS += \
	$(SOBJ)/dos/file.o \
	$(SOBJ)/dos/target.o \
	$(SOBJ)/dos/os.o \
	$(SOBJ)/dos/sseal.o \
	$(SOBJ)/dos/salleg.o \
	$(SOBJ)/dos/svsync.o
endif

ifeq ($(CONF_SYSTEM),windows)
SCFLAGS += \
	-I$(srcdir)/advance/windows
SLIBS += -lm
SOBJDIRS += \
	$(SOBJ)/windows \
	$(SOBJ)/dos
SOBJS += \
	$(SOBJ)/dos/file.o \
	$(SOBJ)/windows/target.o \
	$(SOBJ)/windows/os.o
ifeq ($(CONF_LIB_SDL),yes)
SCFLAGS += \
	$(SDLCFLAGS) \
	-I$(srcdir)/advance/sdl \
	-DUSE_SOUND_SDL
SLIBS += $(SDLLIBS)
SOBJDIRS += \
	$(SOBJ)/sdl
SOBJS += \
	$(SOBJ)/sdl/ssdl.o
endif
endif

############################################################################
# zlib

$(SOBJ)/libz.a: $(SOBJ)/zlib/adler32.o $(SOBJ)/zlib/crc32.o $(SOBJ)/zlib/deflate.o \
	$(SOBJ)/zlib/inffast.o $(SOBJ)/zlib/inflate.o \
	$(SOBJ)/zlib/infback.o $(SOBJ)/zlib/inftrees.o $(SOBJ)/zlib/trees.o \
	$(SOBJ)/zlib/zutil.o $(SOBJ)/zlib/uncompr.o $(SOBJ)/zlib/compress.o
	$(ECHO) $@
	$(AR) crs $@ $^

ifeq ($(CONF_LIB_ZLIB),yes)
SLIBS += -lz
else
CFLAGS += \
	-I$(srcdir)/advance/zlib
SOBJDIRS += \
	$(SOBJ)/zlib
SOBJS += \
	$(SOBJ)/libz.a
endif

############################################################################
# s

$(SOBJ)/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(SCFLAGS) -c $< -o $@

$(SOBJ):
	$(ECHO) $@
	$(MD) $@

$(sort $(SOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(SOBJ)/advs$(EXE) : $(sort $(SOBJDIRS)) $(SOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(SOBJS) $(SLIBS) $(SLDFLAGS) $(LDFLAGS) -o $@
	$(RM) advs$(EXE)
	$(LN_S) $@ advs$(EXE)

