dnl configure.ac
dnl
dnl xfce4-fsguard-plugin - <Plugin for xfce4-panel>
dnl
dnl 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
dnl

m4_define([fsguard_version],[0.3.0])

AC_INIT([xfce4-fsguard-plugin], [fsguard_version()],
	[xfce-goodies-dev@lists.berlios.de])

FSGUARD_VERSION=fsguard_version()
AM_INIT_AUTOMAKE([xfce4-fsguard-plugin], [$FSGUARD_VERSION])
AM_CONFIG_HEADER([config.h])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_INTLTOOL

dnl Check for standard header files
AC_HEADER_STDC

dnl configure the panel plugin
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.3.22])

dnl check for i18n support
XDT_I18N([de fr hu ko lt pl])

dnl Check for debugging support
XDT_FEATURE_DEBUG()

AC_OUTPUT([
Makefile
panel-plugin/Makefile
icons/Makefile
po/Makefile.in
])
