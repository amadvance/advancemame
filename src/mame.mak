###########################################################################
#
#   mame.mak
#
#   MAME target makefile
#
#   Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


#-------------------------------------------------
# mamedriv.c is MAME-specific and contains the
# list of drivers
#-------------------------------------------------

COREOBJS += $(OBJ)/mamedriv.o



#-------------------------------------------------
# specify available CPU cores; some of these are
# only for MESS and so aren't included
#-------------------------------------------------

CPUS += Z80
CPUS += Z180
CPUS += 8080
CPUS += 8085A
CPUS += M6502
CPUS += M65C02
CPUS += M65SC02
#CPUS += M65CE02
#CPUS += M6509
CPUS += M6510
#CPUS += M6510T
#CPUS += M7501
#CPUS += M8502
CPUS += N2A03
CPUS += DECO16
#CPUS += M4510
CPUS += H6280
CPUS += I86
CPUS += I88
CPUS += I186
#CPUS += I188
#CPUS += I286
CPUS += V20
CPUS += V30
CPUS += V33
CPUS += V60
CPUS += V70
CPUS += I8035
CPUS += I8039
CPUS += I8048
CPUS += N7751
CPUS += I8X41
CPUS += I8051
CPUS += I8052
CPUS += I8751
CPUS += I8752
CPUS += M6800
CPUS += M6801
CPUS += M6802
CPUS += M6803
CPUS += M6808
CPUS += HD63701
CPUS += NSC8105
CPUS += M6805
CPUS += M68705
CPUS += HD63705
CPUS += HD6309
CPUS += M6809
CPUS += M6809E
CPUS += KONAMI
CPUS += M68000
CPUS += M68010
CPUS += M68EC020
CPUS += M68020
CPUS += M68040
CPUS += T11
CPUS += S2650
CPUS += TMS34010
CPUS += TMS34020
#CPUS += TMS9900
#CPUS += TMS9940
CPUS += TMS9980
#CPUS += TMS9985
#CPUS += TMS9989
CPUS += TMS9995
#CPUS += TMS99105A
#CPUS += TMS99110A
CPUS += Z8000
CPUS += TMS32010
CPUS += TMS32025
CPUS += TMS32026
CPUS += TMS32031
CPUS += TMS32051
CPUS += CCPU
CPUS += ADSP2100
CPUS += ADSP2101
CPUS += ADSP2104
CPUS += ADSP2105
CPUS += ADSP2115
CPUS += ADSP2181
CPUS += PSXCPU
CPUS += ASAP
CPUS += UPD7810
CPUS += UPD7807
CPUS += ARM
CPUS += ARM7
CPUS += JAGUAR
CPUS += R3000
CPUS += R4600
CPUS += R4650
CPUS += R4700
CPUS += R5000
CPUS += QED5271
CPUS += RM7000
CPUS += SH2
CPUS += DSP32C
#CPUS += PIC16C54
CPUS += PIC16C55
#CPUS += PIC16C56
CPUS += PIC16C57
#CPUS += PIC16C58
CPUS += G65816
CPUS += SPC700
CPUS += E116T
#CPUS += E116XT
#CPUS += E116XS
#CPUS += E116XSR
CPUS += E132N
#CPUS += E132T
#CPUS += E132XN
CPUS += E132XT
#CPUS += E132XS
#CPUS += E132XSR
CPUS += GMS30C2116
#CPUS += GMS30C2132
#CPUS += GMS30C2216
#CPUS += GMS30C2232
CPUS += I386
#CPUS += I486
CPUS += PENTIUM
CPUS += MEDIAGX
CPUS += I960
CPUS += H83002
CPUS += V810
CPUS += M37702
CPUS += M37710
CPUS += PPC403
CPUS += PPC602
CPUS += PPC603
CPUS += SE3208
CPUS += MC68HC11
CPUS += ADSP21062
CPUS += DSP56156
CPUS += RSP
CPUS += ALPHA8201
CPUS += ALPHA8301


#-------------------------------------------------
# specify available sound cores; some of these are
# only for MESS and so aren't included
#-------------------------------------------------

SOUNDS += CUSTOM
SOUNDS += SAMPLES
SOUNDS += DAC
SOUNDS += DMADAC
SOUNDS += DISCRETE
SOUNDS += AY8910
SOUNDS += YM2203
SOUNDS += YM2151
SOUNDS += YM2608
SOUNDS += YM2610
SOUNDS += YM2610B
SOUNDS += YM2612
SOUNDS += YM3438
SOUNDS += YM2413
SOUNDS += YM3812
SOUNDS += YMZ280B
SOUNDS += YM3526
SOUNDS += Y8950
SOUNDS += SN76477
SOUNDS += SN76496
SOUNDS += POKEY
SOUNDS += TIA
SOUNDS += NES
SOUNDS += ASTROCADE
SOUNDS += NAMCO
SOUNDS += NAMCO_15XX
SOUNDS += NAMCO_CUS30
SOUNDS += NAMCO_52XX
SOUNDS += NAMCO_54XX
SOUNDS += NAMCO_63701X
SOUNDS += NAMCONA
SOUNDS += TMS36XX
SOUNDS += TMS5110
SOUNDS += TMS5220
SOUNDS += VLM5030
SOUNDS += ADPCM
SOUNDS += OKIM6295
SOUNDS += MSM5205
SOUNDS += MSM5232
SOUNDS += UPD7759
SOUNDS += HC55516
SOUNDS += K005289
SOUNDS += K007232
SOUNDS += K051649
SOUNDS += K053260
SOUNDS += K054539
SOUNDS += SEGAPCM
SOUNDS += RF5C68
SOUNDS += CEM3394
SOUNDS += C140
SOUNDS += QSOUND
SOUNDS += SAA1099
SOUNDS += IREMGA20
SOUNDS += ES5503
SOUNDS += ES5505
SOUNDS += ES5506
SOUNDS += BSMT2000
SOUNDS += YMF262
SOUNDS += YMF278B
SOUNDS += GAELCO_CG1V
SOUNDS += GAELCO_GAE1
SOUNDS += X1_010
SOUNDS += MULTIPCM
SOUNDS += C6280
SOUNDS += SP0250
SOUNDS += SCSP
SOUNDS += YMF271
SOUNDS += PSXSPU
SOUNDS += CDDA
SOUNDS += ICS2115
SOUNDS += ST0016
SOUNDS += C352
SOUNDS += VRENDER0
#SOUNDS += VOTRAX
SOUNDS += ES8712
SOUNDS += RF5C400



#-------------------------------------------------
# this is the list of driver libaries that
# comprise MAME
#-------------------------------------------------

DRVLIBS = \
	$(OBJ)/alba.a \
	$(OBJ)/alliedl.a \
	$(OBJ)/alpha.a \
	$(OBJ)/amiga.a \
	$(OBJ)/atari.a \
	$(OBJ)/atlus.a \
	$(OBJ)/bfm.a \
	$(OBJ)/capcom.a \
	$(OBJ)/cinemat.a \
	$(OBJ)/comad.a \
	$(OBJ)/cvs.a \
	$(OBJ)/dataeast.a \
	$(OBJ)/dooyong.a \
	$(OBJ)/dynax.a \
	$(OBJ)/edevices.a \
	$(OBJ)/eolith.a \
	$(OBJ)/excelent.a \
	$(OBJ)/exidy.a \
	$(OBJ)/f32.a \
	$(OBJ)/fuuki.a \
	$(OBJ)/gaelco.a \
	$(OBJ)/gameplan.a \
	$(OBJ)/gametron.a \
	$(OBJ)/gottlieb.a \
	$(OBJ)/greyhnd.a \
	$(OBJ)/igs.a \
	$(OBJ)/irem.a \
	$(OBJ)/itech.a \
	$(OBJ)/jaleco.a \
	$(OBJ)/kaneko.a \
	$(OBJ)/konami.a \
	$(OBJ)/meadows.a \
	$(OBJ)/merit.a \
	$(OBJ)/metro.a \
	$(OBJ)/midcoin.a \
	$(OBJ)/midw8080.a \
	$(OBJ)/midway.a \
	$(OBJ)/namco.a \
	$(OBJ)/nasco.a \
	$(OBJ)/neogeo.a \
	$(OBJ)/nichibut.a \
	$(OBJ)/nintendo.a \
	$(OBJ)/nix.a \
	$(OBJ)/nmk.a \
	$(OBJ)/omori.a \
	$(OBJ)/olympia.a \
	$(OBJ)/orca.a \
	$(OBJ)/pacific.a \
	$(OBJ)/pacman.a \
	$(OBJ)/phoenix.a \
	$(OBJ)/playmark.a \
	$(OBJ)/psikyo.a \
	$(OBJ)/ramtek.a \
	$(OBJ)/rare.a \
	$(OBJ)/sanritsu.a \
	$(OBJ)/sega.a \
	$(OBJ)/seibu.a \
	$(OBJ)/seta.a \
	$(OBJ)/sigma.a \
	$(OBJ)/snk.a \
	$(OBJ)/stern.a \
	$(OBJ)/sun.a \
	$(OBJ)/suna.a \
	$(OBJ)/tad.a \
	$(OBJ)/taito.a \
	$(OBJ)/tatsumi.a \
	$(OBJ)/tch.a \
	$(OBJ)/tecfri.a \
	$(OBJ)/technos.a \
	$(OBJ)/tehkan.a \
	$(OBJ)/thepit.a \
	$(OBJ)/toaplan.a \
	$(OBJ)/tong.a \
	$(OBJ)/unico.a \
	$(OBJ)/univers.a \
	$(OBJ)/upl.a \
	$(OBJ)/valadon.a \
	$(OBJ)/veltmjr.a \
	$(OBJ)/venture.a \
	$(OBJ)/vsystem.a \
	$(OBJ)/yunsung.a \
	$(OBJ)/zaccaria.a \
	$(OBJ)/misc.a \
	$(OBJ)/shared.a \



#-------------------------------------------------
# the following files are general components and
# shared across a number of drivers
#-------------------------------------------------

$(OBJ)/shared.a: \
	$(OBJ)/machine/53c810.o \
	$(OBJ)/machine/6532riot.o \
	$(OBJ)/machine/6522via.o \
	$(OBJ)/machine/6526cia.o \
	$(OBJ)/machine/6821pia.o \
	$(OBJ)/machine/6840ptm.o \
	$(OBJ)/machine/6850acia.o \
	$(OBJ)/machine/7474.o \
	$(OBJ)/machine/74123.o \
	$(OBJ)/machine/74148.o \
	$(OBJ)/machine/74153.o \
	$(OBJ)/machine/74181.o \
	$(OBJ)/machine/8042kbdc.o \
	$(OBJ)/machine/8237dma.o \
	$(OBJ)/machine/8255ppi.o \
	$(OBJ)/machine/adc083x.o \
 	$(OBJ)/machine/am53cf96.o \
	$(OBJ)/machine/ds2404.o \
 	$(OBJ)/machine/idectrl.o \
 	$(OBJ)/machine/intelfsh.o \
	$(OBJ)/machine/mc146818.o \
	$(OBJ)/machine/nmk112.o \
	$(OBJ)/machine/pci.o \
	$(OBJ)/machine/pckeybrd.o \
	$(OBJ)/machine/pcshare.o \
	$(OBJ)/machine/pd4990a.o \
	$(OBJ)/machine/pic8259.o \
	$(OBJ)/machine/pit8253.o \
	$(OBJ)/machine/scsicd.o \
	$(OBJ)/machine/scsihd.o \
	$(OBJ)/machine/segacrpt.o \
 	$(OBJ)/machine/smc91c9x.o \
	$(OBJ)/machine/ticket.o \
	$(OBJ)/machine/timekpr.o \
	$(OBJ)/machine/tmp68301.o \
	$(OBJ)/machine/z80ctc.o \
	$(OBJ)/machine/z80pio.o \
	$(OBJ)/machine/z80sio.o \
	$(OBJ)/vidhrdw/crtc6845.o \
	$(OBJ)/vidhrdw/avgdvg.o \
	$(OBJ)/vidhrdw/poly.o \
	$(OBJ)/vidhrdw/res_net.o \
	$(OBJ)/vidhrdw/tlc34076.o \
	$(OBJ)/vidhrdw/tms34061.o \
 	$(OBJ)/vidhrdw/voodoo.o \



#-------------------------------------------------
# manufacturer-specific groupings for drivers
#-------------------------------------------------

$(OBJ)/alba.a: \
	$(OBJ)/drivers/hanaroku.o \
	$(OBJ)/drivers/rmhaihai.o \
	$(OBJ)/drivers/yumefuda.o \

$(OBJ)/alliedl.a: \
	$(OBJ)/drivers/ace.o \
	$(OBJ)/drivers/clayshoo.o $(OBJ)/machine/clayshoo.o $(OBJ)/vidhrdw/clayshoo.o \

$(OBJ)/alpha.a: \
	$(OBJ)/drivers/alpha68k.o $(OBJ)/vidhrdw/alpha68k.o \
	$(OBJ)/drivers/champbas.o $(OBJ)/vidhrdw/champbas.o \
	$(OBJ)/drivers/equites.o $(OBJ)/machine/equites.o $(OBJ)/vidhrdw/equites.o \
	$(OBJ)/drivers/exctsccr.o $(OBJ)/machine/exctsccr.o $(OBJ)/vidhrdw/exctsccr.o \
	$(OBJ)/drivers/meijinsn.o \
	$(OBJ)/drivers/shougi.o \
	$(OBJ)/drivers/talbot.o \

