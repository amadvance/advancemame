# this is a MESS build
MESS = 1

# core defines
COREDEFS += -DNEOFREE -DMESS



#-------------------------------------------------
# specify available CPU cores; some of these are
# only for MAME and so aren't included
#-------------------------------------------------

CPUS+=Z80
CPUS+=Z180
CPUS+=8080
#CPUS+=8085A
CPUS+=M6502
CPUS+=M65C02
CPUS+=M65SC02
#CPUS+=M65CE02
CPUS+=M6509
CPUS+=M6510
CPUS+=M6510T
CPUS+=M7501
CPUS+=M8502
CPUS+=N2A03
#CPUS+=DECO16
CPUS+=M4510
CPUS+=H6280
CPUS+=I86
CPUS+=I88
CPUS+=I186
#CPUS+=I188
CPUS+=I286
CPUS+=V20
CPUS+=V30
#CPUS+=V33
#CPUS+=V60
#CPUS+=V70
#CPUS+=I8035
CPUS+=I8039
CPUS+=I8048
#CPUS+=N7751
#CPUS+=I8X41
#CPUS+=I8051
#CPUS+=I8052
#CPUS+=I8751
#CPUS+=I8752
CPUS+=M6800
#CPUS+=M6801
#CPUS+=M6802
CPUS+=M6803
CPUS+=M6808
CPUS+=HD63701
CPUS+=NSC8105
CPUS+=M6805
#CPUS+=M68705
#CPUS+=HD63705
CPUS+=HD6309
CPUS+=M6809
CPUS+=M6809E
#CPUS+=KONAMI
CPUS+=M68000
CPUS+=M68008
CPUS+=M68010
CPUS+=M68EC020
CPUS+=M68020
#CPUS+=T11
CPUS+=S2650
#CPUS+=TMS34010
#CPUS+=TMS34020
CPUS+=TMS9900
#CPUS+=TMS9940
CPUS+=TMS9980
#CPUS+=TMS9985
#CPUS+=TMS9989
CPUS+=TMS9995
#CPUS+=TMS99000
CPUS+=TMS99010
#CPUS+=TMS99105A
#CPUS+=TMS99110A
#CPUS+=Z8000
#CPUS+=TMS32010
#CPUS+=TMS32025
#CPUS+=TMS32026
#CPUS+=TMS32031
#CPUS+=CCPU
#CPUS+=ADSP2100
#CPUS+=ADSP2101
#CPUS+=ADSP2104
#CPUS+=ADSP2105
#CPUS+=ADSP2115
#CPUS+=ADSP2181
CPUS+=PSXCPU
#CPUS+=ASAP
#CPUS+=UPD7810
#CPUS+=UPD7807
CPUS+=ARM
#CPUS+=ARM7
CPUS+=JAGUAR
#CPUS+=R3000
CPUS+=R4600
#CPUS+=R4700
CPUS+=R5000
#CPUS+=QED5271
#CPUS+=RM7000
CPUS+=SH2
#CPUS+=DSP32C
#CPUS+=PIC16C54
#CPUS+=PIC16C55
#CPUS+=PIC16C56
#CPUS+=PIC16C57
#CPUS+=PIC16C58
CPUS+=G65816
CPUS+=SPC700
CPUS+=E116T
#CPUS+=E116XT
#CPUS+=E116XS
#CPUS+=E116XSR
#CPUS+=E132N
#CPUS+=E132T
#CPUS+=E132XN
#CPUS+=E132XT
#CPUS+=E132XS
#CPUS+=E132XSR
#CPUS+=GMS30C2116
#CPUS+=GMS30C2132
#CPUS+=GMS30C2216
#CPUS+=GMS30C2232
CPUS+=I386
CPUS+=I486
CPUS+=PENTIUM
#CPUS+=MEDIAGX
#CPUS+=I960
#CPUS+=H83002
#CPUS+=V810
#CPUS+=M37710
#CPUS+=PPC403
#CPUS+=PPC602
CPUS+=PPC603
#CPUS+=SE3208
#CPUS+=MC68HC11
#CPUS += ADSP21062
#CPUS += DSP56156
CPUS += RSP
CPUS+=Z80GB
CPUS+=CDP1802
CPUS+=SC61860
CPUS+=SATURN
CPUS+=APEXC
CPUS+=F8
CPUS+=CP1610
#CPUS+=TMS99010
CPUS+=PDP1
#CPUS+=TMS7000
CPUS+=TMS7000_EXL
CPUS+=TX0
CPUS+=COP411
CPUS+=SM8500
CPUS+=V30MZ

# SOUND cores used in MESS
SOUNDS+=CUSTOM
SOUNDS+=SAMPLES
SOUNDS+=DAC
SOUNDS+=DMADAC
SOUNDS+=DISCRETE
SOUNDS+=AY8910
SOUNDS+=YM2203
# enable only one of the following two
SOUNDS+=YM2151
SOUNDS+=YM2151_ALT
SOUNDS+=YM2608
SOUNDS+=YM2610
SOUNDS+=YM2610B
SOUNDS+=YM2612
#SOUNDS+=YM3438
SOUNDS+=YM2413
SOUNDS+=YM3812
#SOUNDS+=YMZ280B
#SOUNDS+=YM3526
#SOUNDS+=Y8950
SOUNDS+=SN76477
SOUNDS+=SN76496
SOUNDS+=POKEY
SOUNDS+=TIA
SOUNDS+=NES
SOUNDS+=ASTROCADE
#SOUNDS+=NAMCO
#SOUNDS+=NAMCO_15XX
#SOUNDS+=NAMCO_CUS30
#SOUNDS+=NAMCO_52XX
#SOUNDS+=NAMCO_54XX
#SOUNDS+=NAMCO_63701X
#SOUNDS+=NAMCONA
#SOUNDS+=TMS36XX
SOUNDS+=TMS5110
SOUNDS+=TMS5220
#SOUNDS+=VLM5030
#SOUNDS+=ADPCM
SOUNDS+=OKIM6295
#SOUNDS+=MSM5205
#SOUNDS+=MSM5232
#SOUNDS+=UPD7759
#SOUNDS+=HC55516
#SOUNDS+=K005289
#SOUNDS+=K007232
SOUNDS+=K051649
#SOUNDS+=K053260
#SOUNDS+=K054539
#SOUNDS+=SEGAPCM
#SOUNDS+=RF5C68
#SOUNDS+=CEM3394
#SOUNDS+=C140
SOUNDS+=QSOUND
SOUNDS+=SAA1099
#SOUNDS+=IREMGA20
#SOUNDS+=ES5505
#SOUNDS+=ES5506
#SOUNDS+=BSMT2000
#SOUNDS+=YMF262
#SOUNDS+=YMF278B
#SOUNDS+=GAELCO_CG1V
#SOUNDS+=GAELCO_GAE1
#SOUNDS+=X1_010
#SOUNDS+=MULTIPCM
SOUNDS+=C6280
#SOUNDS+=SP0250
#SOUNDS+=SCSP
#SOUNDS+=YMF271
SOUNDS+=PSXSPU
#SOUNDS+=CDDA
#SOUNDS+=ICS2115
#SOUNDS+=ST0016
#SOUNDS+=C352
#SOUNDS+=VRENDER0
SOUNDS+=SPEAKER
SOUNDS+=WAVE
SOUNDS+=BEEP
SOUNDS+=SID6581
SOUNDS+=SID8580
SOUNDS+=ES5503

