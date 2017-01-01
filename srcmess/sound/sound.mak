###########################################################################
#
#   sound.mak
#
#   Rules for building sound cores
#
#   Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


#-------------------------------------------------
# Core sound types
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_CUSTOM=$(if $(filter CUSTOM,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_SAMPLES=$(if $(filter SAMPLES,$(MESSSOUNDS)),1,0)

ifneq ($(filter CUSTOM,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/custom.o
endif

ifneq ($(filter SAMPLES,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/samples.o
endif



#-------------------------------------------------
# DACs
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_DAC=$(if $(filter DAC,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_DMADAC=$(if $(filter DMADAC,$(MESSSOUNDS)),1,0)

ifneq ($(filter DAC,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/dac.o
endif

ifneq ($(filter DMADAC,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/dmadac.o
endif



#-------------------------------------------------
# CD audio
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_CDDA=$(if $(filter CDDA,$(MESSSOUNDS)),1,0)

ifneq ($(filter CDDA,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/cdda.o
endif



#-------------------------------------------------
# Discrete component audio
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_DISCRETE=$(if $(filter DISCRETE,$(MESSSOUNDS)),1,0)

ifneq ($(filter DISCRETE,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/discrete.o
$(MESSOBJ)/sound/discrete.o: discrete.c discrete.h \
		disc_dev.c disc_flt.c disc_inp.c \
		disc_mth.c disc_wav.c
endif



#-------------------------------------------------
# Atari custom sound chips
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_POKEY=$(if $(filter POKEY,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_TIA=$(if $(filter TIA,$(MESSSOUNDS)),1,0)

ifneq ($(filter POKEY,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/pokey.o
endif

ifneq ($(filter TIA,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/tiasound.o $(MESSOBJ)/sound/tiaintf.o
endif



#-------------------------------------------------
# Bally Astrocade sound system
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_ASTROCADE=$(if $(filter ASTROCADE,$(MESSSOUNDS)),1,0)

ifneq ($(filter ASTROCADE,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/astrocde.o
endif



#-------------------------------------------------
# CEM 3394 analog synthesizer chip
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_CEM3394=$(if $(filter CEM3394,$(MESSSOUNDS)),1,0)

ifneq ($(filter CEM3394,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/cem3394.o
endif



#-------------------------------------------------
# Data East custom sound chips
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_BSMT2000=$(if $(filter BSMT2000,$(MESSSOUNDS)),1,0)

ifneq ($(filter BSMT2000,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/bsmt2000.o
endif



#-------------------------------------------------
# Ensoniq 5503 (Apple IIgs)
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_ES5503=$(if $(filter ES5503,$(MESSSOUNDS)),1,0)

ifneq ($(filter ES5503,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/es5503.o
endif



#-------------------------------------------------
# Ensoniq 5505/5506
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_ES5505=$(if $(filter ES5505,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_ES5506=$(if $(filter ES5506,$(MESSSOUNDS)),1,0)

ifneq ($(filter ES5505 ES5506,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/es5506.o
endif



#-------------------------------------------------
# Excellent Systems ADPCM sound chip
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_ES8712=$(if $(filter ES8712,$(MESSSOUNDS)),1,0)

ifneq ($(filter ES8712,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/es8712.o
endif



#-------------------------------------------------
# Gaelco custom sound chips
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_GAELCO_CG1V=$(if $(filter GAELCO_CG1V,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_GAELCO_GAE1=$(if $(filter GAELCO_GAE1,$(MESSSOUNDS)),1,0)

ifneq ($(filter GAELCO_CG1V,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/gaelco.o
endif

ifneq ($(filter GAELCO_GAE1,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/gaelco.o
endif



#-------------------------------------------------
# GI AY-8910
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_AY8910=$(if $(filter AY8910,$(MESSSOUNDS)),1,0)

ifneq ($(filter AY8910,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/ay8910.o
endif



#-------------------------------------------------
# Harris HC55516 CVSD
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_HC55516=$(if $(filter HC55516,$(MESSSOUNDS)),1,0)

ifneq ($(filter HC55516,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/hc55516.o
endif



#-------------------------------------------------
# Hudsonsoft C6280 sound chip
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_C6280=$(if $(filter C6280,$(MESSSOUNDS)),1,0)

ifneq ($(filter C6280,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/c6280.o
endif



#-------------------------------------------------
# ICS2115 sound chip
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_ICS2115=$(if $(filter ICS2115,$(MESSSOUNDS)),1,0)

ifneq ($(filter ICS2115,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/ics2115.o
endif



#-------------------------------------------------
# Irem custom sound chips
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_IREMGA20=$(if $(filter IREMGA20,$(MESSSOUNDS)),1,0)

ifneq ($(filter IREMGA20,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/iremga20.o
endif



#-------------------------------------------------
# Konami custom sound chips
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_K005289=$(if $(filter K005289,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_K007232=$(if $(filter K007232,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_K051649=$(if $(filter K051649,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_K053260=$(if $(filter K053260,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_K054539=$(if $(filter K054539,$(MESSSOUNDS)),1,0)

ifneq ($(filter K005289,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/k005289.o
endif

ifneq ($(filter K007232,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/k007232.o
endif

ifneq ($(filter K051649,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/k051649.o
endif

ifneq ($(filter K053260,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/k053260.o
endif

ifneq ($(filter K054539,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/k054539.o
endif



#-------------------------------------------------
# Namco custom sound chips
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_NAMCO=$(if $(filter NAMCO,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_NAMCO_15XX=$(if $(filter NAMCO_15XX,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_NAMCO_CUS30=$(if $(filter NAMCO_CUS30,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_NAMCO_52XX=$(if $(filter NAMCO_52XX,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_NAMCO_54XX=$(if $(filter NAMCO_54XX,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_NAMCO_63701X=$(if $(filter NAMCO_63701X,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_NAMCONA=$(if $(filter NAMCONA,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_C140=$(if $(filter C140,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_C352=$(if $(filter C352,$(MESSSOUNDS)),1,0)

ifneq ($(filter NAMCO NAMCO_15XX NAMCO_CUS30,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/namco.o
endif

ifneq ($(filter NAMCO_52XX,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/namco52.o
endif

ifneq ($(filter NAMCO_54XX,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/namco54.o
endif

ifneq ($(filter NAMCO_63701X,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/n63701x.o
endif

ifneq ($(filter NAMCONA,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/namcona.o
endif

ifneq ($(filter C140,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/c140.o
endif

ifneq ($(filter C352,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/c352.o
endif



#-------------------------------------------------
# Nintendo custom sound chips
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_NES=$(if $(filter NES,$(MESSSOUNDS)),1,0)

ifneq ($(filter NES,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/nes_apu.o
endif



#-------------------------------------------------
# NEC uPD7759 ADPCM sample player
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_UPD7759=$(if $(filter UPD7759,$(MESSSOUNDS)),1,0)

ifneq ($(filter UPD7759,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/upd7759.o
endif



#-------------------------------------------------
# OKI ADPCM sample players
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_MSM5205=$(if $(filter MSM5205,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_MSM5232=$(if $(filter MSM5232,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_OKIM6295=$(if $(filter OKIM6295,$(MESSSOUNDS)),1,0)

ifneq ($(filter MSM5205,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/msm5205.o
endif

ifneq ($(filter MSM5232,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/msm5232.o
endif

ifneq ($(filter OKIM6295,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/okim6295.o
endif



#-------------------------------------------------
# Philips SAA1099
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_SAA1099=$(if $(filter SAA1099,$(MESSSOUNDS)),1,0)

ifneq ($(filter SAA1099,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/saa1099.o
endif



#-------------------------------------------------
# QSound sample player
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_QSOUND=$(if $(filter QSOUND,$(MESSSOUNDS)),1,0)

ifneq ($(filter QSOUND,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/qsound.o
endif



#-------------------------------------------------
# Ricoh sample players
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_RF5C68=$(if $(filter RF5C68,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_RF5C400=$(if $(filter RF5C400,$(MESSSOUNDS)),1,0)

ifneq ($(filter RF5C68,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/rf5c68.o
endif

ifneq ($(filter RF5C400,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/rf5c400.o
endif



#-------------------------------------------------
# Sega custom sound chips
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_SEGAPCM=$(if $(filter SEGAPCM,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_MULTIPCM=$(if $(filter MULTIPCM,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_SCSP=$(if $(filter SCSP,$(MESSSOUNDS)),1,0)

ifneq ($(filter SEGAPCM,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/segapcm.o
endif

ifneq ($(filter MULTIPCM,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/multipcm.o
endif

ifneq ($(filter SCSP,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/scsp.o
endif



#-------------------------------------------------
# Seta custom sound chips
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_ST0016=$(if $(filter ST0016,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_X1_010=$(if $(filter X1_010,$(MESSSOUNDS)),1,0)

ifneq ($(filter ST0016,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/st0016.o
endif

ifneq ($(filter X1_010,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/x1_010.o
endif



#-------------------------------------------------
# Sony custom sound chips
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_PSXSPU=$(if $(filter PSXSPU,$(MESSSOUNDS)),1,0)

ifneq ($(filter PSXSPU,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/psx.o
endif



#-------------------------------------------------
# SP0250 speech synthesizer
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_SP0250=$(if $(filter SP0250,$(MESSSOUNDS)),1,0)

ifneq ($(filter SP0250,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/sp0250.o
endif



#-------------------------------------------------
# Texas Instruments SN76477 analog chip
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_SN76477=$(if $(filter SN76477,$(MESSSOUNDS)),1,0)

ifneq ($(filter SN76477,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/sn76477.o
endif



#-------------------------------------------------
# Texas Instruments SN76496
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_SN76496=$(if $(filter SN76496,$(MESSSOUNDS)),1,0)

ifneq ($(filter SN76496,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/sn76496.o
endif



#-------------------------------------------------
# Texas Instruments TMS36xx doorbell chime
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_TMS36XX=$(if $(filter TMS36XX,$(MESSSOUNDS)),1,0)

ifneq ($(filter TMS36XX,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/tms36xx.o
endif



#-------------------------------------------------
# Texas Instruments TMS5110 speech synthesizers
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_TMS5110=$(if $(filter TMS5110,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_TMS5220=$(if $(filter TMS5220,$(MESSSOUNDS)),1,0)

ifneq ($(filter TMS5110,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/tms5110.o $(MESSOBJ)/sound/5110intf.o
endif

ifneq ($(filter TMS5220,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/tms5220.o $(MESSOBJ)/sound/5220intf.o
endif



#-------------------------------------------------
# VLM5030 speech synthesizer
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_VLM5030=$(if $(filter VLM5030,$(MESSSOUNDS)),1,0)

ifneq ($(filter VLM5030,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/vlm5030.o
endif



#-------------------------------------------------
# Votrax speech synthesizer
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_VOTRAX=$(if $(filter VOTRAX,$(MESSSOUNDS)),1,0)

ifneq ($(filter VOTRAX,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/votrax.o
endif



#-------------------------------------------------
# VRender0 custom sound chip
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_VRENDER0=$(if $(filter VRENDER0,$(MESSSOUNDS)),1,0)

ifneq ($(filter VRENDER0,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/vrender0.o
endif



#-------------------------------------------------
# Yamaha FM synthesizers
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_YM2151=$(if $(filter YM2151,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_YM2203=$(if $(filter YM2203,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_YM2413=$(if $(filter YM2413,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_YM2608=$(if $(filter YM2608,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_YM2610=$(if $(filter YM2610,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_YM2610B=$(if $(filter YM2610B,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_YM2612=$(if $(filter YM2612,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_YM3438=$(if $(filter YM3438,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_YM3812=$(if $(filter YM3812,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_YM3526=$(if $(filter YM3526,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_Y8950=$(if $(filter Y8950,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_YMF262=$(if $(filter YMF262,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_YMF271=$(if $(filter YMF271,$(MESSSOUNDS)),1,0)
MESSSOUNDDEFS += -DHAS_YMF278B=$(if $(filter YMF278B,$(MESSSOUNDS)),1,0)

ifneq ($(filter YM2151,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/2151intf.o $(MESSOBJ)/sound/ym2151.o
endif

ifneq ($(filter YM2203,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/2203intf.o $(MESSOBJ)/sound/ay8910.o $(MESSOBJ)/sound/fm.o
endif

ifneq ($(filter YM2413,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/2413intf.o $(MESSOBJ)/sound/ym2413.o
endif

ifneq ($(filter YM2608,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/2608intf.o $(MESSOBJ)/sound/ay8910.o $(MESSOBJ)/sound/fm.o $(MESSOBJ)/sound/ymdeltat.o
endif

ifneq ($(filter YM2610 YM2610B,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/2610intf.o $(MESSOBJ)/sound/ay8910.o $(MESSOBJ)/sound/fm.o $(MESSOBJ)/sound/ymdeltat.o
endif

ifneq ($(filter YM2612 YM3438,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/2612intf.o $(MESSOBJ)/sound/ay8910.o $(MESSOBJ)/sound/fm.o
endif

ifneq ($(filter YM3812 YM3526,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/3812intf.o $(MESSOBJ)/sound/fmopl.o
endif

ifneq ($(filter Y8950,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/3812intf.o $(MESSOBJ)/sound/fmopl.o $(MESSOBJ)/sound/ymdeltat.o
endif

ifneq ($(filter YMF262,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/ymf262.o $(MESSOBJ)/sound/262intf.o
endif

ifneq ($(filter YMF271,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/ymf271.o
endif

ifneq ($(filter YMF278B,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/ymf278b.o
endif



#-------------------------------------------------
# Yamaha YMZ280B ADPCM
#-------------------------------------------------

MESSSOUNDDEFS += -DHAS_YMZ280B=$(if $(filter YMZ280B,$(MESSSOUNDS)),1,0)

ifneq ($(filter YMZ280B,$(MESSSOUNDS)),)
MESSSOUNDOBJS += $(MESSOBJ)/sound/ymz280b.o
endif