$(OBJ)/amiga.a: \
	$(OBJ)/machine/amiga.o $(OBJ)/sndhrdw/amiga.o $(OBJ)/vidhrdw/amiga.o \
	$(OBJ)/drivers/arcadia.o \
	$(OBJ)/drivers/mquake.o \
	$(OBJ)/drivers/upscope.o \

$(OBJ)/atari.a: \
 	$(OBJ)/drivers/atarigx2.o $(OBJ)/vidhrdw/atarigx2.o \
	$(OBJ)/drivers/arcadecl.o $(OBJ)/vidhrdw/arcadecl.o \
	$(OBJ)/drivers/asteroid.o $(OBJ)/machine/asteroid.o $(OBJ)/sndhrdw/asteroid.o $(OBJ)/sndhrdw/llander.o \
	$(OBJ)/drivers/atarifb.o $(OBJ)/machine/atarifb.o $(OBJ)/sndhrdw/atarifb.o $(OBJ)/vidhrdw/atarifb.o \
	$(OBJ)/drivers/atarig1.o $(OBJ)/vidhrdw/atarig1.o \
	$(OBJ)/drivers/atarig42.o $(OBJ)/vidhrdw/atarig42.o \
	$(OBJ)/drivers/atarigt.o $(OBJ)/vidhrdw/atarigt.o \
	$(OBJ)/drivers/atarisy1.o $(OBJ)/vidhrdw/atarisy1.o \
	$(OBJ)/drivers/atarisy2.o $(OBJ)/vidhrdw/atarisy2.o \
	$(OBJ)/drivers/atetris.o $(OBJ)/vidhrdw/atetris.o \
	$(OBJ)/drivers/avalnche.o $(OBJ)/machine/avalnche.o $(OBJ)/sndhrdw/avalnche.o $(OBJ)/vidhrdw/avalnche.o \
	$(OBJ)/drivers/badlands.o $(OBJ)/vidhrdw/badlands.o \
	$(OBJ)/drivers/batman.o $(OBJ)/vidhrdw/batman.o \
	$(OBJ)/drivers/beathead.o $(OBJ)/vidhrdw/beathead.o \
	$(OBJ)/drivers/blstroid.o $(OBJ)/vidhrdw/blstroid.o \
	$(OBJ)/drivers/boxer.o $(OBJ)/vidhrdw/boxer.o \
	$(OBJ)/drivers/bsktball.o $(OBJ)/machine/bsktball.o $(OBJ)/sndhrdw/bsktball.o $(OBJ)/vidhrdw/bsktball.o \
	$(OBJ)/drivers/bwidow.o \
	$(OBJ)/drivers/bzone.o $(OBJ)/sndhrdw/bzone.o \
	$(OBJ)/drivers/canyon.o $(OBJ)/sndhrdw/canyon.o $(OBJ)/vidhrdw/canyon.o \
	$(OBJ)/drivers/cball.o \
	$(OBJ)/drivers/ccastles.o $(OBJ)/vidhrdw/ccastles.o \
	$(OBJ)/drivers/centiped.o $(OBJ)/vidhrdw/centiped.o \
	$(OBJ)/drivers/cloak.o $(OBJ)/vidhrdw/cloak.o \
	$(OBJ)/drivers/cloud9.o $(OBJ)/vidhrdw/cloud9.o \
	$(OBJ)/drivers/cojag.o $(OBJ)/sndhrdw/jaguar.o $(OBJ)/vidhrdw/jaguar.o \
	$(OBJ)/drivers/copsnrob.o $(OBJ)/machine/copsnrob.o $(OBJ)/vidhrdw/copsnrob.o \
	$(OBJ)/drivers/cyberbal.o $(OBJ)/sndhrdw/cyberbal.o $(OBJ)/vidhrdw/cyberbal.o \
	$(OBJ)/drivers/destroyr.o $(OBJ)/vidhrdw/destroyr.o \
	$(OBJ)/drivers/dragrace.o $(OBJ)/sndhrdw/dragrace.o $(OBJ)/vidhrdw/dragrace.o \
	$(OBJ)/drivers/eprom.o $(OBJ)/vidhrdw/eprom.o \
	$(OBJ)/drivers/firetrk.o $(OBJ)/sndhrdw/firetrk.o $(OBJ)/vidhrdw/firetrk.o \
	$(OBJ)/drivers/flyball.o $(OBJ)/vidhrdw/flyball.o \
	$(OBJ)/drivers/foodf.o $(OBJ)/vidhrdw/foodf.o \
	$(OBJ)/drivers/gauntlet.o $(OBJ)/vidhrdw/gauntlet.o \
	$(OBJ)/drivers/harddriv.o $(OBJ)/machine/harddriv.o $(OBJ)/sndhrdw/harddriv.o $(OBJ)/vidhrdw/harddriv.o \
	$(OBJ)/drivers/irobot.o $(OBJ)/machine/irobot.o $(OBJ)/vidhrdw/irobot.o \
	$(OBJ)/drivers/jedi.o $(OBJ)/vidhrdw/jedi.o \
	$(OBJ)/drivers/klax.o $(OBJ)/vidhrdw/klax.o \
	$(OBJ)/drivers/liberatr.o $(OBJ)/vidhrdw/liberatr.o \
	$(OBJ)/drivers/mediagx.o \
	$(OBJ)/drivers/mgolf.o \
	$(OBJ)/drivers/mhavoc.o $(OBJ)/machine/mhavoc.o \
	$(OBJ)/drivers/missile.o $(OBJ)/vidhrdw/missile.o \
	$(OBJ)/drivers/nitedrvr.o $(OBJ)/machine/nitedrvr.o $(OBJ)/sndhrdw/nitedrvr.o $(OBJ)/vidhrdw/nitedrvr.o \
	$(OBJ)/drivers/offtwall.o $(OBJ)/vidhrdw/offtwall.o \
	$(OBJ)/drivers/orbit.o $(OBJ)/sndhrdw/orbit.o $(OBJ)/vidhrdw/orbit.o \
	$(OBJ)/drivers/poolshrk.o $(OBJ)/sndhrdw/poolshrk.o $(OBJ)/vidhrdw/poolshrk.o \
	$(OBJ)/drivers/quantum.o \
	$(OBJ)/drivers/rampart.o $(OBJ)/vidhrdw/rampart.o \
	$(OBJ)/drivers/relief.o $(OBJ)/vidhrdw/relief.o \
	$(OBJ)/drivers/runaway.o $(OBJ)/vidhrdw/runaway.o \
	$(OBJ)/drivers/sbrkout.o $(OBJ)/machine/sbrkout.o $(OBJ)/vidhrdw/sbrkout.o \
	$(OBJ)/drivers/shuuz.o $(OBJ)/vidhrdw/shuuz.o \
	$(OBJ)/drivers/skullxbo.o $(OBJ)/vidhrdw/skullxbo.o \
	$(OBJ)/drivers/skydiver.o $(OBJ)/sndhrdw/skydiver.o $(OBJ)/vidhrdw/skydiver.o \
	$(OBJ)/drivers/skyraid.o $(OBJ)/vidhrdw/skyraid.o \
	$(OBJ)/drivers/sprint2.o $(OBJ)/sndhrdw/sprint2.o $(OBJ)/vidhrdw/sprint2.o \
	$(OBJ)/drivers/sprint4.o $(OBJ)/sndhrdw/sprint4.o $(OBJ)/vidhrdw/sprint4.o \
	$(OBJ)/drivers/sprint8.o $(OBJ)/vidhrdw/sprint8.o \
	$(OBJ)/drivers/starshp1.o $(OBJ)/vidhrdw/starshp1.o \
	$(OBJ)/drivers/starwars.o $(OBJ)/machine/starwars.o $(OBJ)/sndhrdw/starwars.o \
	$(OBJ)/drivers/subs.o $(OBJ)/machine/subs.o $(OBJ)/sndhrdw/subs.o $(OBJ)/vidhrdw/subs.o \
	$(OBJ)/drivers/tank8.o $(OBJ)/sndhrdw/tank8.o $(OBJ)/vidhrdw/tank8.o \
	$(OBJ)/drivers/tempest.o \
	$(OBJ)/drivers/thunderj.o $(OBJ)/vidhrdw/thunderj.o \
	$(OBJ)/drivers/toobin.o $(OBJ)/vidhrdw/toobin.o \
	$(OBJ)/drivers/tourtabl.o $(OBJ)/vidhrdw/tia.o \
	$(OBJ)/drivers/triplhnt.o $(OBJ)/sndhrdw/triplhnt.o $(OBJ)/vidhrdw/triplhnt.o \
	$(OBJ)/drivers/tunhunt.o $(OBJ)/vidhrdw/tunhunt.o \
	$(OBJ)/drivers/videopin.o $(OBJ)/sndhrdw/videopin.o $(OBJ)/vidhrdw/videopin.o \
	$(OBJ)/drivers/vindictr.o $(OBJ)/vidhrdw/vindictr.o \
	$(OBJ)/drivers/wolfpack.o $(OBJ)/vidhrdw/wolfpack.o \
	$(OBJ)/drivers/xybots.o $(OBJ)/vidhrdw/xybots.o \
	$(OBJ)/machine/asic65.o \
	$(OBJ)/machine/atari_vg.o \
	$(OBJ)/machine/atarigen.o \
	$(OBJ)/machine/mathbox.o \
	$(OBJ)/machine/slapstic.o \
	$(OBJ)/sndhrdw/atarijsa.o \
	$(OBJ)/sndhrdw/cage.o \
	$(OBJ)/sndhrdw/redbaron.o \
	$(OBJ)/vidhrdw/atarimo.o \
	$(OBJ)/vidhrdw/atarirle.o \

$(OBJ)/atlus.a: \
	$(OBJ)/drivers/blmbycar.o $(OBJ)/vidhrdw/blmbycar.o \
	$(OBJ)/drivers/ohmygod.o $(OBJ)/vidhrdw/ohmygod.o \
	$(OBJ)/drivers/powerins.o $(OBJ)/vidhrdw/powerins.o \

$(OBJ)/bfm.a: \
	$(OBJ)/drivers/bfm_sc2.o $(OBJ)/vidhrdw/bfm_adr2.o \
	$(OBJ)/drivers/mpu4.o \
	$(OBJ)/machine/lamps.o \
	$(OBJ)/machine/mmtr.o \
	$(OBJ)/machine/steppers.o \
	$(OBJ)/machine/vacfdisp.o \

$(OBJ)/capcom.a: \
	$(OBJ)/drivers/1942.o $(OBJ)/vidhrdw/1942.o \
	$(OBJ)/drivers/1943.o $(OBJ)/vidhrdw/1943.o \
	$(OBJ)/drivers/bionicc.o $(OBJ)/vidhrdw/bionicc.o \
	$(OBJ)/drivers/blktiger.o $(OBJ)/vidhrdw/blktiger.o \
	$(OBJ)/drivers/cbasebal.o $(OBJ)/vidhrdw/cbasebal.o \
	$(OBJ)/drivers/commando.o $(OBJ)/vidhrdw/commando.o \
	$(OBJ)/drivers/cps1.o $(OBJ)/vidhrdw/cps1.o \
	$(OBJ)/drivers/cps2.o \
	$(OBJ)/drivers/cps3.o \
	$(OBJ)/drivers/egghunt.o \
	$(OBJ)/drivers/fcrash.o \
	$(OBJ)/drivers/gng.o $(OBJ)/vidhrdw/gng.o \
	$(OBJ)/drivers/gunsmoke.o $(OBJ)/vidhrdw/gunsmoke.o \
	$(OBJ)/drivers/exedexes.o $(OBJ)/vidhrdw/exedexes.o \
	$(OBJ)/drivers/higemaru.o $(OBJ)/vidhrdw/higemaru.o \
	$(OBJ)/drivers/lastduel.o $(OBJ)/vidhrdw/lastduel.o \
	$(OBJ)/drivers/lwings.o $(OBJ)/vidhrdw/lwings.o \
	$(OBJ)/drivers/mitchell.o $(OBJ)/vidhrdw/mitchell.o \
	$(OBJ)/drivers/sf.o $(OBJ)/vidhrdw/sf.o \
	$(OBJ)/drivers/sidearms.o $(OBJ)/vidhrdw/sidearms.o \
	$(OBJ)/drivers/sonson.o $(OBJ)/vidhrdw/sonson.o \
	$(OBJ)/drivers/srumbler.o $(OBJ)/vidhrdw/srumbler.o \
	$(OBJ)/drivers/vulgus.o $(OBJ)/vidhrdw/vulgus.o \
	$(OBJ)/drivers/tigeroad.o $(OBJ)/vidhrdw/tigeroad.o \
	$(OBJ)/drivers/zn.o $(OBJ)/machine/znsec.o $(OBJ)/machine/at28c16.o $(OBJ)/machine/mb3773.o \
	$(OBJ)/machine/kabuki.o \

