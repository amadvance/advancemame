# Process this file with autoconf to produce a configure script.
AC_INIT([advance],[source])
AC_CANONICAL_HOST
AC_PREREQ(2.57)
AC_COPYRIGHT([Copyright 2003 Andrea Mazzoleni])

# Checks for system.
case "$host" in
	*-*-linux*)
		ac_host=unix
		ac_host_cflags=
		ac_host_ldflags=
		AC_SUBST([ASMFLAGS],["-f elf"])
		AC_SUBST([CFLAGS_BUILD],["-O0 -DUSE_COMPILER_GNUC -DUSE_OBJ_ELF -DUSE_OS_UNIX"])
		AC_SUBST([EXE],[])
	;;
	*-*-darwin*)
		ac_host=unix
		ac_host_cflags=-no-cpp-precomp
		ac_host_ldflags=-L/usr/local/lib
		AC_SUBST([ASMFLAGS],[""])
		AC_SUBST([CFLAGS_BUILD],["-O0"])
		AC_SUBST([EXE],[])
	;;
	*-*-cygwin* | *-*-mingw32*)
		ac_host=windows
		ac_host_cflags=
		ac_host_ldflags=
		AC_SUBST([ASMFLAGS],["-f coff"])
		AC_SUBST([CFLAGS_BUILD],["-O0 -DUSE_COMPILER_GNUC -DUSE_OBJ_COFF -DUSE_OS_WINDOWS"])
		AC_SUBST([EXE],[.exe])
	;;
	*-*-msdos*)
		ac_host=dos
		ac_host_cflags=
		ac_host_ldflags=
		AC_SUBST([ASMFLAGS],["-f coff"])
		AC_SUBST([CFLAGS_BUILD],["-O0 -DUSE_COMPILER_GNUC -DUSE_OBJ_COFF -DUSE_OS_DOS"])
		AC_SUBST([EXE],[.exe])
	;;
	*)
		AC_MSG_WARN([Never tried host platform! Please report if it's working])
		ac_host=unix
		ac_host_cflags=
		ac_host_ldflags=
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
	*-*-darwin*)
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
AC_PROG_CC
AC_PROG_CXX
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

# Checks for header files.
# Checks for typedefs, structures, and compiler characteristics.
# Checks for library functions.

# Checks for optimizations
AC_ARG_ENABLE(
	[debug],
	AC_HELP_STRING([--enable-debug],[enable the test compilation without optimization and with debug info. Only for developers. (default no)]),
	[ac_enable_debug=$enableval],
	[ac_enable_debug=no]
)
AC_SUBST([CONF_DEBUG],[$ac_enable_debug])

AC_ARG_ENABLE(
	[test],
	AC_HELP_STRING([--enable-test],[enable the test compilation without optimization. Only for developers. (default no)]),
	[ac_enable_test=$enableval],
	[ac_enable_test=no]
)

AC_ARG_ENABLE(
	[static],
	AC_HELP_STRING([--enable-static],[enable static compilation. (default no)]),
	[ac_enable_static=$enableval],
	[ac_enable_static=no]
)

if test $ac_auto_cflags = yes; then
	if test $ac_enable_debug = yes ; then
		CFLAGS="-O0 -g -Wall -Wno-sign-compare -Wno-unused"
	elif test $ac_enable_test = yes ; then
		CFLAGS="-O0 -Wall -Wno-sign-compare -Wno-unused"
	else
		CFLAGS=""
		AC_CHECK_CPU_ARCH
		AC_CHECK_CC_ARCH
		if test ! $ac_cc_arch = blend; then
			CFLAGS="$CFLAGS -march=$ac_cc_arch "
			AC_CHECK_CC_OPT([-fomit-frame-pointer], [CFLAGS="$CFLAGS -fomit-frame-pointer"], [])
		fi
		# -O3 don't seem to be safe on x86 platforms, also on latest gcc 3.3.x
		AC_CHECK_CC_OPT([-O2], [CFLAGS="$CFLAGS -O2"], [CFLAGS="$CFLAGS -O"])
		# -fno-merge-constants is required to speedup the link time. ld spends a lot of time trying to merge partial strings.
		AC_CHECK_CC_OPT([-fno-merge-constants], [CFLAGS="$CFLAGS -fno-merge-constants"], [])
		AC_CHECK_CC_OPT([-Wall], [CFLAGS="$CFLAGS -Wall"], [])
		AC_CHECK_CC_OPT([-Wno-sign-compare], [CFLAGS="$CFLAGS -Wno-sign-compare"], [])
		AC_CHECK_CC_OPT([-Wno-unused], [CFLAGS="$CFLAGS -Wno-unused"], [])
	fi
	if test ! -z $ac_host_cflags; then
		CFLAGS="$ac_host_cflags $CFLAGS"
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
	if test $ac_enable_static = yes ; then
		LDFLAGS="-static $LDFLAGS"
	fi
	if test ! -z $ac_host_ldflags; then
		LDFLAGS="$ac_host_ldflags $LDFLAGS"
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
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[
			#if !defined(__GNUC__) || !defined(__i386__)
			choke me
			#endif
		]])],[ac_enable_asm=yes],[ac_enable_asm=no])
	AC_MSG_RESULT([$ac_enable_asm])
	if test $ac_enable_asm = yes; then
		AC_CHECK_PROGS([ASM], [${ac_tool_prefix}nasm], [none])
		if test $ASM = none; then
			ac_enable_asm=no
		else
			CFLAGS_ARCH="$CFLAGS_ARCH -DUSE_ASM_i586"
		fi
	fi
