MESS=1

# a tiny compile is without Neogeo games
MESSCOREDEFS += -DTINY_COMPILE=1
MESSCOREDEFS += -DTINY_NAME="driver_ti99_4a"
MESSCOREDEFS += -DTINY_POINTER="&driver_ti99_4a"
MESSCOREDEFS += -DNEOFREE -DMESS


# uses these CPUs
MESSCPUS+=TMS9900
MESSCPUS+=TMS9980
MESSCPUS+=TMS9995
MESSCPUS+=TMS99010

# uses these SOUNDs
MESSSOUNDS+=SN76496
MESSSOUNDS+=TMS5220

MESSDRVLIBS = \
	$(MESSOBJ)/ti99.a

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
	$(MESSOBJ)/mess/systems/tutor.o

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


