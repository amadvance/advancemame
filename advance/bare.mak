############################################################################
# Bare

# Dependencies on VERSION
$(BAREOBJ)/bare/bare.o: Makefile

BARECFLAGS += \
	-DADV_VERSION=\"$(VERSION)\" \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/bare

BAREOBJDIRS += \
	$(BAREOBJ)/bare \
	$(BAREOBJ)/lib

BAREOBJS += \
	$(BAREOBJ)/bare/bare.o \
	$(BAREOBJ)/lib/portable.o

ifeq ($(CONF_SYSTEM),unix)
BARECFLAGS += \
	-DADV_DATADIR=\"$(datadir)\" \
	-DADV_SYSCONFDIR=\"$(sysconfdir)\" \
	-I$(srcdir)/advance/linux
endif

ifeq ($(CONF_SYSTEM),dos)
BARECFLAGS += \
	-I$(srcdir)/advance/dos
endif

ifeq ($(CONF_SYSTEM),windows)
BARECFLAGS += \
	-I$(srcdir)/advance/windows
endif

############################################################################
# expat

$(BAREOBJ)/libexpat.a: $(BAREOBJ)/expat/xmlparse.o $(BAREOBJ)/expat/xmlrole.o $(BAREOBJ)/expat/xmltok.o
	$(ECHO) $@
	$(AR) crs $@ $^

ifeq ($(CONF_LIB_EXPAT),yes)
BARELIBS += -lexpat
else
BARECFLAGS += \
	-I$(srcdir)/advance/expat
BAREOBJDIRS += \
	$(BAREOBJ)/expat
BAREOBJS += \
	$(BAREOBJ)/libexpat.a
endif

############################################################################
# rules

$(BAREOBJ)/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(BARECFLAGS) -c $< -o $@

$(BAREOBJ):
	$(ECHO) $@
	$(MD) $@

$(sort $(BAREOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(BAREOBJ)/advbare$(EXE) : $(sort $(BAREOBJDIRS)) $(BAREOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(BAREOBJS) $(BARELIBS) $(BARELDFLAGS) $(LDFLAGS) $(LIBS) -o $@
	$(RM) advbare$(EXE)
	$(LN_S) $@ advbare$(EXE)
