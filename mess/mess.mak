# this is a MESS build
MESS = 1

# core defines
MESSCOREDEFS += -DNEOFREE -DMESS



#-------------------------------------------------
# specify available CPU cores; some of these are
# only for MAME and so aren't included
#-------------------------------------------------

MESSCPUS+=Z80
MESSCPUS+=Z180
MESSCPUS+=8080
#MESSCPUS+=8085A
MESSCPUS+=M6502
MESSCPUS+=M65C02
MESSCPUS+=M65SC02
#MESSCPUS+=M65CE02
MESSCPUS+=M6509
MESSCPUS+=M6510
MESSCPUS+=M6510T
MESSCPUS+=M7501
MESSCPUS+=M8502
MESSCPUS+=N2A03
#MESSCPUS+=DECO16
MESSCPUS+=M4510
MESSCPUS+=H6280
MESSCPUS+=I86
MESSCPUS+=I88
MESSCPUS+=I186
#MESSCPUS+=I188
MESSCPUS+=I286
MESSCPUS+=V20
MESSCPUS+=V30
#MESSCPUS+=V33
#MESSCPUS+=V60
#MESSCPUS+=V70
#MESSCPUS+=I8035
MESSCPUS+=I8039
MESSCPUS+=I8048
#MESSCPUS+=N7751
#MESSCPUS+=I8X41
#MESSCPUS+=I8051
#MESSCPUS+=I8052
#MESSCPUS+=I8751
#MESSCPUS+=I8752
MESSCPUS+=M6800
#MESSCPUS+=M6801
#MESSCPUS+=M6802
MESSCPUS+=M6803
MESSCPUS+=M6808
MESSCPUS+=HD63701
MESSCPUS+=NSC8105
MESSCPUS+=M6805
#MESSCPUS+=M68705
#MESSCPUS+=HD63705
MESSCPUS+=HD6309
MESSCPUS+=M6809
MESSCPUS+=M6809E
#MESSCPUS+=KONAMI
MESSCPUS+=M68000
MESSCPUS+=M68008
MESSCPUS+=M68010
MESSCPUS+=M68EC020
MESSCPUS+=M68020
#MESSCPUS+=T11
MESSCPUS+=S2650
#MESSCPUS+=TMS34010
#MESSCPUS+=TMS34020
MESSCPUS+=TMS9900
#MESSCPUS+=TMS9940
MESSCPUS+=TMS9980
#MESSCPUS+=TMS9985
#MESSCPUS+=TMS9989
MESSCPUS+=TMS9995
#MESSCPUS+=TMS99000
MESSCPUS+=TMS99010
#MESSCPUS+=TMS99105A
#MESSCPUS+=TMS99110A
#MESSCPUS+=Z8000
#MESSCPUS+=TMS32010
#MESSCPUS+=TMS32025
#MESSCPUS+=TMS32026
#MESSCPUS+=TMS32031
#MESSCPUS+=CCPU
#MESSCPUS+=ADSP2100
#MESSCPUS+=ADSP2101
#MESSCPUS+=ADSP2104
#MESSCPUS+=ADSP2105
#MESSCPUS+=ADSP2115
#MESSCPUS+=ADSP2181
MESSCPUS+=PSXCPU
#MESSCPUS+=ASAP
#MESSCPUS+=UPD7810
#MESSCPUS+=UPD7807
MESSCPUS+=ARM
#MESSCPUS+=ARM7
MESSCPUS+=JAGUAR
#MESSCPUS+=R3000
MESSCPUS+=R4600
#MESSCPUS+=R4700
MESSCPUS+=R5000
#MESSCPUS+=QED5271
#MESSCPUS+=RM7000
MESSCPUS+=SH2
#MESSCPUS+=DSP32C
#MESSCPUS+=PIC16C54
#MESSCPUS+=PIC16C55
#MESSCPUS+=PIC16C56
#MESSCPUS+=PIC16C57
#MESSCPUS+=PIC16C58
MESSCPUS+=G65816
MESSCPUS+=SPC700
MESSCPUS+=E116T
#MESSCPUS+=E116XT
#MESSCPUS+=E116XS
#MESSCPUS+=E116XSR
#MESSCPUS+=E132N
#MESSCPUS+=E132T
#MESSCPUS+=E132XN
#MESSCPUS+=E132XT
#MESSCPUS+=E132XS
#MESSCPUS+=E132XSR
#MESSCPUS+=GMS30C2116
#MESSCPUS+=GMS30C2132
#MESSCPUS+=GMS30C2216
#MESSCPUS+=GMS30C2232
MESSCPUS+=I386
MESSCPUS+=I486
MESSCPUS+=PENTIUM
#MESSCPUS+=MEDIAGX
#MESSCPUS+=I960
#MESSCPUS+=H83002
#MESSCPUS+=V810
#MESSCPUS+=M37710
#MESSCPUS+=PPC403
#MESSCPUS+=PPC602
MESSCPUS+=PPC603
#MESSCPUS+=SE3208
#MESSCPUS+=MC68HC11
#MESSCPUS += ADSP21062
#MESSCPUS += DSP56156
MESSCPUS += RSP
MESSCPUS+=Z80GB
MESSCPUS+=CDP1802
MESSCPUS+=SC61860
MESSCPUS+=SATURN
MESSCPUS+=APEXC
MESSCPUS+=F8
MESSCPUS+=CP1610
#MESSCPUS+=TMS99010
MESSCPUS+=PDP1
#MESSCPUS+=TMS7000
MESSCPUS+=TMS7000_EXL
MESSCPUS+=TX0
MESSCPUS+=COP411
MESSCPUS+=SM8500
MESSCPUS+=V30MZ

