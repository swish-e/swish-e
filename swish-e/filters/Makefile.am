SUBDIRS = SWISH

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

EXTRA_DIST = \
    README \
    swish-filter-test.in
