############################################################################
# Line

LINECFLAGS += \
	-Iadvance/common
LINEOBJDIRS = \
	$(LINEOBJ) \
	$(LINEOBJ)/line
LINEOBJS = \
	$(LINEOBJ)/line/line.o
LINELIBS = -lm

$(LINEOBJ)/%.o: advance/%.cc
	$(ECHO) $@ $(MSG)
	$(CXX-HOST) $(CFLAGS) $(LINECFLAGS) -c $< -o $@

$(sort $(LINEOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(LINEOBJ)/advline$(EXE-HOST) : $(sort $(LINEOBJDIRS)) $(LINEOBJS)
	$(ECHO) $@ $(MSG)
	$(LDXX-HOST) $(LDFLAGS) $(LINELDFLAGS) $(LINEOBJS) $(LINELIBS) -o $@