# SOUND cores used in MESS
MESSSOUNDS+=CUSTOM
MESSSOUNDS+=SAMPLES
MESSSOUNDS+=DAC
MESSSOUNDS+=DMADAC
MESSSOUNDS+=DISCRETE
MESSSOUNDS+=AY8910
MESSSOUNDS+=YM2203
# enable only one of the following two
MESSSOUNDS+=YM2151
MESSSOUNDS+=YM2151_ALT
MESSSOUNDS+=YM2608
MESSSOUNDS+=YM2610
MESSSOUNDS+=YM2610B
MESSSOUNDS+=YM2612
#MESSSOUNDS+=YM3438
MESSSOUNDS+=YM2413
MESSSOUNDS+=YM3812
#MESSSOUNDS+=YMZ280B
#MESSSOUNDS+=YM3526
#MESSSOUNDS+=Y8950
MESSSOUNDS+=SN76477
MESSSOUNDS+=SN76496
MESSSOUNDS+=POKEY
MESSSOUNDS+=TIA
MESSSOUNDS+=NES
MESSSOUNDS+=ASTROCADE
#MESSSOUNDS+=NAMCO
#MESSSOUNDS+=NAMCO_15XX
#MESSSOUNDS+=NAMCO_CUS30
#MESSSOUNDS+=NAMCO_52XX
#MESSSOUNDS+=NAMCO_54XX
#MESSSOUNDS+=NAMCO_63701X
#MESSSOUNDS+=NAMCONA
#MESSSOUNDS+=TMS36XX
MESSSOUNDS+=TMS5110
MESSSOUNDS+=TMS5220
#MESSSOUNDS+=VLM5030
#MESSSOUNDS+=ADPCM
MESSSOUNDS+=OKIM6295
#MESSSOUNDS+=MSM5205
#MESSSOUNDS+=MSM5232
#MESSSOUNDS+=UPD7759
#MESSSOUNDS+=HC55516
#MESSSOUNDS+=K005289
#MESSSOUNDS+=K007232
MESSSOUNDS+=K051649
#MESSSOUNDS+=K053260
#MESSSOUNDS+=K054539
#MESSSOUNDS+=SEGAPCM
#MESSSOUNDS+=RF5C68
#MESSSOUNDS+=CEM3394
#MESSSOUNDS+=C140
MESSSOUNDS+=QSOUND
MESSSOUNDS+=SAA1099
#MESSSOUNDS+=IREMGA20
#MESSSOUNDS+=ES5505
#MESSSOUNDS+=ES5506
#MESSSOUNDS+=BSMT2000
#MESSSOUNDS+=YMF262
#MESSSOUNDS+=YMF278B
#MESSSOUNDS+=GAELCO_CG1V
#MESSSOUNDS+=GAELCO_GAE1
#MESSSOUNDS+=X1_010
#MESSSOUNDS+=MULTIPCM
MESSSOUNDS+=C6280
#MESSSOUNDS+=SP0250
#MESSSOUNDS+=SCSP
#MESSSOUNDS+=YMF271
MESSSOUNDS+=PSXSPU
#MESSSOUNDS+=CDDA
#MESSSOUNDS+=ICS2115
#MESSSOUNDS+=ST0016
#MESSSOUNDS+=C352
#MESSSOUNDS+=VRENDER0
MESSSOUNDS+=SPEAKER
MESSSOUNDS+=WAVE
MESSSOUNDS+=BEEP
MESSSOUNDS+=SID6581
MESSSOUNDS+=SID8580
MESSSOUNDS+=ES5503

# Archive definitions
MESSDRVLIBS = \
	$(MESSOBJ)/coco.a     \
	$(MESSOBJ)/mc10.a     \
	$(MESSOBJ)/apple.a    \
	$(MESSOBJ)/apexc.a	  \
	$(MESSOBJ)/pdp1.a	  \
	$(MESSOBJ)/sony.a     \
	$(MESSOBJ)/nintendo.a \
	$(MESSOBJ)/pc.a       \
	$(MESSOBJ)/at.a       \
	$(MESSOBJ)/pcshare.a  \
	$(MESSOBJ)/ti99.a     \
	$(MESSOBJ)/amstrad.a  \
	$(MESSOBJ)/sega.a     \
	$(MESSOBJ)/acorn.a    \
	$(MESSOBJ)/atari.a    \
	$(MESSOBJ)/trs80.a	  \
	$(MESSOBJ)/fairch.a   \
	$(MESSOBJ)/bally.a	  \
	$(MESSOBJ)/advision.a \
	$(MESSOBJ)/mbee.a	  \
	$(MESSOBJ)/vtech.a	  \
	$(MESSOBJ)/jupiter.a  \
	$(MESSOBJ)/gce.a	  \
	$(MESSOBJ)/arcadia.a  \
	$(MESSOBJ)/kaypro.a   \
	$(MESSOBJ)/cgenie.a   \
	$(MESSOBJ)/aquarius.a \
	$(MESSOBJ)/tangerin.a \
	$(MESSOBJ)/sord.a     \
	$(MESSOBJ)/exidy.a    \
	$(MESSOBJ)/samcoupe.a \
	$(MESSOBJ)/p2000.a	  \
	$(MESSOBJ)/tatung.a   \
	$(MESSOBJ)/ep128.a	  \
	$(MESSOBJ)/cpschngr.a \
	$(MESSOBJ)/veb.a	  \
	$(MESSOBJ)/nec.a	  \
	$(MESSOBJ)/nascom1.a  \
	$(MESSOBJ)/magnavox.a \
	$(MESSOBJ)/mk1.a      \
	$(MESSOBJ)/mk2.a      \
	$(MESSOBJ)/ti85.a     \
	$(MESSOBJ)/galaxy.a   \
	$(MESSOBJ)/vc4000.a   \
	$(MESSOBJ)/lviv.a     \
	$(MESSOBJ)/pmd85.a    \
	$(MESSOBJ)/sinclair.a \
	$(MESSOBJ)/lynx.a     \
	$(MESSOBJ)/svision.a  \
	$(MESSOBJ)/coleco.a   \
	$(MESSOBJ)/apf.a      \
	$(MESSOBJ)/teamconc.a \
	$(MESSOBJ)/concept.a  \
	$(MESSOBJ)/amiga.a    \
	$(MESSOBJ)/svi.a      \
	$(MESSOBJ)/tutor.a    \
	$(MESSOBJ)/sharp.a    \
	$(MESSOBJ)/aim65.a    \
	$(MESSOBJ)/avigo.a    \
	$(MESSOBJ)/motorola.a	\
	$(MESSOBJ)/ssystem3.a	\
	$(MESSOBJ)/hp48.a		\
	$(MESSOBJ)/cbm.a		\
	$(MESSOBJ)/cbmshare.a	\
	$(MESSOBJ)/kim1.a		\
	$(MESSOBJ)/sym1.a		\
	$(MESSOBJ)/dai.a		\
	$(MESSOBJ)/bandai.a		\
	$(MESSOBJ)/compis.a		\
	$(MESSOBJ)/necpc.a		\
	$(MESSOBJ)/ascii.a		\
	$(MESSOBJ)/mtx.a		\
	$(MESSOBJ)/intv.a		\
	$(MESSOBJ)/rca.a		\
	$(MESSOBJ)/multitch.a	\
	$(MESSOBJ)/telmac.a		\
	$(MESSOBJ)/tx0.a		\
	$(MESSOBJ)/luxor.a		\
	$(MESSOBJ)/sgi.a		\
	$(MESSOBJ)/primo.a		\
	$(MESSOBJ)/dgn_beta.a	\
	$(MESSOBJ)/be.a			\
	$(MESSOBJ)/tiger.a		\


$(MESSOBJ)/neocd.a:						\
	$(MESSOBJ)/mess/systems/neocd.o		\
	$(MESSOBJ)/machine/neogeo.o			\
	$(MESSOBJ)/vidhrdw/neogeo.o			\
	$(MESSOBJ)/machine/pd4990a.o		\

$(MESSOBJ)/coleco.a:   \
	$(MESSOBJ)/mess/machine/coleco.o	\
	$(MESSOBJ)/mess/systems/coleco.o	\
	$(MESSOBJ)/mess/machine/adam.o		\
	$(MESSOBJ)/mess/systems/adam.o		\
	$(MESSOBJ)/mess/formats/adam_dsk.o	\
	$(MESSOBJ)/mess/systems/fnvision.o	\
	

