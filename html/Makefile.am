# $id$
#
# Conditionally install the html documentation


# Where docs are installed
htmldir = $(datadir)/doc/$(PACKAGE)/html

if BUILDDOCS

# build the docs from website src

$(html_files):
	$(SWISH_WEB) -swishsrc $(top_srcdir) -poddest . -v -all

DISTCLEANFILES = \
	$(html_DATA)

endif

if INSTALLDOCS

html_DATA = \
	$(html_files)

endif



html_files = \
    api.html \
    changes.html \
    filter.html \
    index.html \
    install.html \
    readme.html \
    search.cgi.html \
    spider.html \
    swish-3.0.html \
    swish-bugs.html \
    swish.cgi.html \
    swish-config.html \
    swish.css \
    swish-faq.html \
    swish-library.html \
    swish-run.html \
    swish-search.html

EXTRA_DIST = \
    $(html_DATA)

