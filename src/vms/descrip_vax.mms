# 
# Makefile derived from the Makefile coming with swish-e 1.3.2
# (original Makefile for SWISH Kevin Hughes, 3/12/95)
#
# The code has been tested to compile on OpenVMS 7.3
# JF. Piéronne jf.pieronne@laposte.net 29-Mar-2003
# 
# autoconf configuration by Bas Meijer, 1 June 2000
# Cross Platform Compilation on Solaris, HP-UX, IRIX and Linux
# Several ideas from a Makefile by Christian Lindig <lindig@ips.cs.tu-bs.de>  
#
NAME = swish-e.exe
# C compiler
CC = CC

SHELL = /bin/sh
prefix = @prefix@
bindir = $(prefix)/bin
mandir = $(prefix)/man
man1dir = $(mandir)/man1

# Flags for C compiler
#CWARN=
CDEF = /def=(VMS,HAVE_CONFIG_H,STDC_HEADERS, REGEX_MALLOC)
CINCL= /include=([.expat.xmlparse],[.expat.xmltok],libz:)
CWARN=
#CDEBUG= /debug/noopt
CDEBUG=
CFLAGS = /prefix=all$(CINCL)$(CDEF)$(CWARN)$(CDEBUG)/name=short

#LINKFLAGS = /debug
LINKFLAGS =
LIBS=

#
# The objects for the different methods and
# some common aliases
#

FILESYSTEM_OBJS=fs.obj
HTTP_OBJS=http.obj httpserver.obj
FS_OBJS=$(FILESYSTEM_OBJS)
WEB_OBJS=$(HTTP_OBJS)
VMS_OBJS = regex.obj
VSNPRINTF_OBJ = vsnprintf.obj

OBJS=	check.obj file.obj index.obj search.obj error.obj methods.obj\
	hash.obj list.obj mem.obj merge.obj swish2.obj stemmer.obj \
	soundex.obj docprop.obj compress.obj xml.obj txt.obj \
	metanames.obj result_sort.obj html.obj \
	filter.obj parse_conffile.obj result_output.obj date_time.obj \
	keychar_out.obj extprog.obj db_native.obj dump.obj \
	entities.obj swish_words.obj \
	proplimit.obj swish_qsort.obj ramdisk.obj rank.obj \
	xmlparse.obj xmltok.obj xmlrole.obj swregex.obj vsnprintf.obj \
        double_metaphone.obj db_read.obj db_write.obj swstring.obj \
	pre_sort.obj headers.obj docprop_write.obj stemmer.obj\
	$(FILESYSTEM_OBJS) $(HTTP_OBJS) $(VMS_OBJS)\
        api.obj stem_de.obj stem_dk.obj stem_en1.obj stem_en2.obj stem_es.obj\
	stem_fi.obj stem_fr.obj stem_it.obj stem_nl.obj stem_no.obj \
	stem_pt.obj stem_ru.obj stem_se.obj utilities.obj

all :	acconfig.h $(NAME) swish-search.exe ! libtest.exe
	!

xmlparse.obj : [.expat.xmlparse]xmlparse.c

xmltok.obj : [.expat.xmltok]xmltok.c

xmlrole.obj : [.expat.xmltok]xmlrole.c

$(NAME) : $(OBJS) libswish-e.olb swish.obj
        link/exe=$(MMS$TARGET) $(LINKFLAGS) -
		swish.obj, libswish-e.olb/lib, libz:libz.olb/lib

libtest.exe : libtest.obj libswish-e.olb swish.obj
        link/exe=$(MMS$TARGET) $(LINKFLAGS) libtest.obj, libswish-e.olb/lib

libswish-e.olb : $(OBJS)
	library/create $(MMS$TARGET) $(MMS$SOURCE_LIST)

swish-search.exe : $(NAME)
	copy $(NAME) swish-search.exe

regex.obj : [.vms]regex.c [.vms]descrip_vax.mms

acconfig.h : [.vms]acconfig.h_vms
	copy $(MMS$SOURCE) $(MMS$TARGET)

clean :
	delete [...]*.obj;*, [...]*.olb;*, index.swish;*, [-.tests]*.index;*

realclean :
	pur [-...]
	delete [...]*.exe;*, [...]*.obj;*, [...]*.olb;*, index.swish;*, acconfig.h;*, [-.tests]*.index;*

