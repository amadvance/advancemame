############################################################################
# D2

D2OBJDIRS = \
	$(D2OBJ) \
	$(D2OBJ)/d2
D2OBJS = \
	$(D2OBJ)/d2/d2.o

$(D2OBJ)/%.o: advance/%.cc
	$(ECHO) $@ $(MSG)
	$(CXX-HOST) $(CFLAGS) $(D2CFLAGS) -c $< -o $@

$(sort $(D2OBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(D2OBJ)/advd2$(EXE-HOST) : $(sort $(D2OBJDIRS)) $(D2OBJS)
	$(ECHO) $@ $(MSG)
	$(LDXX-HOST) $(LDFLAGS) $(D2LDFLAGS) $(D2OBJS) $(D2LIBS) -o $@

############################################################################
# Doc rules

$(D2OBJ)/%.txt : doc/%.d $(D2OBJ)/advd2$(EXE-HOST)
	$(D2OBJ)/advd2 txt < $< > $@

$(D2OBJ)/%.html : doc/%.d $(D2OBJ)/advd2$(EXE-HOST)
	$(D2OBJ)/advd2 html < $< > $@

$(D2OBJ)/%.1 : doc/%.d $(D2OBJ)/advd2$(EXE-HOST)
	$(D2OBJ)/advd2 man < $< > $@

$(D2OBJ)/%.ps : $(D2OBJ)/%.1
	groff -mandoc -Tps < $^ > $@

$(D2OBJ)/%.pdf : $(D2OBJ)/%.ps
	ps2pdf13 - - < $^ > $@