$(OBJ)/cinemat.a: \
	$(OBJ)/drivers/ataxx.o \
	$(OBJ)/drivers/cinemat.o $(OBJ)/sndhrdw/cinemat.o $(OBJ)/vidhrdw/cinemat.o \
	$(OBJ)/drivers/cchasm.o $(OBJ)/machine/cchasm.o $(OBJ)/sndhrdw/cchasm.o $(OBJ)/vidhrdw/cchasm.o \
	$(OBJ)/drivers/dlair.o \
	$(OBJ)/drivers/embargo.o \
	$(OBJ)/drivers/jack.o $(OBJ)/vidhrdw/jack.o \
	$(OBJ)/drivers/leland.o $(OBJ)/machine/leland.o $(OBJ)/sndhrdw/leland.o $(OBJ)/vidhrdw/leland.o \

$(OBJ)/comad.a: \
	$(OBJ)/drivers/funybubl.o $(OBJ)/vidhrdw/funybubl.o \
	$(OBJ)/drivers/galspnbl.o $(OBJ)/vidhrdw/galspnbl.o \
	$(OBJ)/drivers/pushman.o $(OBJ)/vidhrdw/pushman.o \
	$(OBJ)/drivers/zerozone.o $(OBJ)/vidhrdw/zerozone.o \

$(OBJ)/cvs.a: \
	$(OBJ)/drivers/cvs.o $(OBJ)/vidhrdw/cvs.o $(OBJ)/vidhrdw/s2636.o \
	$(OBJ)/drivers/quasar.o $(OBJ)/vidhrdw/quasar.o \

$(OBJ)/dataeast.a: \
	$(OBJ)/drivers/actfancr.o $(OBJ)/vidhrdw/actfancr.o \
	$(OBJ)/drivers/astrof.o $(OBJ)/sndhrdw/astrof.o $(OBJ)/vidhrdw/astrof.o \
	$(OBJ)/drivers/backfire.o \
	$(OBJ)/drivers/battlera.o $(OBJ)/vidhrdw/battlera.o \
	$(OBJ)/drivers/boogwing.o $(OBJ)/vidhrdw/boogwing.o \
	$(OBJ)/drivers/brkthru.o $(OBJ)/vidhrdw/brkthru.o \
	$(OBJ)/drivers/btime.o $(OBJ)/machine/btime.o $(OBJ)/vidhrdw/btime.o \
	$(OBJ)/drivers/bwing.o $(OBJ)/vidhrdw/bwing.o \
	$(OBJ)/drivers/cbuster.o $(OBJ)/vidhrdw/cbuster.o \
	$(OBJ)/drivers/cninja.o $(OBJ)/vidhrdw/cninja.o \
	$(OBJ)/drivers/cntsteer.o \
	$(OBJ)/drivers/compgolf.o $(OBJ)/vidhrdw/compgolf.o \
	$(OBJ)/drivers/darkseal.o $(OBJ)/vidhrdw/darkseal.o \
	$(OBJ)/drivers/dassault.o $(OBJ)/vidhrdw/dassault.o \
	$(OBJ)/drivers/dblewing.o \
	$(OBJ)/drivers/dec0.o $(OBJ)/machine/dec0.o $(OBJ)/vidhrdw/dec0.o \
	$(OBJ)/drivers/dec8.o $(OBJ)/vidhrdw/dec8.o \
	$(OBJ)/drivers/deco_mlc.o $(OBJ)/vidhrdw/deco_mlc.o \
	$(OBJ)/drivers/deco156.o $(OBJ)/machine/deco156.o \
	$(OBJ)/drivers/deco32.o $(OBJ)/vidhrdw/deco32.o \
	$(OBJ)/drivers/decocass.o $(OBJ)/machine/decocass.o $(OBJ)/vidhrdw/decocass.o \
	$(OBJ)/drivers/dietgo.o $(OBJ)/vidhrdw/dietgo.o \
	$(OBJ)/drivers/exprraid.o $(OBJ)/vidhrdw/exprraid.o \
	$(OBJ)/drivers/firetrap.o $(OBJ)/vidhrdw/firetrap.o \
	$(OBJ)/drivers/funkyjet.o $(OBJ)/vidhrdw/funkyjet.o \
	$(OBJ)/drivers/karnov.o $(OBJ)/vidhrdw/karnov.o \
	$(OBJ)/drivers/kchamp.o $(OBJ)/vidhrdw/kchamp.o \
	$(OBJ)/drivers/kingobox.o $(OBJ)/vidhrdw/kingobox.o \
	$(OBJ)/drivers/lemmings.o $(OBJ)/vidhrdw/lemmings.o \
	$(OBJ)/drivers/liberate.o $(OBJ)/vidhrdw/liberate.o \
	$(OBJ)/drivers/madalien.o \
	$(OBJ)/drivers/madmotor.o $(OBJ)/vidhrdw/madmotor.o \
	$(OBJ)/drivers/metlclsh.o $(OBJ)/vidhrdw/metlclsh.o \
	$(OBJ)/drivers/pcktgal.o $(OBJ)/vidhrdw/pcktgal.o \
	$(OBJ)/drivers/pktgaldx.o $(OBJ)/vidhrdw/pktgaldx.o \
	$(OBJ)/drivers/rohga.o $(OBJ)/vidhrdw/rohga.o \
	$(OBJ)/drivers/shootout.o $(OBJ)/vidhrdw/shootout.o \
	$(OBJ)/drivers/sidepckt.o $(OBJ)/vidhrdw/sidepckt.o \
	$(OBJ)/drivers/simpl156.o $(OBJ)/vidhrdw/simpl156.o \
	$(OBJ)/drivers/sshangha.o $(OBJ)/vidhrdw/sshangha.o \
	$(OBJ)/drivers/stadhero.o $(OBJ)/vidhrdw/stadhero.o \
	$(OBJ)/drivers/supbtime.o $(OBJ)/vidhrdw/supbtime.o \
	$(OBJ)/drivers/tryout.o $(OBJ)/vidhrdw/tryout.o \
	$(OBJ)/drivers/tumbleb.o $(OBJ)/vidhrdw/tumbleb.o \
	$(OBJ)/drivers/tumbleb.o $(OBJ)/vidhrdw/tumbleb.o \
	$(OBJ)/drivers/tumblep.o $(OBJ)/vidhrdw/tumblep.o \
	$(OBJ)/drivers/vaportra.o $(OBJ)/vidhrdw/vaportra.o \
	$(OBJ)/machine/deco102.o \
	$(OBJ)/machine/decocrpt.o \
	$(OBJ)/machine/decoprot.o \
	$(OBJ)/vidhrdw/deco16ic.o \

$(OBJ)/dooyong.a: \
	$(OBJ)/drivers/dooyong.o $(OBJ)/vidhrdw/dooyong.o \
	$(OBJ)/drivers/gundealr.o $(OBJ)/vidhrdw/gundealr.o \

$(OBJ)/dynax.a: \
	$(OBJ)/drivers/cherrym2.o \
	$(OBJ)/drivers/ddenlovr.o \
	$(OBJ)/drivers/dynax.o $(OBJ)/vidhrdw/dynax.o \
	$(OBJ)/drivers/hnayayoi.o $(OBJ)/vidhrdw/hnayayoi.o \
	$(OBJ)/drivers/rcasino.o \
	$(OBJ)/drivers/realbrk.o $(OBJ)/vidhrdw/realbrk.o \
	$(OBJ)/drivers/royalmah.o \

$(OBJ)/edevices.a: \
	$(OBJ)/drivers/diverboy.o $(OBJ)/vidhrdw/diverboy.o \
	$(OBJ)/drivers/fantland.o $(OBJ)/vidhrdw/fantland.o \
	$(OBJ)/drivers/mwarr.o \
	$(OBJ)/drivers/mugsmash.o $(OBJ)/vidhrdw/mugsmash.o \
	$(OBJ)/drivers/stlforce.o $(OBJ)/vidhrdw/stlforce.o \
	$(OBJ)/drivers/ppmast93.o \
	$(OBJ)/drivers/twins.o \

$(OBJ)/eolith.a: \
	$(OBJ)/drivers/eolith.o $(OBJ)/vidhrdw/eolith.o \
	$(OBJ)/drivers/eolith16.o \

$(OBJ)/excelent.a: \
	$(OBJ)/drivers/aquarium.o $(OBJ)/vidhrdw/aquarium.o \
	$(OBJ)/drivers/gcpinbal.o $(OBJ)/vidhrdw/gcpinbal.o \
	$(OBJ)/drivers/vmetal.o \

$(OBJ)/exidy.a: \
	$(OBJ)/drivers/carpolo.o $(OBJ)/machine/carpolo.o $(OBJ)/vidhrdw/carpolo.o \
	$(OBJ)/drivers/circus.o $(OBJ)/sndhrdw/circus.o $(OBJ)/vidhrdw/circus.o \
	$(OBJ)/drivers/exidy.o $(OBJ)/sndhrdw/exidy.o $(OBJ)/vidhrdw/exidy.o \
	$(OBJ)/drivers/exidy440.o $(OBJ)/sndhrdw/exidy440.o $(OBJ)/vidhrdw/exidy440.o \
	$(OBJ)/drivers/maxaflex.o $(OBJ)/machine/atari.o $(OBJ)/vidhrdw/atari.o $(OBJ)/vidhrdw/antic.o $(OBJ)/vidhrdw/gtia.o \
	$(OBJ)/drivers/starfire.o $(OBJ)/vidhrdw/starfire.o \
	$(OBJ)/drivers/vertigo.o $(OBJ)/machine/vertigo.o $(OBJ)/vidhrdw/vertigo.o \
	$(OBJ)/drivers/victory.o $(OBJ)/vidhrdw/victory.o \
	$(OBJ)/sndhrdw/targ.o \

$(OBJ)/f32.a: \
	$(OBJ)/drivers/crospang.o $(OBJ)/vidhrdw/crospang.o \
	$(OBJ)/drivers/f-32.o \

$(OBJ)/fuuki.a: \
	$(OBJ)/drivers/fuukifg2.o $(OBJ)/vidhrdw/fuukifg2.o \
	$(OBJ)/drivers/fuukifg3.o $(OBJ)/vidhrdw/fuukifg3.o \

$(OBJ)/gaelco.a: \
	$(OBJ)/drivers/gaelco.o $(OBJ)/vidhrdw/gaelco.o \
	$(OBJ)/drivers/gaelco2.o $(OBJ)/machine/gaelco2.o $(OBJ)/vidhrdw/gaelco2.o \
	$(OBJ)/drivers/gaelco3d.o $(OBJ)/vidhrdw/gaelco3d.o \
	$(OBJ)/drivers/glass.o $(OBJ)/vidhrdw/glass.o \
	$(OBJ)/drivers/mastboy.o \
	$(OBJ)/drivers/splash.o $(OBJ)/vidhrdw/splash.o \
	$(OBJ)/drivers/targeth.o $(OBJ)/vidhrdw/targeth.o \
	$(OBJ)/drivers/thoop2.o $(OBJ)/vidhrdw/thoop2.o \
	$(OBJ)/drivers/xorworld.o $(OBJ)/vidhrdw/xorworld.o \
	$(OBJ)/drivers/wrally.o $(OBJ)/machine/wrally.o $(OBJ)/vidhrdw/wrally.o \

$(OBJ)/gameplan.a: \
	$(OBJ)/drivers/enigma2.o \
	$(OBJ)/drivers/gameplan.o \
	$(OBJ)/drivers/toratora.o \

$(OBJ)/gametron.a: \
	$(OBJ)/drivers/gotya.o $(OBJ)/sndhrdw/gotya.o $(OBJ)/vidhrdw/gotya.o \
	$(OBJ)/drivers/sbugger.o $(OBJ)/vidhrdw/sbugger.o \

$(OBJ)/gottlieb.a: \
	$(OBJ)/drivers/exterm.o $(OBJ)/vidhrdw/exterm.o \
	$(OBJ)/drivers/gottlieb.o $(OBJ)/sndhrdw/gottlieb.o $(OBJ)/vidhrdw/gottlieb.o \

$(OBJ)/greyhnd.a: \
	$(OBJ)/drivers/findout.o \
	$(OBJ)/drivers/getrivia.o \

$(OBJ)/igs.a: \
	$(OBJ)/drivers/csk.o $(OBJ)/vidhrdw/csk.o \
	$(OBJ)/drivers/goldstar.o $(OBJ)/vidhrdw/goldstar.o \
	$(OBJ)/drivers/igs_blit.o \
	$(OBJ)/drivers/iqblock.o $(OBJ)/vidhrdw/iqblock.o \
	$(OBJ)/drivers/lordgun.o $(OBJ)/vidhrdw/lordgun.o \
	$(OBJ)/drivers/pgm.o $(OBJ)/vidhrdw/pgm.o \
	$(OBJ)/drivers/tarzan.o \
	$(OBJ)/machine/pgmcrypt.o \
	$(OBJ)/machine/pgmprot.o \
	$(OBJ)/machine/pgmy2ks.o \

