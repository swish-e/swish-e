htmldir = $(datadir)/doc/$(PACKAGE)/html
imagedir = $(datadir)/doc/$(PACKAGE)/html/images

HTMLDOCS = API.html          CHANGES.html     Filter.html \
           index.html        index_long.html  INSTALL.html \
           README.html       search.cgi       spider.html \
           style.css         SWISH-3.0.html   SWISH-BUGS.html \
           SWISH-CONFIG.html SWISH-FAQ.html   swish.html \
           SWISH-LIBRARY.html SWISH-PERL.html SWISH-RUN.html \
           SWISH-SEARCH.html

html_DATA = $(HTMLDOCS)


HTMLIMAGES = images/dotrule1.gif \
             images/swish2b.gif \
             images/swish2.gif \
             images/swishbanner1.gif \
             images/swish.gif


image_DATA = $(HTMLIMAGES)


../doc/Version.pm: ../configure
	echo 'package SWISHE::Doc;$$VERSION=q[$(VERSION)];1' > ../doc/Version.pm

html: ../doc/Version.pm
	(cd ../doc && bin/build)


EXTRA_DIST = $(html_DATA) $(image_DATA)

