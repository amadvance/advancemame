############################################################################
# K

KCFLAGS += \
	-Iadvance/$(CONF_SYSTEM) \
	-Iadvance/lib \
	-Iadvance/common
KOBJDIRS = \
	$(KOBJ) \
	$(KOBJ)/k \
	$(KOBJ)/lib \
	$(KOBJ)/$(CONF_SYSTEM)
KOBJS = \
	$(KOBJ)/k/k.o \
	$(KOBJ)/lib/log.o \
	$(KOBJ)/lib/conf.o \
	$(KOBJ)/lib/key.o \
	$(KOBJ)/lib/incstr.o

ifeq ($(CONF_SYSTEM),linux)
KCFLAGS += -DPREFIX=\"$(PREFIX)\"
KLIBS = -lvga
KOBJS += \
	$(KOBJ)/lib/filenix.o \
	$(KOBJ)/lib/targnix.o \
	$(KOBJ)/$(CONF_SYSTEM)/os.o
KCFLAGS += \
	-DUSE_KEYBOARD_SVGALIB
endif

ifeq ($(CONF_SYSTEM),dos)
KLIBS = -lalleg
KOBJS += \
	$(KOBJ)/lib/filedos.o \
	$(KOBJ)/lib/targdos.o \
	$(KOBJ)/$(CONF_SYSTEM)/os.o
endif

ifeq ($(CONF_SYSTEM),sdl)
KCFLAGS += \
	$(SDLCFLAGS) \
	-DPREFIX=\"$(PREFIX)\" \
	-DUSE_KEYBOARD_SDL
KLIBS += $(SDLLIBS)
KOBJS += $(KOBJ)/$(CONF_SYSTEM)/os.o
ifeq ($(CONF_HOST),linux)
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

$(KOBJ)/%.o: advance/%.c
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
	$(LN) $@ advk$(EXE)