elif test $ac_enable_asm = yes; then
	AC_CHECK_PROGS([ASM], [${ac_tool_prefix}nasm], [none])
	if test $ASM = none; then
		AC_MSG_ERROR([the NASM assembler is required for the "asm" compilation])
	else
		CFLAGS_ARCH="$CFLAGS_ARCH -DUSE_ASM_i586"
	fi
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
	if test -f $srcdir/advance/advmame.dif ; then
		ac_with_emu=mame
	elif test -f $srcdir/advance/advmess.dif ; then
		ac_with_emu=mess
	elif test -f $srcdir/advance/advpac.dif ; then
		ac_with_emu=pac
	else
		ac_with_emu=none
	fi
fi
if test $ac_with_emu = mame -o $ac_with_emu = neomame -o $ac_with_emu = cpmame; then
	if test ! -f $srcdir/src/mame.mak ; then
		AC_MSG_ERROR([you need the MAME source in the $srcdir/src/ directory. Check the BUILD file for instructions.])
	fi
	if test ! -f $srcdir/src/advance.pat ; then
		AC_MSG_ERROR([you must patch the MAME source in $srcdir/src/ with the $srcdir/advance/advmame.dif patch. Check the BUILD file for instructions.])
	fi
elif test $ac_with_emu = mess; then
	if test ! -f $srcdir/srcmess/mame.mak ; then
		AC_MSG_ERROR([you need the MESS source in the $srcdir/srcmess/ directory. Check the BUILD file for instructions.])
	fi
	if test ! -f $srcdir/mess/mess.mak ; then
		AC_MSG_ERROR([you need the MESS source in the $srcdir/mess/ directory. Check the BUILD file for instructions.])
	fi
	if test ! -f $srcdir/srcmess/advance.pat ; then
		AC_MSG_ERROR([you must patch the MESS source in $srcdir/srcmess/ with the advance/advmess.dif patch. Check the BUILD file for instructions.])
	fi
	if test ! -f $srcdir/mess/advance.pat ; then
		AC_MSG_ERROR([you must patch the MESS source in $srcdir/mess/ with the advance/mess.dif patch. Check the BUILD file for instructions.])
	fi
elif test $ac_with_emu = pac; then
	if test ! -f $srcdir/srcpac/mame.mak ; then
		AC_MSG_ERROR([you need the PacMAME source in the $srcdir/srcpac/ directory. Check the BUILD file for instructions.])
	fi
	if test ! -f $srcdir/srcpac/advance.pat ; then
		AC_MSG_ERROR([you must patch the PacMAME source in $srcdir/srcpac/ with the advance/advpac.dif patch. Check the BUILD file for instructions.])
	fi
