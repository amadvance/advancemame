############################################################################
# D2

D2OBJDIRS = \
	$(D2OBJ)/d2
D2OBJS = \
	$(D2OBJ)/d2/d2.o

$(D2OBJ)/%.o: $(srcdir)/advance/%.cc
	$(ECHO) $@ $(MSG)
	$(CXX_FOR_BUILD) $(CFLAGS_FOR_BUILD) $(D2CFLAGS) -c $< -o $@

$(D2OBJ):
	$(ECHO) $@
	$(MD) $@

$(sort $(D2OBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(D2OBJ)/advd2$(EXE_FOR_BUILD) : $(sort $(D2OBJDIRS)) $(D2OBJS)
	$(ECHO) $@ $(MSG)
	$(LDXX_FOR_BUILD) $(LDFLAGS_FOR_BUILD) $(D2LDFLAGS) $(D2OBJS) $(D2LIBS) -o $@

############################################################################
# Doc rules

$(DOCOBJ)/%.txt : $(srcdir)/doc/%.d
	$(D2OBJ)/advd2 txt < $< > $@

$(DOCOBJ)/%.html : $(srcdir)/doc/%.d
	$(D2OBJ)/advd2 html < $< > $@

$(DOCOBJ)/%.hh : $(srcdir)/doc/%.d
	$(D2OBJ)/advd2 frame < $< > $@

$(DOCOBJ)/%.1 : $(srcdir)/doc/%.d
	$(D2OBJ)/advd2 man < $< > $@

$(DOCOBJ)/%.ps : $(D2OBJ)/%.1
	groff -mandoc -Tps < $^ > $@

$(DOCOBJ)/%.pdf : $(D2OBJ)/%.ps
	ps2pdf13 - - < $^ > $@

