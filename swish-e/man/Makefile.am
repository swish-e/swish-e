man_MANS = \
    SWISH-CONFIG.1 \
    SWISH-FAQ.1 \
    SWISH-LIBRARY.1 \
    SWISH-PERL.1 \
    SWISH-RUN.1


%.1: $(srcdir)/../pod/%.pod
	-rm -f $@
	pod2man --center="SWISH-E Documentation" --lax --release='$(VERSION)' $< > $(top_srcdir)/man/$@

EXTRA_DIST = $(man_MANS)
