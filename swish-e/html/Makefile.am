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
	SWISH-PERL.html \
	SWISH-RUN.html \
	SWISH-SEARCH.html \
	API.html \
	spider.html \
	swish.html \
	Filter.html \
	index.html \
	index_long.html style.css

image_DATA = images/dotrule1.gif \
             images/swish2b.gif \
             images/swish2.gif \
             images/swishbanner1.gif \
             images/swish.gif




$(version_file): $(top_srcdir)/configure
	rm -f $(version_file)
	echo 'package SWISHE::Doc;$$VERSION=q[$(VERSION)];1' > $(version_file)

html: $(version_file)
	cd $(srcdir)/../doc && bin/build
	
CHANGES.html: $(pod_dir)/CHANGES.pod
	$(MAKE) html

INSTALL.html: $(pod_dir)/INSTALL.pod
	$(MAKE) html

README.html: $(pod_dir)/README.pod
	$(MAKE) html

SWISH-3.0.html: $(pod_dir)/SWISH-3.0.pod
	$(MAKE) html

SWISH-BUGS.html: $(pod_dir)/SWISH-BUGS.pod
	$(MAKE) html

SWISH-CONFIG.html: $(pod_dir)/SWISH-CONFIG.pod
	$(MAKE) html

SWISH-FAQ.html: $(pod_dir)/SWISH-FAQ.pod
	$(MAKE) html

SWISH-LIBRARY.html: $(pod_dir)/SWISH-LIBRARY.pod
	$(MAKE) html

SWISH-PERL.html: $(pod_dir)/SWISH-PERL.pod
	$(MAKE) html

SWISH-RUN.html: $(pod_dir)/SWISH-RUN.pod
	$(MAKE) html

SWISH-SEARCH.html: $(pod_dir)/SWISH-SEARCH.pod
	$(MAKE) html

API.html: $(top_srcdir)/perl/API.pm
	$(MAKE) html

spider.html: $(top_srcdir)/prog-bin/spider.pl
	$(MAKE) html

swish.html: $(top_srcdir)/example/swish.cgi
	$(MAKE) html

Filter.html: $(top_srcdir)/filters/SWISH/Filter.pm
	$(MAKE) html

index.html index_long.html style.css:
	echo "done"


EXTRA_DIST = $(html_DATA) $(image_DATA)

