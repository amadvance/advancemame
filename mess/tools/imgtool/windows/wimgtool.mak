WIMGTOOL_OBJS=\
	$(IMGTOOL_LIB_OBJS)								\
	$(OBJ)/mess/pile.o								\
	$(OBJ)/mess/windows/opcntrl.o					\
	$(OBJ)/mess/windows/strconv.o					\
	$(OBJ)/mess/windows/winutils.o					\
	$(OBJ)/mess/tools/imgtool/stubs.o				\
	$(OBJ)/mess/tools/imgtool/windows/wmain.o		\
	$(OBJ)/mess/tools/imgtool/windows/wimgtool.o	\
	$(OBJ)/mess/tools/imgtool/windows/attrdlg.o		\
	$(OBJ)/mess/tools/imgtool/windows/assoc.o		\
	$(OBJ)/mess/tools/imgtool/windows/assocdlg.o	\
	$(OBJ)/mess/tools/imgtool/windows/hexview.o		\
	$(OBJ)/mess/tools/imgtool/windows/secview.o		\
	$(OBJ)/mess/tools/imgtool/windows/wimgtool.res	\

$(OBJ)/mess/tools/imgtool/$(MAMEOS)/%.res: mess/tools/imgtool/$(MAMEOS)/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir mess/tools/imgtool/$(MAMEOS) -o $@ -i $<
