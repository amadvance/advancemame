############################################################################
# BLUE

# Dependencies on VERSION
$(BLUEOBJ)/blue/blue.o: Makefile

BLUECFLAGS += \
	-DADV_VERSION=\"$(VERSION)\" \
	-I$(srcdir)/advance/lib
BLUEOBJDIRS += \
	$(BLUEOBJ)/blue \
	$(BLUEOBJ)/lib
BLUEOBJS += \
	$(BLUEOBJ)/blue/blue.o \
	$(BLUEOBJ)/lib/portable.o

ifeq ($(CONF_SYSTEM),unix)
BLUECFLAGS += \
	-DADV_DATADIR=\"$(datadir)\" \
	-DADV_SYSCONFDIR=\"$(sysconfdir)\" \
	-I$(srcdir)/advance/linux
endif

$(BLUEOBJ)/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(BLUECFLAGS) -c $< -o $@

$(BLUEOBJ):
	$(ECHO) $@
	$(MD) $@

$(sort $(BLUEOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(BLUEOBJ)/advblue$(EXE) : $(sort $(BLUEOBJDIRS)) $(BLUEOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(BLUEOBJS) $(BLUELIBS) $(BLUELDFLAGS) $(LDFLAGS) $(LIBS) -o $@
	$(RM) advblue$(EXE)
	$(LN_S) $@ advblue$(EXE)

