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
dnl Sets ac_cpu_arch and ac_cpu_arch_secondary. If unrecognized set
dnl ac_cpu_arch=blend and ac_cpu_arch_secondary=blend

AC_DEFUN(AC_CHECK_CPU_ARCH,
[
if test -f /proc/cpuinfo ; then
	if grep -i "vendor_id.*:.*GenuineIntel" /proc/cpuinfo > /dev/null ; then
		if grep -i "family.*: 15" /proc/cpuinfo > /dev/null ; then
dnl The pentium4 generation is generally slower than pentium2
			ac_cpu_arch=pentium2
			ac_cpu_arch_secondary=i686
		elif grep -i "family.*: 6" /proc/cpuinfo > /dev/null ; then
			ac_cpu_arch=pentiumpro
			ac_cpu_arch_secondary=i686
		elif grep -i "family.*: 5" /proc/cpuinfo > /dev/null ; then
			ac_cpu_arch=pentium
			ac_cpu_arch_secondary=i586
		else
			ac_cpu_arch=blend
			ac_cpu_arch_secondary=blend
		fi
	elif grep -i "vendor_id.*:.*AuthenticAMD" /proc/cpuinfo > /dev/null ; then
		if grep -i "family.*: 6" /proc/cpuinfo > /dev/null ; then
			ac_cpu_arch=athlon
			ac_cpu_arch_secondary=i586
		elif grep -i "family.*: 5" /proc/cpuinfo > /dev/null ; then
dnl The gcc 3.2.x/3.3.x fails on the the k6 compilation. It should be fixed from 3.3.2.
			ac_cpu_arch=pentium
			ac_cpu_arch_secondary=i586
		else
			ac_cpu_arch=blend
			ac_cpu_arch_secondary=blend
		fi
	else
		ac_cpu_arch=blend
		ac_cpu_arch_secondary=blend
	fi
else
	ac_cpu_arch=blend
	ac_cpu_arch_secondary=blend
fi

AC_MSG_CHECKING([build arch])
AC_MSG_RESULT([$ac_cpu_arch])
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
		AC_CHECK_CC_OPT([-march=$ac_cpu_arch_secondary],[ac_cc_family_arch=yes],[ac_cc_family_arch=no])
		if test $ac_cc_family_arch = yes ; then
			ac_cc_arch=$ac_cpu_arch_secondary
		else
			ac_cc_arch=blend
		fi
	fi
fi
])

