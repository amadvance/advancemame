############################################################################
# K

KCFLAGS += \
	-Iadvance/$(HOST_SYSTEM) \
	-Iadvance/lib \
	-Iadvance/common
KOBJDIRS = \
	$(KOBJ) \
	$(KOBJ)/k \
	$(KOBJ)/lib \
	$(KOBJ)/$(HOST_SYSTEM)
KOBJS = \
	$(KOBJ)/k/k.o \
	$(KOBJ)/lib/log.o \
	$(KOBJ)/lib/conf.o \
	$(KOBJ)/lib/key.o \
	$(KOBJ)/lib/incstr.o

ifeq ($(HOST_SYSTEM),linux)
KCFLAGS += -DPREFIX=\"$(PREFIX)\"
KLIBS = -lvga
KOBJS += \
	$(KOBJ)/lib/filenix.o \
	$(KOBJ)/lib/targnix.o \
	$(KOBJ)/$(HOST_SYSTEM)/os.o
KCFLAGS += \
	-DUSE_KEYBOARD_SVGALIB
endif

ifeq ($(HOST_SYSTEM),dos)
KLIBS = -lalleg
KOBJS += \
	$(KOBJ)/lib/filedos.o \
	$(KOBJ)/lib/targdos.o \
	$(KOBJ)/$(HOST_SYSTEM)/os.o
endif

ifeq ($(HOST_SYSTEM),sdl)
KCFLAGS += \
	$(SDLCFLAGS) \
	-DPREFIX=\"$(PREFIX)\" \
	-DUSE_KEYBOARD_SDL
KLIBS += $(SDLLIBS)
KOBJS += $(KOBJ)/$(HOST_SYSTEM)/os.o
ifeq ($(HOST_TARGET),linux)
KOBJS += \
	$(KOBJ)/lib/filenix.o \
	$(KOBJ)/lib/targnix.o
endif
ifeq ($(HOST_TARGET),windows)
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
ifeq ($(COMPRESS),1)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advk$(EXE)
	$(LN) $@ advk$(EXE)

