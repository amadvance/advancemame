# Rules for MESS CPU's

ifneq ($(filter COP411,$(CPUS)),)
COP411D = mess/cpu/cop411
OBJDIRS += $(OBJ)/$(COP411D)
CPUDEFS += -DHAS_COP411=1
CPUOBJS += $(OBJ)/$(COP411D)/cop411.o
DBGOBJS += $(OBJ)/$(COP411D)/cop411ds.o
$(OBJ)/$(COP411D)/cop411.o: $(COP411D)/cop411.c $(COP411D)/cop411.h
else
CPUDEFS += -DHAS_COP411=0
endif

ifneq ($(filter APEXC,$(CPUS)),)
APEXCD = mess/cpu/apexc
OBJDIRS += $(OBJ)/$(APEXCD)
CPUDEFS += -DHAS_APEXC=1
CPUOBJS += $(OBJ)/$(APEXCD)/apexc.o
DBGOBJS += $(OBJ)/$(APEXCD)/apexcdsm.o
$(OBJ)/$(APEXCD)/apexc.o: $(APEXCD)/apexc.c $(APEXCD)/apexc.h
else
CPUDEFS += -DHAS_APEXC=0
endif


ifneq ($(filter CDP1802,$(CPUS)),)
CDPD = mess/cpu/cdp1802
OBJDIRS += $(OBJ)/$(CDPD)
CPUDEFS += -DHAS_CDP1802=1
CPUOBJS += $(OBJ)/$(CDPD)/cdp1802.o
DBGOBJS += $(OBJ)/$(CDPD)/1802dasm.o
$(OBJ)/$(CDPD)/cdp1802.o: $(CDPD)/1802tbl.c
else
CPUDEFS += -DHAS_CDP1802=0
endif


ifneq ($(filter CP1610,$(CPUS)),)
CPD = mess/cpu/cp1610
OBJDIRS += $(OBJ)/$(CPD)
CPUDEFS += -DHAS_CP1610=1
CPUOBJS += $(OBJ)/$(CPD)/cp1610.o
DBGOBJS += $(OBJ)/$(CPD)/1610dasm.o
$(OBJ)/$(CPD)/cp1610.o: $(CPD)/cp1610.c $(CPD)/cp1610.h
else
CPUDEFS += -DHAS_CP1610=0
endif


ifneq ($(filter F8,$(CPUS)),)
F8D = mess/cpu/f8
OBJDIRS += $(OBJ)/$(F8D)
CPUDEFS += -DHAS_F8=1
CPUOBJS += $(OBJ)/$(F8D)/f8.o
DBGOBJS += $(OBJ)/$(F8D)/f8dasm.o
$(OBJ)/$(F8D)/f8.o: $(F8D)/f8.c $(F8D)/f8.h
else
CPUDEFS += -DHAS_F8=0
endif


ifneq ($(filter LH5801,$(CPUS)),)
LHD = mess/cpu/lh5801
OBJDIRS += $(OBJ)/$(LHD)
CPUDEFS += -DHAS_LH5801=1
CPUOBJS += $(OBJ)/$(LHD)/lh5801.o
DBGOBJS += $(OBJ)/$(LHD)/5801dasm.o
$(OBJ)/$(LHD)/lh5801.o: $(LHD)/lh5801.c $(LHD)/5801tbl.c $(LHD)/lh5801.h
else
CPUDEFS += -DHAS_LH5801=0
endif


ifneq ($(filter PDP1,$(CPUS)),)
PDPD = mess/cpu/pdp1
OBJDIRS += $(OBJ)/$(PDPD)
CPUDEFS += -DHAS_PDP1=1
CPUOBJS += $(OBJ)/$(PDPD)/pdp1.o
DBGOBJS += $(OBJ)/$(PDPD)/pdp1dasm.o
$(OBJ)/$(PDPD)/pdp1.o: $(PDPD)/pdp1.c $(PDPD)/pdp1.h
else
CPUDEFS += -DHAS_PDP1=0
endif


ifneq ($(filter SATURN,$(CPUS)),)
SATD = mess/cpu/saturn
OBJDIRS += $(OBJ)/$(SATD)
CPUDEFS += -DHAS_SATURN=1
CPUOBJS += $(OBJ)/$(SATD)/saturn.o
DBGOBJS += $(OBJ)/$(SATD)/saturnds.o
$(OBJ)/$(SATD)/saturn.o: $(SATD)/saturn.c $(SATD)/sattable.c $(SATD)/satops.c $(SATD)/saturn.h $(SATD)/sat.h
else
CPUDEFS += -DHAS_SATURN=0
endif


