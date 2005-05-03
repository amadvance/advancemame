############################################################################
# M

MCFLAGS += \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/blit
MOBJDIRS += \
	$(MOBJ)/m \
	$(MOBJ)/lib
MOBJS += \
	$(MOBJ)/m/m.o \
	$(MOBJ)/lib/portable.o \
	$(MOBJ)/lib/measure.o \
	$(MOBJ)/lib/snstring.o \
	$(MOBJ)/lib/log.o \
	$(MOBJ)/lib/conf.o \
	$(MOBJ)/lib/incstr.o \
	$(MOBJ)/lib/device.o \
	$(MOBJ)/lib/mousedrv.o \
	$(MOBJ)/lib/mouseall.o \
	$(MOBJ)/lib/mnone.o \
	$(MOBJ)/lib/error.o

ifeq ($(CONF_SYSTEM),unix)
MCFLAGS += \
	-DADV_DATADIR=\"$(datadir)\" \
	-DADV_SYSCONFDIR=\"$(sysconfdir)\" \
	-I$(srcdir)/advance/linux
MOBJDIRS += \
	$(MOBJ)/linux
MOBJS += \
	$(MOBJ)/linux/file.o \
	$(MOBJ)/linux/target.o \
	$(MOBJ)/linux/os.o
ifeq ($(CONF_LIB_SVGALIB),yes)
MCFLAGS += \
	-DUSE_MOUSE_SVGALIB 
MLIBS += -lvga
MOBJS += \
	$(MOBJ)/linux/msvgab.o
endif
ifeq ($(CONF_LIB_MRAW),yes)
MCFLAGS += \
	-DUSE_MOUSE_RAW
MOBJS += \
	$(MOBJ)/linux/mraw.o
endif
ifeq ($(CONF_LIB_MEVENT),yes)
MCFLAGS += \
	-DUSE_MOUSE_EVENT
MOBJS += \
	$(MOBJ)/linux/mevent.o \
	$(MOBJ)/linux/event.o
endif
endif

ifeq ($(CONF_SYSTEM),dos)
MCFLAGS += \
	-I$(srcdir)/advance/dos \
	-DUSE_CONFIG_ALLEGRO_WRAPPER \
	-DUSE_MOUSE_ALLEGRO
MLDFLAGS += \
	-Xlinker --wrap -Xlinker get_config_string \
	-Xlinker --wrap -Xlinker get_config_int \
	-Xlinker --wrap -Xlinker set_config_string \
	-Xlinker --wrap -Xlinker set_config_int \
	-Xlinker --wrap -Xlinker get_config_id \
	-Xlinker --wrap -Xlinker set_config_id
MLIBS += -lalleg
MOBJDIRS += \
	$(MOBJ)/dos
MOBJS += \
	$(MOBJ)/dos/file.o \
	$(MOBJ)/dos/target.o \
	$(MOBJ)/dos/os.o \
	$(MOBJ)/dos/malleg.o
endif

$(MOBJ)/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(MCFLAGS) -c $< -o $@

$(MOBJ):
	$(ECHO) $@
	$(MD) $@

$(sort $(MOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(MOBJ)/advm$(EXE) : $(sort $(MOBJDIRS)) $(MOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(MOBJS) $(MLIBS) $(MLDFLAGS) $(LDFLAGS) -o $@
	$(RM) advm$(EXE)
	$(LN_S) $@ advm$(EXE)