$(MESSOBJ)/arcadia.a:  \
	$(MESSOBJ)/mess/systems/arcadia.o	\
	$(MESSOBJ)/mess/sndhrdw/arcadia.o	\
	$(MESSOBJ)/mess/vidhrdw/arcadia.o	\

$(MESSOBJ)/sega.a:						\
	$(MESSOBJ)/mess/machine/genesis.o	\
	$(MESSOBJ)/mess/systems/genesis.o	\
	$(MESSOBJ)/mess/systems/saturn.o	\
	$(MESSOBJ)/machine/stvcd.o			\
	$(MESSOBJ)/machine/scudsp.o			\
	$(MESSOBJ)/vidhrdw/stvvdp1.o		\
	$(MESSOBJ)/vidhrdw/stvvdp2.o		\
	$(MESSOBJ)/sound/scsp.o				\
	$(MESSOBJ)/mess/vidhrdw/smsvdp.o	\
	$(MESSOBJ)/mess/machine/sms.o		\
	$(MESSOBJ)/mess/systems/sms.o

$(MESSOBJ)/atari.a:						\
	$(MESSOBJ)/vidhrdw/tia.o			\
	$(MESSOBJ)/machine/atari.o			\
	$(MESSOBJ)/vidhrdw/atari.o			\
	$(MESSOBJ)/vidhrdw/antic.o			\
	$(MESSOBJ)/vidhrdw/gtia.o			\
	$(MESSOBJ)/mess/systems/atari.o		\
	$(MESSOBJ)/mess/machine/a7800.o		\
	$(MESSOBJ)/mess/systems/a7800.o		\
	$(MESSOBJ)/mess/vidhrdw/a7800.o		\
	$(MESSOBJ)/mess/systems/a2600.o		\
	$(MESSOBJ)/mess/systems/jaguar.o	\
	$(MESSOBJ)/sndhrdw/jaguar.o			\
	$(MESSOBJ)/vidhrdw/jaguar.o			\
#	$(MESSOBJ)/mess/systems/atarist.o

$(MESSOBJ)/gce.a:	                     \
	$(MESSOBJ)/mess/systems/vectrex.o	\
	$(MESSOBJ)/mess/vidhrdw/vectrex.o	\
	$(MESSOBJ)/mess/machine/vectrex.o	\

$(MESSOBJ)/nintendo.a:					\
	$(MESSOBJ)/mess/sndhrdw/gb.o		\
	$(MESSOBJ)/mess/vidhrdw/gb.o		\
	$(MESSOBJ)/mess/machine/gb.o		\
	$(MESSOBJ)/mess/systems/gb.o		\
	$(MESSOBJ)/mess/machine/nes_mmc.o	\
	$(MESSOBJ)/vidhrdw/ppu2c03b.o		\
	$(MESSOBJ)/mess/vidhrdw/nes.o		\
	$(MESSOBJ)/mess/machine/nes.o		\
	$(MESSOBJ)/mess/systems/nes.o		\
	$(MESSOBJ)/sndhrdw/snes.o			\
	$(MESSOBJ)/machine/snes.o			\
	$(MESSOBJ)/vidhrdw/snes.o			\
	$(MESSOBJ)/mess/systems/snes.o	 	\
	$(MESSOBJ)/mess/systems/n64.o		\
	$(MESSOBJ)/machine/n64.o			\
	$(MESSOBJ)/vidhrdw/n64.o			\

$(MESSOBJ)/amiga.a: \
	$(MESSOBJ)/vidhrdw/amiga.o			\
	$(MESSOBJ)/machine/amiga.o			\
	$(MESSOBJ)/sndhrdw/amiga.o			\
	$(MESSOBJ)/machine/6526cia.o		\
	$(MESSOBJ)/mess/machine/amigafdc.o	\
	$(MESSOBJ)/mess/systems/amiga.o

$(MESSOBJ)/cbmshare.a: \
	$(MESSOBJ)/machine/6526cia.o		\
	$(MESSOBJ)/mess/machine/tpi6525.o	\
	$(MESSOBJ)/mess/machine/cbm.o		\
	$(MESSOBJ)/mess/machine/cbmdrive.o	\
	$(MESSOBJ)/mess/machine/vc1541.o	 \
	$(MESSOBJ)/mess/machine/cbmieeeb.o \
	$(MESSOBJ)/mess/machine/cbmserb.o  \
	$(MESSOBJ)/mess/machine/c64.o      \
	$(MESSOBJ)/mess/machine/c65.o		\
	$(MESSOBJ)/mess/vidhrdw/vic6567.o	 \
	$(MESSOBJ)/mess/machine/vc20tape.o

$(MESSOBJ)/cbm.a: \
	$(MESSOBJ)/mess/vidhrdw/pet.o		\
	$(MESSOBJ)/mess/systems/pet.o		\
	$(MESSOBJ)/mess/machine/pet.o		\
	$(MESSOBJ)/mess/systems/c64.o		\
	$(MESSOBJ)/mess/machine/vc20.o		\
	$(MESSOBJ)/mess/systems/vc20.o		\
	$(MESSOBJ)/mess/sndhrdw/ted7360.o	\
	$(MESSOBJ)/mess/sndhrdw/t6721.o		\
	$(MESSOBJ)/mess/machine/c16.o		\
	$(MESSOBJ)/mess/systems/c16.o		\
	$(MESSOBJ)/mess/systems/cbmb.o		\
	$(MESSOBJ)/mess/machine/cbmb.o		\
	$(MESSOBJ)/mess/vidhrdw/cbmb.o		\
	$(MESSOBJ)/mess/systems/c65.o		\
	$(MESSOBJ)/mess/vidhrdw/vdc8563.o	\
	$(MESSOBJ)/mess/systems/c128.o		\
	$(MESSOBJ)/mess/machine/c128.o		\
	$(MESSOBJ)/mess/sndhrdw/vic6560.o	\
	$(MESSOBJ)/mess/vidhrdw/ted7360.o	\
	$(MESSOBJ)/mess/vidhrdw/vic6560.o  

$(MESSOBJ)/coco.a:   \
	$(MESSOBJ)/mess/machine/6883sam.o	\
	$(MESSOBJ)/mess/machine/cococart.o	\
	$(MESSOBJ)/mess/machine/ds1315.o	\
	$(MESSOBJ)/mess/machine/m6242b.o	\
	$(MESSOBJ)/mess/machine/coco.o		\
	$(MESSOBJ)/mess/vidhrdw/coco.o		\
	$(MESSOBJ)/mess/systems/coco.o		\
	$(MESSOBJ)/mess/vidhrdw/coco3.o		\
	$(MESSOBJ)/mess/formats/cocopak.o	\
	$(MESSOBJ)/mess/formats/coco_cas.o	\
	$(MESSOBJ)/mess/formats/coco_dsk.o	\
	$(MESSOBJ)/mess/devices/coco_vhd.o	\

