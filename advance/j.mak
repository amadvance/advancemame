############################################################################
# J

JCFLAGS += \
	-Iadvance/$(CONF_SYSTEM) \
	-Iadvance/lib \
	-Iadvance/common
JOBJDIRS = \
	$(JOBJ) \
	$(JOBJ)/j \
	$(JOBJ)/lib \
	$(JOBJ)/$(CONF_SYSTEM)
JOBJS = \
	$(JOBJ)/j/j.o \
	$(JOBJ)/lib/log.o \
	$(JOBJ)/lib/conf.o \
	$(JOBJ)/lib/incstr.o

ifeq ($(CONF_SYSTEM),linux)
JCFLAGS += -DPREFIX=\"$(PREFIX)\"
JCFLAGS += \
	-DUSE_JOYSTICK_SVGALIB
JLIBS = -lvga
JOBJS += \
	$(JOBJ)/lib/filenix.o \
	$(JOBJ)/lib/targnix.o \
	$(JOBJ)/$(CONF_SYSTEM)/os.o
endif

ifeq ($(CONF_SYSTEM),dos)
JCFLAGS += -DUSE_CONFIG_ALLEGRO_WRAPPER
JLDFLAGS += \
	-Xlinker --wrap -Xlinker get_config_string \
	-Xlinker --wrap -Xlinker get_config_int \
	-Xlinker --wrap -Xlinker set_config_string \
	-Xlinker --wrap -Xlinker set_config_int \
	-Xlinker --wrap -Xlinker get_config_id \
	-Xlinker --wrap -Xlinker set_config_id
JLIBS = -lalleg
JOBJS += \
	$(JOBJ)/lib/filedos.o \
	$(JOBJ)/lib/targdos.o \
	$(JOBJ)/$(CONF_SYSTEM)/os.o
endif

ifeq ($(CONF_SYSTEM),sdl)
JCFLAGS += \
	$(SDLCFLAGS) \
	-DPREFIX=\"$(PREFIX)\" \
	-DUSE_JOYSTICK_SDL
JLIBS += $(SDLLIBS)
JOBJS += $(JOBJ)/$(CONF_SYSTEM)/os.o
ifeq ($(CONF_HOST),linux)
JOBJS += \
	$(JOBJ)/lib/filenix.o \
	$(JOBJ)/lib/targnix.o
endif
ifeq ($(CONF_HOST),windows)
JOBJS += \
	$(JOBJ)/lib/filedos.o \
	$(JOBJ)/lib/targwin.o
endif
endif

$(JOBJ)/%.o: advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(JCFLAGS) -c $< -o $@

$(sort $(JOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(JOBJ)/advj$(EXE) : $(sort $(JOBJDIRS)) $(JOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(JLDFLAGS) $(JOBJS) $(JLIBS) -o $@
ifeq ($(CONF_COMPRESS),yes)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advj$(EXE)
	$(LN) $@ advj$(EXE)
