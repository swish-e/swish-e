SUBDIRS = expat replace snowball

AM_CPPFLAGS = -Dlibexecdir=\"${libexecdir}\"  \
		-DPATH_SEPARATOR=\"${PATH_SEPARATOR}\" \
		$(Z_CFLAGS) $(PCRE_CFLAGS) $(LIBXML2_CFLAGS) -Ireplace

bin_PROGRAMS = swish-e
swish_e_SOURCES = swish.c swish.h  keychar_out.c keychar_out.h dump.c dump.h result_output.c result_output.h
swish_e_LDADD = libswishindex.la libswish-e.la

EXTRA_PROGRAMS = libtest
libtest_SOURCES = libtest.c 
libtest_LDADD = libswish-e.la
libtest_LDFLAGS = -static
## can also use -all-static for a stand-alone binary.



## The search library 
## -- note that libreplace may have code specific for indexing only.
## -- will that be a problem on systems?

lib_LTLIBRARIES = libswish-e.la
libswish_e_la_LDFLAGS = -version-info 2:0:0 $(Z_LIBS) $(PCRE_LIBS)
libswish_e_la_LIBADD = $(BTREE_OBJS) replace/libreplace.la snowball/libsnowball.la

libswish_e_la_DEPENDENCIES = $(libswish_e_la_LIBADD)



libswish_e_la_SOURCES =  \
	config.h \
	search.c search.h \
	swish2.c  \
	swish_words.c swish_words.h \
	proplimit.c proplimit.h \
        rank.c rank.h \
	db_read.c db.h \
	result_sort.c result_sort.h \
	hash.c hash.h \
	compress.c compress.h \
	db_native.c db_native.h \
	ramdisk.c ramdisk.h \
	check.c check.h \
	error.c error.h \
	list.c list.h \
	mem.c mem.h sys.h\
	swstring.c swstring.h \
	docprop.c docprop.h \
	metanames.c metanames.h \
	headers.c headers.h \
	swish_qsort.c swish_qsort.h \
	date_time.c date_time.h \
	double_metaphone.c double_metaphone.h \
	stemmer.c stemmer.h \
	soundex.c soundex.h

EXTRA_libswish_e_la_SOURCES =  \
	btree.c btree.h \
	array.c array.h \
	worddata.c worddata.h \
	fhash.c fhash.h

## Convenience lib for indexing code

noinst_LTLIBRARIES = libswishindex.la
libswishindex_la_LIBADD =  expat/libswexpat.la $(LIBXML2_OBJS) snowball/libsnowball.la
libswishindex_la_LDFLAGS = $(LIBXML2_LIB) $(Z_LIBS) $(PCRE_LIBS) 

## in case these change
libswishindex_la_DEPENDENCIES = $(libswishindex_la_LIBADD)

EXTRA_libswishindex_la_SOURCES = parser.c parser.h

libswishindex_la_SOURCES = \
	fs.c fs.h \
	http.c http.h \
	httpserver.c httpserver.h \
	extprog.c extprog.h \
        bash.c bash.h \
	methods.c \
	html.c html.h \
	txt.c txt.h \
	xml.c xml.h \
	entities.c entities.h \
	index.c index.h \
	merge.c merge.h \
	pre_sort.c \
	file.c file.h \
        filter.c filter.h \
	parse_conffile.c parse_conffile.h \
	swregex.c swregex.h \
	db_write.c  \
	docprop_write.c \
        getruntime.c getruntime.h




include_HEADERS = swish-e.h


libexec_SCRIPTS = swishspider

EXTRA_DIST = swishspider