elif test ! $ac_with_emu = none; then
	AC_MSG_ERROR([unknown emulator $ac_with_emu])
fi
AC_SUBST(CONF_EMU,$ac_with_emu)

AC_ARG_ENABLE(
	[debugger],
	AC_HELP_STRING([--enable-debugger],[enable the emulator debugger (default no)]),
	[ac_enable_debugger=$enableval],
	[ac_enable_debugger=no]
)
AC_SUBST([CONF_DEBUGGER],[$ac_enable_debugger])

# Checks for libraries.

ac_lib_video_flag=no
ac_lib_sound_flag=no
ac_lib_keyboard_flag=no

if test $ac_host = dos; then
	ac_lib_video="$ac_lib_video svgaline vbeline vgaline vbe"
	ac_lib_sound="$ac_lib_sound seal vsync allegro"
	ac_lib_keyboard="$ac_lib_keyboard allegro"
	ac_lib_joystick="$ac_lib_joystick allegro"
	ac_lib_mouse="$ac_lib_mouse allegro"
	ac_lib_video_flag=yes
	ac_lib_sound_flag=yes
	ac_lib_keyboard_flag=yes
fi

AC_CHECK_LIB(
	[z],
	[adler32],
	[],
	AC_MSG_ERROR([the zlib library is required])
)
ZLIBS="-lz"
AC_SUBST([ZLIBS],[$ZLIBS])

AC_ARG_ENABLE(
	[svgalib],
	AC_HELP_STRING([--enable-svgalib],[enable the use of the SVGALIB library. (default auto)]),
	[ac_lib_svgalib=$enableval],
	[ac_lib_svgalib=auto]
)
if test $ac_lib_svgalib = auto; then
	AC_CHECK_LIB(
		[vga],
		[vga_init],
		[ac_lib_svgalib=yes],
		[ac_lib_svgalib=no],
		[-lm]
	)
	if test $ac_lib_svgalib = yes; then
		AC_MSG_CHECKING([for svgalib version])
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <vga.h>]], [[
				#if !defined(SVGALIB_VER) || (SVGALIB_VER<0x010900)
				choke me
				#endif
			]])],[],[ac_lib_svgalib=no])
		AC_MSG_RESULT([$ac_lib_svgalib])
	fi
elif test $ac_lib_svgalib = yes; then
	AC_CHECK_LIB(
		[vga],
		[vga_init],
		[],
		[AC_MSG_ERROR([the SVGALIB library is missing])],
		[-lm]
	)
	AC_MSG_CHECKING([for svgalib version])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <vga.h>]], [[
			#if !defined(SVGALIB_VER) || (SVGALIB_VER<0x010900)
			choke me
			#endif
		]])],[],[AC_MSG_ERROR([the SVGALIB library is too old. Please upgrade to the 1.9 or 2.0 version])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_svgalib = yes; then
	ac_lib_video="$ac_lib_video svgalib"
	ac_lib_keyboard="$ac_lib_keyboard svgalib"
	ac_lib_mouse="$ac_lib_mouse svgalib"
	ac_lib_joystick="$ac_lib_joystick svgalib"
	ac_lib_video_flag=yes
	ac_lib_keyboard_flag=yes
fi
AC_SUBST([CONF_LIB_SVGALIB],[$ac_lib_svgalib])

AC_ARG_ENABLE(
	[fb],
	AC_HELP_STRING([--enable-fb],[enable the use of the Frame Buffer library. (default auto)]),
	[ac_lib_fb=$enableval],
	[ac_lib_fb=auto]
)
if test $ac_lib_fb = auto; then
	AC_MSG_CHECKING([for FrameBuffer])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <linux/fb.h>]], [[
			struct fb_var_screeninfo dummy;
		]])],[ac_lib_fb=yes],[ac_lib_fb=no])
	AC_MSG_RESULT([$ac_lib_fb])
