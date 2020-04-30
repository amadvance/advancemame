dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.65])
dnl Get version number from git
m4_define([git_revision], m4_esyscmd_s([./autover.sh]))
AC_INIT([advancemame], [git_revision], [], [], [http://www.advancemame.it])
AC_CONFIG_SRCDIR([advance/advance.mak])
AC_CONFIG_HEADERS([advance/lib/config.h:advance/lib/config.hin])
AC_CANONICAL_HOST

dnl Checks for system.
case "$host" in
	*-*-linux*)
		ac_system=unix
		ac_host=linux
		ac_host_cflags=
		ac_host_ldflags=
		AC_SUBST([ASMFLAGS],["-f elf"])
		AC_SUBST([CFLAGS_FOR_BUILD],["-O0 -DUSE_COMPILER_GNUC -DUSE_OBJ_ELF -DUSE_OS_UNIX"])
		AC_SUBST([EXE],[])
	;;
	*-*-darwin*)
		ac_system=unix
		ac_host=darwin
		ac_host_cflags=-no-cpp-precomp
		ac_host_ldflags=
		AC_SUBST([ASMFLAGS],[""])
		AC_SUBST([CFLAGS_FOR_BUILD],["-O0"])
		AC_SUBST([EXE],[])
	;;
	*-*-mingw32*)
		ac_system=windows
		ac_host=windows
		ac_host_cflags=
		ac_host_ldflags=
		AC_SUBST([ASMFLAGS],["-f coff"])
		AC_SUBST([CFLAGS_FOR_BUILD],["-O0 -DUSE_COMPILER_GNUC -DUSE_OBJ_COFF -DUSE_OS_WINDOWS"])
		AC_SUBST([EXE],[.exe])
	;;
	*-*-msdos*)
		ac_system=dos
		ac_host=dos
		ac_host_cflags=
		ac_host_ldflags=
		AC_SUBST([ASMFLAGS],["-f coff"])
		AC_SUBST([CFLAGS_FOR_BUILD],["-O0 -DUSE_COMPILER_GNUC -DUSE_OBJ_COFF -DUSE_OS_DOS"])
		AC_SUBST([EXE],[.exe])
	;;
	*)
		ac_system=unix
		ac_host=generic
		ac_host_cflags=
		ac_host_ldflags=
		AC_SUBST([ASMFLAGS],[""])
		AC_SUBST([CFLAGS_FOR_BUILD],["-O0"])
		AC_SUBST([EXE],[])
	;;
esac
AC_SUBST([CONF_SYSTEM],[$ac_system])
AC_SUBST([CONF_HOST],[$ac_host])

case "$build" in
	*-*-linux*)
		ac_build=linux
		AC_SUBST([EXE_FOR_BUILD],[])
	;;
	*-*-darwin*)
		ac_build=darwin
		AC_SUBST([EXE_FOR_BUILD],[])
	;;
	*-*-cygwin* | *-*-mingw32*)
		ac_build=windows
		AC_SUBST([EXE_FOR_BUILD],[.exe])
	;;
	*-*-msdos*)
		ac_build=dos
		AC_SUBST([EXE_FOR_BUILD],[.exe])
	;;
	*)
		ac_build=unix
		AC_SUBST([EXE_FOR_BUILD],[])
	;;
esac
AC_SUBST([CONF_BUILD],[$ac_build])

if test -z "${CFLAGS+set}" ; then
	ac_auto_cflags=yes
else
	ac_auto_cflags=no
fi
if test -z "${LDFLAGS+set}" ; then
	ac_auto_ldflags=yes
else
	ac_auto_ldflags=no
fi

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_GREP
PKG_PROG_PKG_CONFIG
AC_CHECK_TOOLS(AR, ar)
AC_CHECK_TOOLS(RC, windres)
AC_CHECK_PROGS(RM, rm)
AC_CHECK_PROGS(CP, cp)
AC_CHECK_PROGS(MKDIR, mkdir)
AC_CHECK_PROGS(ECHO, echo)
AC_CHECK_PROGS(TAR, tar)
AC_CHECK_PROGS(GZIP, gzip)
AC_CHECK_PROGS(ZIP, zip)

if test "x$cross_compiling" = "xyes"; then
	dnl if cross compiling autodetect the system compilers
	AC_CHECK_PROGS(CC_FOR_BUILD, gcc cc)
	AC_CHECK_PROGS(CXX_FOR_BUILD, g++ gxx c++ gpp aCC CC cxx cc++ cl FCC KCC RCC xlC_r xlC)
else
	#if not cross compiling use the already detected compilers
	CC_FOR_BUILD=$CC
	CXX_FOR_BUILD=$CXX
	AC_SUBST(CC_FOR_BUILD)
	AC_SUBST(CXX_FOR_BUILD)
fi

dnl Checks for optimizations
AC_ARG_ENABLE(
	[perf],
	AC_HELP_STRING([--enable-perf],[enable the test compilation with profiler. Only for developers. (default no)]),
	[ac_enable_perf=$enableval],
	[ac_enable_perf=no]
)
AC_SUBST([CONF_PERF],[$ac_enable_perf])

AC_ARG_ENABLE(
	[debug],
	AC_HELP_STRING([--enable-debug],[enable the test compilation without optimization and with debug info. Only for developers. (default no)]),
	[ac_enable_debug=$enableval],
	[ac_enable_debug=no]
)
AC_SUBST([CONF_DEBUG],[$ac_enable_debug])

AC_ARG_ENABLE(
	[bare],
	AC_HELP_STRING([--enable-bare],[enable compilation without drivers. (default no)]),
	[ac_enable_bare=$enableval],
	[ac_enable_bare=no]
)

AC_ARG_ENABLE(
	[tiny],
	AC_HELP_STRING([--enable-tiny],[enable compilation with MAME/MESS tiny configuration. (default no)]),
	[ac_enable_tiny=$enableval],
	[ac_enable_tiny=no]
)
AC_SUBST([CONF_TINY],[$ac_enable_tiny])

AC_ARG_ENABLE(
	[32],
	AC_HELP_STRING([--enable-32],[force compilation for x86 32 bit. (default no)]),
	[ac_enable_32=$enableval],
	[ac_enable_32=no]
)