$(MESSOBJ)/mc10.a:	\
	$(MESSOBJ)/mess/machine/mc10.o		\
	$(MESSOBJ)/mess/systems/mc10.o		\
	$(MESSOBJ)/mess/formats/coco_cas.o	\

$(MESSOBJ)/dgn_beta.a:	\
	$(MESSOBJ)/mess/machine/dgn_beta.o	\
	$(MESSOBJ)/mess/vidhrdw/dgn_beta.o	\
	$(MESSOBJ)/mess/systems/dgn_beta.o	\
	$(MESSOBJ)/mess/formats/coco_dsk.o	\

$(MESSOBJ)/trs80.a:    \
	$(MESSOBJ)/mess/machine/trs80.o	 \
	$(MESSOBJ)/mess/vidhrdw/trs80.o	 \
	$(MESSOBJ)/mess/systems/trs80.o

$(MESSOBJ)/cgenie.a:   \
	$(MESSOBJ)/mess/systems/cgenie.o	\
	$(MESSOBJ)/mess/vidhrdw/cgenie.o	 \
	$(MESSOBJ)/mess/sndhrdw/cgenie.o	 \
	$(MESSOBJ)/mess/machine/cgenie.o	 \

$(MESSOBJ)/pdp1.a:	   \
	$(MESSOBJ)/mess/vidhrdw/pdp1.o	\
	$(MESSOBJ)/mess/machine/pdp1.o	\
	$(MESSOBJ)/mess/systems/pdp1.o	\

$(MESSOBJ)/apexc.a:     \
	$(MESSOBJ)/mess/systems/apexc.o

$(MESSOBJ)/kaypro.a:   \
	$(MESSOBJ)/mess/systems/kaypro.o	\
	$(MESSOBJ)/mess/machine/cpm_bios.o	\
	$(MESSOBJ)/mess/vidhrdw/kaypro.o	 \
	$(MESSOBJ)/mess/sndhrdw/kaypro.o	 \
	$(MESSOBJ)/mess/machine/kaypro.o	 \

$(MESSOBJ)/sinclair.a: \
	$(MESSOBJ)/mess/vidhrdw/border.o		\
	$(MESSOBJ)/mess/vidhrdw/spectrum.o		\
	$(MESSOBJ)/mess/vidhrdw/zx.o		\
	$(MESSOBJ)/mess/systems/zx.o		\
	$(MESSOBJ)/mess/machine/zx.o		\
	$(MESSOBJ)/mess/systems/spectrum.o		\
	$(MESSOBJ)/mess/machine/spectrum.o		\
	$(MESSOBJ)/mess/formats/zx81_p.o		\
	$(MESSOBJ)/mess/systems/ql.o		\

$(MESSOBJ)/apple.a:   \
	$(MESSOBJ)/mess/vidhrdw/apple2.o		\
	$(MESSOBJ)/mess/machine/apple2.o		\
	$(MESSOBJ)/mess/systems/apple2.o		\
	$(MESSOBJ)/mess/vidhrdw/apple2gs.o		\
	$(MESSOBJ)/mess/machine/apple2gs.o		\
	$(MESSOBJ)/mess/systems/apple2gs.o		\
	$(MESSOBJ)/mess/formats/ap2_dsk.o		\
	$(MESSOBJ)/mess/formats/ap_dsk35.o		\
	$(MESSOBJ)/mess/machine/ay3600.o		\
	$(MESSOBJ)/mess/machine/lisa.o			\
	$(MESSOBJ)/mess/systems/lisa.o			\
	$(MESSOBJ)/mess/machine/applefdc.o		\
	$(MESSOBJ)/mess/machine/8530scc.o		\
	$(MESSOBJ)/mess/devices/sonydriv.o		\
	$(MESSOBJ)/mess/devices/appldriv.o		\
	$(MESSOBJ)/mess/sndhrdw/mac.o			\
	$(MESSOBJ)/mess/vidhrdw/mac.o			\
	$(MESSOBJ)/mess/machine/mac.o			\
	$(MESSOBJ)/mess/systems/mac.o			\
	$(MESSOBJ)/mess/vidhrdw/apple1.o		\
	$(MESSOBJ)/mess/machine/apple1.o		\
	$(MESSOBJ)/mess/systems/apple1.o		\
	$(MESSOBJ)/mess/vidhrdw/apple3.o		\
	$(MESSOBJ)/mess/machine/apple3.o		\
	$(MESSOBJ)/mess/systems/apple3.o		\
	$(MESSOBJ)/mess/machine/ncr5380.o


$(MESSOBJ)/avigo.a: \
	$(MESSOBJ)/mess/vidhrdw/avigo.o		\
	$(MESSOBJ)/mess/systems/avigo.o		\

$(MESSOBJ)/ti85.a: \
	$(MESSOBJ)/mess/systems/ti85.o		\
	$(MESSOBJ)/mess/formats/ti85_ser.o	\
	$(MESSOBJ)/mess/vidhrdw/ti85.o		\
	$(MESSOBJ)/mess/machine/ti85.o		\

$(MESSOBJ)/rca.a: \
	$(MESSOBJ)/mess/systems/studio2.o  \
	$(MESSOBJ)/mess/vidhrdw/studio2.o  

$(MESSOBJ)/fairch.a: \
	$(MESSOBJ)/mess/vidhrdw/channelf.o \
	$(MESSOBJ)/mess/sndhrdw/channelf.o \
	$(MESSOBJ)/mess/systems/channelf.o 

$(MESSOBJ)/ti99.a:	   \
	$(MESSOBJ)/mess/machine/tms9901.o	\
	$(MESSOBJ)/mess/machine/tms9902.o	\
	$(MESSOBJ)/mess/machine/ti99_4x.o	\
	$(MESSOBJ)/mess/machine/990_hd.o	\
	$(MESSOBJ)/mess/machine/990_tap.o	\
	$(MESSOBJ)/mess/machine/ti990.o		\
	$(MESSOBJ)/mess/machine/mm58274c.o	\
	$(MESSOBJ)/mess/machine/994x_ser.o	\
	$(MESSOBJ)/mess/machine/at29040.o	\
	$(MESSOBJ)/mess/machine/99_dsk.o	\
	$(MESSOBJ)/mess/machine/99_ide.o	\
	$(MESSOBJ)/mess/machine/99_peb.o	\
	$(MESSOBJ)/mess/machine/99_hsgpl.o	\
	$(MESSOBJ)/mess/machine/99_usbsm.o	\
	$(MESSOBJ)/mess/machine/smc92x4.o	\
	$(MESSOBJ)/mess/machine/strata.o	\
	$(MESSOBJ)/mess/machine/rtc65271.o	\
	$(MESSOBJ)/mess/machine/geneve.o	\
	$(MESSOBJ)/mess/machine/990_dk.o	\
	$(MESSOBJ)/mess/sndhrdw/spchroms.o	\
	$(MESSOBJ)/mess/systems/ti990_4.o	\
	$(MESSOBJ)/mess/systems/ti99_4x.o	\
	$(MESSOBJ)/mess/systems/ti99_4p.o	\
	$(MESSOBJ)/mess/systems/geneve.o	\
	$(MESSOBJ)/mess/systems/tm990189.o	\
	$(MESSOBJ)/mess/systems/ti99_8.o	\
	$(MESSOBJ)/mess/vidhrdw/911_vdt.o	\
	$(MESSOBJ)/mess/vidhrdw/733_asr.o	\
	$(MESSOBJ)/mess/systems/ti990_10.o	\
	$(MESSOBJ)/mess/systems/ti99_2.o	\
	$(MESSOBJ)/mess/systems/tutor.o		\