elif test $ac_lib_fb = yes; then
	AC_MSG_CHECKING([for FrameBuffer])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <linux/fb.h>]], [[
			struct fb_var_screeninfo dummy;
		]])],[],[AC_MSG_ERROR([the Frame Buffer library is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_fb = yes; then
	ac_lib_video="$ac_lib_video fb"
	ac_lib_video_flag=yes
fi
AC_SUBST([CONF_LIB_FB],[$ac_lib_fb])

AC_ARG_ENABLE(
	[alsa],
	AC_HELP_STRING([--enable-alsa],[enable the use of the ALSA library. (default auto)]),
	[ac_lib_alsa=$enableval],
	[ac_lib_alsa=auto]
)
if test $ac_lib_alsa = auto; then
	AC_CHECK_LIB(
		[asound],
		[snd_pcm_open],
		[ac_lib_alsa=yes],
		[ac_lib_alsa=no]
	)
	if test $ac_lib_alsa = yes; then
		AC_MSG_CHECKING([for ALSA version])
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <alsa/asoundlib.h>]], [[
				#if SND_LIB_VERSION < ((0<<16)|(9<<8)|0)
				choke me
				#endif
			]])],[],[ac_lib_alsa=no])
		AC_MSG_RESULT([$ac_lib_alsa])
	fi
elif test $ac_lib_alsa = yes; then
	AC_CHECK_LIB(
		[asound],
		[snd_pcm_open],
		[],
		[AC_MSG_ERROR([the ALSA library is missing])]
	)
	AC_MSG_CHECKING([for ALSA version])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <alsa/asoundlib.h>]], [[
			#if SND_LIB_VERSION < ((0<<16)|(9<<8)|0)
			choke me
			#endif
		]])],[],[AC_MSG_ERROR([the ALSA library is too old])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_alsa = yes; then
	ac_lib_sound="$ac_lib_sound alsa"
	ac_lib_sound_flag=yes
fi
AC_SUBST([CONF_LIB_ALSA],[$ac_lib_alsa])

AC_ARG_ENABLE(
	[oss],
	AC_HELP_STRING([--enable-oss],[enable the use of the OSS library. (default auto)]),
	[ac_lib_oss=$enableval],
	[ac_lib_oss=auto]
)
if test $ac_lib_oss = auto; then
	AC_MSG_CHECKING([for OSS])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/soundcard.h>]], [[
			int dummy = SNDCTL_DSP_SETFMT;
		]])],[ac_lib_oss=yes],[ac_lib_oss=no])
	AC_MSG_RESULT([$ac_lib_oss])
