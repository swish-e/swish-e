man_MANS = \
    SWISH-CONFIG.1 \
    SWISH-FAQ.1 \
    SWISH-LIBRARY.1 \
    SWISH-RUN.1



SWISH-CONFIG.1: $(srcdir)/../pod/SWISH-CONFIG.pod
	-rm -f $@
	-pod2man --center="SWISH-E Documentation" --lax --release='$(VERSION)'  $(srcdir)/../pod/SWISH-CONFIG.pod > $(top_srcdir)/man/SWISH-CONFIG.1


SWISH-FAQ.1: $(srcdir)/../pod/SWISH-FAQ.pod
	-rm -f $@
	-pod2man --center="SWISH-E Documentation" --lax --release='$(VERSION)'  $(srcdir)/../pod/SWISH-FAQ.pod > $(top_srcdir)/man/SWISH-FAQ.1


SWISH-LIBRARY.1: $(srcdir)/../pod/SWISH-LIBRARY.pod
	-rm -f $@
	-pod2man --center="SWISH-E Documentation" --lax --release='$(VERSION)'  $(srcdir)/../pod/SWISH-LIBRARY.pod > $(top_srcdir)/man/SWISH-LIBRARY.1


SWISH-RUN.1: $(srcdir)/../pod/SWISH-RUN.pod
	-rm -f $@
	-pod2man --center="SWISH-E Documentation" --lax --release='$(VERSION)'  $(srcdir)/../pod/SWISH-RUN.pod > $(top_srcdir)/man/SWISH-RUN.1

EXTRA_DIST = $(man_MANS)