# Archive definitions
DRVLIBS = \
	$(OBJ)/coco.a     \
	$(OBJ)/mc10.a     \
	$(OBJ)/apple.a    \
	$(OBJ)/apexc.a	  \
	$(OBJ)/pdp1.a	  \
	$(OBJ)/sony.a     \
	$(OBJ)/nintendo.a \
	$(OBJ)/pc.a       \
	$(OBJ)/at.a       \
	$(OBJ)/pcshare.a  \
	$(OBJ)/ti99.a     \
	$(OBJ)/amstrad.a  \
	$(OBJ)/sega.a     \
	$(OBJ)/acorn.a    \
	$(OBJ)/atari.a    \
	$(OBJ)/trs80.a	  \
	$(OBJ)/fairch.a   \
	$(OBJ)/bally.a	  \
	$(OBJ)/advision.a \
	$(OBJ)/mbee.a	  \
	$(OBJ)/vtech.a	  \
	$(OBJ)/jupiter.a  \
	$(OBJ)/gce.a	  \
	$(OBJ)/arcadia.a  \
	$(OBJ)/kaypro.a   \
	$(OBJ)/cgenie.a   \
	$(OBJ)/aquarius.a \
	$(OBJ)/tangerin.a \
	$(OBJ)/sord.a     \
	$(OBJ)/exidy.a    \
	$(OBJ)/samcoupe.a \
	$(OBJ)/p2000.a	  \
	$(OBJ)/tatung.a   \
	$(OBJ)/ep128.a	  \
	$(OBJ)/cpschngr.a \
	$(OBJ)/veb.a	  \
	$(OBJ)/nec.a	  \
	$(OBJ)/nascom1.a  \
	$(OBJ)/magnavox.a \
	$(OBJ)/mk1.a      \
	$(OBJ)/mk2.a      \
	$(OBJ)/ti85.a     \
	$(OBJ)/galaxy.a   \
	$(OBJ)/vc4000.a   \
	$(OBJ)/lviv.a     \
	$(OBJ)/pmd85.a    \
	$(OBJ)/sinclair.a \
	$(OBJ)/lynx.a     \
	$(OBJ)/svision.a  \
	$(OBJ)/coleco.a   \
	$(OBJ)/apf.a      \
	$(OBJ)/teamconc.a \
	$(OBJ)/concept.a  \
	$(OBJ)/amiga.a    \
	$(OBJ)/svi.a      \
	$(OBJ)/tutor.a    \
	$(OBJ)/sharp.a    \
	$(OBJ)/aim65.a    \
	$(OBJ)/avigo.a    \
	$(OBJ)/motorola.a	\
	$(OBJ)/ssystem3.a	\
	$(OBJ)/hp48.a		\
	$(OBJ)/cbm.a		\
	$(OBJ)/cbmshare.a	\
	$(OBJ)/kim1.a		\
	$(OBJ)/sym1.a		\
	$(OBJ)/dai.a		\
	$(OBJ)/bandai.a		\
	$(OBJ)/compis.a		\
	$(OBJ)/necpc.a		\
	$(OBJ)/ascii.a		\
	$(OBJ)/mtx.a		\
	$(OBJ)/intv.a		\
	$(OBJ)/rca.a		\
	$(OBJ)/multitch.a	\
	$(OBJ)/telmac.a		\
	$(OBJ)/tx0.a		\
	$(OBJ)/luxor.a		\
	$(OBJ)/sgi.a		\
	$(OBJ)/primo.a		\
	$(OBJ)/dgn_beta.a	\
	$(OBJ)/be.a			\
	$(OBJ)/tiger.a		\


$(OBJ)/neocd.a:						\
	$(OBJ)/mess/systems/neocd.o		\
	$(OBJ)/machine/neogeo.o			\
	$(OBJ)/vidhrdw/neogeo.o			\
	$(OBJ)/machine/pd4990a.o		\

$(OBJ)/coleco.a:   \
	$(OBJ)/mess/machine/coleco.o	\
	$(OBJ)/mess/systems/coleco.o	\
	$(OBJ)/mess/machine/adam.o		\
	$(OBJ)/mess/systems/adam.o		\
	$(OBJ)/mess/formats/adam_dsk.o	\
	$(OBJ)/mess/systems/fnvision.o	\
	

$(OBJ)/arcadia.a:  \
	$(OBJ)/mess/systems/arcadia.o	\
	$(OBJ)/mess/sndhrdw/arcadia.o	\
	$(OBJ)/mess/vidhrdw/arcadia.o	\

