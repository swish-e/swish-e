perlmoduledir = $(libexecdir)/perl
exampledir = $(datadir)/doc/$(PACKAGE)/examples/filters

bin_SCRIPTS = swish-filter-test

example_DATA = README

CLEANFILES = swish-filter-test

swish-filter-test: swish-filter-test.in
	@rm -f swish-filter-test
	@sed \
		-e 's,@@perlmoduledir@@,$(libexecdir)/perl,' \
		-e 's,@@swishbindir@@,$(bindir),' \
		-e 's,@@perlbinary@@,$(PERL),' \
			$(srcdir)/swish-filter-test.in > swish-filter-test


nobase_perlmodule_SCRIPTS = \
    SWISH/Filter.pm \
    SWISH/Filters/Doc2txt.pm \
    SWISH/Filters/Pdf2HTML.pm \
    SWISH/Filters/ID3toHTML.pm \
    SWISH/Filters/XLtoHTML.pm

EXTRA_DIST = \
    README \
    swish-filter-test.in \
    SWISH/Filter.pm \
    SWISH/Filters/Doc2txt.pm \
    SWISH/Filters/Pdf2HTML.pm \
    SWISH/Filters/ID3toHTML.pm \
    SWISH/Filters/XLtoHTML.pm
