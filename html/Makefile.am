pod_dir 	= $(top_srcdir)/pod
version_file 	= $(top_srcdir)/doc/Version.pm
timestamp 	= $(srcdir)/timestamp

htmldir 	= $(datadir)/doc/$(PACKAGE)/html
imagedir 	= $(datadir)/doc/$(PACKAGE)/html/images

if BUILDDOCS

# List of all files that are built by the doc system

# This is a pain because for non-GNU Make (like on BSD) must specify
# the srcdir for these targets, which means they must be separated by
# directory -- and that means need to write things twice below.  All has to do with VPATH...

targets = \
	$(srcdir)/CHANGES.html \
	$(srcdir)/INSTALL.html \
	$(srcdir)/README.html \
	$(srcdir)/SWISH-3.0.html \
	$(srcdir)/SWISH-BUGS.html \
	$(srcdir)/SWISH-CONFIG.html \
	$(srcdir)/SWISH-FAQ.html \
	$(srcdir)/SWISH-LIBRARY.html \
	$(srcdir)/SWISH-RUN.html \
	$(srcdir)/SWISH-SEARCH.html \
	$(srcdir)/API.html \
	$(srcdir)/spider.html \
	$(srcdir)/swish.html \
	$(srcdir)/Filter.html \
        $(srcdir)/style.css \
	$(srcdir)/index.html \
	$(srcdir)/index_long.html

image_targets = \
	$(srcdir)/images/dotrule1.gif \
	$(srcdir)/images/swish2b.gif \
	$(srcdir)/images/swish2.gif \
	$(srcdir)/images/swishbanner1.gif \
	$(srcdir)/images/swish.gif

endif

# These are the source files -- if any are newer than the timestamp
# then the docs need to be rebuilt

source_files = \
	$(pod_dir)/CHANGES.pod \
	$(pod_dir)/INSTALL.pod \
	$(pod_dir)/README.pod \
	$(pod_dir)/SWISH-3.0.pod \
	$(pod_dir)/SWISH-BUGS.pod \
	$(pod_dir)/SWISH-CONFIG.pod \
	$(pod_dir)/SWISH-FAQ.pod \
	$(pod_dir)/SWISH-LIBRARY.pod \
	$(pod_dir)/SWISH-RUN.pod \
	$(pod_dir)/SWISH-SEARCH.pod \
	$(top_srcdir)/perl/API.pm \
	$(top_srcdir)/prog-bin/spider.pl.in \
	$(top_srcdir)/example/swish.cgi.in \
	$(top_srcdir)/filters/SWISH/Filter.pm


# These are what get installed -- they will also be 
# a dependency of the all-am target (because of _DATA)

html_DATA  = $(targets)
image_DATA = $(image_targets)



# These are all placed in the distribution

EXTRA_DIST = $(targets) $(image_targets) $(source_files) $(timestamp)


## Here's the work 

if BUILDDOCS



# Deal with missing targets - which is the case in a fresh CVS checkout
# Automake automatically adds this target by the "html_DATA" entry above
# Touch the time-stamp, but make it very old so any source_file will be newer
# (is touch -t portable?)


$(targets) $(image_targets):
	@touch -t 197001010000 $(timestamp)



# Make sure the version is up to date -- the version is in configure.in

$(version_file): $(top_srcdir)/configure.in
	@echo "updating version file $(version_file) = $(VERSION)"
	@echo 'package SWISHE::Doc;$$VERSION=q[$(VERSION)];1' > $(version_file)
	@touch -t 197001010000 $(timestamp)


# Need some way to have make check the source files against the timestamp

all-local:  $(version_file) $(timestamp)





# If any source files are *newer* than the timestamp then
# run the "build_docs" program, and then update the timestamp

$(timestamp) : $(source_files)
	@(cd $(top_srcdir)/doc && bin/build -f)
	@touch $(timestamp)

endif
