############################################################################
# I

ICFLAGS += \
	-I$(srcdir)/advance/$(CONF_SYSTEM) \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/common
IOBJDIRS = \
	$(IOBJ) \
	$(IOBJ)/i \
	$(IOBJ)/lib \
	$(IOBJ)/$(CONF_SYSTEM)
IOBJS = \
	$(IOBJ)/i/i.o \
	$(IOBJ)/lib/log.o \
	$(IOBJ)/lib/conf.o \
	$(IOBJ)/lib/incstr.o \
	$(IOBJ)/lib/device.o \
	$(IOBJ)/lib/inputdrv.o \
	$(IOBJ)/lib/inputall.o \
	$(IOBJ)/lib/inone.o \

ifeq ($(CONF_SYSTEM),linux)
ICFLAGS += -DPREFIX=\"$(PREFIX)\"
ILIBS = -lslang
IOBJS += \
	$(IOBJ)/lib/filenix.o \
	$(IOBJ)/lib/targnix.o \
	$(IOBJ)/$(CONF_SYSTEM)/os.o \
	$(IOBJ)/$(CONF_SYSTEM)/islang.o
ICFLAGS += \
	-DUSE_INPUT_SLANG -DUSE_INPUT_NONE
endif

ifeq ($(CONF_SYSTEM),dos)
ILIBS = -lalleg
IOBJS += \
	$(IOBJ)/lib/filedos.o \
	$(IOBJ)/lib/targdos.o \
	$(IOBJ)/$(CONF_SYSTEM)/os.o \
	$(IOBJ)/$(CONF_SYSTEM)/idos.o
ICFLAGS += \
	-DUSE_INPUT_DOS -DUSE_INPUT_NONE
endif

ifeq ($(CONF_SYSTEM),sdl)
ICFLAGS += \
	$(SDLCFLAGS) \
	-DPREFIX=\"$(PREFIX)\" \
	-DUSE_INPUT_SDL -DUSE_INPUT_NONE
ILIBS += $(SDLLIBS)
IOBJS += \
	$(IOBJ)/$(CONF_SYSTEM)/os.o \
	$(IOBJ)/$(CONF_SYSTEM)/isdl.o
ifeq ($(CONF_HOST),unix)
IOBJS += \
	$(IOBJ)/lib/filenix.o \
	$(IOBJ)/lib/targnix.o
endif
ifeq ($(CONF_HOST),windows)
IOBJS += \
	$(IOBJ)/lib/filedos.o \
	$(IOBJ)/lib/targwin.o
endif
endif

$(IOBJ)/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(ICFLAGS) -c $< -o $@

$(sort $(IOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(IOBJ)/advi$(EXE) : $(sort $(IOBJDIRS)) $(IOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(ILDFLAGS) $(IOBJS) $(ILIBS) -o $@
ifeq ($(CONF_COMPRESS),yes)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advi$(EXE)
	$(LN_S) $@ advi$(EXE)

