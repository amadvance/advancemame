############################################################################
# J

JCFLAGS += \
	-I$(srcdir)/advance/lib
JOBJDIRS += \
	$(JOBJ) \
	$(JOBJ)/j \
	$(JOBJ)/lib
JOBJS += \
	$(JOBJ)/j/j.o \
	$(JOBJ)/lib/portable.o \
	$(JOBJ)/lib/snstring.o \
	$(JOBJ)/lib/log.o \
	$(JOBJ)/lib/conf.o \
	$(JOBJ)/lib/incstr.o \
	$(JOBJ)/lib/device.o \
	$(JOBJ)/lib/joyall.o \
	$(JOBJ)/lib/joydrv.o \
	$(JOBJ)/lib/jnone.o \
	$(JOBJ)/lib/error.o

ifeq ($(CONF_HOST),unix)
JCFLAGS += \
	-DDATADIR=\"$(DATADIR)\" \
	-I$(srcdir)/advance/linux
JOBJDIRS += \
	$(JOBJ)/linux
JOBJS += \
	$(JOBJ)/lib/filenix.o \
	$(JOBJ)/lib/targnix.o \
	$(JOBJ)/linux/os.o
ifeq ($(CONF_LIB_SVGALIB),yes)
JCFLAGS += \
	-DUSE_JOYSTICK_SVGALIB 
JLIBS += -lvga
JOBJS += \
	$(JOBJ)/linux/jsvgab.o
endif
ifeq ($(CONF_LIB_SDL),yes)
JCFLAGS += \
	$(SDLCFLAGS) \
	-I$(srcdir)/advance/sdl \
	-DUSE_JOYSTICK_SDL
JLIBS += $(SDLLIBS)
JOBJDIRS += \
	$(JOBJ)/sdl
JOBJS += \
	$(JOBJ)/sdl/jsdl.o
endif
ifeq ($(CONF_LIB_JEVENT),yes)
JCFLAGS += \
	-DUSE_JOYSTICK_EVENT
JOBJS += \
	$(JOBJ)/linux/jevent.o \
	$(JOBJ)/linux/event.o
endif
endif

ifeq ($(CONF_HOST),dos)
JCFLAGS += \
	-I$(srcdir)/advance/dos \
	-DUSE_CONFIG_ALLEGRO_WRAPPER \
	-DUSE_JOYSTICK_ALLEGRO
JLDFLAGS += \
	-Xlinker --wrap -Xlinker get_config_string \
	-Xlinker --wrap -Xlinker get_config_int \
	-Xlinker --wrap -Xlinker set_config_string \
	-Xlinker --wrap -Xlinker set_config_int \
	-Xlinker --wrap -Xlinker get_config_id \
	-Xlinker --wrap -Xlinker set_config_id
JLIBS += -lalleg
JOBJDIRS += \
	$(JOBJ)/dos
JOBJS += \
	$(JOBJ)/lib/filedos.o \
	$(JOBJ)/lib/targdos.o \
	$(JOBJ)/dos/os.o \
	$(JOBJ)/dos/jalleg.o
endif

ifeq ($(CONF_HOST),windows)
JCFLAGS += \
	-I$(srcdir)/advance/windows
JOBJDIRS += \
	$(JOBJ)/windows
JOBJS += \
	$(JOBJ)/lib/filedos.o \
	$(JOBJ)/lib/targwin.o \
	$(JOBJ)/windows/os.o
ifeq ($(CONF_LIB_SDL),yes)
JCFLAGS += \
	$(SDLCFLAGS) \
	-I$(srcdir)/advance/sdl \
	-DUSE_JOYSTICK_SDL
JLIBS += $(SDLLIBS)
JOBJDIRS += \
	$(JOBJ)/sdl
JOBJS += \
	$(JOBJ)/sdl/jsdl.o
endif
endif

$(JOBJ)/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(JCFLAGS) -c $< -o $@

$(sort $(JOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(JOBJ)/advj$(EXE) : $(sort $(JOBJDIRS)) $(JOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(JLDFLAGS) $(JOBJS) $(JLIBS) -o $@
	$(RM) advj$(EXE)
	$(LN_S) $@ advj$(EXE)