$(MESSOBJ)/tutor.a:   \
	$(MESSOBJ)/mess/systems/tutor.o

$(MESSOBJ)/bally.a:    \
	$(MESSOBJ)/sound/astrocde.o	 \
	$(MESSOBJ)/mess/vidhrdw/astrocde.o \
	$(MESSOBJ)/mess/machine/astrocde.o \
	$(MESSOBJ)/mess/systems/astrocde.o

$(MESSOBJ)/pcshare.a:					\
	$(MESSOBJ)/machine/8237dma.o	\
	$(MESSOBJ)/machine/pic8259.o	\
	$(MESSOBJ)/machine/pcshare.o	\
	$(MESSOBJ)/mess/machine/pc_turbo.o	\
	$(MESSOBJ)/mess/sndhrdw/pc.o		\
	$(MESSOBJ)/mess/sndhrdw/sblaster.o	\
	$(MESSOBJ)/mess/machine/pc_fdc.o	\
	$(MESSOBJ)/mess/machine/pc_hdc.o	\
	$(MESSOBJ)/mess/machine/pc_joy.o	\
	$(MESSOBJ)/mess/vidhrdw/pc_video.o	\
	$(MESSOBJ)/mess/vidhrdw/pc_mda.o	\
	$(MESSOBJ)/mess/vidhrdw/pc_cga.o	\
	$(MESSOBJ)/mess/vidhrdw/cgapal.o	\
	$(MESSOBJ)/mess/vidhrdw/pc_vga.o	\

$(MESSOBJ)/pc.a:	   \
	$(MESSOBJ)/mess/vidhrdw/pc_aga.o	 \
	$(MESSOBJ)/mess/machine/ibmpc.o	 \
	$(MESSOBJ)/mess/machine/tandy1t.o  \
	$(MESSOBJ)/mess/machine/amstr_pc.o \
	$(MESSOBJ)/mess/machine/europc.o	 \
	$(MESSOBJ)/mess/machine/pc.o       \
	$(MESSOBJ)/mess/systems/pc.o		\
	$(MESSOBJ)/mess/vidhrdw/pc_t1t.o	 

$(MESSOBJ)/at.a:	   \
	$(MESSOBJ)/machine/8042kbdc.o    \
	$(MESSOBJ)/mess/machine/pc_ide.o   \
	$(MESSOBJ)/mess/machine/ps2.o	 \
	$(MESSOBJ)/mess/machine/at.o       \
	$(MESSOBJ)/mess/systems/at.o	\
	$(MESSOBJ)/mess/machine/i82439tx.o

$(MESSOBJ)/p2000.a:    \
	$(MESSOBJ)/mess/vidhrdw/saa5050.o  \
	$(MESSOBJ)/mess/vidhrdw/p2000m.o	 \
	$(MESSOBJ)/mess/systems/p2000t.o	 \
	$(MESSOBJ)/mess/machine/p2000t.o	 \
	$(MESSOBJ)/mess/machine/mc6850.o	 \
	$(MESSOBJ)/mess/vidhrdw/osi.o	 \
	$(MESSOBJ)/mess/sndhrdw/osi.o	 \
	$(MESSOBJ)/mess/systems/osi.o	\
	$(MESSOBJ)/mess/machine/osi.o	 \

$(MESSOBJ)/amstrad.a:  \
	$(MESSOBJ)/mess/systems/amstrad.o  \
	$(MESSOBJ)/mess/machine/amstrad.o  \
	$(MESSOBJ)/mess/vidhrdw/amstrad.o  \
	$(MESSOBJ)/mess/vidhrdw/pcw.o	 \
	$(MESSOBJ)/mess/systems/pcw.o	 \
	$(MESSOBJ)/mess/systems/pcw16.o	 \
	$(MESSOBJ)/mess/vidhrdw/pcw16.o	 \
	$(MESSOBJ)/mess/vidhrdw/nc.o	 \
	$(MESSOBJ)/mess/systems/nc.o	 \
	$(MESSOBJ)/mess/machine/nc.o	 \

$(MESSOBJ)/veb.a:      \
	$(MESSOBJ)/mess/vidhrdw/kc.o	\
	$(MESSOBJ)/mess/systems/kc.o	\
	$(MESSOBJ)/mess/machine/kc.o	\

$(MESSOBJ)/nec.a:	   \
	$(MESSOBJ)/mess/vidhrdw/vdc.o	 \
	$(MESSOBJ)/mess/machine/pce.o	 \
	$(MESSOBJ)/mess/systems/pce.o

$(MESSOBJ)/necpc.a:	   \
	$(MESSOBJ)/mess/systems/pc8801.o	 \
	$(MESSOBJ)/mess/machine/pc8801.o	 \
	$(MESSOBJ)/mess/vidhrdw/pc8801.o	\

$(MESSOBJ)/ep128.a :   \
	$(MESSOBJ)/mess/sndhrdw/dave.o	 \
	$(MESSOBJ)/mess/vidhrdw/epnick.o	 \
	$(MESSOBJ)/mess/vidhrdw/enterp.o	 \
	$(MESSOBJ)/mess/machine/enterp.o	 \
	$(MESSOBJ)/mess/systems/enterp.o

$(MESSOBJ)/ascii.a :   \
	$(MESSOBJ)/mess/formats/fmsx_cas.o \
	$(MESSOBJ)/mess/systems/msx.o	\
	$(MESSOBJ)/mess/machine/msx_slot.o	 \
	$(MESSOBJ)/mess/machine/msx.o	 \

$(MESSOBJ)/kim1.a :    \
	$(MESSOBJ)/mess/vidhrdw/kim1.o	 \
	$(MESSOBJ)/mess/machine/kim1.o	 \
	$(MESSOBJ)/mess/systems/kim1.o

$(MESSOBJ)/sym1.a :    \
	$(MESSOBJ)/mess/vidhrdw/sym1.o	 \
	$(MESSOBJ)/mess/machine/sym1.o	 \
	$(MESSOBJ)/mess/systems/sym1.o

$(MESSOBJ)/aim65.a :    \
	$(MESSOBJ)/mess/vidhrdw/aim65.o	 \
	$(MESSOBJ)/mess/machine/aim65.o	 \
	$(MESSOBJ)/mess/systems/aim65.o