$(OBJ)/irem.a: \
	$(OBJ)/drivers/m62.o $(OBJ)/vidhrdw/m62.o \
	$(OBJ)/drivers/m72.o $(OBJ)/sndhrdw/m72.o $(OBJ)/vidhrdw/m72.o \
	$(OBJ)/drivers/m90.o $(OBJ)/vidhrdw/m90.o \
	$(OBJ)/drivers/m92.o $(OBJ)/vidhrdw/m92.o \
	$(OBJ)/drivers/m107.o $(OBJ)/vidhrdw/m107.o \
	$(OBJ)/drivers/mpatrol.o $(OBJ)/vidhrdw/mpatrol.o \
	$(OBJ)/drivers/olibochu.o \
	$(OBJ)/drivers/redalert.o $(OBJ)/sndhrdw/redalert.o $(OBJ)/vidhrdw/redalert.o \
	$(OBJ)/drivers/shisen.o $(OBJ)/vidhrdw/shisen.o \
	$(OBJ)/drivers/skychut.o $(OBJ)/vidhrdw/skychut.o \
	$(OBJ)/drivers/travrusa.o $(OBJ)/vidhrdw/travrusa.o \
	$(OBJ)/drivers/troangel.o $(OBJ)/vidhrdw/troangel.o \
	$(OBJ)/drivers/vigilant.o $(OBJ)/vidhrdw/vigilant.o \
	$(OBJ)/drivers/wilytowr.o \
	$(OBJ)/drivers/yard.o $(OBJ)/vidhrdw/yard.o \
	$(OBJ)/machine/irem_cpu.o \
	$(OBJ)/sndhrdw/fghtbskt.o \
	$(OBJ)/sndhrdw/irem.o \

$(OBJ)/itech.a: \
	$(OBJ)/drivers/capbowl.o $(OBJ)/vidhrdw/capbowl.o \
	$(OBJ)/drivers/itech8.o $(OBJ)/machine/slikshot.o $(OBJ)/vidhrdw/itech8.o \
	$(OBJ)/drivers/itech32.o $(OBJ)/vidhrdw/itech32.o \

$(OBJ)/jaleco.a: \
	$(OBJ)/drivers/aeroboto.o $(OBJ)/vidhrdw/aeroboto.o \
	$(OBJ)/drivers/argus.o $(OBJ)/vidhrdw/argus.o \
	$(OBJ)/drivers/bestleag.o \
	$(OBJ)/drivers/bigstrkb.o $(OBJ)/vidhrdw/bigstrkb.o \
	$(OBJ)/drivers/blueprnt.o $(OBJ)/vidhrdw/blueprnt.o \
	$(OBJ)/drivers/cischeat.o $(OBJ)/vidhrdw/cischeat.o \
	$(OBJ)/drivers/citycon.o $(OBJ)/vidhrdw/citycon.o \
	$(OBJ)/drivers/ddayjlc.o \
	$(OBJ)/drivers/exerion.o $(OBJ)/vidhrdw/exerion.o \
	$(OBJ)/drivers/fcombat.o $(OBJ)/vidhrdw/fcombat.o \
	$(OBJ)/drivers/ginganin.o $(OBJ)/vidhrdw/ginganin.o \
	$(OBJ)/drivers/homerun.o $(OBJ)/vidhrdw/homerun.o \
	$(OBJ)/drivers/megasys1.o $(OBJ)/vidhrdw/megasys1.o \
	$(OBJ)/drivers/momoko.o $(OBJ)/vidhrdw/momoko.o \
	$(OBJ)/drivers/ms32.o $(OBJ)/vidhrdw/ms32.o \
	$(OBJ)/drivers/psychic5.o $(OBJ)/vidhrdw/psychic5.o \
	$(OBJ)/drivers/pturn.o \
	$(OBJ)/drivers/skyfox.o $(OBJ)/vidhrdw/skyfox.o \
	$(OBJ)/drivers/stepstag.o \
	$(OBJ)/drivers/tetrisp2.o $(OBJ)/vidhrdw/tetrisp2.o \

$(OBJ)/kaneko.a: \
	$(OBJ)/drivers/airbustr.o $(OBJ)/vidhrdw/airbustr.o \
	$(OBJ)/drivers/djboy.o $(OBJ)/vidhrdw/djboy.o \
	$(OBJ)/drivers/galpanic.o $(OBJ)/vidhrdw/galpanic.o \
	$(OBJ)/drivers/galpani2.o $(OBJ)/vidhrdw/galpani2.o \
	$(OBJ)/drivers/galpani3.o \
	$(OBJ)/drivers/jchan.o \
	$(OBJ)/drivers/kaneko16.o $(OBJ)/machine/kaneko16.o $(OBJ)/vidhrdw/kaneko16.o \
	$(OBJ)/drivers/suprnova.o $(OBJ)/vidhrdw/suprnova.o \

$(OBJ)/konami.a: \
	$(OBJ)/drivers/88games.o $(OBJ)/vidhrdw/88games.o \
	$(OBJ)/drivers/ajax.o $(OBJ)/machine/ajax.o $(OBJ)/vidhrdw/ajax.o \
	$(OBJ)/drivers/aliens.o $(OBJ)/vidhrdw/aliens.o \
	$(OBJ)/drivers/amidar.o \
	$(OBJ)/drivers/asterix.o $(OBJ)/vidhrdw/asterix.o \
	$(OBJ)/drivers/battlnts.o $(OBJ)/vidhrdw/battlnts.o \
	$(OBJ)/drivers/bishi.o $(OBJ)/vidhrdw/bishi.o \
	$(OBJ)/drivers/bladestl.o $(OBJ)/vidhrdw/bladestl.o \
	$(OBJ)/drivers/blockhl.o $(OBJ)/vidhrdw/blockhl.o \
	$(OBJ)/drivers/bottom9.o $(OBJ)/vidhrdw/bottom9.o \
	$(OBJ)/drivers/chqflag.o $(OBJ)/vidhrdw/chqflag.o \
	$(OBJ)/drivers/circusc.o $(OBJ)/vidhrdw/circusc.o \
	$(OBJ)/drivers/combatsc.o $(OBJ)/vidhrdw/combatsc.o \
	$(OBJ)/drivers/contra.o $(OBJ)/vidhrdw/contra.o \
	$(OBJ)/drivers/crimfght.o $(OBJ)/vidhrdw/crimfght.o \
	$(OBJ)/drivers/dbz.o $(OBJ)/vidhrdw/dbz.o \
	$(OBJ)/drivers/ddrible.o $(OBJ)/vidhrdw/ddrible.o \
	$(OBJ)/drivers/djmain.o $(OBJ)/vidhrdw/djmain.o \
	$(OBJ)/drivers/fastfred.o $(OBJ)/vidhrdw/fastfred.o \
	$(OBJ)/drivers/fastlane.o $(OBJ)/vidhrdw/fastlane.o \
	$(OBJ)/drivers/finalizr.o $(OBJ)/vidhrdw/finalizr.o \
	$(OBJ)/drivers/flkatck.o $(OBJ)/vidhrdw/flkatck.o \
	$(OBJ)/drivers/frogger.o \
	$(OBJ)/drivers/gberet.o $(OBJ)/vidhrdw/gberet.o \
	$(OBJ)/drivers/gbusters.o $(OBJ)/vidhrdw/gbusters.o \
	$(OBJ)/drivers/gijoe.o $(OBJ)/vidhrdw/gijoe.o \
	$(OBJ)/drivers/gradius3.o $(OBJ)/vidhrdw/gradius3.o \
	$(OBJ)/drivers/gticlub.o \
	$(OBJ)/drivers/gyruss.o $(OBJ)/sndhrdw/gyruss.o $(OBJ)/vidhrdw/gyruss.o \
	$(OBJ)/drivers/hcastle.o $(OBJ)/vidhrdw/hcastle.o \
	$(OBJ)/drivers/hexion.o $(OBJ)/vidhrdw/hexion.o \
	$(OBJ)/drivers/hornet.o $(OBJ)/machine/konppc.o \
	$(OBJ)/drivers/hyperspt.o $(OBJ)/vidhrdw/hyperspt.o \
	$(OBJ)/drivers/ironhors.o $(OBJ)/vidhrdw/ironhors.o \
	$(OBJ)/drivers/jackal.o $(OBJ)/machine/jackal.o $(OBJ)/vidhrdw/jackal.o \
	$(OBJ)/drivers/jailbrek.o $(OBJ)/vidhrdw/jailbrek.o \
	$(OBJ)/drivers/junofrst.o \
	$(OBJ)/drivers/konamigq.o \
	$(OBJ)/drivers/konamigv.o \
	$(OBJ)/drivers/konamigx.o $(OBJ)/machine/konamigx.o $(OBJ)/vidhrdw/konamigx.o \
	$(OBJ)/drivers/konamim2.o \
	$(OBJ)/drivers/labyrunr.o $(OBJ)/vidhrdw/labyrunr.o \
	$(OBJ)/drivers/lethal.o $(OBJ)/vidhrdw/lethal.o \
	$(OBJ)/drivers/mainevt.o $(OBJ)/vidhrdw/mainevt.o \
	$(OBJ)/drivers/megazone.o $(OBJ)/vidhrdw/megazone.o \
	$(OBJ)/drivers/mikie.o $(OBJ)/vidhrdw/mikie.o \
	$(OBJ)/drivers/mogura.o \
	$(OBJ)/drivers/moo.o $(OBJ)/vidhrdw/moo.o \
	$(OBJ)/drivers/mystwarr.o $(OBJ)/vidhrdw/mystwarr.o \
	$(OBJ)/drivers/nemesis.o $(OBJ)/vidhrdw/nemesis.o \
	$(OBJ)/drivers/nwk-tr.o \
	$(OBJ)/drivers/overdriv.o $(OBJ)/vidhrdw/overdriv.o \
	$(OBJ)/drivers/pandoras.o $(OBJ)/vidhrdw/pandoras.o \
	$(OBJ)/drivers/parodius.o $(OBJ)/vidhrdw/parodius.o \
	$(OBJ)/drivers/pingpong.o $(OBJ)/vidhrdw/pingpong.o \
	$(OBJ)/drivers/plygonet.o $(OBJ)/vidhrdw/plygonet.o \
	$(OBJ)/drivers/pooyan.o $(OBJ)/vidhrdw/pooyan.o \
	$(OBJ)/drivers/qdrmfgp.o $(OBJ)/vidhrdw/qdrmfgp.o \
	$(OBJ)/drivers/rockrage.o $(OBJ)/vidhrdw/rockrage.o \
	$(OBJ)/drivers/rocnrope.o $(OBJ)/vidhrdw/rocnrope.o \
	$(OBJ)/drivers/rollerg.o $(OBJ)/vidhrdw/rollerg.o \
	$(OBJ)/drivers/rungun.o $(OBJ)/vidhrdw/rungun.o \
	$(OBJ)/drivers/sbasketb.o $(OBJ)/vidhrdw/sbasketb.o \
	$(OBJ)/drivers/scobra.o \
	$(OBJ)/drivers/scotrsht.o $(OBJ)/vidhrdw/scotrsht.o \
	$(OBJ)/drivers/scramble.o $(OBJ)/machine/scramble.o $(OBJ)/sndhrdw/scramble.o \
	$(OBJ)/drivers/shaolins.o $(OBJ)/vidhrdw/shaolins.o \
	$(OBJ)/drivers/simpsons.o $(OBJ)/machine/simpsons.o $(OBJ)/vidhrdw/simpsons.o \
	$(OBJ)/drivers/spy.o $(OBJ)/vidhrdw/spy.o \
	$(OBJ)/drivers/surpratk.o $(OBJ)/vidhrdw/surpratk.o \
	$(OBJ)/drivers/thunderx.o $(OBJ)/vidhrdw/thunderx.o \
	$(OBJ)/drivers/timeplt.o $(OBJ)/sndhrdw/timeplt.o $(OBJ)/vidhrdw/timeplt.o \
	$(OBJ)/drivers/tmnt.o $(OBJ)/vidhrdw/tmnt.o \
	$(OBJ)/drivers/tp84.o $(OBJ)/vidhrdw/tp84.o \
	$(OBJ)/drivers/trackfld.o $(OBJ)/machine/konami.o $(OBJ)/sndhrdw/trackfld.o $(OBJ)/vidhrdw/trackfld.o \
	$(OBJ)/drivers/tutankhm.o $(OBJ)/vidhrdw/tutankhm.o \
	$(OBJ)/drivers/twin16.o $(OBJ)/vidhrdw/twin16.o \
	$(OBJ)/drivers/ultraman.o $(OBJ)/vidhrdw/ultraman.o \
	$(OBJ)/drivers/vendetta.o $(OBJ)/vidhrdw/vendetta.o \
	$(OBJ)/drivers/wecleman.o $(OBJ)/vidhrdw/wecleman.o \
	$(OBJ)/drivers/xexex.o $(OBJ)/vidhrdw/xexex.o \
	$(OBJ)/drivers/xmen.o $(OBJ)/vidhrdw/xmen.o \
	$(OBJ)/drivers/yiear.o $(OBJ)/vidhrdw/yiear.o \
	$(OBJ)/drivers/zr107.o \
	$(OBJ)/vidhrdw/konamiic.o \

$(OBJ)/meadows.a: \
	$(OBJ)/drivers/lazercmd.o $(OBJ)/vidhrdw/lazercmd.o \
	$(OBJ)/drivers/meadows.o $(OBJ)/sndhrdw/meadows.o $(OBJ)/vidhrdw/meadows.o \

$(OBJ)/merit.a: \
	$(OBJ)/drivers/couple.o \
	$(OBJ)/drivers/merit.o \

