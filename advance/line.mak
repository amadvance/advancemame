############################################################################
# Line

LINECFLAGS += \
	-I$(srcdir)/advance/common
LINEOBJDIRS = \
	$(LINEOBJ) \
	$(LINEOBJ)/line
LINEOBJS = \
	$(LINEOBJ)/line/line.o
LINELIBS = -lm

$(LINEOBJ)/%.o: $(srcdir)/advance/%.cc
	$(ECHO) $@ $(MSG)
	$(CXX_BUILD) $(CFLAGS_BUILD) $(LINECFLAGS) -c $< -o $@

$(sort $(LINEOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(LINEOBJ)/advline$(EXE_BUILD) : $(sort $(LINEOBJDIRS)) $(LINEOBJS)
	$(ECHO) $@ $(MSG)
	$(LDXX_BUILD) $(LDFLAGS_BUILD) $(LINELDFLAGS) $(LINEOBJS) $(LINELIBS) -o $@