$(MESSOBJ)/vc4000.a :   \
	$(MESSOBJ)/mess/sndhrdw/vc4000.o	\
	$(MESSOBJ)/mess/systems/vc4000.o	\
	$(MESSOBJ)/mess/vidhrdw/vc4000.o	\

$(MESSOBJ)/tangerin.a :\
	$(MESSOBJ)/mess/devices/mfmdisk.o	\
	$(MESSOBJ)/mess/vidhrdw/microtan.o	\
	$(MESSOBJ)/mess/machine/microtan.o	\
	$(MESSOBJ)/mess/systems/microtan.o	\
	$(MESSOBJ)/mess/formats/oric_tap.o	\
	$(MESSOBJ)/mess/systems/oric.o		\
	$(MESSOBJ)/mess/vidhrdw/oric.o		\
	$(MESSOBJ)/mess/machine/oric.o		\

$(MESSOBJ)/vtech.a :   \
	$(MESSOBJ)/mess/vidhrdw/vtech1.o	\
	$(MESSOBJ)/mess/machine/vtech1.o	\
	$(MESSOBJ)/mess/systems/vtech1.o	\
	$(MESSOBJ)/mess/vidhrdw/vtech2.o	\
	$(MESSOBJ)/mess/machine/vtech2.o	\
	$(MESSOBJ)/mess/systems/vtech2.o	\
	$(MESSOBJ)/mess/formats/vt_cas.o	\
	$(MESSOBJ)/mess/formats/vt_dsk.o	\

$(MESSOBJ)/jupiter.a : \
	$(MESSOBJ)/mess/systems/jupiter.o	\
	$(MESSOBJ)/mess/vidhrdw/jupiter.o	\
	$(MESSOBJ)/mess/machine/jupiter.o	\

$(MESSOBJ)/mbee.a :    \
	$(MESSOBJ)/mess/vidhrdw/mbee.o	 \
	$(MESSOBJ)/mess/machine/mbee.o	 \
	$(MESSOBJ)/mess/systems/mbee.o

$(MESSOBJ)/advision.a: \
	$(MESSOBJ)/mess/vidhrdw/advision.o \
	$(MESSOBJ)/mess/machine/advision.o \
	$(MESSOBJ)/mess/systems/advision.o

$(MESSOBJ)/nascom1.a:  \
	$(MESSOBJ)/mess/vidhrdw/nascom1.o  \
	$(MESSOBJ)/mess/machine/nascom1.o  \
	$(MESSOBJ)/mess/systems/nascom1.o

$(MESSOBJ)/cpschngr.a: \
	$(MESSOBJ)/machine/eeprom.o	     \
	$(MESSOBJ)/mess/systems/cpschngr.o \
	$(MESSOBJ)/vidhrdw/cps1.o

$(MESSOBJ)/mtx.a:	   \
	$(MESSOBJ)/mess/systems/mtx.o

$(MESSOBJ)/acorn.a:    \
	$(MESSOBJ)/mess/machine/i8271.o	 \
	$(MESSOBJ)/mess/machine/upd7002.o  \
	$(MESSOBJ)/mess/vidhrdw/saa505x.o	     \
	$(MESSOBJ)/mess/vidhrdw/bbc.o	     \
	$(MESSOBJ)/mess/machine/bbc.o	     \
	$(MESSOBJ)/mess/systems/bbc.o	     \
	$(MESSOBJ)/mess/systems/a310.o	 \
	$(MESSOBJ)/mess/systems/z88.o	     \
	$(MESSOBJ)/mess/vidhrdw/z88.o      \
	$(MESSOBJ)/mess/vidhrdw/atom.o	 \
	$(MESSOBJ)/mess/systems/atom.o	 \
	$(MESSOBJ)/mess/machine/atom.o	 \
	$(MESSOBJ)/mess/formats/uef_cas.o	\
	$(MESSOBJ)/mess/vidhrdw/electron.o	\
	$(MESSOBJ)/mess/machine/electron.o	\
	$(MESSOBJ)/mess/systems/electron.o

$(MESSOBJ)/samcoupe.a: \
	$(MESSOBJ)/mess/vidhrdw/coupe.o	 \
	$(MESSOBJ)/mess/systems/coupe.o	\
	$(MESSOBJ)/mess/machine/coupe.o	 \

$(MESSOBJ)/sharp.a:    \
	$(MESSOBJ)/mess/vidhrdw/mz700.o		\
	$(MESSOBJ)/mess/systems/mz700.o		\
	$(MESSOBJ)/mess/formats/mz_cas.o	\
	$(MESSOBJ)/mess/systems/pocketc.o	\
	$(MESSOBJ)/mess/vidhrdw/pc1401.o	\
	$(MESSOBJ)/mess/machine/pc1401.o	\
	$(MESSOBJ)/mess/vidhrdw/pc1403.o	\
	$(MESSOBJ)/mess/machine/pc1403.o	\
	$(MESSOBJ)/mess/vidhrdw/pc1350.o	\
	$(MESSOBJ)/mess/machine/pc1350.o	\
	$(MESSOBJ)/mess/vidhrdw/pc1251.o	\
	$(MESSOBJ)/mess/machine/pc1251.o	\
	$(MESSOBJ)/mess/vidhrdw/pocketc.o	\
	$(MESSOBJ)/mess/machine/mz700.o		\

$(MESSOBJ)/hp48.a:     \
	$(MESSOBJ)/mess/machine/hp48.o     \
	$(MESSOBJ)/mess/vidhrdw/hp48.o     \
	$(MESSOBJ)/mess/systems/hp48.o

$(MESSOBJ)/aquarius.a: \
	$(MESSOBJ)/mess/systems/aquarius.o	\
	$(MESSOBJ)/mess/vidhrdw/aquarius.o \
	$(MESSOBJ)/mess/machine/aquarius.o \

$(MESSOBJ)/exidy.a:    \
	$(MESSOBJ)/mess/machine/hd6402.o     \
	$(MESSOBJ)/mess/systems/exidy.o		\
	$(MESSOBJ)/mess/vidhrdw/exidy.o      \

$(MESSOBJ)/galaxy.a:   \
	$(MESSOBJ)/mess/vidhrdw/galaxy.o   \
	$(MESSOBJ)/mess/systems/galaxy.o	\
	$(MESSOBJ)/mess/machine/galaxy.o   \

$(MESSOBJ)/lviv.a:   \
	$(MESSOBJ)/mess/vidhrdw/lviv.o   \
	$(MESSOBJ)/mess/systems/lviv.o   \
	$(MESSOBJ)/mess/machine/lviv.o   \
	$(MESSOBJ)/mess/formats/lviv_lvt.o

$(MESSOBJ)/pmd85.a:   \
	$(MESSOBJ)/mess/vidhrdw/pmd85.o   \
	$(MESSOBJ)/mess/systems/pmd85.o   \
	$(MESSOBJ)/mess/machine/pmd85.o   \
	$(MESSOBJ)/mess/formats/pmd_pmd.o

