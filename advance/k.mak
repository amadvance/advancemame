############################################################################
# K

KCFLAGS += \
	-I$(srcdir)/advance/$(CONF_SYSTEM) \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/common
KOBJDIRS = \
	$(KOBJ) \
	$(KOBJ)/k \
	$(KOBJ)/lib \
	$(KOBJ)/$(CONF_SYSTEM)
KOBJS = \
	$(KOBJ)/k/k.o \
	$(KOBJ)/lib/log.o \
	$(KOBJ)/lib/conf.o \
	$(KOBJ)/lib/incstr.o \
	$(KOBJ)/lib/key.o \
	$(KOBJ)/lib/device.o \
	$(KOBJ)/lib/keydrv.o \
	$(KOBJ)/lib/keyall.o \
	$(KOBJ)/lib/knone.o \

ifeq ($(CONF_SYSTEM),linux)
KCFLAGS += -DPREFIX=\"$(PREFIX)\"
KLIBS = -lvga
KOBJS += \
	$(KOBJ)/lib/filenix.o \
	$(KOBJ)/lib/targnix.o \
	$(KOBJ)/$(CONF_SYSTEM)/os.o \
	$(KOBJ)/$(CONF_SYSTEM)/ksvgab.o
KCFLAGS += \
	-DUSE_KEYBOARD_SVGALIB -DUSE_KEYBOARD_NONE
endif

ifeq ($(CONF_SYSTEM),dos)
KLIBS = -lalleg
KOBJS += \
	$(KOBJ)/lib/filedos.o \
	$(KOBJ)/lib/targdos.o \
	$(KOBJ)/$(CONF_SYSTEM)/os.o \
	$(KOBJ)/$(CONF_SYSTEM)/kalleg.o
KCFLAGS += \
	-DUSE_KEYBOARD_ALLEGRO -DUSE_KEYBOARD_NONE
endif

ifeq ($(CONF_SYSTEM),sdl)
KCFLAGS += \
	$(SDLCFLAGS) \
	-DPREFIX=\"$(PREFIX)\" \
	-DUSE_KEYBOARD_SDL -DUSE_KEYBOARD_NONE
KLIBS += $(SDLLIBS)
KOBJS += \
	$(KOBJ)/$(CONF_SYSTEM)/os.o \
	$(KOBJ)/$(CONF_SYSTEM)/ksdl.o
ifeq ($(CONF_HOST),unix)
KOBJS += \
	$(KOBJ)/lib/filenix.o \
	$(KOBJ)/lib/targnix.o
endif
ifeq ($(CONF_HOST),windows)
KOBJS += \
	$(KOBJ)/lib/filedos.o \
	$(KOBJ)/lib/targwin.o
endif
endif

$(KOBJ)/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(KCFLAGS) -c $< -o $@

$(sort $(KOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(KOBJ)/advk$(EXE) : $(sort $(KOBJDIRS)) $(KOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(KLDFLAGS) $(KOBJS) $(KLIBS) -o $@
ifeq ($(CONF_COMPRESS),yes)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advk$(EXE)
	$(LN_S) $@ advk$(EXE)

