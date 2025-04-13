plugindir = $(libdir)/xfce4/panel/plugins
plugin_LTLIBRARIES =	libfsguard.la

libfsguard_la_SOURCES = fsguard.c

libfsguard_la_CFLAGS =											\
	-I$(top_srcdir)											\
	@GLIB_CFLAGS@											\
	@GTK_CFLAGS@											\
	@LIBXFCE4UI_CFLAGS@										\
	@LIBXFCE4UTIL_CFLAGS@										\
	@LIBXFCE4PANEL_CFLAGS@										\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

libfsguard_la_LIBADD =												\
	@GLIB_LIBS@												\
	@GTK_LIBS@												\
	@LIBXFCE4UI_LIBS@											\
	@LIBXFCE4UTIL_LIBS@											\
	@LIBXFCE4PANEL_LIBS@

libfsguard_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

# .desktop file
#
desktop_in_files = fsguard.desktop.in
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
$(desktop_DATA): $(desktop_in_files) Makefile
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = \
	$(desktop_in_files) \
	meson.build \
	$(NULL)

DISTCLEANFILES = $(desktop_DATA)