$(OBJ)/metro.a: \
	$(OBJ)/drivers/hyprduel.o $(OBJ)/vidhrdw/hyprduel.o \
	$(OBJ)/drivers/metro.o $(OBJ)/vidhrdw/metro.o \
	$(OBJ)/drivers/rabbit.o \

$(OBJ)/midcoin.a: \
	$(OBJ)/drivers/wallc.o \
	$(OBJ)/drivers/wink.o \

$(OBJ)/midw8080.a: \
	$(OBJ)/drivers/8080bw.o $(OBJ)/machine/8080bw.o $(OBJ)/sndhrdw/8080bw.o $(OBJ)/vidhrdw/8080bw.o \
	$(OBJ)/drivers/m79amb.o $(OBJ)/vidhrdw/m79amb.o \
	$(OBJ)/drivers/rotaryf.o \
	$(OBJ)/drivers/sspeedr.o $(OBJ)/vidhrdw/sspeedr.o \

$(OBJ)/midway.a: \
	$(OBJ)/drivers/astrocde.o $(OBJ)/machine/astrocde.o $(OBJ)/vidhrdw/astrocde.o \
	$(OBJ)/drivers/balsente.o $(OBJ)/machine/balsente.o $(OBJ)/vidhrdw/balsente.o \
	$(OBJ)/drivers/gridlee.o $(OBJ)/sndhrdw/gridlee.o $(OBJ)/vidhrdw/gridlee.o \
	$(OBJ)/drivers/mcr.o $(OBJ)/machine/mcr.o $(OBJ)/sndhrdw/mcr.o $(OBJ)/vidhrdw/mcr.o \
	$(OBJ)/drivers/mcr3.o $(OBJ)/vidhrdw/mcr3.o \
	$(OBJ)/drivers/mcr68.o $(OBJ)/vidhrdw/mcr68.o \
	$(OBJ)/drivers/midtunit.o $(OBJ)/machine/midtunit.o $(OBJ)/vidhrdw/midtunit.o \
	$(OBJ)/drivers/midvunit.o $(OBJ)/vidhrdw/midvunit.o \
	$(OBJ)/drivers/midwunit.o $(OBJ)/machine/midwunit.o \
	$(OBJ)/drivers/midxunit.o \
	$(OBJ)/drivers/midyunit.o $(OBJ)/machine/midyunit.o $(OBJ)/vidhrdw/midyunit.o \
	$(OBJ)/drivers/midzeus.o \
	$(OBJ)/drivers/omegrace.o \
	$(OBJ)/drivers/seattle.o \
	$(OBJ)/drivers/vegas.o \
	$(OBJ)/drivers/williams.o $(OBJ)/machine/williams.o $(OBJ)/sndhrdw/williams.o $(OBJ)/vidhrdw/williams.o \
	$(OBJ)/machine/midwayic.o \
	$(OBJ)/sndhrdw/dcs.o \
	$(OBJ)/sndhrdw/gorf.o \
	$(OBJ)/sndhrdw/wow.o \

$(OBJ)/namco.a: \
	$(OBJ)/drivers/baraduke.o $(OBJ)/vidhrdw/baraduke.o \
	$(OBJ)/drivers/galaga.o $(OBJ)/vidhrdw/galaga.o \
	$(OBJ)/drivers/galaxian.o $(OBJ)/sndhrdw/galaxian.o $(OBJ)/vidhrdw/galaxian.o \
	$(OBJ)/drivers/gaplus.o $(OBJ)/machine/gaplus.o $(OBJ)/vidhrdw/gaplus.o \
	$(OBJ)/drivers/mappy.o $(OBJ)/vidhrdw/mappy.o \
	$(OBJ)/drivers/namcofl.o $(OBJ)/vidhrdw/namcofl.o \
	$(OBJ)/drivers/namcoic.o \
	$(OBJ)/drivers/namcona1.o $(OBJ)/vidhrdw/namcona1.o \
	$(OBJ)/drivers/namconb1.o $(OBJ)/vidhrdw/namconb1.o \
	$(OBJ)/drivers/namcond1.o $(OBJ)/machine/namcond1.o $(OBJ)/vidhrdw/ygv608.o \
	$(OBJ)/drivers/namcos1.o $(OBJ)/machine/namcos1.o $(OBJ)/vidhrdw/namcos1.o \
	$(OBJ)/drivers/namcos10.o \
	$(OBJ)/drivers/namcos11.o \
	$(OBJ)/drivers/namcos12.o \
	$(OBJ)/drivers/namcos2.o $(OBJ)/machine/namcos2.o $(OBJ)/vidhrdw/namcos2.o \
	$(OBJ)/drivers/namcos21.o $(OBJ)/vidhrdw/namcos21.o \
	$(OBJ)/drivers/namcos22.o $(OBJ)/vidhrdw/namcos22.o \
	$(OBJ)/drivers/namcos23.o \
	$(OBJ)/drivers/namcos86.o $(OBJ)/vidhrdw/namcos86.o \
	$(OBJ)/drivers/pacland.o $(OBJ)/vidhrdw/pacland.o \
	$(OBJ)/drivers/polepos.o $(OBJ)/sndhrdw/polepos.o $(OBJ)/vidhrdw/polepos.o \
	$(OBJ)/drivers/rallyx.o $(OBJ)/vidhrdw/rallyx.o \
	$(OBJ)/drivers/skykid.o $(OBJ)/vidhrdw/skykid.o \
	$(OBJ)/drivers/tankbatt.o $(OBJ)/vidhrdw/tankbatt.o \
	$(OBJ)/drivers/tceptor.o $(OBJ)/vidhrdw/tceptor.o \
	$(OBJ)/drivers/toypop.o $(OBJ)/vidhrdw/toypop.o \
	$(OBJ)/drivers/warpwarp.o $(OBJ)/sndhrdw/warpwarp.o $(OBJ)/vidhrdw/warpwarp.o \
	$(OBJ)/machine/namcoio.o \
	$(OBJ)/sndhrdw/geebee.o \
	$(OBJ)/sndhrdw/namcoc7x.o \
	$(OBJ)/vidhrdw/bosco.o \
	$(OBJ)/vidhrdw/digdug.o \
	$(OBJ)/machine/psx.o $(OBJ)/vidhrdw/psx.o \
	$(OBJ)/machine/xevious.o $(OBJ)/vidhrdw/xevious.o \

$(OBJ)/nasco.a: \
	$(OBJ)/drivers/crgolf.o $(OBJ)/vidhrdw/crgolf.o \
	$(OBJ)/drivers/suprgolf.o \

$(OBJ)/neogeo.a: \
	$(OBJ)/drivers/neogeo.o $(OBJ)/machine/neogeo.o $(OBJ)/vidhrdw/neogeo.o \
	$(OBJ)/machine/neoboot.o \
	$(OBJ)/machine/neocrypt.o \
	$(OBJ)/machine/neoprot.o \

$(OBJ)/nichibut.a: \
	$(OBJ)/drivers/armedf.o $(OBJ)/vidhrdw/armedf.o \
	$(OBJ)/drivers/bigfghtr.o \
	$(OBJ)/drivers/cclimber.o $(OBJ)/sndhrdw/cclimber.o $(OBJ)/vidhrdw/cclimber.o \
	$(OBJ)/drivers/clshroad.o $(OBJ)/vidhrdw/clshroad.o \
	$(OBJ)/drivers/cop01.o $(OBJ)/vidhrdw/cop01.o \
	$(OBJ)/drivers/dacholer.o \
	$(OBJ)/drivers/galivan.o $(OBJ)/vidhrdw/galivan.o \
	$(OBJ)/drivers/gomoku.o $(OBJ)/sndhrdw/gomoku.o $(OBJ)/vidhrdw/gomoku.o \
	$(OBJ)/drivers/hyhoo.o $(OBJ)/vidhrdw/hyhoo.o \
	$(OBJ)/drivers/magmax.o $(OBJ)/vidhrdw/magmax.o \
	$(OBJ)/drivers/nbmj8688.o $(OBJ)/vidhrdw/nbmj8688.o \
	$(OBJ)/drivers/nbmj8891.o $(OBJ)/vidhrdw/nbmj8891.o \
	$(OBJ)/drivers/nbmj8991.o $(OBJ)/vidhrdw/nbmj8991.o \
	$(OBJ)/drivers/nbmj9195.o $(OBJ)/vidhrdw/nbmj9195.o \
	$(OBJ)/drivers/niyanpai.o $(OBJ)/machine/m68kfmly.o $(OBJ)/vidhrdw/niyanpai.o \
	$(OBJ)/drivers/pastelg.o $(OBJ)/vidhrdw/pastelg.o \
	$(OBJ)/drivers/seicross.o $(OBJ)/vidhrdw/seicross.o \
	$(OBJ)/drivers/terracre.o $(OBJ)/vidhrdw/terracre.o \
	$(OBJ)/drivers/tubep.o $(OBJ)/vidhrdw/tubep.o \
	$(OBJ)/drivers/wiping.o $(OBJ)/sndhrdw/wiping.o $(OBJ)/vidhrdw/wiping.o \
	$(OBJ)/drivers/yamato.o \
	$(OBJ)/machine/nb1413m3.o \

$(OBJ)/nintendo.a: \
	$(OBJ)/drivers/dkong.o $(OBJ)/sndhrdw/dkong.o $(OBJ)/vidhrdw/dkong.o \
	$(OBJ)/drivers/mario.o $(OBJ)/sndhrdw/mario.o $(OBJ)/vidhrdw/mario.o \
	$(OBJ)/drivers/n8080.o $(OBJ)/sndhrdw/n8080.o $(OBJ)/vidhrdw/n8080.o \
	$(OBJ)/drivers/nss.o $(OBJ)/machine/snes.o $(OBJ)/sndhrdw/snes.o $(OBJ)/vidhrdw/snes.o \
	$(OBJ)/drivers/playch10.o $(OBJ)/machine/playch10.o $(OBJ)/vidhrdw/playch10.o \
	$(OBJ)/drivers/popeye.o $(OBJ)/vidhrdw/popeye.o \
	$(OBJ)/drivers/punchout.o $(OBJ)/vidhrdw/punchout.o \
	$(OBJ)/drivers/spacefb.o $(OBJ)/vidhrdw/spacefb.o \
	$(OBJ)/drivers/vsnes.o $(OBJ)/machine/vsnes.o $(OBJ)/vidhrdw/vsnes.o \
	$(OBJ)/machine/drakton.o \
	$(OBJ)/machine/rp5h01.o \
	$(OBJ)/machine/strtheat.o \
	$(OBJ)/vidhrdw/ppu2c03b.o \

$(OBJ)/nix.a: \
	$(OBJ)/drivers/fitfight.o $(OBJ)/vidhrdw/fitfight.o \
	$(OBJ)/drivers/pirates.o $(OBJ)/vidhrdw/pirates.o \

$(OBJ)/nmk.a: \
	$(OBJ)/drivers/acommand.o \
	$(OBJ)/drivers/jalmah.o \
	$(OBJ)/drivers/macrossp.o $(OBJ)/vidhrdw/macrossp.o \
	$(OBJ)/drivers/nmk16.o $(OBJ)/machine/nmk004.o $(OBJ)/vidhrdw/nmk16.o \
	$(OBJ)/drivers/quizdna.o $(OBJ)/vidhrdw/quizdna.o \
	$(OBJ)/drivers/quizpani.o $(OBJ)/vidhrdw/quizpani.o \

$(OBJ)/olympia.a: \
	$(OBJ)/drivers/dday.o $(OBJ)/vidhrdw/dday.o \
	$(OBJ)/drivers/portrait.o $(OBJ)/vidhrdw/portrait.o \

$(OBJ)/omori.a: \
	$(OBJ)/drivers/battlex.o $(OBJ)/vidhrdw/battlex.o \
	$(OBJ)/drivers/carjmbre.o $(OBJ)/vidhrdw/carjmbre.o \
	$(OBJ)/drivers/popper.o $(OBJ)/vidhrdw/popper.o \
	$(OBJ)/drivers/spaceg.o \

$(OBJ)/orca.a: \
	$(OBJ)/drivers/espial.o $(OBJ)/vidhrdw/espial.o \
	$(OBJ)/drivers/funkybee.o $(OBJ)/vidhrdw/funkybee.o \
	$(OBJ)/drivers/marineb.o $(OBJ)/vidhrdw/marineb.o \
	$(OBJ)/drivers/vastar.o $(OBJ)/vidhrdw/vastar.o \
	$(OBJ)/drivers/zodiack.o $(OBJ)/vidhrdw/zodiack.o \

$(OBJ)/pacific.a: \
	$(OBJ)/drivers/mrflea.o $(OBJ)/vidhrdw/mrflea.o \
	$(OBJ)/drivers/thief.o $(OBJ)/vidhrdw/thief.o \

$(OBJ)/pacman.a: \
	$(OBJ)/drivers/jrpacman.o \
	$(OBJ)/drivers/pacman.o $(OBJ)/vidhrdw/pacman.o \
	$(OBJ)/drivers/pengo.o \
	$(OBJ)/machine/acitya.o \
	$(OBJ)/machine/jumpshot.o \
	$(OBJ)/machine/mspacman.o \
	$(OBJ)/machine/pacplus.o \
	$(OBJ)/machine/theglobp.o \

