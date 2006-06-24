############################################################################
# J

JCFLAGS += \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/blit
JOBJDIRS += \
	$(JOBJ)/j \
	$(JOBJ)/lib
JOBJS += \
	$(JOBJ)/j/j.o \
	$(JOBJ)/lib/portable.o \
	$(JOBJ)/lib/snstring.o \
	$(JOBJ)/lib/log.o \
	$(JOBJ)/lib/measure.o \
	$(JOBJ)/lib/conf.o \
	$(JOBJ)/lib/incstr.o \
	$(JOBJ)/lib/device.o \
	$(JOBJ)/lib/joyall.o \
	$(JOBJ)/lib/joydrv.o \
	$(JOBJ)/lib/jnone.o \
	$(JOBJ)/lib/error.o

ifeq ($(CONF_SYSTEM),unix)
JCFLAGS += \
	-DADV_DATADIR=\"$(datadir)\" \
	-DADV_SYSCONFDIR=\"$(sysconfdir)\" \
	-I$(srcdir)/advance/linux
JOBJDIRS += \
	$(JOBJ)/linux
JOBJS += \
	$(JOBJ)/linux/file.o \
	$(JOBJ)/linux/target.o \
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
ifeq ($(CONF_LIB_JRAW),yes)
JCFLAGS += \
	-DUSE_JOYSTICK_RAW
JOBJS += \
	$(JOBJ)/linux/jraw.o
endif
ifeq ($(CONF_LIB_JEVENT),yes)
JCFLAGS += \
	-DUSE_JOYSTICK_EVENT
JOBJS += \
	$(JOBJ)/linux/jevent.o \
	$(JOBJ)/linux/event.o
endif
endif

ifeq ($(CONF_SYSTEM),dos)
JCFLAGS += \
	-I$(srcdir)/advance/dos \
	-DUSE_CONFIG_ALLEGRO_WRAPPER \
	-DUSE_JOYSTICK_ALLEGRO -DUSE_JOYSTICK_LGALLEGRO
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
	$(JOBJ)/dos/file.o \
	$(JOBJ)/dos/target.o \
	$(JOBJ)/dos/os.o \
	$(JOBJ)/dos/jalleg.o \
	$(JOBJ)/dos/jlgalleg.o
endif

ifeq ($(CONF_SYSTEM),windows)
JCFLAGS += \
	-I$(srcdir)/advance/windows
JOBJDIRS += \
	$(JOBJ)/windows \
	$(JOBJ)/dos
JOBJS += \
	$(JOBJ)/dos/file.o \
	$(JOBJ)/windows/target.o \
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

$(JOBJ):
	$(ECHO) $@
	$(MD) $@

$(sort $(JOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(JOBJ)/advj$(EXE) : $(sort $(JOBJDIRS)) $(JOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(JOBJS) $(JLIBS) $(JLDFLAGS) $(LDFLAGS) -o $@
	$(RM) advj$(EXE)
	$(LN_S) $@ advj$(EXE)