$(MESSOBJ)/magnavox.a: \
	$(MESSOBJ)/mess/machine/odyssey2.o \
	$(MESSOBJ)/mess/vidhrdw/odyssey2.o \
	$(MESSOBJ)/mess/sndhrdw/odyssey2.o \
	$(MESSOBJ)/mess/systems/odyssey2.o

$(MESSOBJ)/teamconc.a: \
	$(MESSOBJ)/mess/vidhrdw/comquest.o \
	$(MESSOBJ)/mess/systems/comquest.o

$(MESSOBJ)/svision.a:  \
	$(MESSOBJ)/mess/systems/svision.o \
	$(MESSOBJ)/mess/sndhrdw/svision.o

$(MESSOBJ)/lynx.a:     \
	$(MESSOBJ)/mess/systems/lynx.o     \
	$(MESSOBJ)/mess/sndhrdw/lynx.o     \
	$(MESSOBJ)/mess/machine/lynx.o

$(MESSOBJ)/mk1.a:      \
	$(MESSOBJ)/mess/cpu/f8/f3853.o	 \
	$(MESSOBJ)/mess/vidhrdw/mk1.o      \
	$(MESSOBJ)/mess/systems/mk1.o

$(MESSOBJ)/mk2.a:      \
	$(MESSOBJ)/mess/vidhrdw/mk2.o      \
	$(MESSOBJ)/mess/systems/mk2.o

$(MESSOBJ)/ssystem3.a: \
	$(MESSOBJ)/mess/vidhrdw/ssystem3.o \
	$(MESSOBJ)/mess/systems/ssystem3.o

$(MESSOBJ)/motorola.a: \
	$(MESSOBJ)/mess/vidhrdw/mekd2.o    \
	$(MESSOBJ)/mess/machine/mekd2.o    \
	$(MESSOBJ)/mess/systems/mekd2.o

$(MESSOBJ)/svi.a:      \
	$(MESSOBJ)/mess/machine/svi318.o   \
	$(MESSOBJ)/mess/systems/svi318.o   \
	$(MESSOBJ)/mess/formats/svi_cas.o

$(MESSOBJ)/intv.a:     \
	$(MESSOBJ)/mess/vidhrdw/intv.o	\
	$(MESSOBJ)/mess/vidhrdw/stic.o	\
	$(MESSOBJ)/mess/machine/intv.o	\
	$(MESSOBJ)/mess/sndhrdw/intv.o	\
	$(MESSOBJ)/mess/systems/intv.o

$(MESSOBJ)/apf.a:      \
	$(MESSOBJ)/mess/systems/apf.o	\
	$(MESSOBJ)/mess/machine/apf.o	\
	$(MESSOBJ)/mess/vidhrdw/apf.o   \
	$(MESSOBJ)/mess/formats/apf_apt.o

$(MESSOBJ)/sord.a:     \
	$(MESSOBJ)/mess/systems/sord.o	\
	$(MESSOBJ)/mess/formats/sord_cas.o

$(MESSOBJ)/tatung.a:     \
	$(MESSOBJ)/mess/systems/einstein.o

$(MESSOBJ)/sony.a:     \
	$(MESSOBJ)/mess/systems/psx.o	\
	$(MESSOBJ)/machine/psx.o	\
	$(MESSOBJ)/vidhrdw/psx.o

$(MESSOBJ)/dai.a:     \
	$(MESSOBJ)/mess/systems/dai.o     \
	$(MESSOBJ)/mess/vidhrdw/dai.o     \
	$(MESSOBJ)/mess/sndhrdw/dai.o     \
	$(MESSOBJ)/mess/machine/tms5501.o \
	$(MESSOBJ)/mess/machine/dai.o     \

$(MESSOBJ)/concept.a:  \
	$(MESSOBJ)/mess/systems/concept.o   \
	$(MESSOBJ)/mess/machine/concept.o	\
	$(MESSOBJ)/mess/machine/corvushd.o

$(MESSOBJ)/bandai.a:     \
	$(MESSOBJ)/mess/systems/wswan.o   \
	$(MESSOBJ)/mess/machine/wswan.o   \
	$(MESSOBJ)/mess/vidhrdw/wswan.o   \
	$(MESSOBJ)/mess/sndhrdw/wswan.o

$(MESSOBJ)/compis.a:					\
	$(MESSOBJ)/mess/systems/compis.o	\
	$(MESSOBJ)/mess/machine/compis.o	\
	$(MESSOBJ)/mess/machine/mm58274c.o	\
	$(MESSOBJ)/mess/formats/cpis_dsk.o	\
	$(MESSOBJ)/mess/vidhrdw/i82720.o 

$(MESSOBJ)/multitch.a:					\
	$(MESSOBJ)/mess/systems/mpf1.o		\

$(MESSOBJ)/telmac.a:					\
	$(MESSOBJ)/mess/systems/telmac.o	\
	$(MESSOBJ)/mess/vidhrdw/cdp186x.o	\

$(MESSOBJ)/exeltel.a:					\
	$(MESSOBJ)/mess/systems/exelv.o		\

$(MESSOBJ)/tx0.a:				\
	$(MESSOBJ)/mess/vidhrdw/crt.o	\
	$(MESSOBJ)/mess/systems/tx0.o	\
	$(MESSOBJ)/mess/machine/tx0.o	\
	$(MESSOBJ)/mess/vidhrdw/tx0.o	\

$(MESSOBJ)/luxor.a:					\
	$(MESSOBJ)/mess/systems/abc80.o	\

$(MESSOBJ)/sgi.a:						\
	$(MESSOBJ)/mess/machine/sgi.o		\
	$(MESSOBJ)/mess/systems/ip20.o		\
	$(MESSOBJ)/mess/systems/ip22.o	\
	$(MESSOBJ)/mess/machine/wd33c93.o \
	$(MESSOBJ)/machine/scsihd.o	\
	$(MESSOBJ)/machine/scsicd.o	\
	$(MESSOBJ)/mess/vidhrdw/newport.o

$(MESSOBJ)/primo.a:				\
	$(MESSOBJ)/mess/systems/primo.o	\
	$(MESSOBJ)/mess/machine/primo.o	\
	$(MESSOBJ)/mess/vidhrdw/primo.o	\
	$(MESSOBJ)/mess/formats/primoptp.o

$(MESSOBJ)/be.a:						\
	$(MESSOBJ)/mess/systems/bebox.o		\
	$(MESSOBJ)/mess/machine/bebox.o		\
	$(MESSOBJ)/machine/pci.o		\
	$(MESSOBJ)/mess/machine/mpc105.o	\
	$(MESSOBJ)/mess/vidhrdw/cirrus.o	\
	$(MESSOBJ)/machine/intelfsh.o		\
	$(MESSOBJ)/machine/53c810.o