$(OBJ)/sega.a:						\
	$(OBJ)/mess/machine/genesis.o	\
	$(OBJ)/mess/systems/genesis.o	\
	$(OBJ)/mess/systems/saturn.o	\
	$(OBJ)/machine/stvcd.o			\
	$(OBJ)/machine/scudsp.o			\
	$(OBJ)/vidhrdw/stvvdp1.o		\
	$(OBJ)/vidhrdw/stvvdp2.o		\
	$(OBJ)/sound/scsp.o				\
	$(OBJ)/mess/vidhrdw/smsvdp.o	\
	$(OBJ)/mess/machine/sms.o		\
	$(OBJ)/mess/systems/sms.o

$(OBJ)/atari.a:						\
	$(OBJ)/vidhrdw/tia.o			\
	$(OBJ)/machine/atari.o			\
	$(OBJ)/vidhrdw/atari.o			\
	$(OBJ)/vidhrdw/antic.o			\
	$(OBJ)/vidhrdw/gtia.o			\
	$(OBJ)/mess/systems/atari.o		\
	$(OBJ)/mess/machine/a7800.o		\
	$(OBJ)/mess/systems/a7800.o		\
	$(OBJ)/mess/vidhrdw/a7800.o		\
	$(OBJ)/mess/systems/a2600.o		\
	$(OBJ)/mess/systems/jaguar.o	\
	$(OBJ)/sndhrdw/jaguar.o			\
	$(OBJ)/vidhrdw/jaguar.o			\
#	$(OBJ)/mess/systems/atarist.o

$(OBJ)/gce.a:	                     \
	$(OBJ)/mess/systems/vectrex.o	\
	$(OBJ)/mess/vidhrdw/vectrex.o	\
	$(OBJ)/mess/machine/vectrex.o	\

$(OBJ)/nintendo.a:					\
	$(OBJ)/mess/sndhrdw/gb.o		\
	$(OBJ)/mess/vidhrdw/gb.o		\
	$(OBJ)/mess/machine/gb.o		\
	$(OBJ)/mess/systems/gb.o		\
	$(OBJ)/mess/machine/nes_mmc.o	\
	$(OBJ)/vidhrdw/ppu2c03b.o		\
	$(OBJ)/mess/vidhrdw/nes.o		\
	$(OBJ)/mess/machine/nes.o		\
	$(OBJ)/mess/systems/nes.o		\
	$(OBJ)/sndhrdw/snes.o			\
	$(OBJ)/machine/snes.o			\
	$(OBJ)/vidhrdw/snes.o			\
	$(OBJ)/mess/systems/snes.o	 	\
	$(OBJ)/mess/systems/n64.o		\
	$(OBJ)/machine/n64.o			\
	$(OBJ)/vidhrdw/n64.o			\

$(OBJ)/amiga.a: \
	$(OBJ)/vidhrdw/amiga.o			\
	$(OBJ)/machine/amiga.o			\
	$(OBJ)/sndhrdw/amiga.o			\
	$(OBJ)/machine/6526cia.o		\
	$(OBJ)/mess/machine/amigafdc.o	\
	$(OBJ)/mess/systems/amiga.o

$(OBJ)/cbmshare.a: \
	$(OBJ)/machine/6526cia.o		\
	$(OBJ)/mess/machine/tpi6525.o	\
	$(OBJ)/mess/machine/cbm.o		\
	$(OBJ)/mess/machine/cbmdrive.o	\
	$(OBJ)/mess/machine/vc1541.o	 \
	$(OBJ)/mess/machine/cbmieeeb.o \
	$(OBJ)/mess/machine/cbmserb.o  \
	$(OBJ)/mess/machine/c64.o      \
	$(OBJ)/mess/machine/c65.o		\
	$(OBJ)/mess/vidhrdw/vic6567.o	 \
	$(OBJ)/mess/machine/vc20tape.o

$(OBJ)/cbm.a: \
	$(OBJ)/mess/vidhrdw/pet.o		\
	$(OBJ)/mess/systems/pet.o		\
	$(OBJ)/mess/machine/pet.o		\
	$(OBJ)/mess/systems/c64.o		\
	$(OBJ)/mess/machine/vc20.o		\
	$(OBJ)/mess/systems/vc20.o		\
	$(OBJ)/mess/sndhrdw/ted7360.o	\
	$(OBJ)/mess/sndhrdw/t6721.o		\
	$(OBJ)/mess/machine/c16.o		\
	$(OBJ)/mess/systems/c16.o		\
	$(OBJ)/mess/systems/cbmb.o		\
	$(OBJ)/mess/machine/cbmb.o		\
	$(OBJ)/mess/vidhrdw/cbmb.o		\
	$(OBJ)/mess/systems/c65.o		\
	$(OBJ)/mess/vidhrdw/vdc8563.o	\
	$(OBJ)/mess/systems/c128.o		\
	$(OBJ)/mess/machine/c128.o		\
	$(OBJ)/mess/sndhrdw/vic6560.o	\
	$(OBJ)/mess/vidhrdw/ted7360.o	\
	$(OBJ)/mess/vidhrdw/vic6560.o  

$(OBJ)/coco.a:   \
	$(OBJ)/mess/machine/6883sam.o	\
	$(OBJ)/mess/machine/cococart.o	\
	$(OBJ)/mess/machine/ds1315.o	\
	$(OBJ)/mess/machine/m6242b.o	\
	$(OBJ)/mess/machine/coco.o		\
	$(OBJ)/mess/vidhrdw/coco.o		\
	$(OBJ)/mess/systems/coco.o		\
	$(OBJ)/mess/vidhrdw/coco3.o		\
	$(OBJ)/mess/formats/cocopak.o	\
	$(OBJ)/mess/formats/coco_cas.o	\
	$(OBJ)/mess/formats/coco_dsk.o	\
	$(OBJ)/mess/devices/coco_vhd.o	\

$(OBJ)/mc10.a:	\
	$(OBJ)/mess/machine/mc10.o		\
	$(OBJ)/mess/systems/mc10.o		\
	$(OBJ)/mess/formats/coco_cas.o	\

