perlmoduledir = $(libexecdir)/perl/SWISH

libexec_SCRIPTS = swish.cgi search.cgi


pkgdata_DATA = \
	search.tt \
	swish.tmpl


perlmodule_SCRIPTS = \
    modules/SWISH/DateRanges.pm \
    modules/SWISH/DefaultHighlight.pm \
    modules/SWISH/PhraseHighlight.pm \
    modules/SWISH/SimpleHighlight.pm \
    modules/SWISH/TemplateDefault.pm \
    modules/SWISH/TemplateDumper.pm \
    modules/SWISH/TemplateFrame.pm \
    modules/SWISH/TemplateHTMLTemplate.pm \
    modules/SWISH/TemplateToolkit.pm \
    modules/SWISH/ParseQuery.pm

CLEANFILES = swish.cgi search.cgi

swish.cgi: swish.cgi.in
	@rm -f swish.cgi
	@sed \
		-e 's,@@perlmoduledir@@,$(libexecdir)/perl,' \
		-e 's,@@swishbinary@@,$(bindir)/swish-e$(EXEEXT),' \
		-e 's,@@perlbinary@@,$(PERL),' \
		-e 's,@@pkgdatadir@@,$(pkgdatadir),' \
			 $(srcdir)/swish.cgi.in > swish.cgi

search.cgi: search.cgi.in
	@rm -f swish.cgi
	@sed \
		-e 's,@@perlmoduledir@@,$(libexecdir)/perl,' \
		-e 's,@@swishbinary@@,$(bindir)/swish-e$(EXEEXT),' \
		-e 's,@@perlbinary@@,$(PERL),' \
		-e 's,@@pkgdatadir@@,$(pkgdatadir),' \
			 $(srcdir)/search.cgi.in > search.cgi



EXTRA_DIST = \
    README \
    SWISH-Stemmer-0.05.tar.gz \
    search.tt \
    swish.cgi.in \
    search.cgi.in \
    swish.gif \
    swish.tmpl \
    $(perlmodule_SCRIPTS)