$(MESSOBJ)/tiger.a:				\
	$(MESSOBJ)/mess/systems/gamecom.o	\
	$(MESSOBJ)/mess/machine/gamecom.o	\
	$(MESSOBJ)/mess/vidhrdw/gamecom.o

# MESS specific core $(MESSOBJ)s
MESSCOREOBJS +=							\
	$(EXPAT)						\
	$(ZLIB)							\
	$(MESSOBJ)/vidhrdw/tms9928a.o		\
	$(MESSOBJ)/machine/8255ppi.o		\
	$(MESSOBJ)/machine/6522via.o		\
	$(MESSOBJ)/machine/6821pia.o		\
	$(MESSOBJ)/machine/z80ctc.o			\
	$(MESSOBJ)/machine/z80pio.o			\
	$(MESSOBJ)/machine/z80sio.o			\
	$(MESSOBJ)/machine/idectrl.o		\
	$(MESSOBJ)/machine/6532riot.o		\
	$(MESSOBJ)/mess/mess.o				\
	$(MESSOBJ)/mess/mesvalid.o			\
	$(MESSOBJ)/mess/image.o				\
	$(MESSOBJ)/mess/messdriv.o			\
	$(MESSOBJ)/mess/device.o			\
	$(MESSOBJ)/mess/hashfile.o			\
	$(MESSOBJ)/mess/inputx.o			\
	$(MESSOBJ)/mess/unicode.o			\
	$(MESSOBJ)/mess/artworkx.o			\
	$(MESSOBJ)/mess/mesintrf.o			\
	$(MESSOBJ)/mess/filemngr.o			\
	$(MESSOBJ)/mess/tapectrl.o			\
	$(MESSOBJ)/mess/compcfg.o			\
	$(MESSOBJ)/mess/utils.o				\
	$(MESSOBJ)/mess/eventlst.o			\
	$(MESSOBJ)/mess/mscommon.o			\
	$(MESSOBJ)/mess/pool.o				\
	$(MESSOBJ)/mess/cheatms.o			\
	$(MESSOBJ)/mess/opresolv.o			\
	$(MESSOBJ)/mess/mui_text.o			\
	$(MESSOBJ)/mess/infomess.o			\
	$(MESSOBJ)/mess/formats/ioprocs.o	\
	$(MESSOBJ)/mess/formats/flopimg.o	\
	$(MESSOBJ)/mess/formats/cassimg.o	\
	$(MESSOBJ)/mess/formats/basicdsk.o	\
	$(MESSOBJ)/mess/formats/pc_dsk.o	\
	$(MESSOBJ)/mess/devices/mflopimg.o	\
	$(MESSOBJ)/mess/devices/cassette.o	\
	$(MESSOBJ)/mess/devices/cartslot.o	\
	$(MESSOBJ)/mess/devices/printer.o	\
	$(MESSOBJ)/mess/devices/bitbngr.o	\
	$(MESSOBJ)/mess/devices/snapquik.o	\
	$(MESSOBJ)/mess/devices/basicdsk.o	\
	$(MESSOBJ)/mess/devices/flopdrv.o	\
	$(MESSOBJ)/mess/devices/harddriv.o	\
	$(MESSOBJ)/mess/devices/idedrive.o	\
	$(MESSOBJ)/mess/devices/dsk.o		\
	$(MESSOBJ)/mess/devices/z80bin.o	\
	$(MESSOBJ)/mess/devices/chd_cd.o	\
	$(MESSOBJ)/mess/machine/6551.o		\
	$(MESSOBJ)/mess/machine/smartmed.o	\
	$(MESSOBJ)/mess/vidhrdw/m6847.o		\
	$(MESSOBJ)/mess/vidhrdw/m6845.o		\
	$(MESSOBJ)/mess/machine/msm8251.o  \
	$(MESSOBJ)/mess/machine/tc8521.o   \
	$(MESSOBJ)/mess/vidhrdw/v9938.o    \
	$(MESSOBJ)/mess/vidhrdw/crtc6845.o \
	$(MESSOBJ)/mess/machine/28f008sa.o \
	$(MESSOBJ)/mess/machine/am29f080.o \
	$(MESSOBJ)/mess/machine/rriot.o    \
	$(MESSOBJ)/mess/machine/riot6532.o \
	$(MESSOBJ)/machine/pit8253.o  \
	$(MESSOBJ)/machine/mc146818.o \
	$(MESSOBJ)/mess/machine/uart8250.o \
	$(MESSOBJ)/mess/machine/pc_mouse.o \
	$(MESSOBJ)/mess/machine/pclpt.o    \
	$(MESSOBJ)/mess/machine/centroni.o \
	$(MESSOBJ)/machine/pckeybrd.o \
	$(MESSOBJ)/mess/machine/d88.o      \
	$(MESSOBJ)/mess/machine/nec765.o   \
	$(MESSOBJ)/mess/machine/wd17xx.o   \
	$(MESSOBJ)/mess/machine/serial.o   \
	$(MESSOBJ)/mess/formats/wavfile.o



# additional tools
TOOLS = dat2html$(EXE) messtest$(EXE) chdman$(EXE) messdocs$(EXE) imgtool$(EXE)

include mess/tools/imgtool/imgtool.mak

DAT2HTML_OBJS =								\
	$(MESSOBJ)/mamecore.o						\
	$(MESSOBJ)/mess/tools/dat2html/dat2html.o	\
	$(MESSOBJ)/mess/tools/imgtool/stubs.o		\
	$(MESSOBJ)/mess/utils.o						\

MESSDOCS_OBJS =								\
	$(MESSOBJ)/mamecore.o						\
	$(MESSOBJ)/mess/tools/messdocs/messdocs.o	\
	$(MESSOBJ)/mess/utils.o						\
	$(MESSOBJ)/mess/pool.o						\
	$(EXPAT)								\

MESSTEST_OBJS =								\
	$(EXPAT)								\
	$(IMGTOOL_LIB_OBJS)						\
	$(MESSOBJ)/mess/pile.o						\
	$(MESSOBJ)/mess/tools/messtest/main.o		\
	$(MESSOBJ)/mess/tools/messtest/core.o		\
	$(MESSOBJ)/mess/tools/messtest/testmess.o	\
	$(MESSOBJ)/mess/tools/messtest/testimgt.o	\
	$(MESSOBJ)/mess/tools/messtest/tststubs.o	\
	$(MESSOBJ)/mess/tools/messtest/tstutils.o	\



# text files
TEXTS = sysinfo.htm

mess.txt: $(EMULATORCLI)
	@echo Generating $@...
	@$(CURPATH)$(EMULATORCLI) -listtext -noclones -sortname > docs/mess.txt

sysinfo.htm: dat2html$(EXE)
	@echo Generating $@...
	@$(CURPATH)dat2html$(EXE) sysinfo.dat

mess/makedep/makedep$(EXE): $(wildcard mess/makedep/*.c) $(wildcard mess/makedep/*.h)
	make -Cmess/makedep
