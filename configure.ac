dnl xfce4-fsguard-plugin - <Plugin for xfce4-panel>
dnl
dnl 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
dnl 2007 Mike Massonnet <mmassonnet@gmail.com>
dnl

m4_define([copyright_year], [2025])
XDT_VERSION_INIT([1.2.0], [git])

AC_COPYRIGHT([Copyright (c) 2003-copyright_year() The Xfce development team. All rights reserved.])
AC_INIT([xfce4-fsguard-plugin], [xdt_version], [https://gitlab.xfce.org/panel-plugins/xfce4-fsguard-plugin], [xfce4-fsguard-plugin], [https://docs.xfce.org/panel-plugins/xfce4-fsguard-plugin])
AC_REVISION([xdt_version_build])
AC_PREREQ([2.69])

AM_INIT_AUTOMAKE([1.8 dist-bzip2 tar-ustar no-dist-gzip foreign])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE
AC_CONFIG_MACRO_DIRS([m4])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_DEFINE([VERSION_FULL], [PACKAGE_VERSION], [Alias for VERSION and PACKAGE_VERSION for meson compatibility])
COPYRIGHT_YEAR=copyright_year()
AC_DEFINE_UNQUOTED([COPYRIGHT_YEAR], ["$COPYRIGHT_YEAR"], [Copyright year])
AC_SUBST([COPYRIGHT_YEAR])

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL

dnl Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl configure the panel plugin
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.50.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [3.22.0])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [4.16.0])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.16.0])
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-2.0], [4.16.0])

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
