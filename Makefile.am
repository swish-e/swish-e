AUTOMAKE_OPTIONS = foreign
SUBDIRS = filters prog-bin conf filter-bin example html man src tests


if BUILDDOCS
docdir = $(datadir)/doc/$(PACKAGE)

doc_DATA = \
	$(srcdir)/INSTALL \
	$(srcdir)/README \
        README.cvs

$(srcdir)/INSTALL: $(top_srcdir)/pod/INSTALL.pod
	-rm -f $(top_srcdir)/INSTALL
	-pod2text $(top_srcdir)/pod/INSTALL.pod > $(top_srcdir)/INSTALL

$(srcdir)/README: $(top_srcdir)/pod/README.pod
	-rm -f $(top_srcdir)/README
	-pod2text $(top_srcdir)/pod/README.pod > $(top_srcdir)/README
endif

config_dir  = \
     config/config.guess \
     config/config.sub \
     config/install-sh \
     config/ltmain.sh \
     config/missing \
     config/mkinstalldirs


doc_dir = \
    doc/Makefile.in \
    doc/Pod/HtmlPsPdf.pm \
    doc/Pod/HtmlPsPdf/Book.pm \
    doc/Pod/HtmlPsPdf/Chapter.pm \
    doc/Pod/HtmlPsPdf/Common.pm \
    doc/Pod/HtmlPsPdf/Config.pm \
    doc/Pod/HtmlPsPdf/Html.pm \
    doc/Pod/HtmlPsPdf/RunTime.pm \
    doc/Version.pm \
    doc/bin/build \
    doc/bin/html2ps \
    doc/bin/pod2hpp \
    doc/conf/html2ps-global.conf \
    doc/conf/html2ps-slides.conf \
    doc/conf/html2ps.conf \
    doc/conf/html2ps.html \
    doc/conf/hyphen.tex \
    doc/split.conf \
    doc/swish.conf \
    doc/tmpl/index.tmpl \
    doc/tmpl/indexps.tmpl \
    doc/tmpl/page.tmpl \
    doc/tmpl/pageps.tmpl \
    doc/tmpl/splitpage.tmpl


perl_dir = \
    perl/Changes \
    perl/MANIFEST \
    perl/README \
    perl/Makefile.PL.in \
    perl/API.pm \
    perl/API.xs \
    perl/typemap \
    perl/t/test.t \
    perl/t/test.conf \
    perl/t/first.html \
    perl/t/second.html \
    perl/t/third.html



pod_dir = \
    pod/.htaccess \
    pod/CHANGES.pod \
    pod/INSTALL.pod \
    pod/README.pod \
    pod/SWISH-3.0.pod \
    pod/SWISH-BUGS.pod \
    pod/SWISH-CONFIG.pod \
    pod/SWISH-FAQ.pod \
    pod/SWISH-LIBRARY.pod \
    pod/SWISH-RUN.pod \
    pod/SWISH-SEARCH.pod \
    pod/images/dotrule1.gif \
    pod/images/swish.gif \
    pod/images/swish2.gif \
    pod/images/swish2b.gif \
    pod/images/swishbanner1.gif \
    pod/search.cgi \
    pod/style.css


vms_dir = \
    src/vms/acconfig.h_vms \
    src/vms/build_swish-e.com \
    src/vms/config.h \
    src/vms/readme_vms.txt \
    src/vms/regex.c \
    src/vms/regex.h \
    src/vms/descrip_vax.mms \
    src/vms/descrip_axp.mms \
    src/vms/descrip_libxml.mms \
    src/vms/sprintf_2_2/install   \
    src/vms/sprintf_2_2/license.txt   \
    src/vms/sprintf_2_2/makefile   \
    src/vms/sprintf_2_2/readme   \
    src/vms/sprintf_2_2/readme.html  \
    src/vms/sprintf_2_2/snprintf.c   \
    src/vms/sprintf_2_2/snprintf.c_orig   \
    src/vms/sprintf_2_2/snprintf.h   \
    src/vms/sprintf_2_2/test.c

win32_dir = \
    src/win32/dirent.c \
    src/win32/dirent.h \
    src/win32/libswishe.dsp \
    src/win32/swishe.dsp \
    src/win32/swishe.dsw \
    src/win32/installer.nsi \
    src/win32/build.sh \
    src/win32/dist.sh \
    src/worddata.c \
    src/worddata.h

rpm_dir = \
    rpm/swish-e.spec.in \
    rpm/swish-e.xpm



EXTRA_DIST = \
    $(config_dir) $(doc_dir) $(perl_dir) \
    $(pod_dir) $(vms_dir) $(win32_dir) $(rpm_dir)  \
    $(doc_DATA) \
    win32.nsi \
    win32src.nsi


.PHONEY: test

test: check