$(OBJ)/dgn_beta.a:	\
	$(OBJ)/mess/machine/dgn_beta.o	\
	$(OBJ)/mess/vidhrdw/dgn_beta.o	\
	$(OBJ)/mess/systems/dgn_beta.o	\
	$(OBJ)/mess/formats/coco_dsk.o	\

$(OBJ)/trs80.a:    \
	$(OBJ)/mess/machine/trs80.o	 \
	$(OBJ)/mess/vidhrdw/trs80.o	 \
	$(OBJ)/mess/systems/trs80.o

$(OBJ)/cgenie.a:   \
	$(OBJ)/mess/systems/cgenie.o	\
	$(OBJ)/mess/vidhrdw/cgenie.o	 \
	$(OBJ)/mess/sndhrdw/cgenie.o	 \
	$(OBJ)/mess/machine/cgenie.o	 \

$(OBJ)/pdp1.a:	   \
	$(OBJ)/mess/vidhrdw/pdp1.o	\
	$(OBJ)/mess/machine/pdp1.o	\
	$(OBJ)/mess/systems/pdp1.o	\

$(OBJ)/apexc.a:     \
	$(OBJ)/mess/systems/apexc.o

$(OBJ)/kaypro.a:   \
	$(OBJ)/mess/systems/kaypro.o	\
	$(OBJ)/mess/machine/cpm_bios.o	\
	$(OBJ)/mess/vidhrdw/kaypro.o	 \
	$(OBJ)/mess/sndhrdw/kaypro.o	 \
	$(OBJ)/mess/machine/kaypro.o	 \

$(OBJ)/sinclair.a: \
	$(OBJ)/mess/vidhrdw/border.o		\
	$(OBJ)/mess/vidhrdw/spectrum.o		\
	$(OBJ)/mess/vidhrdw/zx.o		\
	$(OBJ)/mess/systems/zx.o		\
	$(OBJ)/mess/machine/zx.o		\
	$(OBJ)/mess/systems/spectrum.o		\
	$(OBJ)/mess/machine/spectrum.o		\
	$(OBJ)/mess/formats/zx81_p.o		\
	$(OBJ)/mess/systems/ql.o		\

$(OBJ)/apple.a:   \
	$(OBJ)/mess/vidhrdw/apple2.o		\
	$(OBJ)/mess/machine/apple2.o		\
	$(OBJ)/mess/systems/apple2.o		\
	$(OBJ)/mess/vidhrdw/apple2gs.o		\
	$(OBJ)/mess/machine/apple2gs.o		\
	$(OBJ)/mess/systems/apple2gs.o		\
	$(OBJ)/mess/formats/ap2_dsk.o		\
	$(OBJ)/mess/formats/ap_dsk35.o		\
	$(OBJ)/mess/machine/ay3600.o		\
	$(OBJ)/mess/machine/lisa.o			\
	$(OBJ)/mess/systems/lisa.o			\
	$(OBJ)/mess/machine/applefdc.o		\
	$(OBJ)/mess/machine/8530scc.o		\
	$(OBJ)/mess/devices/sonydriv.o		\
	$(OBJ)/mess/devices/appldriv.o		\
	$(OBJ)/mess/sndhrdw/mac.o			\
	$(OBJ)/mess/vidhrdw/mac.o			\
	$(OBJ)/mess/machine/mac.o			\
	$(OBJ)/mess/systems/mac.o			\
	$(OBJ)/mess/vidhrdw/apple1.o		\
	$(OBJ)/mess/machine/apple1.o		\
	$(OBJ)/mess/systems/apple1.o		\
	$(OBJ)/mess/vidhrdw/apple3.o		\
	$(OBJ)/mess/machine/apple3.o		\
	$(OBJ)/mess/systems/apple3.o		\
	$(OBJ)/mess/machine/ncr5380.o


$(OBJ)/avigo.a: \
	$(OBJ)/mess/vidhrdw/avigo.o		\
	$(OBJ)/mess/systems/avigo.o		\

$(OBJ)/ti85.a: \
	$(OBJ)/mess/systems/ti85.o		\
	$(OBJ)/mess/formats/ti85_ser.o	\
	$(OBJ)/mess/vidhrdw/ti85.o		\
	$(OBJ)/mess/machine/ti85.o		\

$(OBJ)/rca.a: \
	$(OBJ)/mess/systems/studio2.o  \
	$(OBJ)/mess/vidhrdw/studio2.o  

$(OBJ)/fairch.a: \
	$(OBJ)/mess/vidhrdw/channelf.o \
	$(OBJ)/mess/sndhrdw/channelf.o \
	$(OBJ)/mess/systems/channelf.o 

$(OBJ)/ti99.a:	   \
	$(OBJ)/mess/machine/tms9901.o	\
	$(OBJ)/mess/machine/tms9902.o	\
	$(OBJ)/mess/machine/ti99_4x.o	\
	$(OBJ)/mess/machine/990_hd.o	\
	$(OBJ)/mess/machine/990_tap.o	\
	$(OBJ)/mess/machine/ti990.o		\
	$(OBJ)/mess/machine/mm58274c.o	\
	$(OBJ)/mess/machine/994x_ser.o	\
	$(OBJ)/mess/machine/at29040.o	\
	$(OBJ)/mess/machine/99_dsk.o	\
	$(OBJ)/mess/machine/99_ide.o	\
	$(OBJ)/mess/machine/99_peb.o	\
	$(OBJ)/mess/machine/99_hsgpl.o	\
	$(OBJ)/mess/machine/99_usbsm.o	\
	$(OBJ)/mess/machine/smc92x4.o	\
	$(OBJ)/mess/machine/strata.o	\
	$(OBJ)/mess/machine/rtc65271.o	\
	$(OBJ)/mess/machine/geneve.o	\
	$(OBJ)/mess/machine/990_dk.o	\
	$(OBJ)/mess/sndhrdw/spchroms.o	\
	$(OBJ)/mess/systems/ti990_4.o	\
	$(OBJ)/mess/systems/ti99_4x.o	\
	$(OBJ)/mess/systems/ti99_4p.o	\
	$(OBJ)/mess/systems/geneve.o	\
	$(OBJ)/mess/systems/tm990189.o	\
	$(OBJ)/mess/systems/ti99_8.o	\
	$(OBJ)/mess/vidhrdw/911_vdt.o	\
	$(OBJ)/mess/vidhrdw/733_asr.o	\
	$(OBJ)/mess/systems/ti990_10.o	\
	$(OBJ)/mess/systems/ti99_2.o	\
	$(OBJ)/mess/systems/tutor.o		\

