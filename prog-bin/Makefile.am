perlmoduledir = $(libexecdir)/perl

libexec_SCRIPTS = spider.pl

# Thise are really out dated

perlmodule_SCRIPTS = \
	doc2txt.pm \
	pdf2html.pm \
	pdf2xml.pm

CLEANFILES = spider.pl

spider.pl: spider.pl.in
	@rm -f spider.pl
	@sed \
		-e 's,@@perlmoduledir@@,$(libexecdir)/perl,' \
		-e 's,@@swishbindir@@,$(bindir),' \
		-e 's,@@perlbinary@@,$(PERL),' \
			 $(srcdir)/spider.pl.in > spider.pl


EXTRA_DIST = \
    spider.pl.in \
    README \
    file.pl \
    DirTree.pl \
    SwishSpiderConfig.pl \
    pdf2xml.pm \
    pdf2html.pm \
    doc2txt.pm \
    MySQL.pl \
    index_hypermail.pl
