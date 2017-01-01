# Rules for MESS CPU's

ifneq ($(filter COP411,$(MESSCPUS)),)
COP411D = mess/cpu/cop411
MESSOBJDIRS += $(MESSOBJ)/$(COP411D)
MESSCPUDEFS += -DHAS_COP411=1
MESSCPUOBJS += $(MESSOBJ)/$(COP411D)/cop411.o
MESSDBGOBJS += $(MESSOBJ)/$(COP411D)/cop411ds.o
$(MESSOBJ)/$(COP411D)/cop411.o: $(COP411D)/cop411.c $(COP411D)/cop411.h
else
MESSCPUDEFS += -DHAS_COP411=0
endif

ifneq ($(filter APEXC,$(MESSCPUS)),)
APEXCD = mess/cpu/apexc
MESSOBJDIRS += $(MESSOBJ)/$(APEXCD)
MESSCPUDEFS += -DHAS_APEXC=1
MESSCPUOBJS += $(MESSOBJ)/$(APEXCD)/apexc.o
MESSDBGOBJS += $(MESSOBJ)/$(APEXCD)/apexcdsm.o
$(MESSOBJ)/$(APEXCD)/apexc.o: $(APEXCD)/apexc.c $(APEXCD)/apexc.h
else
MESSCPUDEFS += -DHAS_APEXC=0
endif


ifneq ($(filter CDP1802,$(MESSCPUS)),)
CDPD = mess/cpu/cdp1802
MESSOBJDIRS += $(MESSOBJ)/$(CDPD)
MESSCPUDEFS += -DHAS_CDP1802=1
MESSCPUOBJS += $(MESSOBJ)/$(CDPD)/cdp1802.o
MESSDBGOBJS += $(MESSOBJ)/$(CDPD)/1802dasm.o
$(MESSOBJ)/$(CDPD)/cdp1802.o: $(CDPD)/1802tbl.c
else
MESSCPUDEFS += -DHAS_CDP1802=0
endif


ifneq ($(filter CP1610,$(MESSCPUS)),)
CPD = mess/cpu/cp1610
MESSOBJDIRS += $(MESSOBJ)/$(CPD)
MESSCPUDEFS += -DHAS_CP1610=1
MESSCPUOBJS += $(MESSOBJ)/$(CPD)/cp1610.o
MESSDBGOBJS += $(MESSOBJ)/$(CPD)/1610dasm.o
$(MESSOBJ)/$(CPD)/cp1610.o: $(CPD)/cp1610.c $(CPD)/cp1610.h
else
MESSCPUDEFS += -DHAS_CP1610=0
endif


ifneq ($(filter F8,$(MESSCPUS)),)
F8D = mess/cpu/f8
MESSOBJDIRS += $(MESSOBJ)/$(F8D)
MESSCPUDEFS += -DHAS_F8=1
MESSCPUOBJS += $(MESSOBJ)/$(F8D)/f8.o
MESSDBGOBJS += $(MESSOBJ)/$(F8D)/f8dasm.o
$(MESSOBJ)/$(F8D)/f8.o: $(F8D)/f8.c $(F8D)/f8.h
else
MESSCPUDEFS += -DHAS_F8=0
endif


ifneq ($(filter LH5801,$(MESSCPUS)),)
LHD = mess/cpu/lh5801
MESSOBJDIRS += $(MESSOBJ)/$(LHD)
MESSCPUDEFS += -DHAS_LH5801=1
MESSCPUOBJS += $(MESSOBJ)/$(LHD)/lh5801.o
MESSDBGOBJS += $(MESSOBJ)/$(LHD)/5801dasm.o
$(MESSOBJ)/$(LHD)/lh5801.o: $(LHD)/lh5801.c $(LHD)/5801tbl.c $(LHD)/lh5801.h
else
MESSCPUDEFS += -DHAS_LH5801=0
endif


ifneq ($(filter PDP1,$(MESSCPUS)),)
PDPD = mess/cpu/pdp1
MESSOBJDIRS += $(MESSOBJ)/$(PDPD)
MESSCPUDEFS += -DHAS_PDP1=1
MESSCPUOBJS += $(MESSOBJ)/$(PDPD)/pdp1.o
MESSDBGOBJS += $(MESSOBJ)/$(PDPD)/pdp1dasm.o
$(MESSOBJ)/$(PDPD)/pdp1.o: $(PDPD)/pdp1.c $(PDPD)/pdp1.h
else
MESSCPUDEFS += -DHAS_PDP1=0
endif


ifneq ($(filter SATURN,$(MESSCPUS)),)
SATD = mess/cpu/saturn
MESSOBJDIRS += $(MESSOBJ)/$(SATD)
MESSCPUDEFS += -DHAS_SATURN=1
MESSCPUOBJS += $(MESSOBJ)/$(SATD)/saturn.o
MESSDBGOBJS += $(MESSOBJ)/$(SATD)/saturnds.o
$(MESSOBJ)/$(SATD)/saturn.o: $(SATD)/saturn.c $(SATD)/sattable.c $(SATD)/satops.c $(SATD)/saturn.h $(SATD)/sat.h
else
MESSCPUDEFS += -DHAS_SATURN=0
endif