$(OBJ)/tutor.a:   \
	$(OBJ)/mess/systems/tutor.o

$(OBJ)/bally.a:    \
	$(OBJ)/sound/astrocde.o	 \
	$(OBJ)/mess/vidhrdw/astrocde.o \
	$(OBJ)/mess/machine/astrocde.o \
	$(OBJ)/mess/systems/astrocde.o

$(OBJ)/pcshare.a:					\
	$(OBJ)/machine/8237dma.o	\
	$(OBJ)/machine/pic8259.o	\
	$(OBJ)/machine/pcshare.o	\
	$(OBJ)/mess/machine/pc_turbo.o	\
	$(OBJ)/mess/sndhrdw/pc.o		\
	$(OBJ)/mess/sndhrdw/sblaster.o	\
	$(OBJ)/mess/machine/pc_fdc.o	\
	$(OBJ)/mess/machine/pc_hdc.o	\
	$(OBJ)/mess/machine/pc_joy.o	\
	$(OBJ)/mess/vidhrdw/pc_video.o	\
	$(OBJ)/mess/vidhrdw/pc_mda.o	\
	$(OBJ)/mess/vidhrdw/pc_cga.o	\
	$(OBJ)/mess/vidhrdw/cgapal.o	\
	$(OBJ)/mess/vidhrdw/pc_vga.o	\

$(OBJ)/pc.a:	   \
	$(OBJ)/mess/vidhrdw/pc_aga.o	 \
	$(OBJ)/mess/machine/ibmpc.o	 \
	$(OBJ)/mess/machine/tandy1t.o  \
	$(OBJ)/mess/machine/amstr_pc.o \
	$(OBJ)/mess/machine/europc.o	 \
	$(OBJ)/mess/machine/pc.o       \
	$(OBJ)/mess/systems/pc.o		\
	$(OBJ)/mess/vidhrdw/pc_t1t.o	 

$(OBJ)/at.a:	   \
	$(OBJ)/machine/8042kbdc.o    \
	$(OBJ)/mess/machine/pc_ide.o   \
	$(OBJ)/mess/machine/ps2.o	 \
	$(OBJ)/mess/machine/at.o       \
	$(OBJ)/mess/systems/at.o	\
	$(OBJ)/mess/machine/i82439tx.o

$(OBJ)/p2000.a:    \
	$(OBJ)/mess/vidhrdw/saa5050.o  \
	$(OBJ)/mess/vidhrdw/p2000m.o	 \
	$(OBJ)/mess/systems/p2000t.o	 \
	$(OBJ)/mess/machine/p2000t.o	 \
	$(OBJ)/mess/machine/mc6850.o	 \
	$(OBJ)/mess/vidhrdw/osi.o	 \
	$(OBJ)/mess/sndhrdw/osi.o	 \
	$(OBJ)/mess/systems/osi.o	\
	$(OBJ)/mess/machine/osi.o	 \

$(OBJ)/amstrad.a:  \
	$(OBJ)/mess/systems/amstrad.o  \
	$(OBJ)/mess/machine/amstrad.o  \
	$(OBJ)/mess/vidhrdw/amstrad.o  \
	$(OBJ)/mess/vidhrdw/pcw.o	 \
	$(OBJ)/mess/systems/pcw.o	 \
	$(OBJ)/mess/systems/pcw16.o	 \
	$(OBJ)/mess/vidhrdw/pcw16.o	 \
	$(OBJ)/mess/vidhrdw/nc.o	 \
	$(OBJ)/mess/systems/nc.o	 \
	$(OBJ)/mess/machine/nc.o	 \

$(OBJ)/veb.a:      \
	$(OBJ)/mess/vidhrdw/kc.o	\
	$(OBJ)/mess/systems/kc.o	\
	$(OBJ)/mess/machine/kc.o	\

$(OBJ)/nec.a:	   \
	$(OBJ)/mess/vidhrdw/vdc.o	 \
	$(OBJ)/mess/machine/pce.o	 \
	$(OBJ)/mess/systems/pce.o

$(OBJ)/necpc.a:	   \
	$(OBJ)/mess/systems/pc8801.o	 \
	$(OBJ)/mess/machine/pc8801.o	 \
	$(OBJ)/mess/vidhrdw/pc8801.o	\

$(OBJ)/ep128.a :   \
	$(OBJ)/mess/sndhrdw/dave.o	 \
	$(OBJ)/mess/vidhrdw/epnick.o	 \
	$(OBJ)/mess/vidhrdw/enterp.o	 \
	$(OBJ)/mess/machine/enterp.o	 \
	$(OBJ)/mess/systems/enterp.o

$(OBJ)/ascii.a :   \
	$(OBJ)/mess/formats/fmsx_cas.o \
	$(OBJ)/mess/systems/msx.o	\
	$(OBJ)/mess/machine/msx_slot.o	 \
	$(OBJ)/mess/machine/msx.o	 \

$(OBJ)/kim1.a :    \
	$(OBJ)/mess/vidhrdw/kim1.o	 \
	$(OBJ)/mess/machine/kim1.o	 \
	$(OBJ)/mess/systems/kim1.o

$(OBJ)/sym1.a :    \
	$(OBJ)/mess/vidhrdw/sym1.o	 \
	$(OBJ)/mess/machine/sym1.o	 \
	$(OBJ)/mess/systems/sym1.o

$(OBJ)/aim65.a :    \
	$(OBJ)/mess/vidhrdw/aim65.o	 \
	$(OBJ)/mess/machine/aim65.o	 \
	$(OBJ)/mess/systems/aim65.o

