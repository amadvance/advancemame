###########################################################################
#
#   tiny.mak
#
#   Small driver-specific example makefile
#	Use make TARGET=tiny to build
#
#   Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


#-------------------------------------------------
# tiny.c contains the list of drivers
#-------------------------------------------------

COREOBJS += $(OBJ)/tiny.o



#-------------------------------------------------
# You need to define two strings:
#
#	TINY_NAME is a comma-separated list of driver
#	names that will be referenced.
#
#	TINY_DRIVER should be the same list but with
#	an & in front of each name.
#-------------------------------------------------

COREDEFS += -DTINY_NAME="driver_robby,driver_gridlee,driver_polyplay,driver_alienar"
COREDEFS += -DTINY_POINTER="&driver_robby,&driver_gridlee,&driver_polyplay,&driver_alienar"



#-------------------------------------------------
# Specify all the CPU cores necessary for these
# drivers.
#-------------------------------------------------

CPUS += Z80
CPUS += M6809



#-------------------------------------------------
# Specify all the sound cores necessary for these
# drivers.
#-------------------------------------------------

SOUNDS += CUSTOM
SOUNDS += SAMPLES
SOUNDS += SN76496
SOUNDS += ASTROCADE
SOUNDS += DAC
SOUNDS += HC55516
SOUNDS += YM2151
SOUNDS += OKIM6295



#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# above.
#-------------------------------------------------

DRVLIBS = \
	$(OBJ)/machine/6821pia.o \
	$(OBJ)/machine/ticket.o \
	$(OBJ)/vidhrdw/res_net.o \
	$(OBJ)/drivers/astrocde.o $(OBJ)/machine/astrocde.o $(OBJ)/vidhrdw/astrocde.o \
	$(OBJ)/sndhrdw/gorf.o $(OBJ)/sndhrdw/wow.o \
	$(OBJ)/drivers/gridlee.o $(OBJ)/sndhrdw/gridlee.o $(OBJ)/vidhrdw/gridlee.o \
	$(OBJ)/drivers/polyplay.o $(OBJ)/sndhrdw/polyplay.o $(OBJ)/vidhrdw/polyplay.o \
	$(OBJ)/drivers/williams.o $(OBJ)/machine/williams.o $(OBJ)/sndhrdw/williams.o $(OBJ)/vidhrdw/williams.o \
