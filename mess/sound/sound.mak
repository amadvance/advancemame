ifneq ($(filter BEEP,$(MESSSOUNDS)),)
MESSSOUNDDEFS += -DHAS_BEEP=1
MESSSOUNDOBJS += $(MESSOBJ)/mess/sound/beep.o
else
MESSSOUNDDEFS += -DHAS_BEEP=0
endif

ifneq ($(filter SPEAKER,$(MESSSOUNDS)),)
MESSSOUNDDEFS += -DHAS_SPEAKER=1
MESSSOUNDOBJS += $(MESSOBJ)/mess/sound/speaker.o
else
MESSSOUNDDEFS += -DHAS_SPEAKER=0
endif

ifneq ($(filter WAVE,$(MESSSOUNDS)),)
MESSSOUNDDEFS += -DHAS_WAVE=1
MESSSOUNDOBJS += $(MESSOBJ)/mess/sound/wave.o
else
MESSSOUNDDEFS += -DHAS_WAVE=0
endif

ifneq ($(filter SID6581,$(MESSSOUNDS)),)
MESSSOUNDDEFS += -DHAS_SID6581=1
MESSSOUNDOBJS += $(MESSOBJ)/mess/sound/sid6581.o $(MESSOBJ)/mess/sound/sid.o $(MESSOBJ)/mess/sound/sidenvel.o $(MESSOBJ)/mess/sound/sidvoice.o
else
MESSSOUNDDEFS += -DHAS_SID6581=0
endif

ifneq ($(filter SID8580,$(MESSSOUNDS)),)
MESSSOUNDDEFS += -DHAS_SID8580=1
MESSSOUNDOBJS += $(MESSOBJ)/mess/sound/sid6581.o $(MESSOBJ)/mess/sound/sid.o $(MESSOBJ)/mess/sound/sidenvel.o $(MESSOBJ)/mess/sound/sidvoice.o
else
MESSSOUNDDEFS += -DHAS_SID8580=0
endif
