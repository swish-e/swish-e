htmldir = $(datadir)/doc/$(PACKAGE)/html
imagedir = $(datadir)/doc/$(PACKAGE)/html/images
pod_dir = $(srcdir)/../pod
version_file = $(srcdir)/../doc/Version.pm


html_DATA = \
	CHANGES.html \
	INSTALL.html \
	README.html \
	SWISH-3.0.html \
	SWISH-BUGS.html \
	SWISH-CONFIG.html \
	SWISH-FAQ.html \
	SWISH-LIBRARY.html \
	SWISH-RUN.html \
	SWISH-SEARCH.html \
	API.html \
	spider.html \
	swish.html \
	Filter.html \
	index.html \
	index_long.html \
        style.css

image_DATA = \
	images/dotrule1.gif \
	images/swish2b.gif \
	images/swish2.gif \
	images/swishbanner1.gif \
	images/swish.gif


depends = \
	$(pod_dir)/CHANGES.pod \
	$(pod_dir)/INSTALL.pod \
	$(pod_dir)/README.pod \
	$(pod_dir)/SWISH-3.0.pod \
	$(pod_dir)/SWISH-BUGS.pod \
	$(pod_dir)/SWISH-CONFIG.pod \
	$(pod_dir)/SWISH-FAQ.pod \
	$(pod_dir)/SWISH-LIBRARY.pod \
	$(pod_dir)/SWISH-RUN.pod \
	$(pod_dir)/SWISH-SEARCH.pod \
	$(top_srcdir)/perl/API.pm \
	$(top_srcdir)/prog-bin/spider.pl \
	$(top_srcdir)/example/swish.cgi \
	$(top_srcdir)/filters/SWISH/Filter.pm

all: .html-stamp

.html-stamp: $(version_file) $(depends)
	cd $(srcdir)/../doc && bin/build
	@touch $(srcdir)/.html-stamp


EXTRA_DIST = $(html_DATA) $(image_DATA) .html-stamp

