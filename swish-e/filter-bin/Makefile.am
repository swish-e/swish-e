exampledir = $(datadir)/doc/$(PACKAGE)/examples/filter-bin

example_DATA = \
    README \
    swish_filter.pl \
    _binfilter.sh \
    _pdf2html.pl

CLEANFILES = swish_filter.pl

swish_filter.pl: swish_filter.pl.in
	@rm -f swish_filter.pl
	@sed \
		-e 's,@@perlmoduledir@@,$(libexecdir)/perl,' \
		-e 's,@@swishbindir@@,$(bindir),' \
		-e 's,@@perlbinary@@,$(PERL),' \
		$(srcdir)/swish_filter.pl.in > swish_filter.pl


EXTRA_DIST = \
    README \
    swish_filter.pl.in \
    _binfilter.sh \
    _pdf2html.pl



