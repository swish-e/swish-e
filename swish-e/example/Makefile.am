perlmoduledir = $(libexecdir)/perl/SWISH

libexec_SCRIPTS = swish.cgi

perlmodule_SCRIPTS = \
    modules/SWISH/DateRanges.pm \
    modules/SWISH/DefaultHighlight.pm \
    modules/SWISH/PhraseHighlight.pm \
    modules/SWISH/SimpleHighlight.pm \
    modules/SWISH/TemplateDefault.pm \
    modules/SWISH/TemplateDumper.pm \
    modules/SWISH/TemplateHTMLTemplate.pm \
    modules/SWISH/TemplateToolkit.pm \
    modules/SWISH/ParseQuery.pm

CLEANFILES = swish.cgi
swish.cgi: swish.cgi.in
	@rm -f swish.cgi
	@sed \
		-e 's,@@perlmoduledir@@,$(libexecdir)/perl,' \
		-e 's,@@swishbindir@@,$(bindir),' \
		-e 's,@@perlbinary@@,$(PERL),' \
			 $(srcdir)/swish.cgi.in > swish.cgi


EXTRA_DIST = \
    README \
    SWISH-Stemmer-0.05.tar.gz \
    search.tt \
    swish.cgi.in \
    swish.gif \
    swish.tmpl \
    $(perlmodule_SCRIPTS)