if test $ac_auto_cflags = yes; then
	if test $ac_enable_perf = yes ; then
		CFLAGS="-O1 -pg -Wall -Wno-sign-compare -Wno-unused"
	elif test $ac_enable_debug = yes ; then
		CFLAGS="-O0 -g -Wall -Wno-sign-compare -Wno-unused"
	else
		CFLAGS=""
		dnl O3 generally gives better performance than O2 in both Intel and ARM
		AC_CHECK_CC_OPT([-O3], [CFLAGS="$CFLAGS -O3"], [])

		if test $ac_enable_32 = yes ; then
			dnl Force build for 32 bit x86
			AC_CHECK_CC_OPT([-m32 -march=i686], [CFLAGS="$CFLAGS -m32 -march=i686"], [])
			AC_CHECK_CC_OPT([-fomit-frame-pointer], [CFLAGS="$CFLAGS -fomit-frame-pointer"], [])
		elif $GREP a03111\\\|b03111\\\|c03111 /proc/cpuinfo >/dev/null 2>&1; then
			dnl Raspberry Pi 4
			AC_CHECK_CC_OPT([-march=armv8-a+crc], [CFLAGS="$CFLAGS -march=armv8-a+crc"], [])
			AC_CHECK_CC_OPT([-mtune=cortex-a72], [CFLAGS="$CFLAGS -mtune=cortex-a72"], [])
			AC_CHECK_CC_OPT([-fomit-frame-pointer], [CFLAGS="$CFLAGS -fomit-frame-pointer"], [])
			AC_CHECK_CC_OPT([-funsafe-math-optimizations], [CFLAGS="$CFLAGS -funsafe-math-optimizations"], [])
		elif $GREP a02082\\\|a22082 /proc/cpuinfo >/dev/null 2>&1; then
			dnl Raspberry Pi 3
			dnl GCC 4 has problems with -march=native in Raspbian,
			dnl resulting in a "double free or corruption" error
			dnl See: https://www.raspberrypi.org/forums/viewtopic.php?f=28&t=139091
			AC_CHECK_CC_OPT([-march=armv8-a+crc], [CFLAGS="$CFLAGS -march=armv8-a+crc"], [])
			AC_CHECK_CC_OPT([-mtune=cortex-a53], [CFLAGS="$CFLAGS -mtune=cortex-a53"], [])
			AC_CHECK_CC_OPT([-mfloat-abi=hard], [CFLAGS="$CFLAGS -mfloat-abi=hard"], [])
			AC_CHECK_CC_OPT([-mfpu=crypto-neon-fp-armv8], [CFLAGS="$CFLAGS -mfpu=crypto-neon-fp-armv8"], [])
			AC_CHECK_CC_OPT([-funsafe-math-optimizations], [CFLAGS="$CFLAGS -funsafe-math-optimizations"], [])
		elif $GREP a01040\\\|a01041\\\|a21041\\\|a22042 /proc/cpuinfo >/dev/null 2>&1; then
			dnl Raspberry Pi 2
			AC_CHECK_CC_OPT([-march=armv7-a], [CFLAGS="$CFLAGS -march=armv7-a"], [])
			AC_CHECK_CC_OPT([-mfpu=neon-vfpv4], [CFLAGS="$CFLAGS -mfpu=neon-vfpv4"], [])
		else
			dnl Generic
			AC_CHECK_CC_OPT([-march=native], [CFLAGS="$CFLAGS -march=native"], [])
			AC_CHECK_CC_OPT([-fomit-frame-pointer], [CFLAGS="$CFLAGS -fomit-frame-pointer"], [])
		fi

		dnl Code was written when compilers where not aggressively optimizing undefined behaviour about aliasing
		dnl WARNING! At present disabled to maximize performance
		dnl AC_CHECK_CC_OPT([-fno-strict-aliasing], [CFLAGS="$CFLAGS -fno-strict-aliasing"], [])
		dnl Code was written when compilers where not aggressively optimizing undefined behaviour about overflow in signed integers
		dnl WARNING! At present disabled to maximize performance
		dnl AC_CHECK_CC_OPT([-fno-strict-overflow], [CFLAGS="$CFLAGS -fno-strict-overflow"], [])
		dnl Code was written on Intel where char is signed
		AC_CHECK_CC_OPT([-fsigned-char], [CFLAGS="$CFLAGS -fsigned-char"], [])
		dnl We are not interested in stack protection
		AC_CHECK_CC_OPT([-fno-stack-protector], [CFLAGS="$CFLAGS -fno-stack-protector"], [])
		dnl Some useful warnings
		AC_CHECK_CC_OPT([-Wall], [CFLAGS="$CFLAGS -Wall"], [])
		AC_CHECK_CC_OPT([-Wno-sign-compare], [CFLAGS="$CFLAGS -Wno-sign-compare"], [])
		AC_CHECK_CC_OPT([-Wno-unused], [CFLAGS="$CFLAGS -Wno-unused"], [])
	fi
	if test ! -z $ac_host_cflags; then
		CFLAGS="$ac_host_cflags $CFLAGS"
	fi
fi
if test $ac_auto_ldflags = yes ; then
	if test $ac_enable_perf = yes ; then
		LDFLAGS="-pg"
	elif test $ac_enable_debug = yes ; then
		if test $ac_system = unix ; then
			LDFLAGS="-rdynamic"
		else
			LDFLAGS="-g"
		fi
	else
		LDFLAGS="-s"
	fi
	if test ! -z $ac_host_ldflags; then
		LDFLAGS="$ac_host_ldflags $LDFLAGS"
	fi
	if test $ac_enable_32 = yes ; then
		dnl Force build for 32 bit x86
		LDFLAGS="-m32 $LDFLAGS"
	fi
fi

AC_SUBST([CONF_CFLAGS_OPT],[$CFLAGS])
AC_SUBST([CONF_LDFLAGS],[$LDFLAGS])

dnl Checks for architecture
AC_C_BIGENDIAN(
	[ac_is_bigendian=yes],
	[ac_is_bigendian=no]
)
if test $ac_is_bigendian = yes; then
	CFLAGS_ARCH="$CFLAGS_ARCH -DUSE_MSB"
else
	CFLAGS_ARCH="$CFLAGS_ARCH -DUSE_LSB"
fi

AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(void*)

dnl Large file supports
AC_SYS_LARGEFILE

AC_ARG_ENABLE(
	[asm],
	AC_HELP_STRING([--enable-asm],[enable the x86 assembler optimizations (default auto)]),
	[ac_enable_asm=$enableval],
	[ac_enable_asm=auto]
)
if test $ac_enable_asm = auto; then
	AC_MSG_CHECKING([whether ${CC-cc} accepts x86 assembler])
	AC_TRY_COMPILE([
		], [
			#if !defined(__GNUC__) || !defined(__i386__)
			choke me
			#endif
		],[ac_enable_asm=yes],[ac_enable_asm=no])
	AC_MSG_RESULT([$ac_enable_asm])
	if test $ac_enable_asm = yes; then
		AC_CHECK_PROGS([ASM], [${ac_tool_prefix}nasm], [none])
		if test $ASM = none; then
			ac_enable_asm=no
		else
			CFLAGS_ARCH="$CFLAGS_ARCH -DUSE_ASM_INLINE"
		fi
	fi