elif test $ac_lib_oss = yes; then
	AC_MSG_CHECKING([for OSS])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/soundcard.h>]], [[
			int dummy = SNDCTL_DSP_SETFMT;
		]])],[],[AC_MSG_ERROR([the OSS library is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_oss = yes; then
	ac_lib_sound="$ac_lib_sound oss"
	ac_lib_sound_flag=yes
fi
AC_SUBST([CONF_LIB_OSS],[$ac_lib_oss])

AC_ARG_ENABLE(
	[pthread],
	AC_HELP_STRING([--enable-pthread],[enable the use of the pthread library (default auto)]),
	[ac_lib_pthread=$enableval],
	[ac_lib_pthread=auto]
)
if test $ac_lib_pthread = auto; then
	AC_CHECK_LIB(
		[pthread],
		[pthread_create],
		[ac_lib_pthread=yes],
		[ac_lib_pthread=no],
		[]
	)
elif test $ac_lib_pthread = yes; then
	AC_CHECK_LIB(
		[pthread],
		[pthread_create],
		[],
		[AC_MSG_ERROR([the pthread library is missing])],
		[]
	)
fi
if test $ac_lib_pthread = yes; then
	ac_lib_thread="$ac_lib_thread pthread"
fi
AC_SUBST([CONF_LIB_PTHREAD],[$ac_lib_pthread])

AC_ARG_ENABLE(
	[slang],
	AC_HELP_STRING([--enable-slang],[enable the use of the sLang library (default auto)]),
	[ac_lib_slang=$enableval],
	[ac_lib_slang=auto]
)
if test $ac_lib_slang = auto; then
	AC_CHECK_LIB(
		[slang],
		[SLang_init_tty],
		[ac_lib_slang=yes],
		[ac_lib_slang=no],
		[]
	)
elif test $ac_lib_slang = yes; then
	AC_CHECK_LIB(
		[slang],
		[SLang_init_tty],
		[],
		[AC_MSG_ERROR([the sLang library is missing])],
		[]
	)
fi
if test $ac_lib_slang = yes; then
	ac_lib_video="$ac_lib_video slang"
fi
AC_SUBST([CONF_LIB_SLANG],[$ac_lib_slang])

ac_lib_sdl_args=
ac_lib_sdl_path="$PATH"
AC_ARG_WITH(
	[sdl-prefix],
	AC_HELP_STRING([--with-sdl-prefix=DIR],[Prefix where SDL is installed (optional)]),
	[ac_lib_sdl_prefix="$withval"],
	[ac_lib_sdl_prefix=""]
)
if test ! -z $ac_lib_sdl_prefix; then
	ac_lib_sdl_args="--prefix=${ac_lib_sdl_prefix}"
	ac_lib_sdl_path="${ac_lib_sdl_prefix}/bin"
fi
AC_ARG_ENABLE(
	[sdl],
	AC_HELP_STRING([--enable-sdl],[enable the use of the SDL library. (default auto)]),
	[ac_lib_sdl=$enableval],
	[ac_lib_sdl=auto]
)
if test $ac_lib_sdl = auto; then
	AC_PATH_PROGS(
		[ac_lib_sdl_config],
		[${ac_tool_prefix}sdl-config],
		[],
		[${ac_lib_sdl_path}]
	)
	if test -z $ac_lib_sdl_config; then
		ac_lib_sdl=no
	else
		SDLCFLAGS=`$ac_lib_sdl_config $ac_lib_sdl_args --cflags`
		if test $ac_enable_static = yes ; then
			SDLLIBS=`$ac_lib_sdl_config $ac_lib_sdl_args --static-libs`
		else
			SDLLIBS=`$ac_lib_sdl_config $ac_lib_sdl_args --libs`
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
	AC_PATH_PROGS(
		[ac_lib_sdl_config],
		[${ac_tool_prefix}sdl-config],
		[],
		[${ac_lib_sdl_path}]
	)
	if test -z $ac_lib_sdl_config ; then
		AC_MSG_ERROR([the ${ac_tool_prefix}sdl-config tool is missing])
	else
		SDLCFLAGS=`$ac_lib_sdl_config $ac_lib_sdl_args --cflags`
		if test $ac_enable_static = yes ; then
			SDLLIBS=`$ac_lib_sdl_config $ac_lib_sdl_args --static-libs`
		else
			SDLLIBS=`$ac_lib_sdl_config $ac_lib_sdl_args --libs`
		fi
		AC_CHECK_LIB(
			[SDL],
			[SDL_Init],
			[],
			[AC_MSG_ERROR([the SDL library is missing])],
			[$SDLLIBS]
		)
	fi
fi
if test $ac_lib_sdl = yes; then
	ac_lib_video="$ac_lib_video sdl"
	ac_lib_sound="$ac_lib_sound sdl"
	ac_lib_keyboard="$ac_lib_keyboard sdl"
	ac_lib_joystick="$ac_lib_joystick sdl"
	ac_lib_mouse="$ac_lib_mouse sdl"
	ac_lib_video_flag=yes
	ac_lib_sound_flag=yes
	ac_lib_keyboard_flag=yes
fi
AC_SUBST([SDLCFLAGS])
AC_SUBST([SDLLIBS])
AC_SUBST([CONF_LIB_SDL],[$ac_lib_sdl])

AC_ARG_ENABLE(
	[svgawin],
	AC_HELP_STRING([--enable-svgawin],[enable the use of the SVGAWIN library. (default auto)]),
	[ac_lib_svgawin=$enableval],
	[ac_lib_svgawin=auto]
)
if test $ac_lib_svgawin = auto; then
	if test $ac_host = windows; then
		if test $ac_lib_sdl = yes; then
			ac_lib_svgawin=yes
		else
			ac_lib_svgawin=no
		fi
	else
		ac_lib_svgawin=no
	fi
fi
if test $ac_lib_svgawin = yes; then
	if test ! $ac_host = windows; then
		AC_MSG_ERROR([the SVGAWIN library not supported in this host])
	fi
	if test $ac_lib_sdl = no; then
		AC_MSG_ERROR([the SVGAWIN library requires the SDL library])
	fi
fi
if test $ac_lib_svgawin = yes; then
	ac_lib_video="$ac_lib_video svgawin"
	ac_lib_video_flag=yes
fi
AC_SUBST([CONF_LIB_SVGAWIN],[$ac_lib_svgawin])

AC_ARG_ENABLE(
	[kraw],
	AC_HELP_STRING([--enable-kraw],[enable the use of the raw keyboard interface. (default auto)]),
	[ac_lib_kraw=$enableval],
	[ac_lib_kraw=auto]
)
if test $ac_lib_kraw = auto; then
	AC_MSG_CHECKING([for raw keyboard interface])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
			#include <sys/kd.h>
			#include <sys/vt.h>
			#include <sys/ioctl.h>
		]], [[
			int dummy0 = K_MEDIUMRAW;
			int dummy1 = VT_ACTIVATE;
		]])],[ac_lib_kraw=yes],[ac_lib_kraw=no])
	AC_MSG_RESULT([$ac_lib_kraw])
