############################################################################
# J

JCFLAGS += \
	-Iadvance/$(HOST_SYSTEM) \
	-Iadvance/lib \
	-Iadvance/common
JOBJDIRS = \
	$(JOBJ) \
	$(JOBJ)/j \
	$(JOBJ)/lib \
	$(JOBJ)/$(HOST_SYSTEM)
JOBJS = \
	$(JOBJ)/j/j.o \
	$(JOBJ)/lib/log.o \
	$(JOBJ)/lib/conf.o \
	$(JOBJ)/lib/incstr.o

ifeq ($(HOST_SYSTEM),linux)
JCFLAGS += -DPREFIX=\"$(PREFIX)\"
JCFLAGS += \
	-DUSE_JOYSTICK_SVGALIB
JLIBS = -lvga
JOBJS += \
	$(JOBJ)/lib/filenix.o \
	$(JOBJ)/lib/targnix.o \
	$(JOBJ)/$(HOST_SYSTEM)/os.o
endif

ifeq ($(HOST_SYSTEM),dos)
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
	$(JOBJ)/$(HOST_SYSTEM)/os.o
endif

ifeq ($(HOST_SYSTEM),sdl)
JCFLAGS += \
	$(SDLCFLAGS) \
	-DPREFIX=\"$(PREFIX)\" \
	-DUSE_JOYSTICK_SDL
JLIBS += $(SDLLIBS)
JOBJS += $(JOBJ)/$(HOST_SYSTEM)/os.o
ifeq ($(HOST_TARGET),linux)
JOBJS += \
	$(JOBJ)/lib/filenix.o \
	$(JOBJ)/lib/targnix.o
endif
ifeq ($(HOST_TARGET),windows)
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
ifeq ($(COMPRESS),1)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advj$(EXE)
	$(LN) $@ advj$(EXE)