elif test $ac_enable_asm = yes; then
	AC_CHECK_PROGS([ASM], [${ac_tool_prefix}nasm], [none])
	if test $ASM = none; then
		AC_MSG_ERROR([the NASM assembler is required for the "asm" compilation])
	else
		CFLAGS_ARCH="$CFLAGS_ARCH -DUSE_ASM_INLINE"
	fi
fi

AC_ARG_ENABLE(
	[asm-mips3],
	AC_HELP_STRING([--enable-asm-mips3],[enable the x86 assembler MIPS3 emulator (default no)]),
	[ac_enable_asm_mips3=$enableval],
	[ac_enable_asm_mips3=no]
)
if test $ac_enable_asm_mips3 = auto; then
	if test $ac_enable_asm = yes; then
		ac_enable_asm_mips3=yes
	else
		ac_enable_asm_mips3=no
	fi
fi
if test $ac_enable_asm_mips3 = yes; then
	if test ! $ac_enable_asm = yes; then
		AC_MSG_ERROR([the x86 assembler is required for the "asm-mips3" compilation])
	fi
	CFLAGS_ARCH="$CFLAGS_ARCH -DUSE_ASM_EMUMIPS3"
fi

AC_SUBST([CONF_CFLAGS_ARCH],[$CFLAGS_ARCH])

AC_ARG_ENABLE(
	[debugger],
	AC_HELP_STRING([--enable-debugger],[enable the emulator debugger (default no)]),
	[ac_enable_debugger=$enableval],
	[ac_enable_debugger=no]
)
AC_SUBST([CONF_DEBUGGER],[$ac_enable_debugger])

dnl Checks for header files.
dnl Checks for typedefs, structures, and compiler characteristics.
dnl Checks for library functions.