elif test $ac_lib_kraw = yes; then
	AC_MSG_CHECKING([for raw keyboard interface])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
			#include <sys/kd.h>
			#include <sys/vt.h>
			#include <sys/ioctl.h>
		]], [[
			int dummy0 = K_MEDIUMRAW;
			int dummy1 = VT_ACTIVATE;
		]])],[],[AC_MSG_ERROR([the KRAW keyboard interface is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_kraw = yes; then
	ac_lib_keyboard="$ac_lib_keyboard raw"
	ac_lib_keyboard_flag=yes
fi
AC_SUBST([CONF_LIB_KRAW],[$ac_lib_kraw])

AC_ARG_ENABLE(
	[jraw],
	AC_HELP_STRING([--enable-jraw],[enable the use of the raw joystick interface. (default auto)]),
	[ac_lib_jraw=$enableval],
	[ac_lib_jraw=auto]
)
if test $ac_lib_jraw = auto; then
	AC_MSG_CHECKING([for raw joystick interface])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
			#include <linux/joystick.h>
		]], [[
			#ifndef JS_VERSION
			choke me
			#endif
		]])],[ac_lib_jraw=yes],[ac_lib_jraw=no])
	AC_MSG_RESULT([$ac_lib_jraw])
elif test $ac_lib_jraw = yes; then
	AC_MSG_CHECKING([for raw joystick interface])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
			#include <linux/joystick.h>
		]], [[
			#ifndef JS_VERSION
			choke me
			#endif
		]])],[],[AC_MSG_ERROR([the JRAW joystick interface is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_jraw = yes; then
	ac_lib_joystick="$ac_lib_joystick raw"
fi
AC_SUBST([CONF_LIB_JRAW],[$ac_lib_jraw])

AC_ARG_ENABLE(
	[mraw],
	AC_HELP_STRING([--enable-mraw],[enable the use of the raw mouse interface. (default auto)]),
	[ac_lib_mraw=$enableval],
	[ac_lib_mraw=auto]
)
if test $ac_lib_mraw = auto; then
	AC_MSG_CHECKING([for raw mouse interface])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <termios.h>]], [[
			void dummy(void) {
				tcsetattr(0, TCSAFLUSH, 0);
			}
		]])],[ac_lib_mraw=yes],[ac_lib_mraw=no])
	AC_MSG_RESULT([$ac_lib_mraw])
