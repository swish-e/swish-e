
# $id$
#
# Conditionally install the pod docs


# Where docs are installed

poddir = $(datadir)/doc/$(PACKAGE)/pod

if BUILDDOCS
pod_DATA = \
    $(pod_files)

endif

pod_files = \
    CHANGES.pod \
    INSTALL.pod \
    README.pod \
    SWISH-3.0.pod \
    SWISH-BUGS.pod \
    SWISH-CONFIG.pod \
    swish-e.pod \
    SWISH-FAQ.pod \
    SWISH-LIBRARY.pod \
    SWISH-RUN.pod \
    SWISH-SEARCH.pod


EXTRA_DIST = \
    $(pod_files)