$(OBJ)/phoenix.a: \
	$(OBJ)/drivers/naughtyb.o $(OBJ)/vidhrdw/naughtyb.o \
	$(OBJ)/drivers/phoenix.o $(OBJ)/sndhrdw/phoenix.o $(OBJ)/vidhrdw/phoenix.o \
	$(OBJ)/drivers/safarir.o \
	$(OBJ)/sndhrdw/pleiads.o \

$(OBJ)/playmark.a: \
	$(OBJ)/drivers/drtomy.o \
	$(OBJ)/drivers/playmark.o $(OBJ)/vidhrdw/playmark.o \
	$(OBJ)/drivers/powerbal.o \
	$(OBJ)/drivers/sderby.o $(OBJ)/vidhrdw/sderby.o \
	$(OBJ)/drivers/sslam.o $(OBJ)/vidhrdw/sslam.o \

$(OBJ)/psikyo.a: \
	$(OBJ)/drivers/psikyo.o $(OBJ)/vidhrdw/psikyo.o \
	$(OBJ)/drivers/psikyo4.o $(OBJ)/vidhrdw/psikyo4.o \
	$(OBJ)/drivers/psikyosh.o $(OBJ)/vidhrdw/psikyosh.o \

$(OBJ)/ramtek.a: \
	$(OBJ)/drivers/hitme.o $(OBJ)/sndhrdw/hitme.o \
	$(OBJ)/drivers/starcrus.o $(OBJ)/vidhrdw/starcrus.o \

$(OBJ)/rare.a: \
	$(OBJ)/drivers/btoads.o $(OBJ)/vidhrdw/btoads.o \
	$(OBJ)/drivers/kinst.o \
	$(OBJ)/drivers/xtheball.o \

$(OBJ)/sanritsu.a: \
	$(OBJ)/drivers/appoooh.o $(OBJ)/vidhrdw/appoooh.o \
	$(OBJ)/drivers/bankp.o $(OBJ)/vidhrdw/bankp.o \
	$(OBJ)/drivers/drmicro.o $(OBJ)/vidhrdw/drmicro.o \
	$(OBJ)/drivers/mayumi.o $(OBJ)/vidhrdw/mayumi.o \
	$(OBJ)/drivers/mermaid.o $(OBJ)/vidhrdw/mermaid.o \
	$(OBJ)/drivers/mjkjidai.o $(OBJ)/vidhrdw/mjkjidai.o \

$(OBJ)/sega.a: \
	$(OBJ)/drivers/angelkds.o $(OBJ)/vidhrdw/angelkds.o \
	$(OBJ)/drivers/blockade.o $(OBJ)/sndhrdw/blockade.o $(OBJ)/vidhrdw/blockade.o \
	$(OBJ)/drivers/calorie.o \
	$(OBJ)/drivers/coolridr.o \
	$(OBJ)/drivers/deniam.o $(OBJ)/vidhrdw/deniam.o \
	$(OBJ)/drivers/dotrikun.o $(OBJ)/vidhrdw/dotrikun.o \
	$(OBJ)/drivers/genesis.o $(OBJ)/vidhrdw/genesis.o \
	$(OBJ)/drivers/kopunch.o $(OBJ)/vidhrdw/kopunch.o \
	$(OBJ)/drivers/megaplay.o \
	$(OBJ)/drivers/megatech.o \
	$(OBJ)/drivers/model1.o $(OBJ)/machine/model1.o $(OBJ)/vidhrdw/model1.o \
	$(OBJ)/drivers/model2.o \
	$(OBJ)/drivers/model3.o $(OBJ)/vidhrdw/model3.o $(OBJ)/machine/model3.o \
	$(OBJ)/drivers/puckpkmn.o \
	$(OBJ)/drivers/sega.o $(OBJ)/sndhrdw/sega.o $(OBJ)/vidhrdw/sega.o \
	$(OBJ)/drivers/segac2.o \
	$(OBJ)/drivers/segahang.o $(OBJ)/vidhrdw/segahang.o \
	$(OBJ)/drivers/segaorun.o $(OBJ)/vidhrdw/segaorun.o \
	$(OBJ)/drivers/segar.o $(OBJ)/machine/segar.o $(OBJ)/sndhrdw/segar.o $(OBJ)/vidhrdw/segar.o \
	$(OBJ)/drivers/segas16a.o $(OBJ)/vidhrdw/segas16a.o \
	$(OBJ)/drivers/segas16b.o \
	$(OBJ)/drivers/segas16b.o $(OBJ)/vidhrdw/segas16b.o \
	$(OBJ)/drivers/segas18.o $(OBJ)/vidhrdw/segas18.o \
	$(OBJ)/drivers/segas32.o $(OBJ)/machine/segas32.o $(OBJ)/vidhrdw/segas32.o \
	$(OBJ)/drivers/segasyse.o $(OBJ)/vidhrdw/segasyse.o \
	$(OBJ)/drivers/segaxbd.o $(OBJ)/vidhrdw/segaxbd.o \
	$(OBJ)/drivers/segaybd.o $(OBJ)/vidhrdw/segaybd.o \
	$(OBJ)/drivers/sg1000a.o $(OBJ)/vidhrdw/tms9928a.o \
	$(OBJ)/drivers/stactics.o $(OBJ)/machine/stactics.o $(OBJ)/vidhrdw/stactics.o \
	$(OBJ)/drivers/stv.o $(OBJ)/drivers/stvhacks.o $(OBJ)/machine/stvcd.o \
	$(OBJ)/drivers/suprloco.o $(OBJ)/vidhrdw/suprloco.o \
	$(OBJ)/drivers/system1.o $(OBJ)/vidhrdw/system1.o \
	$(OBJ)/drivers/system16.o $(OBJ)/machine/system16.o $(OBJ)/sndhrdw/system16.o $(OBJ)/vidhrdw/system16.o $(OBJ)/vidhrdw/sys16spr.o \
	$(OBJ)/drivers/system18.o \
	$(OBJ)/drivers/system24.o $(OBJ)/machine/system24.o $(OBJ)/vidhrdw/system24.o \
	$(OBJ)/drivers/topshoot.o \
	$(OBJ)/drivers/turbo.o $(OBJ)/machine/turbo.o $(OBJ)/sndhrdw/turbo.o $(OBJ)/vidhrdw/turbo.o \
	$(OBJ)/drivers/vicdual.o $(OBJ)/sndhrdw/vicdual.o $(OBJ)/vidhrdw/vicdual.o \
	$(OBJ)/drivers/zaxxon.o $(OBJ)/sndhrdw/zaxxon.o $(OBJ)/vidhrdw/zaxxon.o \
	$(OBJ)/machine/fd1089.o \
	$(OBJ)/machine/fd1094.o \
	$(OBJ)/machine/mc8123.o \
	$(OBJ)/machine/s16fd.o \
	$(OBJ)/machine/s24fd.o \
	$(OBJ)/machine/scudsp.o \
	$(OBJ)/machine/segaic16.o \
	$(OBJ)/sndhrdw/carnival.o \
	$(OBJ)/sndhrdw/depthch.o \
	$(OBJ)/sndhrdw/invinco.o \
	$(OBJ)/sndhrdw/pulsar.o \
	$(OBJ)/sndhrdw/segasnd.o \
	$(OBJ)/vidhrdw/segaic16.o \
	$(OBJ)/vidhrdw/segaic24.o \
	$(OBJ)/vidhrdw/stvvdp1.o $(OBJ)/vidhrdw/stvvdp2.o \

$(OBJ)/seibu.a: \
	$(OBJ)/drivers/cshooter.o \
	$(OBJ)/drivers/dcon.o $(OBJ)/vidhrdw/dcon.o \
	$(OBJ)/drivers/deadang.o $(OBJ)/vidhrdw/deadang.o \
	$(OBJ)/drivers/dynduke.o $(OBJ)/vidhrdw/dynduke.o \
	$(OBJ)/drivers/kncljoe.o $(OBJ)/vidhrdw/kncljoe.o \
	$(OBJ)/drivers/mustache.o $(OBJ)/vidhrdw/mustache.o \
	$(OBJ)/drivers/panicr.o \
	$(OBJ)/drivers/raiden.o $(OBJ)/vidhrdw/raiden.o \
	$(OBJ)/drivers/raiden2.o \
	$(OBJ)/drivers/seibuspi.o $(OBJ)/machine/seibuspi.o $(OBJ)/vidhrdw/seibuspi.o \
	$(OBJ)/drivers/sengokmj.o $(OBJ)/vidhrdw/sengokmj.o \
	$(OBJ)/drivers/stfight.o $(OBJ)/machine/stfight.o $(OBJ)/vidhrdw/stfight.o \
	$(OBJ)/drivers/wiz.o $(OBJ)/vidhrdw/wiz.o \
	$(OBJ)/machine/spisprit.o \
	$(OBJ)/sndhrdw/seibu.o \

$(OBJ)/seta.a: \
	$(OBJ)/drivers/aleck64.o $(OBJ)/machine/n64.o $(OBJ)/vidhrdw/n64.o \
	$(OBJ)/drivers/darkhors.o \
	$(OBJ)/drivers/hanaawas.o $(OBJ)/vidhrdw/hanaawas.o \
	$(OBJ)/drivers/macs.o \
	$(OBJ)/drivers/seta.o $(OBJ)/vidhrdw/seta.o \
	$(OBJ)/drivers/seta2.o $(OBJ)/vidhrdw/seta2.o \
	$(OBJ)/drivers/speedatk.o $(OBJ)/vidhrdw/speedatk.o \
	$(OBJ)/drivers/srmp2.o $(OBJ)/vidhrdw/srmp2.o \
	$(OBJ)/drivers/srmp6.o \
	$(OBJ)/drivers/ssv.o $(OBJ)/vidhrdw/ssv.o \
	$(OBJ)/drivers/st0016.o $(OBJ)/vidhrdw/st0016.o \

$(OBJ)/sigma.a: \
	$(OBJ)/drivers/nyny.o $(OBJ)/vidhrdw/nyny.o \
	$(OBJ)/drivers/r2dtank.o \
	$(OBJ)/drivers/spiders.o $(OBJ)/machine/spiders.o $(OBJ)/sndhrdw/spiders.o $(OBJ)/vidhrdw/spiders.o \

$(OBJ)/snk.a: \
	$(OBJ)/drivers/bbusters.o $(OBJ)/vidhrdw/bbusters.o \
	$(OBJ)/drivers/dmndrby.o \
	$(OBJ)/drivers/hal21.o \
	$(OBJ)/drivers/hng64.o $(OBJ)/vidhrdw/hng64.o \
	$(OBJ)/drivers/jcross.o $(OBJ)/vidhrdw/jcross.o \
	$(OBJ)/drivers/lasso.o $(OBJ)/vidhrdw/lasso.o \
	$(OBJ)/drivers/mainsnk.o $(OBJ)/vidhrdw/mainsnk.o \
	$(OBJ)/drivers/marvins.o $(OBJ)/vidhrdw/marvins.o \
	$(OBJ)/drivers/munchmo.o $(OBJ)/vidhrdw/munchmo.o \
	$(OBJ)/drivers/prehisle.o $(OBJ)/vidhrdw/prehisle.o \
	$(OBJ)/drivers/rockola.o $(OBJ)/sndhrdw/rockola.o $(OBJ)/vidhrdw/rockola.o \
	$(OBJ)/drivers/sgladiat.o \
	$(OBJ)/drivers/snk.o $(OBJ)/vidhrdw/snk.o \
	$(OBJ)/drivers/snk68.o $(OBJ)/vidhrdw/snk68.o \

$(OBJ)/stern.a: \
	$(OBJ)/drivers/astinvad.o $(OBJ)/sndhrdw/astinvad.o $(OBJ)/vidhrdw/astinvad.o \
	$(OBJ)/drivers/berzerk.o $(OBJ)/machine/berzerk.o $(OBJ)/sndhrdw/berzerk.o $(OBJ)/vidhrdw/berzerk.o \
	$(OBJ)/drivers/mazerbla.o \
	$(OBJ)/drivers/supdrapo.o \

$(OBJ)/sun.a: \
	$(OBJ)/drivers/arabian.o $(OBJ)/vidhrdw/arabian.o \
	$(OBJ)/drivers/ikki.o $(OBJ)/vidhrdw/ikki.o \
	$(OBJ)/drivers/kangaroo.o $(OBJ)/vidhrdw/kangaroo.o \
	$(OBJ)/drivers/markham.o $(OBJ)/vidhrdw/markham.o \
	$(OBJ)/drivers/route16.o $(OBJ)/vidhrdw/route16.o \
	$(OBJ)/drivers/shanghai.o \
	$(OBJ)/drivers/shangha3.o $(OBJ)/vidhrdw/shangha3.o \
	$(OBJ)/drivers/strnskil.o $(OBJ)/vidhrdw/strnskil.o \
	$(OBJ)/drivers/ttmahjng.o $(OBJ)/vidhrdw/ttmahjng.o \

$(OBJ)/suna.a: \
	$(OBJ)/drivers/goindol.o $(OBJ)/vidhrdw/goindol.o \
	$(OBJ)/drivers/suna8.o $(OBJ)/sndhrdw/suna8.o $(OBJ)/vidhrdw/suna8.o \
	$(OBJ)/drivers/suna16.o $(OBJ)/vidhrdw/suna16.o \