$(OBJ)/vc4000.a :   \
	$(OBJ)/mess/sndhrdw/vc4000.o	\
	$(OBJ)/mess/systems/vc4000.o	\
	$(OBJ)/mess/vidhrdw/vc4000.o	\

$(OBJ)/tangerin.a :\
	$(OBJ)/mess/devices/mfmdisk.o	\
	$(OBJ)/mess/vidhrdw/microtan.o	\
	$(OBJ)/mess/machine/microtan.o	\
	$(OBJ)/mess/systems/microtan.o	\
	$(OBJ)/mess/formats/oric_tap.o	\
	$(OBJ)/mess/systems/oric.o		\
	$(OBJ)/mess/vidhrdw/oric.o		\
	$(OBJ)/mess/machine/oric.o		\

$(OBJ)/vtech.a :   \
	$(OBJ)/mess/vidhrdw/vtech1.o	\
	$(OBJ)/mess/machine/vtech1.o	\
	$(OBJ)/mess/systems/vtech1.o	\
	$(OBJ)/mess/vidhrdw/vtech2.o	\
	$(OBJ)/mess/machine/vtech2.o	\
	$(OBJ)/mess/systems/vtech2.o	\
	$(OBJ)/mess/formats/vt_cas.o	\
	$(OBJ)/mess/formats/vt_dsk.o	\

$(OBJ)/jupiter.a : \
	$(OBJ)/mess/systems/jupiter.o	\
	$(OBJ)/mess/vidhrdw/jupiter.o	\
	$(OBJ)/mess/machine/jupiter.o	\

$(OBJ)/mbee.a :    \
	$(OBJ)/mess/vidhrdw/mbee.o	 \
	$(OBJ)/mess/machine/mbee.o	 \
	$(OBJ)/mess/systems/mbee.o

$(OBJ)/advision.a: \
	$(OBJ)/mess/vidhrdw/advision.o \
	$(OBJ)/mess/machine/advision.o \
	$(OBJ)/mess/systems/advision.o

$(OBJ)/nascom1.a:  \
	$(OBJ)/mess/vidhrdw/nascom1.o  \
	$(OBJ)/mess/machine/nascom1.o  \
	$(OBJ)/mess/systems/nascom1.o

$(OBJ)/cpschngr.a: \
	$(OBJ)/machine/eeprom.o	     \
	$(OBJ)/mess/systems/cpschngr.o \
	$(OBJ)/vidhrdw/cps1.o

$(OBJ)/mtx.a:	   \
	$(OBJ)/mess/systems/mtx.o

$(OBJ)/acorn.a:    \
	$(OBJ)/mess/machine/i8271.o	 \
	$(OBJ)/mess/machine/upd7002.o  \
	$(OBJ)/mess/vidhrdw/saa505x.o	     \
	$(OBJ)/mess/vidhrdw/bbc.o	     \
	$(OBJ)/mess/machine/bbc.o	     \
	$(OBJ)/mess/systems/bbc.o	     \
	$(OBJ)/mess/systems/a310.o	 \
	$(OBJ)/mess/systems/z88.o	     \
	$(OBJ)/mess/vidhrdw/z88.o      \
	$(OBJ)/mess/vidhrdw/atom.o	 \
	$(OBJ)/mess/systems/atom.o	 \
	$(OBJ)/mess/machine/atom.o	 \
	$(OBJ)/mess/formats/uef_cas.o	\
	$(OBJ)/mess/vidhrdw/electron.o	\
	$(OBJ)/mess/machine/electron.o	\
	$(OBJ)/mess/systems/electron.o

$(OBJ)/samcoupe.a: \
	$(OBJ)/mess/vidhrdw/coupe.o	 \
	$(OBJ)/mess/systems/coupe.o	\
	$(OBJ)/mess/machine/coupe.o	 \

$(OBJ)/sharp.a:    \
	$(OBJ)/mess/vidhrdw/mz700.o		\
	$(OBJ)/mess/systems/mz700.o		\
	$(OBJ)/mess/formats/mz_cas.o	\
	$(OBJ)/mess/systems/pocketc.o	\
	$(OBJ)/mess/vidhrdw/pc1401.o	\
	$(OBJ)/mess/machine/pc1401.o	\
	$(OBJ)/mess/vidhrdw/pc1403.o	\
	$(OBJ)/mess/machine/pc1403.o	\
	$(OBJ)/mess/vidhrdw/pc1350.o	\
	$(OBJ)/mess/machine/pc1350.o	\
	$(OBJ)/mess/vidhrdw/pc1251.o	\
	$(OBJ)/mess/machine/pc1251.o	\
	$(OBJ)/mess/vidhrdw/pocketc.o	\
	$(OBJ)/mess/machine/mz700.o		\

$(OBJ)/hp48.a:     \
	$(OBJ)/mess/machine/hp48.o     \
	$(OBJ)/mess/vidhrdw/hp48.o     \
	$(OBJ)/mess/systems/hp48.o

$(OBJ)/aquarius.a: \
	$(OBJ)/mess/systems/aquarius.o	\
	$(OBJ)/mess/vidhrdw/aquarius.o \
	$(OBJ)/mess/machine/aquarius.o \

$(OBJ)/exidy.a:    \
	$(OBJ)/mess/machine/hd6402.o     \
	$(OBJ)/mess/systems/exidy.o		\
	$(OBJ)/mess/vidhrdw/exidy.o      \

$(OBJ)/galaxy.a:   \
	$(OBJ)/mess/vidhrdw/galaxy.o   \
	$(OBJ)/mess/systems/galaxy.o	\
	$(OBJ)/mess/machine/galaxy.o   \

$(OBJ)/lviv.a:   \
	$(OBJ)/mess/vidhrdw/lviv.o   \
	$(OBJ)/mess/systems/lviv.o   \
	$(OBJ)/mess/machine/lviv.o   \
	$(OBJ)/mess/formats/lviv_lvt.o

