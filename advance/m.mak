############################################################################
# M

MCFLAGS += \
	-I$(srcdir)/advance/lib
MOBJDIRS += \
	$(MOBJ) \
	$(MOBJ)/m \
	$(MOBJ)/lib
MOBJS += \
	$(MOBJ)/m/m.o \
	$(MOBJ)/lib/portable.o \
	$(MOBJ)/lib/log.o \
	$(MOBJ)/lib/conf.o \
	$(MOBJ)/lib/incstr.o \
	$(MOBJ)/lib/device.o \
	$(MOBJ)/lib/mousedrv.o \
	$(MOBJ)/lib/mouseall.o \
	$(MOBJ)/lib/mnone.o \
	$(MOBJ)/lib/error.o

ifeq ($(CONF_HOST),unix)
MCFLAGS += \
	-DDATADIR=\"$(DATADIR)\" \
	-I$(srcdir)/advance/linux \
	-DUSE_MOUSE_NONE
MOBJDIRS += \
	$(MOBJ)/linux
MOBJS += \
	$(MOBJ)/lib/filenix.o \
	$(MOBJ)/lib/targnix.o \
	$(MOBJ)/linux/os.o
ifeq ($(CONF_LIB_SVGALIB),yes)
MCFLAGS += \
	-DUSE_MOUSE_SVGALIB 
MLIBS += -lvga
MOBJS += \
	$(MOBJ)/linux/msvgab.o
endif
endif

ifeq ($(CONF_HOST),dos)
MCFLAGS += \
	-I$(srcdir)/advance/dos \
	-DUSE_CONFIG_ALLEGRO_WRAPPER \
	-DUSE_MOUSE_ALLEGRO -DUSE_MOUSE_NONE
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
	$(MOBJ)/lib/filedos.o \
	$(MOBJ)/lib/targdos.o \
	$(MOBJ)/dos/os.o \
	$(MOBJ)/dos/malleg.o
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