if test $ac_system = unix ; then
	AC_HEADER_STDC
	AC_HEADER_SYS_WAIT
	AC_HEADER_DIRENT
	AC_HEADER_TIME
	AC_HEADER_TIOCGWINSZ
	AC_CHECK_HEADERS([unistd.h sched.h netdb.h termios.h execinfo.h])
	AC_CHECK_HEADERS([sys/utsname.h sys/types.h sys/stat.h sys/socket.h sys/select.h sys/ioctl.h sys/time.h sys/mman.h sys/io.h sys/kd.h sys/vt.h])
	AC_CHECK_HEADERS([netinet/in.h ucontext.h])
	AC_C_CONST
	AC_C_RESTRICT
	AC_C_INLINE
	AC_FUNC_ALLOCA
	AC_FUNC_MMAP
	AC_FUNC_SELECT_ARGTYPES
	AC_FUNC_VPRINTF
	AC_CHECK_FUNCS([strcasecmp strerror utimes])
	AC_CHECK_FUNCS([uname sysconf backtrace backtrace_symbols])
	AC_CHECK_FUNCS([flockfile funlockfile fread_unlocked fwrite_unlocked fgetc_unlocked feof_unlocked fseeko ftello])
	AC_CHECK_FUNCS([fsync renameat openat fdopen])
	AC_CHECK_FUNCS([iopl mprotect sched_getscheduler sched_setscheduler sched_get_priority_max sched_yield])
	AC_MSG_CHECKING([for port in/out])
	AC_TRY_LINK([
			#include <sys/io.h>
		], [
			outb(0x80, inb(0x80));
		],[ac_func_inout=yes],[ac_func_inout=no])
	AC_MSG_RESULT([$ac_func_inout])
	if test $ac_func_inout = yes; then
		AC_DEFINE([HAVE_INOUT],[1],[Define to 1 if you have the `inb' and `outb' functions.])
	fi
fi

dnl Checks for libraries.

ac_lib_direct_flag=no
ac_lib_video_flag=no
ac_lib_text_flag=no
ac_lib_sound_flag=no
ac_lib_keyboard_flag=no

if test $ac_system = dos; then
	ac_lib_video="$ac_lib_video svgaline vbeline vgaline vbe"
	ac_lib_sound="$ac_lib_sound seal vsync allegro"
	ac_lib_keyboard="$ac_lib_keyboard allegro"
	ac_lib_joystick="$ac_lib_joystick allegro"
	ac_lib_mouse="$ac_lib_mouse allegro"
	ac_lib_direct_flag=yes
	ac_lib_video_flag=yes
	ac_lib_sound_flag=yes
	ac_lib_keyboard_flag=yes
	ac_lib_text_flag=yes
fi

if test $ac_system = windows; then
	ac_lib_text_flag=yes
fi

dnl Checks for math. In some platforms it's not needed to use -lm, like Haiku.
AC_CHECK_LIB(
	[m],
	[cos],
	[ac_lib_system="$ac_lib_system -lm"],
	[]
)

dnl Checks for network lib. In some platforms it's needed to use -lnetwork, like Haiku.
AC_CHECK_LIB(
	[socket],
	[socket],
	[ac_lib_system="$ac_lib_system -lsocket"],
	[]
)
AC_CHECK_LIB(
	[network],
	[socket],
	[ac_lib_system="$ac_lib_system -lnetwork"],
	[]
)

AC_SUBST([CONF_LIBS],[$ac_lib_system])

dnl Checks for zlib
AC_ARG_ENABLE(
	[zlib],
	AC_HELP_STRING([--enable-zlib],[enable the use of the zlib library. (default auto)]),
	[ac_lib_zlib=$enableval],
	[ac_lib_zlib=auto]
)
if test $ac_lib_zlib = auto; then
	AC_CHECK_LIB(
		[z],
		[adler32],
		[ac_lib_zlib=yes],
		[ac_lib_zlib=no]
	)
fi
if test $ac_lib_zlib = yes; then
	ac_lib_other="$ac_lib_other zlib"
else
	ac_lib_other="$ac_lib_other zlib(internal)"
fi
AC_SUBST([CONF_LIB_ZLIB],[$ac_lib_zlib])

dnl Checks for expat
AC_ARG_ENABLE(
	[expat],
	AC_HELP_STRING([--enable-expat],[enable the use of the expat library. (default auto)]),
	[ac_lib_expat=$enableval],
	[ac_lib_expat=auto]
)
if test $ac_lib_expat = auto; then
	AC_CHECK_LIB(
		[expat],
		[XML_ParserCreate],
		[ac_lib_expat=yes],
		[ac_lib_expat=no]
	)
fi
if test $ac_lib_expat = yes; then
	ac_lib_other="$ac_lib_other expat"
else
	ac_lib_other="$ac_lib_other expat(internal)"
fi
AC_SUBST([CONF_LIB_EXPAT],[$ac_lib_expat])

dnl Checks for svgalib
AC_ARG_ENABLE(
	[svgalib],
	AC_HELP_STRING([--enable-svgalib],[enable the use of the SVGALIB library. (default no)]),
	[ac_lib_svgalib=$enableval],
	[ac_lib_svgalib=no]
)
if test $ac_lib_svgalib = auto; then
	AC_CHECK_LIB(
		[vga],
		[vga_init],
		[ac_lib_svgalib=yes],
		[ac_lib_svgalib=no]
	)
	if test $ac_lib_svgalib = yes; then
		AC_MSG_CHECKING([for svgalib version])
		AC_TRY_COMPILE([
				#include <vga.h>
			], [
				#if !defined(SVGALIB_VER) || (SVGALIB_VER<0x010900)
				choke me
				#endif
			],[ac_lib_svgalib=yes],[ac_lib_svgalib=no])
		AC_MSG_RESULT([$ac_lib_svgalib])
	fi
elif test $ac_lib_svgalib = yes; then
	AC_CHECK_LIB(
		[vga],
		[vga_init],
		[],
		[AC_MSG_ERROR([the SVGALIB library is missing])]
	)
	AC_MSG_CHECKING([for svgalib version])
	AC_TRY_COMPILE([
			#include <vga.h>
		], [
			#if !defined(SVGALIB_VER) || (SVGALIB_VER<0x010900)
			choke me
			#endif
		],[],[AC_MSG_ERROR([the SVGALIB library is too old. Please upgrade to the 1.9 or 2.0 version])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_svgalib = yes; then
	ac_lib_video="$ac_lib_video svgalib"
	ac_lib_keyboard="$ac_lib_keyboard svgalib"
	ac_lib_mouse="$ac_lib_mouse svgalib"
	ac_lib_joystick="$ac_lib_joystick svgalib"
	ac_lib_direct_flag=yes
	ac_lib_video_flag=yes
	ac_lib_keyboard_flag=yes
fi
AC_SUBST([CONF_LIB_SVGALIB],[$ac_lib_svgalib])

dnl Checks for fb
AC_ARG_ENABLE(
	[fb],
	AC_HELP_STRING([--enable-fb],[enable the use of the Frame Buffer library. (default auto)]),
	[ac_lib_fb=$enableval],
	[ac_lib_fb=auto]
)
if test $ac_lib_fb = auto; then
	AC_MSG_CHECKING([for FrameBuffer])
	AC_TRY_COMPILE([
			#include <linux/fb.h>
		], [
			struct fb_var_screeninfo dummy;
		],[ac_lib_fb=yes],[ac_lib_fb=no])
	AC_MSG_RESULT([$ac_lib_fb])
elif test $ac_lib_fb = yes; then
	AC_MSG_CHECKING([for FrameBuffer])
	AC_TRY_COMPILE([
			#include <linux/fb.h>
		], [
			struct fb_var_screeninfo dummy;
		],[],[AC_MSG_ERROR([the Frame Buffer library is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_fb = yes; then
	ac_lib_video="$ac_lib_video fb"
	ac_lib_direct_flag=yes
	ac_lib_video_flag=yes
fi
AC_SUBST([CONF_LIB_FB],[$ac_lib_fb])

dnl Checks for Raspberry Pi VideoCore
AC_ARG_WITH(
	[vc-prefix],
	AC_HELP_STRING([--with-vc-prefix=DIR],[Prefix where the Raspberry Pi VideoCore is installed (optional)]),
	[ac_lib_vc_prefix="$withval"],
	[ac_lib_vc_prefix="/opt/vc"]
)
AC_ARG_ENABLE(
	[vc],
	AC_HELP_STRING([--enable-vc],[enable the use of the Raspberry Pi VideoCore library. (default auto)]),
	[ac_lib_vc=$enableval],
	[ac_lib_vc=auto]
)
if test $ac_lib_vc = auto; then
	if test ! -d $ac_lib_vc_prefix; then
		ac_lib_vc=no
	else
		ac_save_CFLAGS="$CFLAGS"
		ac_save_LIBS="$LIBS"
		VCCFLAGS="-I$ac_lib_vc_prefix/include"
		VCLIBS="-L$ac_lib_vc_prefix/lib -lbcm_host -lvcos -lvchiq_arm"
		CFLAGS="$VCCFLAGS $CFLAGS"
		LIBS="$VCLIBS $LIBS"
		AC_MSG_CHECKING([for VideoCore])
		AC_TRY_LINK([
				#include "interface/vcos/vcos.h"
			], [
				vcos_init();
			],[ac_lib_vc=yes],[ac_lib_vc=no])
		AC_MSG_RESULT([$ac_lib_vc])
		CFLAGS="$ac_save_CFLAGS"
		LIBS="$ac_save_LIBS"
	fi
elif test $ac_lib_vc = yes; then
	if test ! -d $ac_lib_vc_prefix; then
		AC_MSG_ERROR([the ${$ac_lib_vc_prefix} dir is missing])
	else
		ac_save_CFLAGS="$CFLAGS"
		ac_save_LIBS="$LIBS"
		VCCFLAGS="-I$ac_lib_vc_prefix/include"
		VCLIBS="-L$ac_lib_vc_prefix/lib -lbcm_host -lvcos -lvchiq_arm"
		CFLAGS="$VCCFLAGS $CFLAGS"
		LIBS="$VCLIBS $LIBS"
		AC_MSG_CHECKING([for VideoCore])
		AC_TRY_LINK([
				#include "interface/vcos/vcos.h"
			], [
				vcos_init();
			],[],[AC_MSG_ERROR([the Raspberry Pi VideoCore library is missing])])
		AC_MSG_RESULT([yes])
		CFLAGS="$ac_save_CFLAGS"
		LIBS="$ac_save_LIBS"
	fi
fi
AC_SUBST([VCCFLAGS])
AC_SUBST([VCLIBS])
AC_SUBST([CONF_LIB_VC],[$ac_lib_vc])
if test $ac_lib_vc = yes; then
	ac_lib_other="$ac_lib_other VideoCore"
fi

dnl Checks for alsa
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
		AC_MSG_CHECKING([for ALSA])
		AC_TRY_COMPILE([
				#include <alsa/asoundlib.h>
			], [
				#if SND_LIB_VERSION < ((0<<16)|(9<<8)|0)
				choke me
				#endif
			],[ac_lib_alsa=yes],[ac_lib_alsa=no])
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
	AC_TRY_COMPILE([
			#include <alsa/asoundlib.h>
		], [
			#if SND_LIB_VERSION < ((0<<16)|(9<<8)|0)
			choke me
			#endif
		],[],[AC_MSG_ERROR([the ALSA library is too old])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_alsa = yes; then
	ac_lib_sound="$ac_lib_sound alsa"
	ac_lib_sound_flag=yes
fi
AC_SUBST([CONF_LIB_ALSA],[$ac_lib_alsa])

dnl Checks for oss
AC_ARG_ENABLE(
	[oss],
	AC_HELP_STRING([--enable-oss],[enable the use of the OSS library. (default auto)]),
	[ac_lib_oss=$enableval],
	[ac_lib_oss=auto]
)
if test $ac_lib_oss = auto; then
	AC_MSG_CHECKING([for OSS])
	AC_TRY_COMPILE([
			#include <sys/soundcard.h>
		], [
			int dummy = SNDCTL_DSP_SETFMT;
		],[ac_lib_oss=yes],[ac_lib_oss=no])
	AC_MSG_RESULT([$ac_lib_oss])
elif test $ac_lib_oss = yes; then
	AC_MSG_CHECKING([for OSS])
	AC_TRY_COMPILE([
			#include <sys/soundcard.h>
		], [
			int dummy = SNDCTL_DSP_SETFMT;
		],[],[AC_MSG_ERROR([the OSS library is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_oss = yes; then
	ac_lib_sound="$ac_lib_sound oss"
	ac_lib_sound_flag=yes
fi
AC_SUBST([CONF_LIB_OSS],[$ac_lib_oss])

dnl Checks for pthread
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
	ac_lib_other="$ac_lib_other pthread"
fi
AC_SUBST([CONF_LIB_PTHREAD],[$ac_lib_pthread])

dnl Checks for slang
AC_ARG_ENABLE(
	[slang],
	AC_HELP_STRING([--enable-slang],[enable the use of the sLang library (default auto)]),
	[ac_lib_slang=$enableval],
	[ac_lib_slang=auto]
)
if test $ac_lib_slang = auto; then
	PKG_CHECK_MODULES([SLANG], [slang], [ac_lib_slang=yes], [ac_lib_slang=no])
	if test $ac_lib_slang = yes; then
		ac_save_CFLAGS="$CFLAGS"
		ac_save_LIBS="$LIBS"
		CFLAGS="SLANG_CFLAGS $CFLAGS"
		LIBS="$SLANG_LIBS $LIBS"
                AC_MSG_CHECKING([for linking slang])
                AC_TRY_LINK([
				#include <slang.h>
			], [
				if (SLang_init_tty(-1, 0, 0) < 0) {
					perror("SLang_init_tty");
					return 1;
				}

				SLang_reset_tty();
				return 0;
			],[ac_lib_slang=yes],[ac_lib_slang=no])
		AC_MSG_RESULT([$ac_lib_slang])
		CFLAGS="$ac_save_CFLAGS"
		LIBS="$ac_save_LIBS"
        fi
elif test $ac_lib_slang = yes; then
	PKG_CHECK_MODULES([SLANG], [slang], [], AC_MSG_ERROR([the slang library is missing]))
fi
if test $ac_lib_slang = yes; then
	AC_CHECK_HEADERS([slang.h slang/slang.h], [break])
	ac_lib_video="$ac_lib_video slang"
	ac_lib_text_flag=yes
fi
AC_SUBST([SLANGCFLAGS],[$SLANG_CFLAGS])
AC_SUBST([SLANGLIBS],[$SLANG_LIBS])
AC_SUBST([CONF_LIB_SLANG],[$ac_lib_slang])


dnl Checks for ncurses
AC_ARG_ENABLE(
	[ncurses],
	AC_HELP_STRING([--enable-ncurses],[enable the use of the ncurses library (default auto)]),
	[ac_lib_ncurses=$enableval],
	[ac_lib_ncurses=auto]
)
if test $ac_lib_ncurses = auto; then
	PKG_CHECK_MODULES([NCURSES], [ncurses], [ac_lib_ncurses=yes], [ac_lib_ncurses=no])
	if test $ac_lib_ncurses = yes; then
		ac_save_CFLAGS="$CFLAGS"
		ac_save_LIBS="$LIBS"
		CFLAGS="$NCURSES_CFLAGS $CFLAGS"
		LIBS="$NCURSES_LIBS $LIBS"
		AC_MSG_CHECKING([for linking ncurses])
		AC_TRY_LINK([
				#include <ncurses.h>
			], [
				initscr();
				noecho();
				curs_set(FALSE);
				endwin();
			],[ac_lib_ncurses=yes],[ac_lib_ncurses=no])
                AC_MSG_RESULT([$ac_lib_ncurses])
                CFLAGS="$ac_save_CFLAGS"
                LIBS="$ac_save_LIBS"
        fi
elif test $ac_lib_ncurses = yes; then
	PKG_CHECK_MODULES([NCURSES], [ncurses], [], AC_MSG_ERROR([the ncurses library is missing]))
fi
if test $ac_lib_ncurses = yes; then
	ac_lib_video="$ac_lib_video ncurses"
	ac_lib_text_flag=yes
fi
AC_SUBST([NCURSESCFLAGS],[$NCURSES_CFLAGS])
AC_SUBST([NCURSESLIBS],[$NCURSES_LIBS])
AC_SUBST([CONF_LIB_NCURSES],[$ac_lib_ncurses])


dnl Checks for freetype
AC_ARG_ENABLE(
	[freetype],
	AC_HELP_STRING([--enable-freetype],[enable the use of the freetype2 library. (default auto)]),
	[ac_lib_freetype=$enableval],
	[ac_lib_freetype=auto]
)
if test $ac_lib_freetype = auto; then
	PKG_CHECK_MODULES([FREETYPE], [freetype2], [ac_lib_freetype=yes], [ac_lib_freetype=no])
	if test $ac_lib_freetype = yes; then
		ac_save_CFLAGS="$CFLAGS"
		ac_save_LIBS="$LIBS"
		CFLAGS="$FREETYPE_CFLAGS $CFLAGS"
		LIBS="$FREETYPE_LIBS $LIBS"
		AC_MSG_CHECKING([for linking freetype2])
		AC_TRY_LINK([
				#include <ft2build.h>
				#include FT_FREETYPE_H
				#include FT_GLYPH_H
			], [
				FT_Library library;
				int dummy = FT_RENDER_MODE_NORMAL;
				FT_Init_FreeType(&library);
				FT_Done_FreeType(library);
			],[ac_lib_freetype=yes],[ac_lib_freetype=no])
		AC_MSG_RESULT([$ac_lib_freetype])
		CFLAGS="$ac_save_CFLAGS"
		LIBS="$ac_save_LIBS"
	fi
elif test $ac_lib_freetype = yes; then
	PKG_CHECK_MODULES([FREETYPE], [freetype2], [], AC_MSG_ERROR([the freetype2 library is missing]))
fi
if test $ac_lib_freetype = yes; then
	ac_lib_other="$ac_lib_other freetype2"
fi
AC_SUBST([FREETYPECFLAGS],[$FREETYPE_CFLAGS])
AC_SUBST([FREETYPELIBS],[$FREETYPE_LIBS])
AC_SUBST([CONF_LIB_FREETYPE],[$ac_lib_freetype])

dnl Checks for SDL v1 and v2
AC_ARG_ENABLE(
	[sdl2],
	AC_HELP_STRING([--enable-sdl2],[enable the use of the SDL2 library. (default auto)]),
	[ac_lib_sdl2=$enableval],
	[ac_lib_sdl2=auto]
)
if test $ac_lib_sdl2 = auto; then
	if test $ac_enable_32 = yes ; then
		ac_save_CFLAGS="$CFLAGS"
		ac_save_LIBS="$LIBS"
		SDL_CFLAGS="-I/usr/include/SDL2 -I/usr/include -D_REENTRANT"
		SDL_LIBS="-pthread -lSDL2"
		CFLAGS="$SDL_CFLAGS $CFLAGS"
		LIBS="$SDL_LIBS $LIBS"
		AC_MSG_CHECKING([for SDL2])
		AC_TRY_LINK([
				#include "SDL.h"
				#undef main
			], [
				SDL_Init(SDL_INIT_EVERYTHING);
			],[ac_lib_sdl2=yes],[ac_lib_sdl2=no])
		AC_MSG_RESULT([$ac_lib_sdl2])
		CFLAGS="$ac_save_CFLAGS"
		LIBS="$ac_save_LIBS"
	else
		PKG_CHECK_MODULES([SDL], [sdl2], [ac_lib_sdl2=yes], [ac_lib_sdl2=no])
		if test $ac_lib_sdl2 = yes; then
			ac_save_CFLAGS="$CFLAGS"
			ac_save_LIBS="$LIBS"
			CFLAGS="$SDL_CFLAGS $CFLAGS"
			LIBS="$SDL_LIBS $LIBS"
			AC_MSG_CHECKING([for linking sdl2])
			AC_TRY_LINK([
					#include "SDL.h"
					#undef main
				], [
					SDL_Init(SDL_INIT_EVERYTHING);
				],[ac_lib_sdl2=yes],[ac_lib_sdl2=no])
			AC_MSG_RESULT([$ac_lib_sdl2])
			CFLAGS="$ac_save_CFLAGS"
			LIBS="$ac_save_LIBS"
		fi
	fi
elif test $ac_lib_sdl2 = yes; then
	PKG_CHECK_MODULES([SDL], [sdl2], [ac_lib_sdl2=yes], AC_MSG_ERROR([the SDL2 library is missing]))
fi
AC_ARG_ENABLE(
	[sdl],
	AC_HELP_STRING([--enable-sdl],[enable the use of the SDL library. (default auto)]),
	[ac_lib_sdl=$enableval],
	[ac_lib_sdl=auto]
)
if test $ac_lib_sdl = auto; then
	if test $ac_lib_sdl2 = yes; then
		ac_lib_sdl=no
	else
		if test $ac_enable_32 = yes ; then
			dnl Force build for 32 bit x86
			ac_save_CFLAGS="$CFLAGS"
			ac_save_LIBS="$LIBS"
			SDL_CFLAGS="-I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT"
			SDL_LIBS="-lSDL -lpthread"
			CFLAGS="$SDL_CFLAGS $CFLAGS"
			LIBS="$SDL_LIBS $LIBS"
			AC_MSG_CHECKING([for SDL])
					AC_TRY_LINK([
					#include "SDL.h"
					#undef main
				], [
					SDL_Init(SDL_INIT_EVERYTHING);
				],[ac_lib_sdl=yes],[ac_lib_sdl=no])
			AC_MSG_RESULT([$ac_lib_sdl])
			CFLAGS="$ac_save_CFLAGS"
			LIBS="$ac_save_LIBS"
		else
			PKG_CHECK_MODULES([SDL], [sdl], [ac_lib_sdl=yes], [ac_lib_sdl=no])
			if test $ac_lib_sdl = yes; then
				ac_save_CFLAGS="$CFLAGS"
				ac_save_LIBS="$LIBS"
				CFLAGS="$SDL_CFLAGS $CFLAGS"
				LIBS="$SDL_LIBS $LIBS"
				AC_MSG_CHECKING([for SDL])
					AC_TRY_LINK([
						#include "SDL.h"
						#undef main
					], [
						SDL_Init(SDL_INIT_EVERYTHING);
					],[ac_lib_sdl=yes],[ac_lib_sdl=no])
				AC_MSG_RESULT([$ac_lib_sdl])
				CFLAGS="$ac_save_CFLAGS"
				LIBS="$ac_save_LIBS"
			fi
		fi
	fi
elif test $ac_lib_sdl = yes; then
	if test $ac_lib_sdl2 = yes; then
		AC_MSG_ERROR([You cannot enable both SDL and SDL2])
	fi
	PKG_CHECK_MODULES([SDL], [sdl], [ac_lib_sdl=yes], AC_MSG_ERROR([the SDL library is missing]))
fi
ac_lib_sdl_flag=no
if test $ac_lib_sdl = yes -o $ac_lib_sdl2 = yes; then
	ac_lib_video="$ac_lib_video sdl"
	ac_lib_sound="$ac_lib_sound sdl"
	ac_lib_keyboard="$ac_lib_keyboard sdl"
	ac_lib_joystick="$ac_lib_joystick sdl"
	ac_lib_mouse="$ac_lib_mouse sdl"
	ac_lib_sdl_flag=yes
	ac_lib_video_flag=yes
	ac_lib_sound_flag=yes
	ac_lib_keyboard_flag=yes
fi
AC_SUBST([CONF_LIB_SDL],[$ac_lib_sdl_flag])
AC_SUBST([SDLCFLAGS],[$SDL_CFLAGS])
AC_SUBST([SDLLIBS],[$SDL_LIBS])

dnl Checks for svgawin
AC_ARG_ENABLE(
	[svgawin],
	AC_HELP_STRING([--enable-svgawin],[enable the use of the SVGAWIN library. (default no)]),
	[ac_lib_svgawin=$enableval],
	[ac_lib_svgawin=no]
)
if test $ac_lib_svgawin = auto; then
	if test $ac_system = windows; then
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
	if test ! $ac_system = windows; then
		AC_MSG_ERROR([the SVGAWIN library is not supported in this host])
	fi
	if test $ac_lib_sdl = no; then
		AC_MSG_ERROR([the SVGAWIN library requires the SDL library])
	fi
fi
if test $ac_lib_svgawin = yes; then
	ac_lib_video="$ac_lib_video svgawin"
	ac_lib_direct_flag=yes
	ac_lib_video_flag=yes
fi
AC_SUBST([CONF_LIB_SVGAWIN],[$ac_lib_svgawin])

dnl Checks for mrawinput
AC_ARG_ENABLE(
	[mrawinput],
	AC_HELP_STRING([--enable-mrawinput],[enable the use of the Windows Raw input mouse interface. (default auto)]),
	[ac_lib_mrawinput=$enableval],
	[ac_lib_mrawinput=auto]
)
if test $ac_lib_mrawinput = auto; then
	if test $ac_system = windows; then
		ac_lib_mrawinput=yes
	else
		ac_lib_mrawinput=no
	fi
fi
if test $ac_lib_mrawinput = yes; then
	if test ! $ac_system = windows; then
		AC_MSG_ERROR([the rawinput library is not supported in this host])
	fi
fi
if test $ac_lib_mrawinput = yes; then
	ac_lib_mouse="$ac_lib_mouse rawinput"
fi
AC_SUBST([CONF_LIB_MRAWINPUT],[$ac_lib_mrawinput])

dnl Checks for mcpn
AC_ARG_ENABLE(
	[mcpn],
	AC_HELP_STRING([--enable-mcpn],[enable the use of the Windows CPN mouse interface. (default auto)]),
	[ac_lib_mcpn=$enableval],
	[ac_lib_mcpn=auto]
)
if test $ac_lib_mcpn = auto; then
	if test $ac_system = windows; then
		ac_lib_mcpn=yes
	else
		ac_lib_mcpn=no
	fi
fi
if test $ac_lib_mcpn = yes; then
	if test ! $ac_system = windows; then
		AC_MSG_ERROR([the cpn library is not supported in this host])
	fi
fi
if test $ac_lib_mcpn = yes; then
	ac_lib_mouse="$ac_lib_mouse cpn"
fi
AC_SUBST([CONF_LIB_MCPN],[$ac_lib_mcpn])

dnl Checks for kraw
AC_ARG_ENABLE(
	[kraw],
	AC_HELP_STRING([--enable-kraw],[enable the use of the raw keyboard interface. (default auto)]),
	[ac_lib_kraw=$enableval],
	[ac_lib_kraw=auto]
)
if test $ac_lib_kraw = auto; then
	AC_MSG_CHECKING([for raw keyboard interface])
	AC_TRY_COMPILE([
			#include <sys/kd.h>
			#include <sys/vt.h>
			#include <sys/ioctl.h>
			#include <termios.h>
		], [
			int dummy0 = K_MEDIUMRAW;
			int dummy1 = VT_ACTIVATE;
			struct termios dummy2;
		],[ac_lib_kraw=yes],[ac_lib_kraw=no])
	AC_MSG_RESULT([$ac_lib_kraw])
elif test $ac_lib_kraw = yes; then
	AC_MSG_CHECKING([for raw keyboard interface])
	AC_TRY_COMPILE([
			#include <sys/kd.h>
			#include <sys/vt.h>
			#include <sys/ioctl.h>
			#include <termios.h>
		], [
			int dummy0 = K_MEDIUMRAW;
			int dummy1 = VT_ACTIVATE;
			struct termios dummy2;
		],[],[AC_MSG_ERROR([the KRAW keyboard interface is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_kraw = yes; then
	ac_lib_keyboard="$ac_lib_keyboard raw"
	ac_lib_keyboard_flag=yes
fi
AC_SUBST([CONF_LIB_KRAW],[$ac_lib_kraw])

dnl Checks for jraw
AC_ARG_ENABLE(
	[jraw],
	AC_HELP_STRING([--enable-jraw],[enable the use of the raw joystick interface. (default auto)]),
	[ac_lib_jraw=$enableval],
	[ac_lib_jraw=auto]
)
if test $ac_lib_jraw = auto; then
	AC_MSG_CHECKING([for raw joystick interface])
	AC_TRY_COMPILE([
			#include <linux/joystick.h>
		], [
			#ifndef JS_VERSION
			choke me
			#endif
		],[ac_lib_jraw=yes],[ac_lib_jraw=no])
	AC_MSG_RESULT([$ac_lib_jraw])
elif test $ac_lib_jraw = yes; then
	AC_MSG_CHECKING([for raw joystick interface])
	AC_TRY_COMPILE([
			#include <linux/joystick.h>
		], [
			#ifndef JS_VERSION
			choke me
			#endif
		],[],[AC_MSG_ERROR([the JRAW joystick interface is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_jraw = yes; then
	ac_lib_joystick="$ac_lib_joystick raw"
fi
AC_SUBST([CONF_LIB_JRAW],[$ac_lib_jraw])

dnl Checks for mraw
AC_ARG_ENABLE(
	[mraw],
	AC_HELP_STRING([--enable-mraw],[enable the use of the raw mouse interface. (default auto)]),
	[ac_lib_mraw=$enableval],
	[ac_lib_mraw=auto]
)
if test $ac_lib_mraw = auto; then
	AC_MSG_CHECKING([for raw mouse interface])
	AC_TRY_COMPILE([
			#include <termios.h>
		], [
			tcsetattr(0, TCSAFLUSH, 0);
		],[ac_lib_mraw=yes],[ac_lib_mraw=no])
	AC_MSG_RESULT([$ac_lib_mraw])
elif test $ac_lib_mraw = yes; then
	AC_MSG_CHECKING([for raw mouse interface])
	AC_TRY_COMPILE([
			#include <termios.h>
		], [
			tcsetattr(0, TCSAFLUSH, 0);
		],[],[AC_MSG_ERROR([the MRAW mouse interface is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_mraw = yes; then
	ac_lib_mouse="$ac_lib_mouse raw"
fi
AC_SUBST([CONF_LIB_MRAW],[$ac_lib_mraw])

dnl Checks for mevent
AC_ARG_ENABLE(
	[mevent],
	AC_HELP_STRING([--enable-mevent],[enable the use of the event mouse interface. (default auto)]),
	[ac_lib_mevent=$enableval],
	[ac_lib_mevent=auto]
)
if test $ac_lib_mevent = auto; then
	AC_MSG_CHECKING([for event mouse interface])
	AC_TRY_COMPILE([
			#include <linux/input.h>
		], [
			int dummy = EVIOCGVERSION;
		],[ac_lib_mevent=yes],[ac_lib_mevent=no])
	AC_MSG_RESULT([$ac_lib_mevent])
elif test $ac_lib_mevent = yes; then
	AC_MSG_CHECKING([for event mouse interface])
	AC_TRY_COMPILE([
			#include <linux/input.h>
		], [
			int dummy = EVIOCGVERSION;
		],[],[AC_MSG_ERROR([the event mouse interface is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_mevent = yes; then
	ac_lib_mouse="$ac_lib_mouse event"
fi
AC_SUBST([CONF_LIB_MEVENT],[$ac_lib_mevent])

dnl Checks for kevent
AC_ARG_ENABLE(
	[kevent],
	AC_HELP_STRING([--enable-kevent],[enable the use of the event keyboard interface. (default auto)]),
	[ac_lib_kevent=$enableval],
	[ac_lib_kevent=auto]
)
if test $ac_lib_kevent = auto; then
	AC_MSG_CHECKING([for event keyboard interface])
	AC_TRY_COMPILE([
			#include <linux/input.h>
		], [
			int dummy = EVIOCGVERSION;
		],[ac_lib_kevent=yes],[ac_lib_kevent=no])
	AC_MSG_RESULT([$ac_lib_kevent])
elif test $ac_lib_kevent = yes; then
	AC_MSG_CHECKING([for event keyboard interface])
	AC_TRY_COMPILE([
			#include <linux/input.h>
		], [
			int dummy = EVIOCGVERSION;
		],[],[AC_MSG_ERROR([the event keyboard interface is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_kevent = yes; then
	ac_lib_keyboard="$ac_lib_keyboard event"
	ac_lib_keyboard_flag=yes
fi
AC_SUBST([CONF_LIB_KEVENT],[$ac_lib_kevent])

dnl Checks for jevent
AC_ARG_ENABLE(
	[jevent],
	AC_HELP_STRING([--enable-jevent],[enable the use of the event joystick interface. (default auto)]),
	[ac_lib_jevent=$enableval],
	[ac_lib_jevent=auto]
)
if test $ac_lib_jevent = auto; then
	AC_MSG_CHECKING([for event joystick interface])
	AC_TRY_COMPILE([
			#include <linux/input.h>
		], [
			int dummy = EVIOCGVERSION;
		],[ac_lib_jevent=yes],[ac_lib_jevent=no])
	AC_MSG_RESULT([$ac_lib_jevent])
elif test $ac_lib_jevent = yes; then
	AC_MSG_CHECKING([for event joystick interface])
	AC_TRY_COMPILE([
			#include <linux/input.h>
		], [
			int dummy = EVIOCGVERSION;
		],[],[AC_MSG_ERROR([the event joystick interface is missing])])
	AC_MSG_RESULT([yes])
fi
if test $ac_lib_jevent = yes; then
	ac_lib_joystick="$ac_lib_joystick event"
fi
AC_SUBST([CONF_LIB_JEVENT],[$ac_lib_jevent])

dnl Final
AC_SUBST([CONF_LIB_DIRECT],[$ac_lib_direct_flag])
AC_SUBST([mandir],['${prefix}/man'])
AC_SUBST([docdir],['${prefix}/doc'])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

dnl Print user information
echo ""
echo "== Host =="
echo "Environment :" $host
if test $ac_is_bigendian = yes; then
	echo "Endianess : big"
else
	echo "Endianess : little"
fi
echo "Bits :" $((8 * $ac_cv_sizeof_voidp))
if test $ac_enable_asm = yes; then
	echo "Assembler for x86 : yes"
else
	echo "Assembler for x86 : no"
fi
if test $ac_enable_asm_mips3 = yes; then
	echo "Assembler MIPS3 emulator for x86 : yes"
else
	echo "Assembler MIPS3 emulator for x86 : no"
fi

echo ""
echo "== Drivers/Libraries =="
if test $ac_lib_direct_flag = no ; then
	echo "Video :" $ac_lib_video
	echo "No video driver is able to program your video board."
	echo "You will be able to use only standard video modes."
else
	echo "Video :" $ac_lib_video
fi
echo "Sound :" $ac_lib_sound
echo "Keyboard :" $ac_lib_keyboard
echo "Joystick :" $ac_lib_joystick
echo "Mouse :" $ac_lib_mouse
echo "Misc :" $ac_lib_other

if test $ac_enable_bare = no ; then
if test $ac_enable_debug = no ; then
if test $ac_enable_perf = no ; then
	if test $ac_lib_video_flag = no; then
		AC_MSG_ERROR([no video library found. If you have the SDL library installed somewhere try using the --with-sdl-prefix or --with-sdl2-prefix options.])
	else
		if test $ac_lib_sdl_flag = no; then
			AC_MSG_WARN([the SDL library was not found. You will not able to use this program in a window manager environment! If you have the SDL library installed somewhere try using the --with-sdl-prefix or --with-sdl2-prefix options.])
		fi
	fi
	if test $ac_lib_sound_flag = no; then
		AC_MSG_ERROR([no sound library found])
	fi
	if test $ac_lib_text_flag = no; then
		AC_MSG_WARN([no text video mode library found. The advv/advcfg utilities will not work!])
	fi
fi
fi
fi

echo ""
echo "== Compiler =="
echo "CC :" $CC
echo "CXX :" $CXX
echo "CFLAGS :" $CFLAGS $CFLAGS_OPTOPT
echo "LDFLAGS :" $LDFLAGS
if test $ac_lib_sdl = yes -o $ac_lib_sdl2 = yes; then
	echo "SDLCFLAGS :" $SDLCFLAGS
	echo "SDLLIBS :" $SDLLIBS
fi

if test ! $ac_with_emu = none ; then
	echo ""
	echo "== Configuration =="
	echo "Emulator :" $ac_with_emu
	echo "Debugger :" $ac_enable_debugger
fi

