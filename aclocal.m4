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
if grep -i "model name.*:.*Celeron.*Willamette" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="pentium4"
	ac_cpu_arch_secondary=i686
elif grep -i "model name.*:.*Celeron.*Coppermine" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="pentium3"
	ac_cpu_arch_secondary=i686
elif grep -i "model name.*:.*Celeron.*Mendocino" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="pentium2"
	ac_cpu_arch_secondary=i686
elif grep -i "model name.*:.*Pentium.*III" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="pentium3"
	ac_cpu_arch_secondary=i686
elif grep -i "model name.*:.*Pentium.*II" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="pentium2"
	ac_cpu_arch_secondary=i686
elif grep -i "model name.*:.*Pentium.*Pro" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="pentiumpro"
	ac_cpu_arch_secondary=i686
elif grep -i "model name.*:.*Pentium.* 4 " /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="pentium4"
	ac_cpu_arch_secondary=i686
elif grep -i "model name.*:.*Pentium" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="pentium"
	ac_cpu_arch_secondary=i586
elif grep -i "model name.*:.*Athlon.*MP" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="athlon-mp"
	ac_cpu_arch_secondary=i686
elif grep -i "model name.*:.*Athlon.*XP" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="athlon-xp"
	ac_cpu_arch_secondary=i686
elif grep -i "model name.*:.*Athlon.*TBird" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="athlon-tbird"
	ac_cpu_arch_secondary=i686
elif grep -i "model name.*:.*Athlon.* 4 " /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="athlon-4"
	ac_cpu_arch_secondary=i686
elif grep -i "model name.*:.*Athlon" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="athlon"
	ac_cpu_arch_secondary=i686
elif grep -i "model name.*:.*Duron" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="pentium"
	ac_cpu_arch_secondary=i586
elif grep -i "model name.*:.*K6-III" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="k6-3"
	ac_cpu_arch_secondary=i586
elif grep -i "model name.*:.*K6-II" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="k6-2"
	ac_cpu_arch_secondary=i586
elif grep -i "model name.*:.*K6" /proc/cpuinfo > /dev/null ; then
	ac_cpu_arch="k6"
	ac_cpu_arch_secondary=i586
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
