if BUILDDOCS
man_MANS = \
    $(srcdir)/swish-e.1 \
    $(srcdir)/SWISH-CONFIG.1 \
    $(srcdir)/SWISH-FAQ.1 \
    $(srcdir)/SWISH-LIBRARY.1 \
    $(srcdir)/SWISH-RUN.1
endif


$(srcdir)/swish-e.1 : $(top_srcdir)/pod/swish-e.pod
	-rm -f $@
	-pod2man --center="SWISH-E Documentation" --lax --release='$(VERSION)'  $<  > $@

$(srcdir)/SWISH-CONFIG.1 : $(top_srcdir)/pod/SWISH-CONFIG.pod
	-rm -f $@
	-pod2man --center="SWISH-E Documentation" --lax --release='$(VERSION)'  $<  > $@

$(srcdir)/SWISH-FAQ.1 : $(top_srcdir)/pod/SWISH-FAQ.pod
	-rm -f $@
	-pod2man --center="SWISH-E Documentation" --lax --release='$(VERSION)'  $<  > $@

$(srcdir)/SWISH-LIBRARY.1 : $(top_srcdir)/pod/SWISH-LIBRARY.pod
	-rm -f $@
	-pod2man --center="SWISH-E Documentation" --lax --release='$(VERSION)'  $<  > $@

$(srcdir)/SWISH-RUN.1 : $(top_srcdir)/pod/SWISH-RUN.pod
	-rm -f $@
	-pod2man --center="SWISH-E Documentation" --lax --release='$(VERSION)'  $<  > $@


EXTRA_DIST = $(man_MANS)