$(OBJ)/pmd85.a:   \
	$(OBJ)/mess/vidhrdw/pmd85.o   \
	$(OBJ)/mess/systems/pmd85.o   \
	$(OBJ)/mess/machine/pmd85.o   \
	$(OBJ)/mess/formats/pmd_pmd.o

$(OBJ)/magnavox.a: \
	$(OBJ)/mess/machine/odyssey2.o \
	$(OBJ)/mess/vidhrdw/odyssey2.o \
	$(OBJ)/mess/sndhrdw/odyssey2.o \
	$(OBJ)/mess/systems/odyssey2.o

$(OBJ)/teamconc.a: \
	$(OBJ)/mess/vidhrdw/comquest.o \
	$(OBJ)/mess/systems/comquest.o

$(OBJ)/svision.a:  \
	$(OBJ)/mess/systems/svision.o \
	$(OBJ)/mess/sndhrdw/svision.o

$(OBJ)/lynx.a:     \
	$(OBJ)/mess/systems/lynx.o     \
	$(OBJ)/mess/sndhrdw/lynx.o     \
	$(OBJ)/mess/machine/lynx.o

$(OBJ)/mk1.a:      \
	$(OBJ)/mess/cpu/f8/f3853.o	 \
	$(OBJ)/mess/vidhrdw/mk1.o      \
	$(OBJ)/mess/systems/mk1.o

$(OBJ)/mk2.a:      \
	$(OBJ)/mess/vidhrdw/mk2.o      \
	$(OBJ)/mess/systems/mk2.o

$(OBJ)/ssystem3.a: \
	$(OBJ)/mess/vidhrdw/ssystem3.o \
	$(OBJ)/mess/systems/ssystem3.o

$(OBJ)/motorola.a: \
	$(OBJ)/mess/vidhrdw/mekd2.o    \
	$(OBJ)/mess/machine/mekd2.o    \
	$(OBJ)/mess/systems/mekd2.o

$(OBJ)/svi.a:      \
	$(OBJ)/mess/machine/svi318.o   \
	$(OBJ)/mess/systems/svi318.o   \
	$(OBJ)/mess/formats/svi_cas.o

$(OBJ)/intv.a:     \
	$(OBJ)/mess/vidhrdw/intv.o	\
	$(OBJ)/mess/vidhrdw/stic.o	\
	$(OBJ)/mess/machine/intv.o	\
	$(OBJ)/mess/sndhrdw/intv.o	\
	$(OBJ)/mess/systems/intv.o

$(OBJ)/apf.a:      \
	$(OBJ)/mess/systems/apf.o	\
	$(OBJ)/mess/machine/apf.o	\
	$(OBJ)/mess/vidhrdw/apf.o   \
	$(OBJ)/mess/formats/apf_apt.o

$(OBJ)/sord.a:     \
	$(OBJ)/mess/systems/sord.o	\
	$(OBJ)/mess/formats/sord_cas.o

$(OBJ)/tatung.a:     \
	$(OBJ)/mess/systems/einstein.o

$(OBJ)/sony.a:     \
	$(OBJ)/mess/systems/psx.o	\
	$(OBJ)/machine/psx.o	\
	$(OBJ)/vidhrdw/psx.o

$(OBJ)/dai.a:     \
	$(OBJ)/mess/systems/dai.o     \
	$(OBJ)/mess/vidhrdw/dai.o     \
	$(OBJ)/mess/sndhrdw/dai.o     \
	$(OBJ)/mess/machine/tms5501.o \
	$(OBJ)/mess/machine/dai.o     \

$(OBJ)/concept.a:  \
	$(OBJ)/mess/systems/concept.o   \
	$(OBJ)/mess/machine/concept.o	\
	$(OBJ)/mess/machine/corvushd.o

$(OBJ)/bandai.a:     \
	$(OBJ)/mess/systems/wswan.o   \
	$(OBJ)/mess/machine/wswan.o   \
	$(OBJ)/mess/vidhrdw/wswan.o   \
	$(OBJ)/mess/sndhrdw/wswan.o

$(OBJ)/compis.a:					\
	$(OBJ)/mess/systems/compis.o	\
	$(OBJ)/mess/machine/compis.o	\
	$(OBJ)/mess/machine/mm58274c.o	\
	$(OBJ)/mess/formats/cpis_dsk.o	\
	$(OBJ)/mess/vidhrdw/i82720.o 

$(OBJ)/multitch.a:					\
	$(OBJ)/mess/systems/mpf1.o		\

$(OBJ)/telmac.a:					\
	$(OBJ)/mess/systems/telmac.o	\
	$(OBJ)/mess/vidhrdw/cdp186x.o	\

$(OBJ)/exeltel.a:					\
	$(OBJ)/mess/systems/exelv.o		\

$(OBJ)/tx0.a:				\
	$(OBJ)/mess/vidhrdw/crt.o	\
	$(OBJ)/mess/systems/tx0.o	\
	$(OBJ)/mess/machine/tx0.o	\
	$(OBJ)/mess/vidhrdw/tx0.o	\

$(OBJ)/luxor.a:					\
	$(OBJ)/mess/systems/abc80.o	\

$(OBJ)/sgi.a:						\
	$(OBJ)/mess/machine/sgi.o		\
	$(OBJ)/mess/systems/ip20.o		\
	$(OBJ)/mess/systems/ip22.o	\
	$(OBJ)/mess/machine/wd33c93.o \
	$(OBJ)/machine/scsihd.o	\
	$(OBJ)/machine/scsicd.o	\
	$(OBJ)/mess/vidhrdw/newport.o

$(OBJ)/primo.a:				\
	$(OBJ)/mess/systems/primo.o	\
	$(OBJ)/mess/machine/primo.o	\
	$(OBJ)/mess/vidhrdw/primo.o	\
	$(OBJ)/mess/formats/primoptp.o

$(OBJ)/be.a:						\
	$(OBJ)/mess/systems/bebox.o		\
	$(OBJ)/mess/machine/bebox.o		\
	$(OBJ)/machine/pci.o		\
	$(OBJ)/mess/machine/mpc105.o	\
	$(OBJ)/mess/vidhrdw/cirrus.o	\
	$(OBJ)/machine/intelfsh.o		\
	$(OBJ)/machine/53c810.o

$(OBJ)/tiger.a:				\
	$(OBJ)/mess/systems/gamecom.o	\
	$(OBJ)/mess/machine/gamecom.o	\
	$(OBJ)/mess/vidhrdw/gamecom.o

# MESS specific core $(OBJ)s
COREOBJS +=							\
	$(EXPAT)						\
	$(ZLIB)							\
	$(OBJ)/vidhrdw/tms9928a.o		\
	$(OBJ)/machine/8255ppi.o		\
	$(OBJ)/machine/6522via.o		\
	$(OBJ)/machine/6821pia.o		\
	$(OBJ)/machine/z80ctc.o			\
	$(OBJ)/machine/z80pio.o			\
	$(OBJ)/machine/z80sio.o			\
	$(OBJ)/machine/idectrl.o		\
	$(OBJ)/machine/6532riot.o		\
	$(OBJ)/mess/mess.o				\
	$(OBJ)/mess/mesvalid.o			\
	$(OBJ)/mess/image.o				\
	$(OBJ)/mess/messdriv.o			\
	$(OBJ)/mess/device.o			\
	$(OBJ)/mess/hashfile.o			\
	$(OBJ)/mess/inputx.o			\
	$(OBJ)/mess/unicode.o			\
	$(OBJ)/mess/artworkx.o			\
	$(OBJ)/mess/mesintrf.o			\
	$(OBJ)/mess/filemngr.o			\
	$(OBJ)/mess/tapectrl.o			\
	$(OBJ)/mess/compcfg.o			\
	$(OBJ)/mess/utils.o				\
	$(OBJ)/mess/eventlst.o			\
	$(OBJ)/mess/mscommon.o			\
	$(OBJ)/mess/pool.o				\
	$(OBJ)/mess/cheatms.o			\
	$(OBJ)/mess/opresolv.o			\
	$(OBJ)/mess/mui_text.o			\
	$(OBJ)/mess/infomess.o			\
	$(OBJ)/mess/formats/ioprocs.o	\
	$(OBJ)/mess/formats/flopimg.o	\
	$(OBJ)/mess/formats/cassimg.o	\
	$(OBJ)/mess/formats/basicdsk.o	\
	$(OBJ)/mess/formats/pc_dsk.o	\
	$(OBJ)/mess/devices/mflopimg.o	\
	$(OBJ)/mess/devices/cassette.o	\
	$(OBJ)/mess/devices/cartslot.o	\
	$(OBJ)/mess/devices/printer.o	\
	$(OBJ)/mess/devices/bitbngr.o	\
	$(OBJ)/mess/devices/snapquik.o	\
	$(OBJ)/mess/devices/basicdsk.o	\
	$(OBJ)/mess/devices/flopdrv.o	\
	$(OBJ)/mess/devices/harddriv.o	\
	$(OBJ)/mess/devices/idedrive.o	\
	$(OBJ)/mess/devices/dsk.o		\
	$(OBJ)/mess/devices/z80bin.o	\
	$(OBJ)/mess/devices/chd_cd.o	\
	$(OBJ)/mess/machine/6551.o		\
	$(OBJ)/mess/machine/smartmed.o	\
	$(OBJ)/mess/vidhrdw/m6847.o		\
	$(OBJ)/mess/vidhrdw/m6845.o		\
	$(OBJ)/mess/machine/msm8251.o  \
	$(OBJ)/mess/machine/tc8521.o   \
	$(OBJ)/mess/vidhrdw/v9938.o    \
	$(OBJ)/mess/vidhrdw/crtc6845.o \
	$(OBJ)/mess/machine/28f008sa.o \
	$(OBJ)/mess/machine/am29f080.o \
	$(OBJ)/mess/machine/rriot.o    \
	$(OBJ)/mess/machine/riot6532.o \
	$(OBJ)/machine/pit8253.o  \
	$(OBJ)/machine/mc146818.o \
	$(OBJ)/mess/machine/uart8250.o \
	$(OBJ)/mess/machine/pc_mouse.o \
	$(OBJ)/mess/machine/pclpt.o    \
	$(OBJ)/mess/machine/centroni.o \
	$(OBJ)/machine/pckeybrd.o \
	$(OBJ)/mess/machine/d88.o      \
	$(OBJ)/mess/machine/nec765.o   \
	$(OBJ)/mess/machine/wd17xx.o   \
	$(OBJ)/mess/machine/serial.o   \
	$(OBJ)/mess/formats/wavfile.o



# additional tools
TOOLS = dat2html$(EXE) messtest$(EXE) chdman$(EXE) messdocs$(EXE) imgtool$(EXE)

include mess/tools/imgtool/imgtool.mak

DAT2HTML_OBJS =								\
	$(OBJ)/mamecore.o						\
	$(OBJ)/mess/tools/dat2html/dat2html.o	\
	$(OBJ)/mess/tools/imgtool/stubs.o		\
	$(OBJ)/mess/utils.o						\

MESSDOCS_OBJS =								\
	$(OBJ)/mamecore.o						\
	$(OBJ)/mess/tools/messdocs/messdocs.o	\
	$(OBJ)/mess/utils.o						\
	$(OBJ)/mess/pool.o						\
	$(EXPAT)								\

MESSTEST_OBJS =								\
	$(EXPAT)								\
	$(IMGTOOL_LIB_OBJS)						\
	$(OBJ)/mess/pile.o						\
	$(OBJ)/mess/tools/messtest/main.o		\
	$(OBJ)/mess/tools/messtest/core.o		\
	$(OBJ)/mess/tools/messtest/testmess.o	\
	$(OBJ)/mess/tools/messtest/testimgt.o	\
	$(OBJ)/mess/tools/messtest/tststubs.o	\
	$(OBJ)/mess/tools/messtest/tstutils.o	\



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
