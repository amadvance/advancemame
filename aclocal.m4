dnl @synopsis AC_CHECK_CC_OPT(flag, ifyes, ifno)
dnl 
dnl Shows a message as like "checking wether gcc accepts flag ... no"
dnl and executess ifyes or ifno.

AC_DEFUN(AC_CHECK_CC_OPT,
[
AC_MSG_CHECKING([whether ${CC-cc} accepts $1])
echo 'void f(){}' > conftest.c
if test -z "`${CC-cc} -c $1 conftest.c 2>&1`"; then
  AC_MSG_RESULT([yes])
  $2
else
  AC_MSG_RESULT([no])
  $3
fi
rm -f conftest*
])

dnl @synopsis AC_CHECK_CPU_ARCH
dnl 
dnl Sets ac_cpu_arch and ac_cpu_family. If unrecognized set
dnl ac_cpu_arch=blend and ac_cpu_family=0

AC_DEFUN(AC_CHECK_CPU_ARCH,
[
if grep -i "family.*:.*3" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="i386"
	ac_cpu_family=3
elif grep -i "family.*:.*4" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="i486"
	ac_cpu_family=4
elif grep -i "family.*:.*5" /proc/cpuinfo > /dev/null ; then
	if grep -i "model name.*:.*K6" /proc/cpuinfo > /dev/null ; then
		ac_cpu_arch="k6"
	elif grep -i "model name.*:.*Pentium MMX" /proc/cpuinfo > /dev/null ; then
		ac_cpu_arch="pentium-mmx"
	elif grep -i "model name.*:.*Pentium" /proc/cpuinfo > /dev/null ; then
		ac_cpu_arch="pentium"
	else
		ac_cpu_arch="i586"
	fi
	ac_cpu_family=5
elif grep "family.*:.*6" /proc/cpuinfo > /dev/null ; then
	if grep -i "model name.*:.*Athlon" /proc/cpuinfo > /dev/null ; then
		ac_cpu_arch="athlon"
	elif grep -i "model name.*:.*Pentium IV" /proc/cpuinfo > /dev/null ; then
		ac_cpu_arch="pentium4"
	elif grep -i "model name.*:.*Pentium III" /proc/cpuinfo > /dev/null ; then
		ac_cpu_arch="pentium3"
	elif grep -i "model name.*:.*Pentium II" /proc/cpuinfo > /dev/null ; then
		ac_cpu_arch="pentium2"
	elif grep -i "model name.*:.*Pentium Pro" /proc/cpuinfo > /dev/null ; then
		ac_cpu_arch="pentiumpro"
	else
		ac_cpu_arch="i686"
	fi
	ac_cpu_family=6
else
	ac_cpu_arch=blend
	ac_cpu_family=0
fi
AC_MSG_CHECKING([build cpu, family])
AC_MSG_RESULT([$ac_cpu_arch, $ac_cpu_family])
])

dnl @synopsis AC_CHECK_CC_ARCH
dnl 
dnl Sets ac_cc_arch. If unrecognized set ac_cc_arch=blend

AC_DEFUN(AC_CHECK_CC_ARCH,
[
if test $ac_cpu_arch = blend; then
	ac_cc_arch=blend
else
	AC_CHECK_CC_OPT([-march=$ac_cpu_arch],[ac_cc_cpu_arch=yes],[ac_cc_cpu_arch=no])
	if test $ac_cc_cpu_arch = yes ; then
		ac_cc_arch=$ac_cpu_arch
	else
		AC_CHECK_CC_OPT([-march=i${ac_cpu_family}86],[ac_cc_family_arch=yes],[ac_cc_family_arch=no])
		if test $ac_cc_family_arch = yes ; then
			ac_cc_arch=i${ac_cpu_family}86
		else
			ac_cc_arch=blend
		fi
	fi
fi
])
