@SET_MAKE@

SUBDIRS = panel-plugin po

EXTRA_DIST =								\
	ChangeLog							\
	INSTALL								\
	NEWS								\
	README

distclean-local:
	rm -rf *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz
