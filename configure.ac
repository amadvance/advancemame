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
		AC_SUBST([CFLAGS_BUILD],["-O0 -DUSE_COMPILER_GNUC -DUSE_OBJ_ELF -DUSE_OS_UNIX"])
		AC_SUBST([EXE],[])
	;;
	*-*-cygwin* | *-*-mingw32*)
		ac_host=windows
		AC_SUBST([ASMFLAGS],["-f coff"])
		AC_SUBST([CFLAGS_BUILD],["-O0 -DUSE_COMPILER_GNUC -DUSE_OBJ_COFF -DUSE_OS_WINDOWS"])
		AC_SUBST([EXE],[.exe])
	;;
	*-*-msdos*)
		ac_host=dos
		ac_lib="Allegro SEAL VSYNC VBELINE SVGALINE VGALINE"
		AC_SUBST([ASMFLAGS],["-f coff"])
		AC_SUBST([CFLAGS_BUILD],["-O0 -DUSE_COMPILER_GNUC -DUSE_OBJ_COFF -DUSE_OS_DOS"])
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
			LDFLAGS="-g"
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
	[ac_enable_asm=$enableval],
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
	AC_CHECK_PROGS([ASM], [nasm nasmw], [none])
	if test $ASM = none; then
		AC_MSG_ERROR([The NASM assembler is required for the "asm" compilation])
	fi
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

