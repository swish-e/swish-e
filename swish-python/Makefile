
PACKAGE = swish-python
VERSION = 0.1
PYTHON_BIN = /usr/bin/python
SWIG_BIN = /usr/bin/swig
INSTALL_BIN = /usr/bin/install -m0644

DEBFILES = \
    debian/changelog \
    debian/compat \
    debian/copyright \
    debian/control \
    debian/rules \
    debian/files \

EXTRA_DIST = \
    BUGS AUTHORS README INSTALL COPYING \
    examples/search.py

DISTFILES = \
    Makefile setup.txt swish.py \
    setup.py swish.i \
    swish_wrap.c $(DEBFILES) $(EXTRA_DIST)


all: swig module

swig: swish_wrap.c
swish_wrap.c:
	$(SWIG_BIN) -python -module swish swish.i

module:
	$(PYTHON_BIN) setup.py build

install: module
	$(PYTHON_BIN) setup.py install `test -n "$(DESTDIR)" && echo --root $(DESTDIR)`

clean:
	rm -fr build debian/swish-python debian/DEBIAN configure-stamp build-stamp *.pyc

realclean: clean
	rm -f swish.py swish_wrap.c

distclean: realclean
	rm -f $(PACKAGE)-$(VERSION).tar.gz

dist: swig
	mkdir -p $(PACKAGE)-$(VERSION)
	for file in $(DISTFILES); do \
	  $(INSTALL_BIN) -D $$file $(PACKAGE)-$(VERSION)/$$file; \
	done
	tar czf $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)-$(VERSION)
	test -d $(PACKAGE)-$(VERSION) && rm -fr $(PACKAGE)-$(VERSION)

deb:
	fakeroot dpkg-buildpackage

