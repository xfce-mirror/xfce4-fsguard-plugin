SUBDIRS =														\
		48x48													\
		36x36													\
		32x32													\
		24x24													\
		22x22													\
		16x16													\
		scalable

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook:
	@-if test -z "$(DESTDIR)"; then								\
		echo "Updating Gtk icon cache.";						\
		$(gtk_update_icon_cache);								\
	else														\
		echo "*** Icon cache not updated. Remember to run:";	\
		echo "***";												\
		echo "***   $(gtk_update_icon_cache)";					\
		echo "***";												\
	fi

