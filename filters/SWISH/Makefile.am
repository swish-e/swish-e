perlmoduledir = $(libexecdir)/perl/SWISH

perlmodule_SCRIPTS = Filter.pm

CLEANFILES = Filter.pm

Filter.pm: Filter.pm.in
	@rm -f Filter.pm
	@sed \
		-e 's,@@mylibexecdir@@,$(libexecdir),' \
			$(srcdir)/Filter.pm.in > Filter.pm


nobase_perlmodule_SCRIPTS = \
    Filters/Doc2txt.pm \
    Filters/Doc2html.pm \
    Filters/Pdf2HTML.pm \
    Filters/ID3toHTML.pm \
    Filters/XLtoHTML.pm \
    Filters/pp2html.pm

EXTRA_DIST = \
    Filter.pm.in \
    Filters/Doc2txt.pm \
    Filters/Doc2html.pm \
    Filters/Pdf2HTML.pm \
    Filters/ID3toHTML.pm \
    Filters/XLtoHTML.pm \
    Filters/pp2html.pm