test :	$(NAME)
	set def [-.tests]
	mc [-.src]swish-e -c test.config
	write sys$output "test 1 (Normal search) ..."
	mc [-.src]swish-e -f test.index -w test
	write sys$output "test 1 (MetaTag search 1) ..."
	mc [-.src]swish-e -f  test.index -w meta1=metatest1
	write sys$output "test 1 (MetaTag search 2) ..."
	mc [-.src]swish-e -f test.index -w meta2=metatest2
	write sys$output "test 1 (XML search) ..."
	mc [-.src]swish-e -f ./test.index -w meta3=metatest3
	write sys$output "test 1 (Phrase search) ..." 
	mc [-.src]swish-e -f test.index -w """three little pigs"""


$(OBJS) :	[.vms]descrip_vax.mms config.h swish.h acconfig.h

swish.obj :	[.vms]descrip_vax.mms config.h swish.h acconfig.h

install :	
	!

man :
	!

#
# dependencies
# 
check.obj : check.c swish.h config.h check.h hash.h
compress.obj : compress.c swish.h config.h error.h mem.h docprop.h index.h search.h merge.h compress.h
deflate.obj : deflate.c swish.h config.h error.h mem.h docprop.h index.h search.h merge.h deflate.h
docprop.obj : docprop.c swish.h config.h file.h hash.h mem.h merge.h \
 error.h search.h docprop.h compress.h
error.obj : error.c swish.h config.h error.h
file.obj : file.c swish.h config.h file.h mem.h error.h list.h \
 hash.h index.h
fs.obj : fs.c swish.h config.h index.h hash.h mem.h file.h \
 list.h
hash.obj : hash.c swish.h config.h hash.h mem.h
http.obj : http.c swish.h config.h index.h hash.h mem.h file.h \
 http.h httpserver.h
httpserver.obj : httpserver.c swish.h config.h mem.h http.h \
 httpserver.h
index.obj : index.c swish.h config.h index.h hash.h mem.h \
 check.h search.h docprop.h stemmer.h compress.h
list.obj : list.c swish.h config.h list.h mem.h
mem.obj : mem.c swish.h config.h mem.h error.h
merge.obj : merge.c swish.h config.h merge.h error.h search.h index.h \
 hash.h mem.h docprop.h compress.h
methods.obj : methods.c swish.h config.h
search.obj : search.c swish.h config.h search.h file.h list.h \
 merge.h hash.h mem.h docprop.h stemmer.h compress.h
stemmer.obj : stemmer.c swish.h config.h stemmer.h
soundex.obj : soundex.c swish.h config.h stemmer.h
swish2.obj : swish2.c swish.h config.h error.h list.h search.h index.h \
 file.h merge.h docprop.h
swish.obj : swish.c swish.h config.h error.h list.h search.h index.h \
 file.h merge.h docprop.h
libtest.obj : libtest.c swish.h config.h error.h list.h search.h index.h \
 file.h merge.h docprop.h
txt.obj : txt.c txt.h swish.h mem.h index.h
xml.obj : xml.c txt.h swish.h mem.h index.h
proplimi.obj : swish.h mem.h merge.h docprop.h index.h metanames.h \
 compress.h error.h db.h result_sort.h swish_qsort.h proplimit.h
metanames.obj : metanames.c
result_sort.obj : result_sort.c
html.obj : html.c
filter.obj : filter.c
parse_conffile.obj : parse_conffile.c
result_output.obj : result_output.c
date_time.obj : date_time.c
keychar_out.obj : keychar_out.c
extprog.obj : extprog.c
db_native.obj : db_native.c
dump.obj : dump.c
entities.obj : entities.c
swish_words.obj : swish_words.c
proplimit.obj : proplimit.c
swish_qsort.obj : swish_qsort.c
ramdisk.obj : ramdisk.c
rank.obj : rank.c
swregex.obj : swregex.c
double_metaphone.obj : double_metaphone.c
vsnprintf.obj : [.replace]vsnprintf.c
db_read.obj : db_read.c
db_write.obj : db_write.c
swstring.obj : swstring.c
pre_sort.obj : pre_sort.c
hearders.obj : headers.c
docprop_write.obj : docprop_write.c

api.obj : [.snowball]api.c
stem_de.obj : [.snowball]stem_de.c
stem_dk.obj : [.snowball]stem_dk.c
stem_en1.obj : [.snowball]stem_en1.c
stem_en2.obj : [.snowball]stem_en2.c
stem_es.obj : [.snowball]stem_es.c
stem_fi.obj : [.snowball]stem_fi.c
stem_fr.obj : [.snowball]stem_fr.c
stem_it.obj : [.snowball]stem_it.c
stem_nl.obj : [.snowball]stem_nl.c
stem_no.obj : [.snowball]stem_no.c
stem_pt.obj : [.snowball]stem_pt.c
stem_ru.obj : [.snowball]stem_ru.c
stem_se.obj : [.snowball]stem_se.c
utilities.obj : [.snowball]utilities.c