elif test $ac_lib_mraw = yes; then
	AC_MSG_CHECKING([for raw mouse interface])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <termios.h>]], [[
			void dummy(void) {
				tcsetattr(0, TCSAFLUSH, 0);
			}
		]])],[],[AC_MSG_ERROR([the MRAW mouse interface is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_mraw = yes; then
	ac_lib_mouse="$ac_lib_mouse raw"
fi
AC_SUBST([CONF_LIB_MRAW],[$ac_lib_mraw])

AC_ARG_ENABLE(
	[mevent],
	AC_HELP_STRING([--enable-mevent],[enable the use of the event mouse interface. (default auto)]),
	[ac_lib_mevent=$enableval],
	[ac_lib_mevent=auto]
)
if test $ac_lib_mevent = auto; then
	AC_MSG_CHECKING([for event mouse interface])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <linux/input.h>]], [[
			int dummy = EVIOCGVERSION;
		]])],[ac_lib_mevent=yes],[ac_lib_mevent=no])
	AC_MSG_RESULT([$ac_lib_mevent])
elif test $ac_lib_mevent = yes; then
	AC_MSG_CHECKING([for event mouse interface])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <linux/input.h>]], [[
			int dummy = EVIOCGVERSION;
		]])],[],[AC_MSG_ERROR([the event mouse interface is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_mevent = yes; then
	ac_lib_mouse="$ac_lib_mouse event"
fi
AC_SUBST([CONF_LIB_MEVENT],[$ac_lib_mevent])

AC_ARG_ENABLE(
	[kevent],
	AC_HELP_STRING([--enable-kevent],[enable the use of the event keyboard interface. (default auto)]),
	[ac_lib_kevent=$enableval],
	[ac_lib_kevent=auto]
)
if test $ac_lib_kevent = auto; then
	AC_MSG_CHECKING([for event keyboard interface])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <linux/input.h>]], [[
			int dummy = EVIOCGVERSION;
		]])],[ac_lib_kevent=yes],[ac_lib_kevent=no])
	AC_MSG_RESULT([$ac_lib_kevent])
elif test $ac_lib_kevent = yes; then
	AC_MSG_CHECKING([for event keyboard interface])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <linux/input.h>]], [[
			int dummy = EVIOCGVERSION;
		]])],[],[AC_MSG_ERROR([the event keyboard interface is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_kevent = yes; then
	ac_lib_keyboard="$ac_lib_keyboard event"
	ac_lib_keyboard_flag=yes
fi
AC_SUBST([CONF_LIB_KEVENT],[$ac_lib_kevent])

AC_ARG_ENABLE(
	[jevent],
	AC_HELP_STRING([--enable-jevent],[enable the use of the event joystick interface. (default auto)]),
	[ac_lib_jevent=$enableval],
	[ac_lib_jevent=auto]
)
if test $ac_lib_jevent = auto; then
	AC_MSG_CHECKING([for event joystick interface])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <linux/input.h>]], [[
			int dummy = EVIOCGVERSION;
		]])],[ac_lib_jevent=yes],[ac_lib_jevent=no])
	AC_MSG_RESULT([$ac_lib_jevent])
elif test $ac_lib_jevent = yes; then
	AC_MSG_CHECKING([for event joystick interface])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <linux/input.h>]], [[
			int dummy = EVIOCGVERSION;
		]])],[],[AC_MSG_ERROR([the event joystick interface is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_jevent = yes; then
	ac_lib_joystick="$ac_lib_joystick event"
fi
AC_SUBST([CONF_LIB_JEVENT],[$ac_lib_jevent])

# Notify that ./configure was runned
AC_SUBST([CONF],[yes])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo ""
echo "== Host =="
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

echo ""
echo "== Drivers =="
echo "Video :" $ac_lib_video
echo "Sound :" $ac_lib_sound
echo "Keyboard :" $ac_lib_keyboard
echo "Joystick :" $ac_lib_joystick
echo "Mouse :" $ac_lib_mouse
echo "Thread :" $ac_lib_thread
if test $ac_lib_video_flag = no; then
	AC_MSG_ERROR([no video library found. If you have the SDL library installed somewhere try using the --with-sdl-prefix option.])
fi
if test $ac_lib_sound_flag = no; then
	AC_MSG_ERROR([no sound library found])
fi
if test $ac_lib_keyboard_flag = no; then
	AC_MSG_ERROR([no keyboard library found])
fi

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

