############################################################################
# M

MCFLAGS += \
	-I$(srcdir)/advance/$(CONF_SYSTEM) \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/common
MOBJDIRS = \
	$(MOBJ) \
	$(MOBJ)/m \
	$(MOBJ)/lib \
	$(MOBJ)/$(CONF_SYSTEM)
MOBJS = \
	$(MOBJ)/m/m.o \
	$(MOBJ)/lib/log.o \
	$(MOBJ)/lib/conf.o \
	$(MOBJ)/lib/incstr.o \
	$(MOBJ)/lib/device.o \
	$(MOBJ)/lib/mousedrv.o \
	$(MOBJ)/lib/mouseall.o \
	$(MOBJ)/lib/mnone.o
ifeq ($(CONF_SYSTEM),linux)
MCFLAGS += -DPREFIX=\"$(PREFIX)\"
MCFLAGS += \
	-DUSE_MOUSE_SVGALIB -DUSE_MOUSE_NONE
MLIBS = -lvga
MOBJS += \
	$(MOBJ)/lib/filenix.o \
	$(MOBJ)/lib/targnix.o \
	$(MOBJ)/$(CONF_SYSTEM)/os.o \
	$(MOBJ)/$(CONF_SYSTEM)/msvgab.o
endif

ifeq ($(CONF_SYSTEM),dos)
MCFLAGS += \
	-DUSE_CONFIG_ALLEGRO_WRAPPER \
	-DUSE_MOUSE_ALLEGRO -DUSE_MOUSE_NONE
MLDFLAGS += \
	-Xlinker --wrap -Xlinker get_config_string \
	-Xlinker --wrap -Xlinker get_config_int \
	-Xlinker --wrap -Xlinker set_config_string \
	-Xlinker --wrap -Xlinker set_config_int \
	-Xlinker --wrap -Xlinker get_config_id \
	-Xlinker --wrap -Xlinker set_config_id
MLIBS = -lalleg
MOBJS += \
	$(MOBJ)/lib/filedos.o \
	$(MOBJ)/lib/targdos.o \
	$(MOBJ)/$(CONF_SYSTEM)/os.o \
	$(MOBJ)/$(CONF_SYSTEM)/malleg.o
endif

ifeq ($(CONF_SYSTEM),sdl)
MCFLAGS += \
	$(SDLCFLAGS) \
	-DPREFIX=\"$(PREFIX)\" \
	-DUSE_MOUSE_SDL -DUSE_MOUSE_NONE
MLIBS += $(SDLLIBS)
MOBJS += \
	$(MOBJ)/$(CONF_SYSTEM)/os.o \
	$(MOBJ)/$(CONF_SYSTEM)/msdl.o
ifeq ($(CONF_HOST),linux)
MOBJS += \
	$(MOBJ)/lib/filenix.o \
	$(MOBJ)/lib/targnix.o
endif
ifeq ($(CONF_HOST),windows)
MOBJS += \
	$(MOBJ)/lib/filedos.o \
	$(MOBJ)/lib/targwin.o
endif
endif

$(MOBJ)/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(MCFLAGS) -c $< -o $@

$(sort $(MOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(MOBJ)/advm$(EXE) : $(sort $(MOBJDIRS)) $(MOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(MLDFLAGS) $(MOBJS) $(MLIBS) -o $@
ifeq ($(CONF_COMPRESS),yes)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advm$(EXE)
	$(LN_S) $@ advm$(EXE)