AC_ARG_ENABLE(
	[svgalib],
	AC_HELP_STRING([--enable-svgalib],[enable the use of the SVGALIB library. (default auto)]),
	[ac_lib_svgalib=$enableval],
	[ac_lib_svgalib=auto]
)
if test $ac_lib_svgalib = auto; then
	if test $ac_host = unix; then
		AC_CHECK_LIB(
			[vga],
			[vga_init],
			[ac_lib_svgalib=yes],
			[ac_lib_svgalib=no]
		)
		AC_MSG_CHECKING([for svgalib version])
		AC_TRY_COMPILE(
			[#include <vga.h>],
			[#if !defined(SVGALIB_VER) || (SVGALIB_VER<0x010900)
			choke me
			#endif],
			[],
			[ac_lib_svgalib=no]
		)
		AC_MSG_RESULT([$ac_lib_svgalib])
	else
		ac_lib_svgalib=no
	fi
elif test $ac_lib_svgalib = yes; then
	AC_CHECK_LIB(
		[vga],
		[vga_init],
		[],
		[AC_MSG_ERROR([The SVGALIB library is missing])]
	)
	AC_TRY_COMPILE(
		[#include <vga.h>],
		[#if !defined(SVGALIB_VER) || (SVGALIB_VER<0x010900)
		choke me
		#endif],
		[],
		[AC_MSG_ERROR([The SVGALIB library is too old. Please upgrade to the 1.9 or 2.0 version])]
	)
fi
if test $ac_lib_svgalib = yes; then
	ac_lib="$ac_lib SVGALIB"
fi
AC_SUBST([CONF_LIB_SVGALIB],[$ac_lib_svgalib])

AC_ARG_ENABLE(
	[fb],
	AC_HELP_STRING([--enable-fb],[enable the use of the Frame Buffer library. (default auto)]),
	[ac_lib_fb=$enableval],
	[ac_lib_fb=auto]
)
if test $ac_lib_fb = auto; then
	if test $ac_host = unix; then
		AC_MSG_CHECKING([for FrameBuffer])
		AC_TRY_COMPILE(
			[#include <linux/fb.h>],
			[struct fb_var_screeninfo dummy;],
			[ac_lib_fb=yes],
			[ac_lib_fb=no]
		)
		AC_MSG_RESULT([$ac_lib_fb])
	else
		ac_lib_fb=no
	fi
elif test $ac_lib_fb = yes; then
	AC_MSG_CHECKING([for FrameBuffer])
	AC_TRY_COMPILE(
		[#include <linux/fb.h>],
		[struct fb_var_screeninfo dummy;],
		[],
		[AC_MSG_ERROR([The Frame Buffer library is missing])]
	)
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_fb = yes; then
	ac_lib="$ac_lib FrameBuffer"
fi
AC_SUBST([CONF_LIB_FB],[$ac_lib_fb])

AC_ARG_ENABLE(
	[oss],
	AC_HELP_STRING([--enable-oss],[enable the use of the OSS library. (default auto)]),
	[ac_lib_oss=$enableval],
	[ac_lib_oss=auto]
)
if test $ac_lib_oss = auto; then
	if test $ac_host = unix; then
		AC_MSG_CHECKING([for OSS])
		AC_TRY_COMPILE(
			[#include <sys/soundcard.h>],
			[int dummy = SNDCTL_DSP_SETFMT;],
			[ac_lib_oss=yes],
			[ac_lib_oss=no]
		)
		AC_MSG_RESULT([$ac_lib_oss])
	else
		ac_lib_oss=no
	fi
elif test $ac_lib_oss = yes; then
	AC_MSG_CHECKING([for OSS])
	AC_TRY_COMPILE(
		[#include <sys/soundcard.h>],
		[int dummy = SNDCTL_DSP_SETFMT;],
		[],
		[AC_MSG_ERROR([The OSS library is missing])]
	)
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_oss = yes; then
	ac_lib="$ac_lib OSS"
fi
AC_SUBST([CONF_LIB_OSS],[$ac_lib_oss])

AC_ARG_ENABLE(
	[pthread],
	AC_HELP_STRING([--enable-pthread],[enable the use of the pthread library (default auto)]),
	[ac_lib_pthread=$enableval],
	[ac_lib_pthread=auto]
)
if test $ac_lib_pthread = auto; then
	if test $ac_host = unix; then
		AC_CHECK_LIB(
			[pthread],
			[pthread_create],
			[ac_lib_pthread=yes],
			[ac_lib_pthread=no],
			[]
		)
	else
		ac_lib_pthread=no
	fi
elif test $ac_lib_pthread = yes; then
	AC_CHECK_LIB(
		[pthread],
		[pthread_create],
		[],
		[AC_MSG_ERROR([The pthread library is missing])],
		[]
	)
fi
if test $ac_lib_pthread = yes; then
	ac_lib="$ac_lib pthread"
fi
AC_SUBST([CONF_LIB_PTHREAD],[$ac_lib_pthread])

AC_ARG_ENABLE(
	[slang],
	AC_HELP_STRING([--enable-slang],[enable the use of the sLang library (default auto)]),
	[ac_lib_slang=$enableval],
	[ac_lib_slang=auto]
)
if test $ac_lib_slang = auto; then
	if test $ac_host = unix; then
		AC_CHECK_LIB(
			[slang],
			[SLang_init_tty],
			[ac_lib_slang=yes],
			[ac_lib_slang=no],
			[]
		)
	else
		ac_lib_slang=no
	fi
elif test $ac_lib_slang = yes; then
	AC_CHECK_LIB(
		[slang],
		[SLang_init_tty],
		[],
		[AC_MSG_ERROR([The sLang library is missing])],
		[]
	)
fi
if test $ac_lib_slang = yes; then
	ac_lib="$ac_lib sLang"
fi
AC_SUBST([CONF_LIB_SLANG],[$ac_lib_slang])

AC_ARG_ENABLE(
	[sdl],
	AC_HELP_STRING([--enable-sdl],[enable the use of the SDL library. (default auto)]),
	[ac_lib_sdl=$enableval],
	[ac_lib_sdl=auto]
)
if test $ac_lib_sdl = auto; then
	if test $ac_host = dos; then
		ac_lib_sdl=no
	else
		if test -z ${host_alias} ; then
			AC_CHECK_PROGS(
				[SDLCONFIG],
				[sdl-config],
				[no]
			)
		else
			AC_CHECK_PROGS(
				[SDLCONFIG],
				[${host_alias}-sdl-config],
				[no]
			)
		fi
		if test ! $SDLCONFIG = no; then
			SDLCFLAGS=`$SDLCONFIG --cflags`
			SDLLIBS=`$SDLCONFIG --libs`
		fi
		AC_CHECK_LIB(
			[SDL],
			[SDL_Init],
			[ac_lib_sdl=yes],
			[ac_lib_sdl=no],
			[$SDLLIBS]
		)
	fi
elif test $ac_lib_sdl = yes; then
	if test -z ${host_alias} ; then
		AC_CHECK_PROGS(
			[SDLCONFIG],
			[sdl-config],
			[no]
		)
	else
		AC_CHECK_PROGS(
			[SDLCONFIG],
			[${host_alias}-sdl-config],
			[no]
		)
	fi
	if test ! $SDLCONFIG = no; then
		SDLCFLAGS=`$SDLCONFIG --cflags`
		SDLLIBS=`$SDLCONFIG --libs`
	fi
	AC_CHECK_LIB(
		[SDL],
		[SDL_Init],
		[],
		[AC_MSG_ERROR([The SDL library is missing])],
		[$SDLLIBS]
	)
fi
if test $ac_lib_sdl = yes; then
	ac_lib="$ac_lib SDL"
fi
AC_SUBST([SDLCFLAGS])
AC_SUBST([SDLLIBS])
AC_SUBST([CONF_LIB_SDL],[$ac_lib_sdl])

AC_ARG_ENABLE(
	[svgawin],
	AC_HELP_STRING([--enable-svgawin],[enable the use of the SVGAWIN library. (default auto)]),
	[ac_lib_sdl=$enableval],
	[ac_lib_sdl=auto]
)
if test $ac_lib_sdl = auto; then
	if test $ac_host = windows; then
		ac_lib_svgawin=yes
	else
		ac_lib_svgawin=no
	fi
fi
if test $ac_lib_svgawin = yes; then
	ac_lib="$ac_lib SVGAWIN"
fi
AC_SUBST([CONF_LIB_SVGAWIN],[$ac_lib_svgawin])

AC_ARG_ENABLE(
	[kraw],
	AC_HELP_STRING([--enable-kraw],[enable the use of the KRAW keyboard library. (default auto)]),
	[ac_lib_kraw=$enableval],
	[ac_lib_kraw=auto]
)
if test $ac_lib_kraw = auto; then
	if test $ac_host = unix; then
		AC_MSG_CHECKING([for KRAW keyboard interface])
		AC_TRY_COMPILE(
			[#include <sys/kd.h>],
			[int dummy = K_MEDIUMRAW;],
			[ac_lib_kraw=yes],
			[ac_lib_kraw=no]
		)
		AC_MSG_RESULT([$ac_lib_kraw])
	else
		ac_lib_kraw=no
	fi
elif test $ac_lib_fb = yes; then
	AC_MSG_CHECKING([for KRAW keyboard interface])
	AC_TRY_COMPILE(
		[#include <sys/kd.h>],
		[int dummy = K_MEDIUMRAW;],
		[],
		[AC_MSG_ERROR([The KRAW keyboard interface is missing])]
	)
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_kraw = yes; then
	ac_lib="$ac_lib KRAW"
fi
AC_SUBST([CONF_LIB_KRAW],[$ac_lib_kraw])

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
if test $ac_enable_asm = yes; then
	echo "Assembler for Pentium : yes"
else
	echo "Assembler for Pentium : no"
fi
echo "Libraries :" $ac_lib
echo ""
echo "== Compiler =="
echo "CC :" $CC
echo "CFLAGS :" $CFLAGS $CFLAGS_OPTOPT
echo "LDFLAGS :" $LDFLAGS
echo ""
echo "== Configuration =="
echo "Emulator :" $ac_with_emu
if test ! $ac_with_emu = none ; then
	echo "Debugger :" $ac_enable_debugger
fi
echo "Compress executables :" $ac_enable_compress


