############################################################################
# K

KCFLAGS += \
	-I$(srcdir)/advance/lib
KOBJDIRS += \
	$(KOBJ) \
	$(KOBJ)/k \
	$(KOBJ)/lib
KOBJS += \
	$(KOBJ)/k/k.o \
	$(KOBJ)/lib/portable.o \
	$(KOBJ)/lib/snstring.o \
	$(KOBJ)/lib/log.o \
	$(KOBJ)/lib/conf.o \
	$(KOBJ)/lib/incstr.o \
	$(KOBJ)/lib/key.o \
	$(KOBJ)/lib/device.o \
	$(KOBJ)/lib/keydrv.o \
	$(KOBJ)/lib/keyall.o \
	$(KOBJ)/lib/knone.o \
	$(KOBJ)/lib/error.o

ifeq ($(CONF_HOST),unix)
KCFLAGS += \
	-DDATADIR=\"$(DATADIR)\" \
	-I$(srcdir)/advance/linux
KOBJDIRS += \
	$(KOBJ)/linux
KOBJS += \
	$(KOBJ)/lib/filenix.o \
	$(KOBJ)/lib/targnix.o \
	$(KOBJ)/linux/os.o
ifeq ($(CONF_LIB_SVGALIB),yes)
KCFLAGS += \
	-DUSE_KEYBOARD_SVGALIB 
KLIBS += -lvga
KOBJS += \
	$(KOBJ)/linux/ksvgab.o
endif
ifeq ($(CONF_LIB_KRAW),yes)
KCFLAGS += \
	-DUSE_KEYBOARD_RAW 
KOBJS += \
	$(KOBJ)/linux/kraw.o
endif
ifeq ($(CONF_LIB_KEVENT),yes)
KCFLAGS += \
	-DUSE_KEYBOARD_EVENT
KOBJS += \
	$(KOBJ)/linux/kevent.o \
	$(KOBJ)/linux/event.o
endif
endif

ifeq ($(CONF_HOST),dos)
KCFLAGS += \
	-I$(srcdir)/advance/dos \
	-DUSE_KEYBOARD_ALLEGRO
KLIBS += -lalleg
KOBJDIRS += \
	$(KOBJ)/dos
KOBJS += \
	$(KOBJ)/lib/filedos.o \
	$(KOBJ)/lib/targdos.o \
	$(KOBJ)/dos/os.o \
	$(KOBJ)/dos/kalleg.o
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
	$(RM) advk$(EXE)
	$(LN_S) $@ advk$(EXE)