ifneq ($(filter SC61860,$(CPUS)),)
SCD = mess/cpu/sc61860
OBJDIRS += $(OBJ)/$(SCD)
CPUDEFS += -DHAS_SC61860=1
CPUOBJS += $(OBJ)/$(SCD)/sc61860.o
DBGOBJS += $(OBJ)/$(SCD)/scdasm.o
$(OBJ)/$(SCD)/sc61860.o: $(SCD)/sc61860.h  $(SCD)/sc.h $(SCD)/scops.c $(SCD)/sctable.c
else
CPUDEFS += -DHAS_SC61860=0
endif


ifneq ($(filter Z80GB,$(CPUS)),)
GBD = mess/cpu/z80gb
OBJDIRS += $(OBJ)/$(GBD)
CPUDEFS += -DHAS_Z80GB=1
CPUOBJS += $(OBJ)/$(GBD)/z80gb.o
DBGOBJS += $(OBJ)/$(GBD)/z80gbd.o
$(OBJ)/$(GBD)/z80gb.o: $(GBD)/z80gb.c $(GBD)/z80gb.h $(GBD)/daa_tab.h $(GBD)/opc_cb.h $(GBD)/opc_main.h
else
CPUDEFS += -DHAS_Z80GB=0
endif

ifneq ($(filter TMS7000,$(CPUS)),)
TM7D = mess/cpu/tms7000
OBJDIRS += $(OBJ)/$(TM7D)
CPUDEFS += -DHAS_TMS7000=1
CPUOBJS += $(OBJ)/$(TM7D)/tms7000.o
DBGOBJS += $(OBJ)/$(TM7D)/7000dasm.o
$(OBJ)/$(TM7D)/tms7000.o:	$(TM7D)/tms7000.h $(TM7D)/tms7000.c
$(OBJ)/$(TM7D)/7000dasm.o:	$(TM7D)/tms7000.h $(TM7D)/7000dasm.c
else
CPUDEFS += -DHAS_TMS7000=0
endif

ifneq ($(filter TMS7000_EXL,$(CPUS)),)
TM7D = mess/cpu/tms7000
OBJDIRS += $(OBJ)/$(TM7D)
CPUDEFS += -DHAS_TMS7000_EXL=1
CPUOBJS += $(OBJ)/$(TM7D)/tms7000.o
DBGOBJS += $(OBJ)/$(TM7D)/7000dasm.o
$(OBJ)/$(TM7D)/tms7000.o:	$(TM7D)/tms7000.h $(TM7D)/tms7000.c
$(OBJ)/$(TM7D)/7000dasm.o:	$(TM7D)/tms7000.h $(TM7D)/7000dasm.c
else
CPUDEFS += -DHAS_TMS7000_EXL=0
endif

ifneq ($(filter TX0,$(CPUS)),)
TX0D = mess/cpu/pdp1
OBJDIRS += $(OBJ)/$(TX0D)
CPUDEFS += -DHAS_TX0_64KW=1 -DHAS_TX0_8KW=1
CPUOBJS += $(OBJ)/$(TX0D)/tx0.o
DBGOBJS += $(OBJ)/$(TX0D)/tx0dasm.o
$(OBJ)/$(TX0D)/tx0.o:		$(TX0D)/tx0.h $(TX0D)/tx0.c
$(OBJ)/$(TX0D)/tx0dasm.o:	$(TX0D)/tx0.h $(TX0D)/tx0dasm.c
else
CPUDEFS += -DHAS_TX0_64KW=0 -DHAS_TX0_8KW=0
endif

ifneq ($(filter SM8500,$(CPUS)),)
SM8500D = mess/cpu/sm8500
OBJDIRS += $(OBJ)/$(SM8500D)
CPUDEFS += -DHAS_SM8500=1
CPUOBJS += $(OBJ)/$(SM8500D)/sm8500.o
DBGOBJS += $(OBJ)/$(SM8500D)/sm8500d.o
$(OBJ)/$(SM8500D)/sm8500.o: $(SM8500D)/sm8500.c $(SM8500D)/sm8500.h $(SM8500D)/sm85ops.h
else
CPUDEFS += -DHAS_SM8500=0
endif

ifneq ($(filter V30MZ,$(CPUS)),)
V30MZD = mess/cpu/v30mz
OBJDIRS += $(OBJ)/$(V30MZD)
CPUDEFS += -DHAS_V30MZ=1
CPUOBJS += $(OBJ)/$(V30MZD)/v30mz.o
DBGOBJS += $(OBJ)/cpu/i386/i386dasm.o
$(OBJ)/$(V30MZD)/v30mz.o:       $(V30MZD)/v30mz.c $(V30MZD)/v30mz.h $(V30MZD)/necmodrm.h $(V30MZD)/necinstr.h $(V30MZD)/necea.h $(V30MZD)/nechost.h $(V30MZD)/necintrf.h
else
CPUDEFS += -DHAS_V30MZ=0
endif

