AUTOMAKE_OPTIONS = foreign
SUBDIRS = filters prog-bin conf filter-bin example html man src tests pod



docdir = $(datadir)/doc/$(PACKAGE)


# Install these three in the doc directory
# INSTALL and README are built at make time from .pod source

doc_DATA = \
    $(srcdir)/INSTALL \
    $(srcdir)/README \
    README.cvs

# These create REAME and INSTALL in the top level *source*
# directory for the distribution.  Created at "make" time.

$(srcdir)/INSTALL: $(top_srcdir)/pod/INSTALL.pod
	-rm -f $(top_srcdir)/INSTALL
	-pod2text $(top_srcdir)/pod/INSTALL.pod > $(top_srcdir)/INSTALL

$(srcdir)/README: $(top_srcdir)/pod/README.pod
	-rm -f $(top_srcdir)/README
	-pod2text $(top_srcdir)/pod/README.pod > $(top_srcdir)/README

config_dir  = \
     config/config.guess \
     config/config.sub \
     config/install-sh \
     config/ltmain.sh \
     config/missing \
     config/mkinstalldirs



perl_dir = \
    perl/Changes \
    perl/MANIFEST \
    perl/README \
    perl/Makefile.PL \
    perl/API.pm \
    perl/API.xs \
    perl/typemap \
    perl/t/test.t \
    perl/t/dummy.t \
    perl/t/test.conf \
    perl/t/first.html \
    perl/t/second.html \
    perl/t/third.html

vms_dir = \
    src/vms/acconfig.h_vms \
    src/vms/build_swish-e.com \
    src/vms/config.h \
    src/vms/descrip_axp.mms \
    src/vms/descrip_libxml2.mms \
    src/vms/descrip_vax.mms \
    src/vms/libtest.opt \
    src/vms/readme_vms.txt \
    src/vms/regex.c \
    src/vms/regex.h \
    src/vms/regexpr.h \
    src/vms/swish.opt


win32_dir = \
    src/win32/acconfig.h \
    src/win32/dirent.c \
    src/win32/dirent.h \
    src/win32/libswishe.dsp \
    src/win32/libswishindex.dsp \
    src/win32/swishe.dsp \
    src/win32/swishe.dsw \
    src/win32/installer.nsi \
    src/win32/release.nsi \
    src/win32/filebase.nsh \
    src/win32/fixperl.pl \
    src/win32/build-perl.bat \
    src/win32/build.sh \
    src/win32/dist.sh \
    src/worddata.c \
    src/worddata.h

rpm_dir = \
    rpm/swish-e.spec.in \
    rpm/swish-e.xpm


debian_dir = \
    debian/README.Debian \
    debian/changelog \
    debian/compat \
    debian/control \
    debian/copyright \
    debian/files \
    debian/rules \
    debian/swish-e.doc-base \
    debian/swish-e.substvars

bin_SCRIPTS = swish-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = swish-e.pc

EXTRA_DIST = \
    $(config_dir) $(perl_dir) \
    $(vms_dir) $(win32_dir) $(rpm_dir)  \
    $(doc_DATA) swish-config.in swish-e.pc.in\
    $(debian_dir)


.PHONEY: test

test: check