ifneq ($(filter SC61860,$(MESSCPUS)),)
SCD = mess/cpu/sc61860
MESSOBJDIRS += $(MESSOBJ)/$(SCD)
MESSCPUDEFS += -DHAS_SC61860=1
MESSCPUOBJS += $(MESSOBJ)/$(SCD)/sc61860.o
MESSDBGOBJS += $(MESSOBJ)/$(SCD)/scdasm.o
$(MESSOBJ)/$(SCD)/sc61860.o: $(SCD)/sc61860.h  $(SCD)/sc.h $(SCD)/scops.c $(SCD)/sctable.c
else
MESSCPUDEFS += -DHAS_SC61860=0
endif


ifneq ($(filter Z80GB,$(MESSCPUS)),)
GBD = mess/cpu/z80gb
MESSOBJDIRS += $(MESSOBJ)/$(GBD)
MESSCPUDEFS += -DHAS_Z80GB=1
MESSCPUOBJS += $(MESSOBJ)/$(GBD)/z80gb.o
MESSDBGOBJS += $(MESSOBJ)/$(GBD)/z80gbd.o
$(MESSOBJ)/$(GBD)/z80gb.o: $(GBD)/z80gb.c $(GBD)/z80gb.h $(GBD)/daa_tab.h $(GBD)/opc_cb.h $(GBD)/opc_main.h
else
MESSCPUDEFS += -DHAS_Z80GB=0
endif

ifneq ($(filter TMS7000,$(MESSCPUS)),)
TM7D = mess/cpu/tms7000
MESSOBJDIRS += $(MESSOBJ)/$(TM7D)
MESSCPUDEFS += -DHAS_TMS7000=1
MESSCPUOBJS += $(MESSOBJ)/$(TM7D)/tms7000.o
MESSDBGOBJS += $(MESSOBJ)/$(TM7D)/7000dasm.o
$(MESSOBJ)/$(TM7D)/tms7000.o:	$(TM7D)/tms7000.h $(TM7D)/tms7000.c
$(MESSOBJ)/$(TM7D)/7000dasm.o:	$(TM7D)/tms7000.h $(TM7D)/7000dasm.c
else
MESSCPUDEFS += -DHAS_TMS7000=0
endif

ifneq ($(filter TMS7000_EXL,$(MESSCPUS)),)
TM7D = mess/cpu/tms7000
MESSOBJDIRS += $(MESSOBJ)/$(TM7D)
MESSCPUDEFS += -DHAS_TMS7000_EXL=1
MESSCPUOBJS += $(MESSOBJ)/$(TM7D)/tms7000.o
MESSDBGOBJS += $(MESSOBJ)/$(TM7D)/7000dasm.o
$(MESSOBJ)/$(TM7D)/tms7000.o:	$(TM7D)/tms7000.h $(TM7D)/tms7000.c
$(MESSOBJ)/$(TM7D)/7000dasm.o:	$(TM7D)/tms7000.h $(TM7D)/7000dasm.c
else
MESSCPUDEFS += -DHAS_TMS7000_EXL=0
endif

ifneq ($(filter TX0,$(MESSCPUS)),)
TX0D = mess/cpu/pdp1
MESSOBJDIRS += $(MESSOBJ)/$(TX0D)
MESSCPUDEFS += -DHAS_TX0_64KW=1 -DHAS_TX0_8KW=1
MESSCPUOBJS += $(MESSOBJ)/$(TX0D)/tx0.o
MESSDBGOBJS += $(MESSOBJ)/$(TX0D)/tx0dasm.o
$(MESSOBJ)/$(TX0D)/tx0.o:		$(TX0D)/tx0.h $(TX0D)/tx0.c
$(MESSOBJ)/$(TX0D)/tx0dasm.o:	$(TX0D)/tx0.h $(TX0D)/tx0dasm.c
else
MESSCPUDEFS += -DHAS_TX0_64KW=0 -DHAS_TX0_8KW=0
endif

ifneq ($(filter SM8500,$(MESSCPUS)),)
SM8500D = mess/cpu/sm8500
MESSOBJDIRS += $(MESSOBJ)/$(SM8500D)
MESSCPUDEFS += -DHAS_SM8500=1
MESSCPUOBJS += $(MESSOBJ)/$(SM8500D)/sm8500.o
MESSDBGOBJS += $(MESSOBJ)/$(SM8500D)/sm8500d.o
$(MESSOBJ)/$(SM8500D)/sm8500.o: $(SM8500D)/sm8500.c $(SM8500D)/sm8500.h $(SM8500D)/sm85ops.h
else
MESSCPUDEFS += -DHAS_SM8500=0
endif

ifneq ($(filter V30MZ,$(MESSCPUS)),)
V30MZD = mess/cpu/v30mz
MESSOBJDIRS += $(MESSOBJ)/$(V30MZD)
MESSCPUDEFS += -DHAS_V30MZ=1
MESSCPUOBJS += $(MESSOBJ)/$(V30MZD)/v30mz.o
MESSDBGOBJS += $(MESSOBJ)/cpu/i386/i386dasm.o
$(MESSOBJ)/$(V30MZD)/v30mz.o:       $(V30MZD)/v30mz.c $(V30MZD)/v30mz.h $(V30MZD)/necmodrm.h $(V30MZD)/necinstr.h $(V30MZD)/necea.h $(V30MZD)/nechost.h $(V30MZD)/necintrf.h
else
MESSCPUDEFS += -DHAS_V30MZ=0
endif

