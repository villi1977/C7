BINDIR  = /usr/bin
SBINDIR = /usr/sbin
LIBDIR  = /usr/share/vzpkg
MANDIR  = /usr/share/man
MAN8DIR = $(MANDIR)/man8

BIN_FILES    = vzpkgcache vzyum vzrpm vzpkgadd vzpkgrm vzpkgls vzosname
LIB_FILES    = functions cache-os
MAN8_FILES   = man/vzpkgcache.8 man/vzyum.8 man/vzrpm.8
MYINIT_FILES = myinit.i386 myinit.x86_64 myinit.ia64

all:

install: install-bin install-lib install-myinit install-man

install-bin: $(BIN_FILES)
	mkdir -p $(DESTDIR)$(BINDIR)
	for f in $(BIN_FILES); do \
		install -m 755 $$f $(DESTDIR)$(BINDIR); \
	done

install-lib: $(LIB_FILES)
	mkdir -p $(DESTDIR)$(LIBDIR)
	for f in $(LIB_FILES); do \
		install -m 644 $$f $(DESTDIR)$(LIBDIR); \
	done

install-myinit: install-lib
	for f in $(MYINIT_FILES); do \
		install -m 755 $$f $(DESTDIR)$(LIBDIR); \
	done

install-man: install-man8
	mkdir -p $(DESTDIR)$(MANDIR)

install-man8: $(MAN8_FILES)
	mkdir -p $(DESTDIR)$(MAN8DIR)
	for f in $(MAN8_FILES); do \
		install -m 644 $$f $(DESTDIR)$(MAN8DIR); \
	done

rpms: dist-bzip2
	cp $(distdir).tar.bz2 `rpm --eval %_sourcedir`
	rpmbuild -ba vzctl.spec

tar:
	(VERSION=`awk '/Version:/{print $$2}' < vzpkg.spec` && \
	rm -f ../vzpkg-$$VERSION; ln -sf `pwd` ../vzpkg-$$VERSION && \
	tar --directory ..  --exclude CVS --exclude .git --exclude \*.tar.bz2 -cvhjf vzpkg-$$VERSION.tar.bz2 vzpkg-$$VERSION; \
	rm -f ../vzpkg-$$VERSION)

.PHONY: all install install-bin install-lib install-myinit install-man install-man8 tar
