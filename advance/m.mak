############################################################################
# M

MCFLAGS += \
	-Iadvance/$(CONF_SYSTEM) \
	-Iadvance/lib \
	-Iadvance/common
MOBJDIRS = \
	$(MOBJ) \
	$(MOBJ)/m \
	$(MOBJ)/lib \
	$(MOBJ)/$(CONF_SYSTEM)
MOBJS = \
	$(MOBJ)/m/m.o \
	$(MOBJ)/lib/log.o \
	$(MOBJ)/lib/conf.o \
	$(MOBJ)/lib/incstr.o

ifeq ($(CONF_SYSTEM),linux)
MCFLAGS += -DPREFIX=\"$(PREFIX)\"
MCFLAGS += \
	-DUSE_MOUSE_SVGALIB
MLIBS = -lvga
MOBJS += \
	$(MOBJ)/lib/filenix.o \
	$(MOBJ)/lib/targnix.o \
	$(MOBJ)/$(CONF_SYSTEM)/os.o
endif

ifeq ($(CONF_SYSTEM),dos)
MCFLAGS += -DUSE_CONFIG_ALLEGRO_WRAPPER
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
	$(MOBJ)/$(CONF_SYSTEM)/os.o
endif

ifeq ($(CONF_SYSTEM),sdl)
MCFLAGS += \
	$(SDLCFLAGS) \
	-DPREFIX=\"$(PREFIX)\" \
	-DUSE_MOUSE_SDL
MLIBS += $(SDLLIBS)
MOBJS += $(MOBJ)/$(CONF_SYSTEM)/os.o
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

$(MOBJ)/%.o: advance/%.c
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
	$(LN) $@ advm$(EXE)
