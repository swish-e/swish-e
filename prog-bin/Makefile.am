perlmoduledir = $(libexecdir)/perl
exampledir = $(datadir)/doc/$(PACKAGE)/examples/prog-bin


libexec_SCRIPTS = spider.pl DirTree.pl

# These are really out dated

perlmodule_SCRIPTS = \
	doc2txt.pm \
	pdf2html.pm \
	pdf2xml.pm

CLEANFILES = spider.pl DirTree.pl


# This is done here to stay in the GNU coding standards
# libexecdir can be modified at make time, so can't use
# variable substitution at configure time

spider.pl: spider.pl.in
	@rm -f spider.pl
	@sed \
		-e 's,@@perlmoduledir@@,$(libexecdir)/perl,' \
		-e 's,@@swishbindir@@,$(bindir),' \
		-e 's,@@perlbinary@@,$(PERL),' \
			 $(srcdir)/spider.pl.in > spider.pl

DirTree.pl: DirTree.pl.in
	@rm -f spider.pl
	@sed \
		-e 's,@@perlmoduledir@@,$(libexecdir)/perl,' \
		-e 's,@@swishbindir@@,$(bindir),' \
		-e 's,@@perlbinary@@,$(PERL),' \
			 $(srcdir)/DirTree.pl.in > DirTree.pl

other_examples = \
    README \
    file.pl \
    SwishSpiderConfig.pl \
    MySQL.pl \
    index_hypermail.pl \
    pdf2xml.pm \
    pdf2html.pm \
    doc2txt.pm

example_DATA = $(other_examples)

EXTRA_DIST = \
    spider.pl.in \
    DirTree.pl.in \
    $(other_examples)



