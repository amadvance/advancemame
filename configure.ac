# Process this file with autoconf to produce a configure script.
AC_INIT([advance],[source])
AC_CANONICAL_HOST
AC_PREREQ([2.53])
AC_COPYRIGHT([Copyright 2002 Andrea Mazzoleni])

# Checks for system.

case "$host" in
	*-*-linux*)
		ac_host=unix
		AC_SUBST([ASMFLAGS],["-f elf"])
		AC_SUBST([CFLAGS_BUILD],["-O0 -DCOMPILER_TARGET_GNUC -DOBJ_TARGET_ELF"])
		AC_SUBST([EXE],[])
	;;
	*-*-cygwin* | *-*-mingw32*)
		ac_host=windows
		AC_SUBST([ASMFLAGS],["-f coff"])
		AC_SUBST([CFLAGS_BUILD],["-O0 -DCOMPILER_TARGET_GNUC -DOBJ_TARGET_COFF"])
		AC_SUBST([EXE],[.exe])
	;;
	*-*-msdos*)
		ac_host=dos
		AC_SUBST([ASMFLAGS],["-f coff"])
		AC_SUBST([CFLAGS_BUILD],["-O0 -DCOMPILER_TARGET_GNUC -DOBJ_TARGET_COFF"])
		AC_SUBST([EXE],[.exe])
	;;
	*)
		AC_MSG_WARN([Never tried host platform! Please report if it's working])
		ac_host=unix
		AC_SUBST([ASMFLAGS],[""])
		AC_SUBST([CFLAGS_BUILD],["-O0"])
		AC_SUBST([EXE],[])
	;;
esac
AC_SUBST([CONF_HOST],[$ac_host])

case "$build" in
	*-*-linux*)
		ac_build=unix
		AC_SUBST([EXE_BUILD],[])
	;;
	*-*-cygwin* | *-*-mingw32*)
		ac_build=windows
		AC_SUBST([EXE_BUILD],[.exe])
	;;
	*-*-msdos*)
		ac_build=dos
		AC_SUBST([EXE_BUILD],[.exe])
	;;
	*)
		AC_MSG_WARN([Never tried build platform! Please report if it's working])
		ac_build=unix
		AC_SUBST([EXE_BUILD],[])
	;;
esac
AC_SUBST([CONF_BUILD],[$ac_build])

if test -z "$CFLAGS" ; then
	ac_auto_cflags=yes
else
	ac_auto_cflags=no
fi
if test -z "$LDFLAGS" ; then
	ac_auto_ldflags=yes
else
	ac_auto_ldflags=no
fi

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_CHECK_TOOLS(AR, ar)
AC_CHECK_TOOLS(RC, windres)
AC_CHECK_PROGS(RM, rm)
AC_CHECK_PROGS(CP, cp)
AC_CHECK_PROGS(MKDIR, mkdir)
AC_CHECK_PROGS(ECHO, echo)
AC_CHECK_PROGS(TAR, tar)
AC_CHECK_PROGS(GZIP, gzip)
AC_CHECK_PROGS(ZIP, zip)
AC_CHECK_PROGS(ASM, nasm)
AC_CHECK_PROGS(CC_BUILD, gcc cc)
AC_CHECK_PROGS(CXX_BUILD, g++ gxx c++ cxx)

# Checks for libraries.
AC_CHECK_LIB(
	[z],
	[adler32],
	[],
	AC_MSG_ERROR([The zlib library is required])
)
if test $ac_host = windows; then
	ZLIBS="-static -lz"
else
	ZLIBS="-lz"
fi
AC_SUBST([ZLIBS],[$ZLIBS])

# Checks for header files.
# Checks for typedefs, structures, and compiler characteristics.
# Checks for library functions.

# Checks for optimizations
AC_ARG_ENABLE(
	[debug],
	AC_HELP_STRING([--enable-debug],[enable the debug information (default no)]),
	[ac_enable_debug=$enableval],
	[ac_enable_debug=no]
)
AC_SUBST([CONF_DEBUG],[$ac_enable_debug])

if test $ac_auto_cflags = yes; then
	if test $ac_enable_debug = yes ; then
		CFLAGS="-O0 -g -Wall -Wno-sign-compare"
	else
		CFLAGS=""
		AC_CHECK_CPU_ARCH
		AC_CHECK_CC_ARCH
		AC_CHECK_CC_OPT([-O3], [CFLAGS="$CFLAGS -O3"], [AC_CHECK_CC_OPT([-O2], [CFLAGS="$CFLAGS -O2"], [CFLAGS="$CFLAGS -O"])])
		if test ! $ac_cc_arch = blend; then
			CFLAGS="-march=$ac_cc_arch $CFLAGS"
			AC_CHECK_CC_OPT([-fomit-frame-pointer], [CFLAGS="$CFLAGS -fomit-frame-pointer"], [])
		fi
		AC_CHECK_CC_OPT([-Wall -Wno-sign-compare -Wno-unused], [CFLAGS="$CFLAGS -Wall -Wno-sign-compare -Wno-unused"], [])
	fi
fi
if test $ac_auto_ldflags = yes ; then
	if test $ac_enable_debug = yes ; then
		if test $ac_host = unix ; then
			LDFLAGS="-rdynamic"
		else
			LDFLAGS=""
		fi
	else
		LDFLAGS="-s"
	fi
fi

AC_SUBST([CONF_CFLAGS_OPT],[$CFLAGS])
AC_SUBST([CONF_LDFLAGS],[$LDFLAGS])

# Checks for architecture
AC_C_BIGENDIAN(
	[ac_is_bigendian=yes],
	[ac_is_bigendian=no]
)
if test $ac_is_bigendian = yes; then
	CFLAGS_ARCH="$CFLAGS_ARCH -DUSE_MSB"
else
	CFLAGS_ARCH="$CFLAGS_ARCH -DUSE_LSB"
fi

AC_ARG_ENABLE(
	[asm],
	AC_HELP_STRING([--enable-asm],[enable the Pentium assembler optimizations (default auto)]),
	[ac_enable_asm=$withval],
	[ac_enable_asm=auto]
)
if test $ac_enable_asm = auto; then
	AC_MSG_CHECKING([whether ${CC-cc} accepts Pentium assembler])
	AC_TRY_COMPILE(
		[],
		[#if !defined(__i386__)
		choke me
		#endif],
		[ac_enable_asm=yes],
		[ac_enable_asm=no]
	)
	AC_MSG_RESULT([$ac_enable_asm])
fi
if test $ac_enable_asm = yes; then
	CFLAGS_ARCH="$CFLAGS_ARCH -DUSE_ASM_i586"
fi

# Export the architecture CFLAGS
AC_SUBST([CONF_CFLAGS_ARCH],[$CFLAGS_ARCH])

# Checks for configuration options.
AC_ARG_WITH(
	[emu],
	AC_HELP_STRING([--with-emu],[select the emulator to use: mame, mess, neomame, cpmame, pac, none (default auto)]),
	[ac_with_emu=$withval],
	[ac_with_emu=auto]
)
if test $ac_with_emu = auto; then
	if test -d $srcdir/src ; then
		ac_with_emu=mame
	elif test -d $srcdir/srcmess ; then
		ac_with_emu=mess
	elif test -d $srcdir/srcpac ; then
		ac_with_emu=pac
	else
		ac_with_emu=none
	fi
fi
if test $ac_with_emu = mame -o $ac_with_emu = neomame -o $ac_with_emu = cpmame; then
	if test ! -f $srcdir/src/mame.mak ; then
		AC_MSG_WARN([You need the MAME source in the $srcdir/src/ directory])
	fi
	if test ! -f $srcdir/src/advance.pat ; then
		AC_MSG_WARN([You must patch the MAME source in $srcdir/src/ with the $srcdir/advance/advmame.dif patch])
	fi
elif test $ac_with_emu = mess; then
	if test ! -f $srcdir/srcmess/mame.mak ; then
		AC_MSG_WARN([You need the MESS source in the $srcdir/srcmess/ directory])
	fi
	if test ! -f $srcdir/mess/mess.mak ; then
		AC_MSG_WARN([You need the MESS source in the $srcdir/srcmess/ directory])
	fi
	if test ! -f $srcdir/srcmess/advance.pat ; then
		AC_MSG_WARN([You must patch the MESS source in $srcdir/srcmess/ with the advance/advmess.dif patch])
	fi
elif test $ac_with_emu = pac; then
	if test ! -f $srcdir/srcpac/mame.mak ; then
		AC_MSG_WARN([You need the PacMAME source in the $srcdir/srcpac/ directory])
	fi
	if test ! -f $srcdir/srcpac/advance.pat ; then
		AC_MSG_WARN([You must patch the PacMAME source in $srcdir/srcpac/ with the advance/advpac.dif patch])
	fi
elif test ! $ac_with_emu = none; then
	AC_MSG_ERROR([Unknown emulator $ac_with_emu])
fi
if test -f $srcdir/makefile ; then
	AC_MSG_WARN([A lowercase makefile exists! Have you deleted the original emulator makefile ?])
fi
AC_SUBST(CONF_EMU,$ac_with_emu)

AC_ARG_WITH(
	[system],
	AC_HELP_STRING([--with-system],[select the system to use: linux, dos or sdl. (default auto)]),
	[ac_with_system=$withval],
	[ac_with_system=auto]
)
if test $ac_with_system = auto; then
	if test $ac_host = dos; then
		ac_with_system=dos
	elif test $ac_host = windows; then
		ac_with_system=sdl
	elif test $ac_with_emu = none; then
		ac_with_system=sdl
	else
		ac_with_system=linux
	fi
elif test $ac_with_system = sdl; then
	if test -z ${host_alias} ; then
		AC_CHECK_PROGS(
			[SDLCONFIG],
			[sdl-config],
			[none]
		)
	else
		AC_CHECK_PROGS(
			[SDLCONFIG],
			[${host_alias}-sdl-config],
			[none]
		)
	fi
	if test $SDLCONFIG = none; then
		AC_MSG_ERROR([The sdl-config script is required for the "sdl" system])
	fi

	SDLCFLAGS=`$SDLCONFIG --cflags`
	SDLLIBS=`$SDLCONFIG --libs`
	AC_CHECK_LIB(
		[SDL],
		[SDL_Init],
		[],
		[AC_MSG_ERROR([The SDL library is required for the "sdl" system])],
		[$SDLLIBS]
	)
	AC_SUBST(SDLCFLAGS)
	AC_SUBST(SDLLIBS)
elif test $ac_with_system = dos; then
	AC_CHECK_LIB(
		[alleg],
		[install_allegro],
		[],
		[AC_MSG_ERROR([The Allegro library is required for the "dos" system])]
	)
	AC_CHECK_LIB(
		[audio],
		[AInitialize],
		[],
		[AC_MSG_ERROR([The SEAL library is required for the "dos" system])]
	)
elif test $ac_with_system = linux; then
	AC_CHECK_LIB(
		[vga],
		[vga_init],
		[ac_library_vga=yes],
		[AC_MSG_ERROR([The vgalib library is required for the "linux" system])]
	)
	AC_TRY_COMPILE(
		[#include <vga.h>],
		[#if !defined(SVGALIB_VER) || (SVGALIB_VER<0x010900)
		choke me
		#endif],
		[],
		[AC_MSG_ERROR([Your vgalib library is too old. Please upgrade to the 1.9 or 2.0 version])]
	)
elif test $ac_with_system = none; then
	AC_MSG_ERROR([You must specify a --with-system= option])
else
	AC_MSG_ERROR([Unknown system $ac_with_system, valid values are linux, dos and sdl])
fi
AC_SUBST([CONF_SYSTEM],[$ac_with_system])

AC_ARG_ENABLE(
	[smp],
	AC_HELP_STRING([--enable-smp],[enable the multi processor support (default auto)]),
	[ac_enable_smp=$enableval],
	[ac_enable_smp=auto]
)
if test $ac_enable_smp = auto; then
	AC_CHECK_LIB(
		[pthread],
		[pthread_create],
		[ac_enable_smp=yes],
		[ac_enable_smp=no],
		[]
	)
elif test $ac_enable_smp = yes; then
	AC_CHECK_LIB(
		[pthread],
		[pthread_create],
		[],
		[AC_MSG_ERROR([The pthread library is required for the Multiprocessor support])],
		[]
	)
fi
AC_SUBST([CONF_SMP],[$ac_enable_smp])

AC_ARG_ENABLE(
	[debugger],
	AC_HELP_STRING([--enable-debugger],[enable the emulator debugger (default no)]),
	[ac_enable_debugger=$enableval],
	[ac_enable_debugger=no]
)
AC_SUBST([CONF_DEBUGGER],[$ac_enable_debugger])

AC_ARG_ENABLE(
	[compress],
	AC_HELP_STRING([--enable-compress],[enable the compression of the executable (default no)]),
	[ac_enable_compress=$enableval],
	[ac_enable_compress=no]
)
if test $ac_enable_compress = yes; then
	AC_CHECK_PROGS(
		[UPX],
		[upx],
		AC_MSG_ERROR([The upx program is required for compressing])
	)
	AC_CHECK_PROGS(
		[TOUCH],
		[touch],
		AC_MSG_ERROR([The touch program is required for compressing])
	)
fi
AC_SUBST([CONF_COMPRESS],[$ac_enable_compress])
AC_SUBST([UPX],[$UPX])
AC_SUBST([TOUCH],[$TOUCH])

# Notify that ./configure was runned
AC_SUBST([CONF],[yes])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo ""
echo "== Host Architecture =="
echo "Environment :" $host
if test $ac_is_bigendian = yes; then
	echo "Endianess : big"
else
	echo "Endianess : little"
fi
echo "Multiprocessor :" $ac_enable_smp
if test $ac_enable_asm = yes; then
	echo "Assembler for Pentium : yes"
else
	echo "Assembler for Pentium : no"
fi
echo ""
echo "== Compiler =="
echo "CC :" $CC
echo "CFLAGS :" $CFLAGS
echo "LDFLAGS :" $LDFLAGS
echo ""
echo "== Configuration =="
echo "System library :" $ac_with_system
echo "Emulator :" $ac_with_emu
if test ! $ac_with_emu = none ; then
	echo "Debugger :" $ac_enable_debugger
fi
echo "Compress executables :" $ac_enable_compress


