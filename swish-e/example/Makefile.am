perlmoduledir = $(libexecdir)/perl/SWISH

libexec_SCRIPTS = swish.cgi search.cgi


pkgdata_DATA = \
    swish.tt \
    swish.tmpl

templatedir = $(pkgdatadir)/templates

template_DATA = \
    templates/search.tt \
    templates/page_layout \
    templates/common_header \
    templates/common_footer \
    templates/style.css \
    templates/markup.css


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
		-e 's,@@templatedir@@,$(templatedir),' \
			 $(srcdir)/search.cgi.in > search.cgi



EXTRA_DIST = \
    README \
    SWISH-Stemmer-0.05.tar.gz \
    swish.cgi.in \
    search.cgi.in \
    swish.gif \
    $(pkgdata_DATA) \
    $(template_DATA) \
    $(perlmodule_SCRIPTS)
