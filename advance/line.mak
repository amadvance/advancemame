############################################################################
# Line

LINEOBJDIRS = \
	$(LINEOBJ)/line
LINEOBJS = \
	$(LINEOBJ)/line/line.o
LINELIBS = -lm

$(LINEOBJ)/%.o: $(srcdir)/advance/%.cc
	$(ECHO) $@ $(MSG)
	$(CXX_FOR_BUILD) $(CFLAGS_FOR_BUILD) $(LINECFLAGS) -c $< -o $@

$(LINEOBJ):
	$(ECHO) $@
	$(MD) $@

$(sort $(LINEOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(LINEOBJ)/advline$(EXE_FOR_BUILD) : $(sort $(LINEOBJDIRS)) $(LINEOBJS)
	$(ECHO) $@ $(MSG)
	$(LDXX_FOR_BUILD) $(LDFLAGS_FOR_BUILD) $(LINELDFLAGS) $(LINEOBJS) $(LINELIBS) -o $@
	$(RM) advline$(EXE)
	$(LN_S) $@ advline$(EXE)
