############################################################################
# Line

LINEOBJDIRS = \
	$(LINEOBJ)/line
LINEOBJS = \
	$(LINEOBJ)/line/line.o
LINELIBS = -lm

$(LINEOBJ)/%.o: $(srcdir)/advance/%.cc
	$(ECHO) $@ $(MSG)
	$(CXX_BUILD) $(CFLAGS_BUILD) $(LINECFLAGS) -c $< -o $@

$(LINEOBJ):
	$(ECHO) $@
	$(MD) $@

$(sort $(LINEOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(LINEOBJ)/advline$(EXE_BUILD) : $(sort $(LINEOBJDIRS)) $(LINEOBJS)
	$(ECHO) $@ $(MSG)
	$(LDXX_BUILD) $(LDFLAGS_BUILD) $(LINELDFLAGS) $(LINEOBJS) $(LINELIBS) -o $@
	$(RM) advline$(EXE)
	$(LN_S) $@ advline$(EXE)
