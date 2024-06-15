dnl xfce4-fsguard-plugin - <Plugin for xfce4-panel>
dnl
dnl 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
dnl 2007 Mike Massonnet <mmassonnet@gmail.com>
dnl

XDT_VERSION_INIT([1.1.3], [git])

AC_COPYRIGHT([Copyright (c) 2003-2023 The Xfce development team. All rights reserved.])
AC_INIT([xfce4-fsguard-plugin], [xdt_version], [https://gitlab.xfce.org/panel-plugins/xfce4-fsguard-plugin], [xfce4-fsguard-plugin], [https://docs.xfce.org/panel-plugins/xfce4-fsguard-plugin])
AC_REVISION([xdt_version_build])

AM_INIT_AUTOMAKE([1.8 dist-bzip2 tar-ustar no-dist-gzip foreign])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE
AC_CONFIG_MACRO_DIRS([m4])

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL

dnl Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl configure the panel plugin
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.50.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [3.22.0])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [4.12.0])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.12.0])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-2.0], [4.12.0])

dnl check for i18n support
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl Check for debugging support
XDT_FEATURE_DEBUG([xdt_debug_default])

AC_CONFIG_FILES([
Makefile
icons/Makefile
icons/48x48/Makefile
icons/36x36/Makefile
icons/32x32/Makefile
icons/24x24/Makefile
icons/22x22/Makefile
icons/16x16/Makefile
icons/scalable/Makefile
panel-plugin/Makefile
po/Makefile.in
])
AC_OUTPUT
