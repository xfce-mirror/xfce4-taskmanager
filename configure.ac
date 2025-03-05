dnl
dnl xfce4-taskmanager - A small taskmanager based on the Xfce 4 libraries.
dnl
dnl 2014-2021 Simon Steinbess
dnl 2018-2019 Rozhuk Ivan
dnl 2014 Landry Breuil
dnl 2014 Harald Judt
dnl 2014 Peter de Ridder
dnl 2008-2010 Mike Massonnet <mmassonnet@xfce.org>
dnl 2005-2008 Johannes Zellner <webmaster@nebulon.de>

dnl ***************************
dnl *** Version information ***
dnl ***************************
XDT_VERSION_INIT([1.5.8], [git])
m4_define([copyright_year], [2025])

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_COPYRIGHT([Copyright (c) 2005-copyright_year() The Xfce development team. All rights reserved.])
AC_INIT([Xfce4 Taskmanager], [xdt_version], [https://gitlab.xfce.org/apps/xfce4-taskmanager/], [xfce4-taskmanager],
        [https://docs.xfce.org/apps/xfce4-taskmanager/start])
AC_PREREQ([2.69])
AC_CONFIG_MACRO_DIRS([m4])
AC_CANONICAL_TARGET()
AC_REVISION([xdt_version_build])
AC_DEFINE([VERSION_FULL], [PACKAGE_VERSION], [Alias for VERSION and PACKAGE_VERSION for meson compatibility])
COPYRIGHT_YEAR=copyright_year()
AC_DEFINE_UNQUOTED([COPYRIGHT_YEAR], ["$COPYRIGHT_YEAR"], [Copyright year])
AC_SUBST([COPYRIGHT_YEAR])

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AM_INIT_AUTOMAKE([1.8 no-dist-gzip dist-bzip2 foreign])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE()
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AC_PROG_CC()
AM_PROG_CC_C_O()
LT_PATH_LD([])
AC_PROG_INSTALL()

dnl ************************************
dnl *** Checks for library functions ***
dnl ************************************
AC_C_CONST()

dnl **********************************
dnl *** Check for standard headers ***
dnl **********************************
AC_CHECK_HEADERS([stdlib.h string.h])

dnl ******************************
dnl *** Check for i18n support ***
dnl ******************************
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
XDT_CHECK_PACKAGE([LIBXMU], [xmu], [1.1.2])
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.72.0])
XDT_CHECK_PACKAGE([GTK3], [gtk+-3.0], [3.24.0])
XDT_CHECK_PACKAGE([CAIRO], [cairo], [1.5.0])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [4.18.0])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.18.0])
XDT_CHECK_PACKAGE([XFCONF], [libxfconf-0], [4.18.0])

XDT_CHECK_PACKAGE_BINARY([GLIB_COMPILE_RESOURCES], [gio-2.0], [glib_compile_resources], [glib-compile-resources])

dnl ***********************************
dnl *** Check for optional packages ***
dnl ***********************************
XDT_CHECK_OPTIONAL_PACKAGE([LIBX11], [x11], [1.6.7], [libx11], [Libx11 support])
XDT_CHECK_OPTIONAL_PACKAGE([WNCK], [libwnck-3.0], [3.2], [wnck3], [building with libwnck3 for window icons/names], [yes])

dnl ***********************************
dnl ********** Check for skel *********
dnl ***********************************
AC_ARG_WITH([skel],
		AS_HELP_STRING([--with-skel],[build with task-manager-skel.c]),
		[ac_skel="$withval"],
		[ac_skel=no])

dnl ***********************************
dnl ******* Check for OS family *******
dnl ***********************************
if test x"$ac_skel" = x"yes"; then
	ac_os_implementation="skel"
else
	case "$target_os" in
		freebsd*)
		ac_os_implementation="freebsd"
		AC_CHECK_LIB([kvm], [kvm_openfiles])
		AC_CHECK_HEADERS([fcntl.h kvm.h paths.h pwd.h sys/param.h sys/proc.h \
				sys/sysctl.h sys/types.h sys/user.h unistd.h])
		;;
		dragonfly*|netbsd*|openbsd*)
		ac_os_implementation="bsd"
		AC_CHECK_HEADERS([err.h pwd.h stdlib.h string.h sys/param.h sys/sched.h \
				sys/swap.h sys/sysctl.h sys/types.h unistd.h])
		;;
		solaris*)
		ac_os_implementation="solaris"
		AC_CHECK_LIB([kstat], [kstat_open])
		AC_CHECK_HEADERS([fcntl.h kstat.h procfs.h pwd.h stdlib.h string.h \
				sys/procfs.h sys/stat.h sys/swap.h sys/types.h])
		;;
		linux*)
		ac_os_implementation="linux"
		AC_CHECK_HEADERS([pwd.h signal.h stdio.h string.h sys/resource.h \
				sys/stat.h sys/types.h unistd.h])
		;;
		darwin*)
		ac_os_implementation="darwin"
		AC_CHECK_HEADERS([err.h mach/mach.h stdlib.h string.h sys/param.h sys/proc.h \
				sys/sysctl.h sys/types.h unistd.h Availability.h TargetConditionals.h])
		;;
		*)
		AC_MSG_CHECKING([for OS implementation])
		AC_MSG_ERROR([no OS implementation for $target_os is available])
		;;
	esac
fi
AC_MSG_CHECKING([for OS implementation])
AC_MSG_RESULT([$ac_os_implementation])

AM_CONDITIONAL([OS_FREEBSD], [test x"$ac_os_implementation" = x"freebsd"])
AM_CONDITIONAL([OS_BSD], [test x"$ac_os_implementation" = x"bsd"])
AM_CONDITIONAL([OS_SOLARIS], [test x"$ac_os_implementation" = x"solaris"])
AM_CONDITIONAL([OS_LINUX], [test x"$ac_os_implementation" = x"linux"])
AM_CONDITIONAL([OS_DARWIN], [test x"$ac_os_implementation" = x"darwin"])
AM_CONDITIONAL([OS_SKEL], [test x"$ac_os_implementation" = x"skel"])

dnl ***********************************
dnl *** Check for debugging support ***
dnl ***********************************
XDT_FEATURE_DEBUG([xdt_debug_default])

dnl ***************
dnl *** Outputs ***
dnl ***************
AC_CONFIG_FILES([
Makefile
src/Makefile
data/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/24x24/Makefile
data/icons/32x32/Makefile
data/icons/48x48/Makefile
data/icons/64x64/Makefile
data/icons/96x96/Makefile
data/icons/128x128/Makefile
data/icons/scalable/Makefile
po/Makefile.in
])
AC_OUTPUT

dnl ***************************
dnl *** Print configuration ***
dnl ***************************
echo
echo "Build Configuration:"
echo
echo "* Libx11:        ${LIBX11_VERSION:-no}"
echo "* Wnck:          ${WNCK_VERSION:-no}"
echo "* Cairo:         ${CAIRO_VERSION}"
echo "* GTK+:          ${GTK3_VERSION}"
echo "* Target OS:     $target_os ($ac_os_implementation)"
echo "* Debug:         $enable_debug"
echo
