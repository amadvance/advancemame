# Process this file with autoconf to produce a configure script.
AC_INIT([advance],[source])
AC_CANONICAL_HOST
AC_PREREQ([2.53])
AC_COPYRIGHT([Copyright 2002 Andrea Mazzoleni])

# Checks for system.

case "$host" in
	*-*-linux*)
		ac_host=linux
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
		AC_MSG_ERROR([Unsupported host environment: Please add to configure.ac])
        ;;
esac
AC_SUBST([CONF_HOST],[$ac_host])

case "$build" in
	*-*-linux*)
		ac_build=linux
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
		AC_MSG_ERROR([Unsupported build environment: Please add to configure.ac])
        ;;
esac
AC_SUBST([CONF_BUILD],[$ac_build])

AC_ARG_ENABLE(
	[debug],
	AC_HELP_STRING([--enable-debug],[enable the debug information (default no)]),
	[ac_enable_debug=$enableval],
	[ac_enable_debug=no]
)
AC_SUBST([CONF_DEBUG],[$ac_enable_debug])

if test -z "$CFLAGS" ; then
	if test $ac_enable_debug = yes ; then
		CFLAGS="-O0 -g -Wall -Wno-sign-compare"
	else
		CFLAGS="-O3 -fomit-frame-pointer -Wall -Wno-sign-compare -Wno-unused"
	fi
fi
if test -z "$LDFLAGS" ; then
	if test $ac_enable_debug = yes ; then
		if test $ac_host = linux ; then
			LDFLAGS="-rdynamic"
		else
			LDFLAGS=""
		fi
	else
		LDFLAGS="-s"
	fi
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

AC_SUBST([CONF_CFLAGS_OPT],[$CFLAGS])
AC_SUBST([CONF_LDFLAGS],[$LDFLAGS])

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

AC_CHECK_CPU_ARCH

AC_ARG_WITH(
	[arch],
	AC_HELP_STRING([--with-arch],[select the host cpu architecture: i586, i686, k6, athlon, blend... (default auto)]),
	[ac_with_arch=$withval],
	[ac_with_arch=auto]
)
if test $ac_with_arch = auto; then
	AC_CHECK_CC_ARCH
	ac_with_arch=$ac_cc_arch
fi
if test ! $ac_with_arch = blend; then
	CFLAGS_ARCH="-march=$ac_with_arch $CFLAGS_ARCH"
fi
AC_SUBST([CONF_ARCH],[$ac_with_arch])

AC_ARG_ENABLE(
	[asm],
	AC_HELP_STRING([--enable-asm],[enable the assembler 586 optimizations (default auto)]),
	[ac_enable_asm=$withval],
	[ac_enable_asm=auto]
)
if test $ac_enable_asm = auto; then
	if test $ac_cpu_family = 3 -o $ac_cpu_family = 4 ; then
		ac_enable_asm=no
	else
		ac_enable_asm=yes
	fi
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
	AC_HELP_STRING([--with-system],[select the system to use: native or sdl. (default auto)]),
	[ac_with_system=$withval],
	[ac_with_system=auto]
)
if test $ac_with_system = auto; then
	if test $ac_host = dos; then
		ac_with_system=native
	elif test $ac_host = windows; then
		ac_with_system=sdl
	elif test $ac_with_emu = none; then
		ac_with_system=sdl
	else
		ac_with_system=native
	fi
fi
if test $ac_with_system = sdl; then
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
		AC_MSG_ERROR([The SDL library is required for the "sdl" system]),
		[$SDLLIBS]
	)
	AC_SUBST(SDLCFLAGS)
	AC_SUBST(SDLLIBS)
elif test $ac_with_system = native; then
	if test $ac_host = dos; then
		AC_CHECK_LIB(
			[alleg],
			[install_allegro],
			[ac_library_allegro=yes],
			[ac_library_allegro=no]
		)
		if test $ac_library_allegro = no; then
			AC_MSG_ERROR([The Allegro library is required for the "native" system])
		fi
		AC_CHECK_LIB(
			[audio],
			[AInitialize],
			[ac_library_seal=yes],
			[ac_library_seal=no]
		)
		if test $ac_library_seal = no; then
			AC_MSG_ERROR([The SEAL library is required for the "native" system])
		fi
	else
		AC_CHECK_LIB(
			[vga],
			[vga_init],
			[ac_library_vga=yes],
			[ac_library_vga=no]
		)
		if test $ac_library_vga = no; then
			AC_MSG_ERROR([The vgalib library is required for the "native" system])
		fi
		AC_TRY_COMPILE(
			[#include <vga.h>],
			[#if !defined(SVGALIB_VER) || (SVGALIB_VER<0x010900)
			choke me
			#endif],
			[ac_library_vga_version=yes],
			[ac_library_vga_version=no]
		)
		if test $ac_library_vga_version = no; then
			AC_MSG_ERROR([Your vgalib library is too old. Please upgrade to the 1.9 or 2.0 version])
		fi
	fi
elif test $ac_with_system = none; then
	AC_MSG_ERROR([You must specify a --with-system= option])
else
	AC_MSG_ERROR([Unknown system $ac_with_system])
fi
AC_SUBST([CONF_SYSTEM],[$ac_with_system])

AC_ARG_ENABLE(
	[smp],
	AC_HELP_STRING([--enable-smp],[enable the multi processor support (default yes)]),
	[ac_enable_smp=$enableval],
	[ac_enable_smp=yes]
)
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
echo "Environment :" $ac_host "("$host")"
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
echo "CPU :" $ac_with_arch
echo ""
echo "== Compiler =="
echo "CFLAGS (optimization) :" $CFLAGS
echo "CFLAGS (architecture) :" $CFLAGS_ARCH
echo "LDFLAGS :" $LDFLAGS
echo ""
echo "== Configuration =="
echo "System library :" $ac_with_system
echo "Emulator :" $ac_with_emu
if test ! $ac_with_emu = none ; then
	echo "Debugger :" $ac_enable_debugger
fi
echo "Compress executables :" $ac_enable_compress


