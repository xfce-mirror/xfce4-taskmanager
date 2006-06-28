dnl Process this file with autoconf to produce a configure script.
AC_INIT([xfce4-taskmanager], [0.4.0-rc2], [xfce4-taskmanger@nebulon.de])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([AC_PACKAGE_TARNAME()], [AC_PACKAGE_VERSION()])
AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL

dnl Checks for header files.
AC_HEADER_STDC

AC_DISABLE_STATIC

AC_PROG_LIBTOOL
AC_PROG_INTLTOOL

XDT_CHECK_PACKAGE([XFCE4_GUI], [libxfcegui4-1.0], [4.2.0])
XDT_CHECK_PACKAGE([XFCE4_UTIL], [libxfce4util-1.0], [4.2.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-2.0], [2.2.0])

dnl Check for i18n support
XDT_I18N([de eu fi fr hu ja pl pt_BR ru vi])

dnl Check for debugging support
BM_DEBUG_SUPPORT()


dnl Checks for library functions.
dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST


AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])

