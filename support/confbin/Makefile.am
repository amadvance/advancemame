## Process this file with automake to produce Makefile.in

bin_SCRIPTS = \
	$(wildcard adv*[!1])

nobase_pkgdata_DATA = \
	$(wildcard *.dat)

# Use a custom MAN install because autoconf doesn't support a MAN wildcard
pkgdata_MAN = \
	$(wildcard *.1)

# Use a custom DOC install because autoconf doesn't support doc
pkgdata_DOC = \
	$(wildcard doc/*)

# Make an empty distribution
EXTRA_DIST = \
	autogen.sh

pkgdir = @prefix@/share/@PACKAGE@
man1dir = $(mandir)/man1

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pkgdir)/doc
	@for i in $(pkgdata_DOC); do \
		echo " $(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir)/doc"; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir)/doc; \
	done
	$(mkinstalldirs) $(DESTDIR)$(man1dir)
	@for i in $(pkgdata_MAN); do \
		echo " $(INSTALL_DATA) $$i $(DESTDIR)$(man1dir)"; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(man1dir); \
	done

uninstall-local:
	@for i in $(pkgdata_DOC); do \
		echo " rm -f $(DESTDIR)$(pkgdir)/$$i"; \
		rm -f $(DESTDIR)$(pkgdir)/$$i; \
	done
	@for i in $(pkgdata_MAN); do \
		echo " rm -f $(DESTDIR)$(man1dir)/$$i"; \
		rm -f $(DESTDIR)$(man1dir)/$$i; \
	done