$(OBJ)/tad.a: \
	$(OBJ)/drivers/bloodbro.o $(OBJ)/vidhrdw/bloodbro.o \
	$(OBJ)/drivers/cabal.o $(OBJ)/vidhrdw/cabal.o \
	$(OBJ)/drivers/goal92.o $(OBJ)/vidhrdw/goal92.o \
	$(OBJ)/drivers/legionna.o $(OBJ)/vidhrdw/legionna.o \
	$(OBJ)/drivers/toki.o $(OBJ)/vidhrdw/toki.o \

$(OBJ)/taito.a: \
	$(OBJ)/drivers/40love.o $(OBJ)/vidhrdw/40love.o \
	$(OBJ)/drivers/arkanoid.o $(OBJ)/machine/arkanoid.o $(OBJ)/vidhrdw/arkanoid.o \
	$(OBJ)/drivers/ashnojoe.o $(OBJ)/vidhrdw/ashnojoe.o \
	$(OBJ)/drivers/asuka.o $(OBJ)/machine/bonzeadv.o $(OBJ)/vidhrdw/asuka.o \
	$(OBJ)/drivers/bigevglf.o $(OBJ)/machine/bigevglf.o $(OBJ)/vidhrdw/bigevglf.o \
	$(OBJ)/drivers/bking2.o $(OBJ)/vidhrdw/bking2.o \
	$(OBJ)/drivers/bublbobl.o $(OBJ)/machine/bublbobl.o $(OBJ)/vidhrdw/bublbobl.o \
	$(OBJ)/drivers/buggychl.o $(OBJ)/machine/buggychl.o $(OBJ)/vidhrdw/buggychl.o \
	$(OBJ)/drivers/chaknpop.o $(OBJ)/machine/chaknpop.o $(OBJ)/vidhrdw/chaknpop.o \
	$(OBJ)/drivers/champbwl.o \
	$(OBJ)/drivers/changela.o \
	$(OBJ)/drivers/crbaloon.o $(OBJ)/vidhrdw/crbaloon.o $(OBJ)/sndhrdw/crbaloon.o \
	$(OBJ)/drivers/darius.o $(OBJ)/vidhrdw/darius.o \
	$(OBJ)/drivers/darkmist.o $(OBJ)/vidhrdw/darkmist.o \
	$(OBJ)/drivers/exzisus.o $(OBJ)/vidhrdw/exzisus.o \
	$(OBJ)/drivers/fgoal.o $(OBJ)/vidhrdw/fgoal.o \
	$(OBJ)/drivers/flstory.o $(OBJ)/machine/flstory.o $(OBJ)/vidhrdw/flstory.o \
	$(OBJ)/drivers/gladiatr.o $(OBJ)/vidhrdw/gladiatr.o \
	$(OBJ)/drivers/grchamp.o $(OBJ)/machine/grchamp.o $(OBJ)/sndhrdw/grchamp.o $(OBJ)/vidhrdw/grchamp.o \
	$(OBJ)/drivers/groundfx.o $(OBJ)/vidhrdw/groundfx.o \
	$(OBJ)/drivers/gsword.o $(OBJ)/machine/tait8741.o $(OBJ)/vidhrdw/gsword.o \
	$(OBJ)/drivers/gunbustr.o $(OBJ)/vidhrdw/gunbustr.o \
	$(OBJ)/drivers/halleys.o \
	$(OBJ)/drivers/jollyjgr.o \
	$(OBJ)/drivers/ksayakyu.o $(OBJ)/vidhrdw/ksayakyu.o \
	$(OBJ)/drivers/lkage.o $(OBJ)/machine/lkage.o $(OBJ)/vidhrdw/lkage.o \
	$(OBJ)/drivers/lsasquad.o $(OBJ)/machine/lsasquad.o $(OBJ)/vidhrdw/lsasquad.o \
	$(OBJ)/drivers/marinedt.o \
	$(OBJ)/drivers/mexico86.o $(OBJ)/machine/mexico86.o $(OBJ)/vidhrdw/mexico86.o \
	$(OBJ)/drivers/minivadr.o $(OBJ)/vidhrdw/minivadr.o \
	$(OBJ)/drivers/missb2.o \
	$(OBJ)/drivers/msisaac.o $(OBJ)/vidhrdw/msisaac.o \
	$(OBJ)/drivers/ninjaw.o $(OBJ)/vidhrdw/ninjaw.o \
	$(OBJ)/drivers/nycaptor.o $(OBJ)/machine/nycaptor.o $(OBJ)/vidhrdw/nycaptor.o \
	$(OBJ)/drivers/opwolf.o $(OBJ)/machine/opwolf.o \
	$(OBJ)/drivers/othunder.o $(OBJ)/vidhrdw/othunder.o \
	$(OBJ)/drivers/pitnrun.o $(OBJ)/machine/pitnrun.o $(OBJ)/vidhrdw/pitnrun.o \
	$(OBJ)/drivers/qix.o $(OBJ)/machine/qix.o $(OBJ)/sndhrdw/qix.o $(OBJ)/vidhrdw/qix.o \
	$(OBJ)/drivers/rainbow.o $(OBJ)/machine/rainbow.o \
	$(OBJ)/drivers/rastan.o $(OBJ)/vidhrdw/rastan.o \
	$(OBJ)/drivers/retofinv.o $(OBJ)/machine/retofinv.o $(OBJ)/vidhrdw/retofinv.o \
	$(OBJ)/drivers/rollrace.o $(OBJ)/vidhrdw/rollrace.o \
	$(OBJ)/drivers/sbowling.o \
	$(OBJ)/drivers/slapshot.o $(OBJ)/vidhrdw/slapshot.o \
	$(OBJ)/drivers/ssrj.o $(OBJ)/vidhrdw/ssrj.o \
	$(OBJ)/drivers/superchs.o $(OBJ)/vidhrdw/superchs.o \
	$(OBJ)/drivers/superqix.o $(OBJ)/vidhrdw/superqix.o \
	$(OBJ)/drivers/taito_b.o $(OBJ)/vidhrdw/taito_b.o \
	$(OBJ)/drivers/taito_f2.o $(OBJ)/vidhrdw/taito_f2.o \
	$(OBJ)/drivers/taito_f3.o $(OBJ)/vidhrdw/taito_f3.o $(OBJ)/sndhrdw/taito_f3.o \
	$(OBJ)/drivers/taito_h.o $(OBJ)/vidhrdw/taito_h.o \
	$(OBJ)/drivers/taito_l.o $(OBJ)/vidhrdw/taito_l.o \
	$(OBJ)/drivers/taito_x.o $(OBJ)/machine/cchip.o \
	$(OBJ)/drivers/taito_z.o $(OBJ)/vidhrdw/taito_z.o \
	$(OBJ)/drivers/taitoair.o $(OBJ)/vidhrdw/taitoair.o \
	$(OBJ)/drivers/taitojc.o \
	$(OBJ)/drivers/taitosj.o $(OBJ)/machine/taitosj.o $(OBJ)/vidhrdw/taitosj.o \
	$(OBJ)/drivers/taitowlf.o \
	$(OBJ)/drivers/tnzs.o $(OBJ)/machine/tnzs.o $(OBJ)/vidhrdw/tnzs.o \
	$(OBJ)/drivers/topspeed.o $(OBJ)/vidhrdw/topspeed.o \
	$(OBJ)/drivers/tsamurai.o $(OBJ)/vidhrdw/tsamurai.o \
	$(OBJ)/drivers/undrfire.o $(OBJ)/vidhrdw/undrfire.o \
	$(OBJ)/drivers/volfied.o $(OBJ)/machine/volfied.o $(OBJ)/vidhrdw/volfied.o \
	$(OBJ)/drivers/warriorb.o $(OBJ)/vidhrdw/warriorb.o \
	$(OBJ)/drivers/wgp.o $(OBJ)/vidhrdw/wgp.o \
	$(OBJ)/machine/mb87078.o \
	$(OBJ)/sndhrdw/taitosnd.o \
	$(OBJ)/vidhrdw/taitoic.o \

$(OBJ)/tatsumi.a: \
	$(OBJ)/drivers/lockon.o \
	$(OBJ)/drivers/tatsumi.o $(OBJ)/machine/tatsumi.o $(OBJ)/vidhrdw/tatsumi.o \
	$(OBJ)/drivers/tx1.o $(OBJ)/machine/tx1.o $(OBJ)/vidhrdw/tx1.o \

$(OBJ)/tch.a: \
	$(OBJ)/drivers/kickgoal.o $(OBJ)/vidhrdw/kickgoal.o \
	$(OBJ)/drivers/littlerb.o \
	$(OBJ)/drivers/speedspn.o $(OBJ)/vidhrdw/speedspn.o \
	$(OBJ)/drivers/wheelfir.o \

$(OBJ)/tecfri.a: \
	$(OBJ)/drivers/holeland.o $(OBJ)/vidhrdw/holeland.o \
	$(OBJ)/drivers/sauro.o $(OBJ)/vidhrdw/sauro.o \
	$(OBJ)/drivers/speedbal.o $(OBJ)/vidhrdw/speedbal.o \

$(OBJ)/technos.a: \
	$(OBJ)/drivers/battlane.o $(OBJ)/vidhrdw/battlane.o \
	$(OBJ)/drivers/blockout.o $(OBJ)/vidhrdw/blockout.o \
	$(OBJ)/drivers/bogeyman.o $(OBJ)/vidhrdw/bogeyman.o \
	$(OBJ)/drivers/chinagat.o \
	$(OBJ)/drivers/ddragon.o $(OBJ)/vidhrdw/ddragon.o \
	$(OBJ)/drivers/ddragon3.o $(OBJ)/vidhrdw/ddragon3.o \
	$(OBJ)/drivers/dogfgt.o $(OBJ)/vidhrdw/dogfgt.o \
	$(OBJ)/drivers/matmania.o $(OBJ)/machine/maniach.o $(OBJ)/vidhrdw/matmania.o \
	$(OBJ)/drivers/mystston.o $(OBJ)/vidhrdw/mystston.o \
	$(OBJ)/drivers/renegade.o $(OBJ)/vidhrdw/renegade.o \
	$(OBJ)/drivers/scregg.o \
	$(OBJ)/drivers/shadfrce.o $(OBJ)/vidhrdw/shadfrce.o \
	$(OBJ)/drivers/spdodgeb.o $(OBJ)/vidhrdw/spdodgeb.o \
	$(OBJ)/drivers/ssozumo.o $(OBJ)/vidhrdw/ssozumo.o \
	$(OBJ)/drivers/tagteam.o $(OBJ)/vidhrdw/tagteam.o \
	$(OBJ)/drivers/vball.o $(OBJ)/vidhrdw/vball.o \
	$(OBJ)/drivers/wwfsstar.o $(OBJ)/vidhrdw/wwfsstar.o \
	$(OBJ)/drivers/wwfwfest.o $(OBJ)/vidhrdw/wwfwfest.o \
	$(OBJ)/drivers/xain.o $(OBJ)/vidhrdw/xain.o \

$(OBJ)/tehkan.a: \
	$(OBJ)/drivers/bombjack.o $(OBJ)/vidhrdw/bombjack.o \
	$(OBJ)/drivers/gaiden.o $(OBJ)/vidhrdw/gaiden.o \
	$(OBJ)/drivers/lvcards.o $(OBJ)/vidhrdw/lvcards.o \
	$(OBJ)/drivers/pbaction.o $(OBJ)/vidhrdw/pbaction.o \
	$(OBJ)/drivers/senjyo.o $(OBJ)/sndhrdw/senjyo.o $(OBJ)/vidhrdw/senjyo.o \
	$(OBJ)/drivers/solomon.o $(OBJ)/vidhrdw/solomon.o \
	$(OBJ)/drivers/spbactn.o $(OBJ)/vidhrdw/spbactn.o \
	$(OBJ)/drivers/tbowl.o $(OBJ)/vidhrdw/tbowl.o \
	$(OBJ)/drivers/tecmo.o $(OBJ)/vidhrdw/tecmo.o \
	$(OBJ)/drivers/tecmo16.o $(OBJ)/vidhrdw/tecmo16.o \
	$(OBJ)/drivers/tecmosys.o \
	$(OBJ)/drivers/tehkanwc.o $(OBJ)/vidhrdw/tehkanwc.o \
	$(OBJ)/drivers/wc90.o $(OBJ)/vidhrdw/wc90.o \
	$(OBJ)/drivers/wc90b.o $(OBJ)/vidhrdw/wc90b.o \

$(OBJ)/thepit.a: \
	$(OBJ)/drivers/thepit.o $(OBJ)/vidhrdw/thepit.o \
	$(OBJ)/drivers/timelimt.o $(OBJ)/vidhrdw/timelimt.o \

$(OBJ)/toaplan.a: \
	$(OBJ)/drivers/mjsister.o $(OBJ)/vidhrdw/mjsister.o \
	$(OBJ)/drivers/slapfght.o $(OBJ)/machine/slapfght.o $(OBJ)/vidhrdw/slapfght.o \
	$(OBJ)/drivers/snowbros.o $(OBJ)/vidhrdw/snowbros.o \
	$(OBJ)/drivers/toaplan1.o $(OBJ)/machine/toaplan1.o $(OBJ)/vidhrdw/toaplan1.o \
	$(OBJ)/drivers/toaplan2.o $(OBJ)/sndhrdw/toaplan2.o $(OBJ)/vidhrdw/toaplan2.o \
	$(OBJ)/drivers/twincobr.o $(OBJ)/machine/twincobr.o $(OBJ)/vidhrdw/twincobr.o \
	$(OBJ)/drivers/wardner.o \

