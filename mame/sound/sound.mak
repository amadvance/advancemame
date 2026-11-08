# license:BSD-3-Clause
# copyright-holders:Aaron Giles
# Shared rules: call with variable prefix, object directory and target name.
# Escape ordinary make references so each instantiation retains its own variables.
define EMU_SOUND
###########################################################################
#
#   sound.mak
#
#   Rules for building sound cores
#
###########################################################################


#-------------------------------------------------
# Core sound types
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_CUSTOM=$$(if $$(filter CUSTOM,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_SAMPLES=$$(if $$(filter SAMPLES,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter CUSTOM,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/custom.o
# This additional CUSTOM device is selected by the arcade build only.
ifeq ($(3),mame)
$(1)SOUNDOBJS += $(2)/sound/s14001a.o
endif
endif

ifneq ($$(filter SAMPLES,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/samples.o
endif



#-------------------------------------------------
# DACs
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_DAC=$$(if $$(filter DAC,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_DMADAC=$$(if $$(filter DMADAC,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter DAC,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/dac.o
endif

ifneq ($$(filter DMADAC,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/dmadac.o
endif



#-------------------------------------------------
# CD audio
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_CDDA=$$(if $$(filter CDDA,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter CDDA,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/cdda.o
endif



#-------------------------------------------------
# Discrete component audio
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_DISCRETE=$$(if $$(filter DISCRETE,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter DISCRETE,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/discrete.o
$(2)/sound/discrete.o: discrete.c discrete.h \
		disc_dev.c disc_flt.c disc_inp.c \
		disc_mth.c disc_wav.c mw_discrete.h mw_discrete.inc \
		mw_disc_dev.c mw_disc_flt.c mw_disc_mth.c mw_disc_wav.c
endif



#-------------------------------------------------
# Atari custom sound chips
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_POKEY=$$(if $$(filter POKEY,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_TIA=$$(if $$(filter TIA,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter POKEY,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/pokey.o
endif

ifneq ($$(filter TIA,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/tiasound.o $(2)/sound/tiaintf.o
endif



#-------------------------------------------------
# Bally Astrocade sound system
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_ASTROCADE=$$(if $$(filter ASTROCADE,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter ASTROCADE,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/astrocde.o
endif



#-------------------------------------------------
# CEM 3394 analog synthesizer chip
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_CEM3394=$$(if $$(filter CEM3394,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter CEM3394,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/cem3394.o
endif



#-------------------------------------------------
# Data East custom sound chips
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_BSMT2000=$$(if $$(filter BSMT2000,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter BSMT2000,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/bsmt2000.o
endif



#-------------------------------------------------
# Ensoniq 5503 (Apple IIgs)
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_ES5503=$$(if $$(filter ES5503,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter ES5503,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/es5503.o
endif



#-------------------------------------------------
# Ensoniq 5505/5506
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_ES5505=$$(if $$(filter ES5505,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_ES5506=$$(if $$(filter ES5506,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter ES5505 ES5506,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/es5506.o
endif



#-------------------------------------------------
# Excellent Systems ADPCM sound chip
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_ES8712=$$(if $$(filter ES8712,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter ES8712,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/es8712.o
endif



#-------------------------------------------------
# Gaelco custom sound chips
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_GAELCO_CG1V=$$(if $$(filter GAELCO_CG1V,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_GAELCO_GAE1=$$(if $$(filter GAELCO_GAE1,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter GAELCO_CG1V,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/gaelco.o
endif

ifneq ($$(filter GAELCO_GAE1,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/gaelco.o
endif



#-------------------------------------------------
# GI AY-8910
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_AY8910=$$(if $$(filter AY8910,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter AY8910,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/ay8910.o
endif



#-------------------------------------------------
# Harris HC55516 CVSD
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_HC55516=$$(if $$(filter HC55516,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter HC55516,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/hc55516.o
endif



#-------------------------------------------------
# Hudsonsoft C6280 sound chip
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_C6280=$$(if $$(filter C6280,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter C6280,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/c6280.o
endif



#-------------------------------------------------
# ICS2115 sound chip
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_ICS2115=$$(if $$(filter ICS2115,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter ICS2115,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/ics2115.o
endif



#-------------------------------------------------
# Irem custom sound chips
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_IREMGA20=$$(if $$(filter IREMGA20,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter IREMGA20,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/iremga20.o
endif



#-------------------------------------------------
# Konami custom sound chips
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_K005289=$$(if $$(filter K005289,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_K007232=$$(if $$(filter K007232,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_K051649=$$(if $$(filter K051649,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_K053260=$$(if $$(filter K053260,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_K054539=$$(if $$(filter K054539,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter K005289,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/k005289.o
endif

ifneq ($$(filter K007232,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/k007232.o
endif

ifneq ($$(filter K051649,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/k051649.o
endif

ifneq ($$(filter K053260,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/k053260.o
endif

ifneq ($$(filter K054539,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/k054539.o
endif



#-------------------------------------------------
# Namco custom sound chips
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_NAMCO=$$(if $$(filter NAMCO,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_NAMCO_15XX=$$(if $$(filter NAMCO_15XX,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_NAMCO_CUS30=$$(if $$(filter NAMCO_CUS30,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_NAMCO_52XX=$$(if $$(filter NAMCO_52XX,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_NAMCO_54XX=$$(if $$(filter NAMCO_54XX,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_NAMCO_63701X=$$(if $$(filter NAMCO_63701X,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_NAMCONA=$$(if $$(filter NAMCONA,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_C140=$$(if $$(filter C140,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_C352=$$(if $$(filter C352,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter NAMCO NAMCO_15XX NAMCO_CUS30,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/namco.o
endif

ifneq ($$(filter NAMCO_52XX,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/namco52.o
endif

ifneq ($$(filter NAMCO_54XX,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/namco54.o
endif

ifneq ($$(filter NAMCO_63701X,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/n63701x.o
endif

ifneq ($$(filter NAMCONA,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/namcona.o
endif

ifneq ($$(filter C140,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/c140.o
endif

ifneq ($$(filter C352,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/c352.o
endif



#-------------------------------------------------
# Nintendo custom sound chips
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_NES=$$(if $$(filter NES,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter NES,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/nes_apu.o
endif



#-------------------------------------------------
# NEC uPD7759 ADPCM sample player
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_UPD7759=$$(if $$(filter UPD7759,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter UPD7759,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/upd7759.o
endif



#-------------------------------------------------
# OKI ADPCM sample players
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_MSM5205=$$(if $$(filter MSM5205,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_MSM5232=$$(if $$(filter MSM5232,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_OKIM6295=$$(if $$(filter OKIM6295,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter MSM5205,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/msm5205.o
endif

ifneq ($$(filter MSM5232,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/msm5232.o
endif

ifneq ($$(filter OKIM6295,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/okim6295.o
endif



#-------------------------------------------------
# Philips SAA1099
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_SAA1099=$$(if $$(filter SAA1099,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter SAA1099,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/saa1099.o
endif



#-------------------------------------------------
# QSound sample player
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_QSOUND=$$(if $$(filter QSOUND,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter QSOUND,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/qsound.o
endif



#-------------------------------------------------
# Ricoh sample players
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_RF5C68=$$(if $$(filter RF5C68,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_RF5C400=$$(if $$(filter RF5C400,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter RF5C68,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/rf5c68.o
endif

ifneq ($$(filter RF5C400,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/rf5c400.o
endif



#-------------------------------------------------
# Sega custom sound chips
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_SEGAPCM=$$(if $$(filter SEGAPCM,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_MULTIPCM=$$(if $$(filter MULTIPCM,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_SCSP=$$(if $$(filter SCSP,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter SEGAPCM,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/segapcm.o
endif

ifneq ($$(filter MULTIPCM,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/multipcm.o
endif

ifneq ($$(filter SCSP,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/scsp.o
endif



#-------------------------------------------------
# Seta custom sound chips
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_ST0016=$$(if $$(filter ST0016,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_X1_010=$$(if $$(filter X1_010,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter ST0016,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/st0016.o
endif

ifneq ($$(filter X1_010,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/x1_010.o
endif



#-------------------------------------------------
# Sony custom sound chips
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_PSXSPU=$$(if $$(filter PSXSPU,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter PSXSPU,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/psx.o
endif



#-------------------------------------------------
# SP0250 speech synthesizer
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_SP0250=$$(if $$(filter SP0250,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter SP0250,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/sp0250.o
endif



#-------------------------------------------------
# Texas Instruments SN76477 analog chip
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_SN76477=$$(if $$(filter SN76477,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter SN76477,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/sn76477.o
endif



#-------------------------------------------------
# Texas Instruments SN76496
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_SN76496=$$(if $$(filter SN76496,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter SN76496,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/sn76496.o
endif



#-------------------------------------------------
# Texas Instruments TMS36xx doorbell chime
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_TMS36XX=$$(if $$(filter TMS36XX,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter TMS36XX,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/tms36xx.o
endif



#-------------------------------------------------
# Texas Instruments TMS5110 speech synthesizers
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_TMS5110=$$(if $$(filter TMS5110,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_TMS5220=$$(if $$(filter TMS5220,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter TMS5110,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/tms5110.o $(2)/sound/5110intf.o
endif

ifneq ($$(filter TMS5220,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/tms5220.o $(2)/sound/5220intf.o
endif



#-------------------------------------------------
# VLM5030 speech synthesizer
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_VLM5030=$$(if $$(filter VLM5030,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter VLM5030,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/vlm5030.o
endif



#-------------------------------------------------
# Votrax speech synthesizer
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_VOTRAX=$$(if $$(filter VOTRAX,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter VOTRAX,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/votrax.o
endif



#-------------------------------------------------
# VRender0 custom sound chip
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_VRENDER0=$$(if $$(filter VRENDER0,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter VRENDER0,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/vrender0.o
endif



#-------------------------------------------------
# Yamaha FM synthesizers
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_YM2151=$$(if $$(filter YM2151,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_YM2203=$$(if $$(filter YM2203,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_YM2413=$$(if $$(filter YM2413,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_YM2608=$$(if $$(filter YM2608,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_YM2610=$$(if $$(filter YM2610,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_YM2610B=$$(if $$(filter YM2610B,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_YM2612=$$(if $$(filter YM2612,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_YM3438=$$(if $$(filter YM3438,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_YM3812=$$(if $$(filter YM3812,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_YM3526=$$(if $$(filter YM3526,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_Y8950=$$(if $$(filter Y8950,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_YMF262=$$(if $$(filter YMF262,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_YMF271=$$(if $$(filter YMF271,$$($(1)SOUNDS)),1,0)
$(1)SOUNDDEFS += -DHAS_YMF278B=$$(if $$(filter YMF278B,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter YM2151,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/2151intf.o $(2)/sound/ym2151.o
endif

ifneq ($$(filter YM2203,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/2203intf.o $(2)/sound/ay8910.o $(2)/sound/fm.o
endif

ifneq ($$(filter YM2413,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/2413intf.o $(2)/sound/ym2413.o
endif

ifneq ($$(filter YM2608,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/2608intf.o $(2)/sound/ay8910.o $(2)/sound/fm.o $(2)/sound/ymdeltat.o
endif

ifneq ($$(filter YM2610 YM2610B,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/2610intf.o $(2)/sound/ay8910.o $(2)/sound/fm.o $(2)/sound/ymdeltat.o
endif

ifneq ($$(filter YM2612 YM3438,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/2612intf.o $(2)/sound/ay8910.o $(2)/sound/fm.o
endif

ifneq ($$(filter YM3812 YM3526,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/3812intf.o $(2)/sound/fmopl.o
endif

ifneq ($$(filter Y8950,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/3812intf.o $(2)/sound/fmopl.o $(2)/sound/ymdeltat.o
endif

ifneq ($$(filter YMF262,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/ymf262.o $(2)/sound/262intf.o
endif

ifneq ($$(filter YMF271,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/ymf271.o
endif

ifneq ($$(filter YMF278B,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/ymf278b.o
endif



#-------------------------------------------------
# Yamaha YMZ280B ADPCM
#-------------------------------------------------

$(1)SOUNDDEFS += -DHAS_YMZ280B=$$(if $$(filter YMZ280B,$$($(1)SOUNDS)),1,0)

ifneq ($$(filter YMZ280B,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/ymz280b.o
endif

$(1)SOUNDDEFS += -DHAS_CDP1869=$$(if $$(filter CDP1869,$$($(1)SOUNDS)),1,0)
ifneq ($$(filter CDP1869,$$($(1)SOUNDS)),)
$(1)SOUNDOBJS += $(2)/sound/cdp1869.o
$(2)/sound/cdp1869.o: mame/sound/cdp1869.h
endif
endef
