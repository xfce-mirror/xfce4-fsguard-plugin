plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libfsguard.la

libfsguard_la_LDFLAGS =						\
	-avoid-version							\
	-export-dynamic							\
	-module

libfsguard_la_SOURCES =						\
	fsguard.c

libfsguard_la_CFLAGS =						\
        -DPACKAGE_LOCALE_DIR=\"$(localedir)\"                           \
	@XFCE4_PANEL_CFLAGS@

libfsguard_la_LIBADD = 						\
	@XFCE4_PANEL_LIBS@

EXTRA_DIST = 							\
	icons.h 						\
	red.xpm 						\
	green.xpm 						\
	yellow.xpm 						\
	unknown.xpm
	
noinst_DATA =							\
	icons.h 						\
	red.xpm 						\
	green.xpm 						\
	yellow.xpm 						\
	unknown.xpm

icons.h: $(srcdir)/red.xpm
	gdk-pixbuf-csource --raw --build-list 			\
	icon_red red.xpm > $(srcdir)/icons.h
	gdk-pixbuf-csource --raw --build-list 			\
	icon_green green.xpm >> $(srcdir)/icons.h
	gdk-pixbuf-csource --raw --build-list 			\
	icon_yellow yellow.xpm >> $(srcdir)/icons.h
	gdk-pixbuf-csource --raw --build-list 			\
	icon_unknown unknown.xpm >> $(srcdir)/icons.h