$(OBJ)/tong.a: \
	$(OBJ)/drivers/leprechn.o $(OBJ)/machine/leprechn.o $(OBJ)/vidhrdw/leprechn.o \
	$(OBJ)/drivers/beezer.o $(OBJ)/machine/beezer.o $(OBJ)/vidhrdw/beezer.o \

$(OBJ)/unico.a: \
	$(OBJ)/drivers/drgnmst.o $(OBJ)/vidhrdw/drgnmst.o \
	$(OBJ)/drivers/silkroad.o $(OBJ)/vidhrdw/silkroad.o \
	$(OBJ)/drivers/unico.o $(OBJ)/vidhrdw/unico.o \

$(OBJ)/univers.a: \
	$(OBJ)/drivers/cheekyms.o $(OBJ)/vidhrdw/cheekyms.o \
	$(OBJ)/drivers/cosmic.o $(OBJ)/vidhrdw/cosmic.o \
	$(OBJ)/drivers/docastle.o $(OBJ)/machine/docastle.o $(OBJ)/vidhrdw/docastle.o \
	$(OBJ)/drivers/ladybug.o $(OBJ)/vidhrdw/ladybug.o \
	$(OBJ)/drivers/mrdo.o $(OBJ)/vidhrdw/mrdo.o \
	$(OBJ)/drivers/redclash.o $(OBJ)/vidhrdw/redclash.o \

$(OBJ)/upl.a: \
	$(OBJ)/drivers/mnight.o $(OBJ)/vidhrdw/mnight.o \
	$(OBJ)/drivers/mouser.o $(OBJ)/vidhrdw/mouser.o \
	$(OBJ)/drivers/ninjakid.o $(OBJ)/vidhrdw/ninjakid.o \
	$(OBJ)/drivers/ninjakd2.o $(OBJ)/vidhrdw/ninjakd2.o \
	$(OBJ)/drivers/nova2001.o $(OBJ)/vidhrdw/nova2001.o \
	$(OBJ)/drivers/omegaf.o $(OBJ)/vidhrdw/omegaf.o \
	$(OBJ)/drivers/pkunwar.o $(OBJ)/vidhrdw/pkunwar.o \
	$(OBJ)/drivers/raiders5.o $(OBJ)/vidhrdw/raiders5.o \
	$(OBJ)/drivers/xxmissio.o $(OBJ)/vidhrdw/xxmissio.o \

$(OBJ)/valadon.a: \
	$(OBJ)/drivers/bagman.o $(OBJ)/machine/bagman.o $(OBJ)/vidhrdw/bagman.o \
	$(OBJ)/drivers/tankbust.o $(OBJ)/vidhrdw/tankbust.o \

$(OBJ)/veltmjr.a: \
	$(OBJ)/drivers/cardline.o \
	$(OBJ)/drivers/witch.o \

$(OBJ)/venture.a: \
	$(OBJ)/drivers/looping.o \
	$(OBJ)/drivers/spcforce.o $(OBJ)/vidhrdw/spcforce.o \
	$(OBJ)/drivers/suprridr.o $(OBJ)/vidhrdw/suprridr.o \

$(OBJ)/vsystem.a: \
	$(OBJ)/drivers/aerofgt.o $(OBJ)/vidhrdw/aerofgt.o \
	$(OBJ)/drivers/crshrace.o $(OBJ)/vidhrdw/crshrace.o \
	$(OBJ)/drivers/f1gp.o $(OBJ)/vidhrdw/f1gp.o \
	$(OBJ)/drivers/fromance.o $(OBJ)/vidhrdw/fromance.o \
	$(OBJ)/drivers/fromanc2.o $(OBJ)/vidhrdw/fromanc2.o \
	$(OBJ)/drivers/gstriker.o $(OBJ)/vidhrdw/gstriker.o \
	$(OBJ)/drivers/inufuku.o $(OBJ)/vidhrdw/inufuku.o \
	$(OBJ)/drivers/ojankohs.o $(OBJ)/vidhrdw/ojankohs.o \
	$(OBJ)/drivers/pipedrm.o \
	$(OBJ)/drivers/rpunch.o $(OBJ)/vidhrdw/rpunch.o \
	$(OBJ)/drivers/suprslam.o $(OBJ)/vidhrdw/suprslam.o \
	$(OBJ)/drivers/tail2nos.o $(OBJ)/vidhrdw/tail2nos.o \
	$(OBJ)/drivers/taotaido.o $(OBJ)/vidhrdw/taotaido.o \
	$(OBJ)/drivers/welltris.o $(OBJ)/vidhrdw/welltris.o \

$(OBJ)/yunsung.a: \
	$(OBJ)/drivers/nmg5.o \
	$(OBJ)/drivers/paradise.o $(OBJ)/vidhrdw/paradise.o \
	$(OBJ)/drivers/yunsung8.o $(OBJ)/vidhrdw/yunsung8.o \
	$(OBJ)/drivers/yunsun16.o $(OBJ)/vidhrdw/yunsun16.o \

$(OBJ)/zaccaria.a: \
	$(OBJ)/drivers/galaxia.o \
	$(OBJ)/drivers/laserbat.o \
	$(OBJ)/drivers/zac2650.o $(OBJ)/vidhrdw/zac2650.o \
	$(OBJ)/drivers/zaccaria.o $(OBJ)/vidhrdw/zaccaria.o \



#-------------------------------------------------
# remaining drivers
#-------------------------------------------------

$(OBJ)/misc.a: \
	$(OBJ)/drivers/1945kiii.o \
	$(OBJ)/drivers/4enraya.o $(OBJ)/vidhrdw/4enraya.o \
	$(OBJ)/drivers/afega.o $(OBJ)/vidhrdw/afega.o \
	$(OBJ)/drivers/ambush.o $(OBJ)/vidhrdw/ambush.o \
	$(OBJ)/drivers/ampoker.o \
	$(OBJ)/drivers/amspdwy.o $(OBJ)/vidhrdw/amspdwy.o \
	$(OBJ)/drivers/artmagic.o $(OBJ)/vidhrdw/artmagic.o \
	$(OBJ)/drivers/attckufo.o $(OBJ)/sndhrdw/attckufo.o $(OBJ)/vidhrdw/attckufo.o \
	$(OBJ)/drivers/aztarac.o $(OBJ)/sndhrdw/aztarac.o $(OBJ)/vidhrdw/aztarac.o \
	$(OBJ)/drivers/beaminv.o $(OBJ)/vidhrdw/beaminv.o \
	$(OBJ)/drivers/bmcbowl.o \
	$(OBJ)/drivers/cave.o $(OBJ)/vidhrdw/cave.o \
	$(OBJ)/drivers/cherrym.o \
	$(OBJ)/drivers/coinmstr.o \
	$(OBJ)/drivers/coolpool.o \
	$(OBJ)/drivers/crystal.o $(OBJ)/machine/ds1302.o $(OBJ)/vidhrdw/vrender0.o \
	$(OBJ)/drivers/cybertnk.o \
	$(OBJ)/drivers/dambustr.o \
	$(OBJ)/drivers/dcheese.o $(OBJ)/vidhrdw/dcheese.o \
	$(OBJ)/drivers/dgpix.o \
	$(OBJ)/drivers/dorachan.o $(OBJ)/vidhrdw/dorachan.o \
	$(OBJ)/drivers/dreamwld.o \
	$(OBJ)/drivers/dribling.o $(OBJ)/vidhrdw/dribling.o \
	$(OBJ)/drivers/dwarfd.o \
	$(OBJ)/drivers/dynadice.o \
	$(OBJ)/drivers/epos.o $(OBJ)/vidhrdw/epos.o \
	$(OBJ)/drivers/ertictac.o \
	$(OBJ)/drivers/esd16.o $(OBJ)/vidhrdw/esd16.o \
	$(OBJ)/drivers/ettrivia.o \
	$(OBJ)/drivers/flower.o $(OBJ)/sndhrdw/flower.o $(OBJ)/vidhrdw/flower.o \
	$(OBJ)/drivers/fortecar.o \
	$(OBJ)/drivers/freekick.o $(OBJ)/vidhrdw/freekick.o \
	$(OBJ)/drivers/funworld.o \
	$(OBJ)/drivers/go2000.o \
	$(OBJ)/drivers/gotcha.o $(OBJ)/vidhrdw/gotcha.o \
	$(OBJ)/drivers/gumbo.o $(OBJ)/vidhrdw/gumbo.o \
	$(OBJ)/drivers/gunpey.o \
	$(OBJ)/drivers/hexa.o $(OBJ)/vidhrdw/hexa.o \
	$(OBJ)/drivers/homedata.o $(OBJ)/vidhrdw/homedata.o \
	$(OBJ)/drivers/hotblock.o \
	$(OBJ)/drivers/intrscti.o \
	$(OBJ)/drivers/jackpool.o \
	$(OBJ)/drivers/kyugo.o $(OBJ)/vidhrdw/kyugo.o \
	$(OBJ)/drivers/ladyfrog.o $(OBJ)/vidhrdw/ladyfrog.o \
	$(OBJ)/drivers/laserbas.o \
	$(OBJ)/drivers/lastfght.o \
	$(OBJ)/drivers/lethalj.o $(OBJ)/vidhrdw/lethalj.o \
	$(OBJ)/drivers/ltcasino.o \
	$(OBJ)/drivers/lucky8.o \
	$(OBJ)/drivers/magic10.o \
	$(OBJ)/drivers/malzak.o $(OBJ)/vidhrdw/malzak.o \
	$(OBJ)/drivers/mcatadv.o $(OBJ)/vidhrdw/mcatadv.o \
	$(OBJ)/drivers/micro3d.o $(OBJ)/vidhrdw/micro3d.o \
	$(OBJ)/drivers/mirax.o \
	$(OBJ)/drivers/mole.o $(OBJ)/vidhrdw/mole.o \
	$(OBJ)/drivers/monzagp.o \
	$(OBJ)/drivers/mosaic.o $(OBJ)/vidhrdw/mosaic.o \
	$(OBJ)/drivers/mrjong.o $(OBJ)/vidhrdw/mrjong.o \
	$(OBJ)/drivers/murogem.o \
	$(OBJ)/drivers/news.o $(OBJ)/vidhrdw/news.o \
	$(OBJ)/drivers/oneshot.o $(OBJ)/vidhrdw/oneshot.o \
	$(OBJ)/drivers/onetwo.o \
	$(OBJ)/drivers/othldrby.o $(OBJ)/vidhrdw/othldrby.o \
	$(OBJ)/drivers/pass.o $(OBJ)/vidhrdw/pass.o \
	$(OBJ)/drivers/pipeline.o \
	$(OBJ)/drivers/pkscram.o \
	$(OBJ)/drivers/pntnpuzl.o \
	$(OBJ)/drivers/policetr.o $(OBJ)/vidhrdw/policetr.o \
	$(OBJ)/drivers/polyplay.o $(OBJ)/sndhrdw/polyplay.o $(OBJ)/vidhrdw/polyplay.o \
	$(OBJ)/drivers/rbmk.o \
	$(OBJ)/drivers/shangkid.o $(OBJ)/vidhrdw/shangkid.o \
	$(OBJ)/drivers/skyarmy.o \
	$(OBJ)/drivers/sliver.o \
	$(OBJ)/drivers/sprcros2.o $(OBJ)/vidhrdw/sprcros2.o \
	$(OBJ)/drivers/ssfindo.o \
	$(OBJ)/drivers/ssingles.o \
	$(OBJ)/drivers/starspnr.o \
	$(OBJ)/drivers/statriv2.o \
	$(OBJ)/drivers/supertnk.o \
	$(OBJ)/drivers/tattack.o \
	$(OBJ)/drivers/taxidrvr.o $(OBJ)/vidhrdw/taxidrvr.o \
	$(OBJ)/drivers/tcl.o \
	$(OBJ)/drivers/thedeep.o $(OBJ)/vidhrdw/thedeep.o \
	$(OBJ)/drivers/tickee.o $(OBJ)/vidhrdw/tickee.o \
	$(OBJ)/drivers/truco.o $(OBJ)/vidhrdw/truco.o \
	$(OBJ)/drivers/trucocl.o $(OBJ)/vidhrdw/trucocl.o \
	$(OBJ)/drivers/trvquest.o \
	$(OBJ)/drivers/ttchamp.o \
	$(OBJ)/drivers/tugboat.o \
	$(OBJ)/drivers/turbosub.o \
	$(OBJ)/drivers/usgames.o $(OBJ)/vidhrdw/usgames.o \
	$(OBJ)/drivers/vamphalf.o \
	$(OBJ)/drivers/vp906iii.o \
	$(OBJ)/drivers/vroulet.o \
	$(OBJ)/drivers/wldarrow.o \
	$(OBJ)/drivers/xyonix.o $(OBJ)/vidhrdw/xyonix.o \

 