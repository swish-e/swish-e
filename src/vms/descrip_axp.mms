# 
# Makefile derived from the Makefile coming with swish-e 1.3.2
# (original Makefile for SWISH Kevin Hughes, 3/12/95)
#
# The code has been tested to compile on OpenVMS 7.2-1
# JF. Piéronne jfp@altavista.net 6/11/00
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
CDEF = /def=(VMS,HAVE_CONFIG_H,STDC_HEADERS,"SWISH_VERSION=""2.1-dev-22""", -
	"XML_SetExternalEntityRefHandlerArg"="XML_SetExternalEntityRefHandArg")
CINCL= /include=([.expat.xmlparse],[.expat.xmltok])
CWARN=/warning=disable=(ZEROELEMENTS,PROTOSCOPE,OUTTYPELEN,PTRMISMATCH1,QUESTCOMPARE,LONGEXTERN)
#CDEBUG= /debug/noopt
CDEBUG=
CFLAGS = /prefix=all$(CINCL)$(CDEF)$(CWARN)$(CDEBUG)

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

OBJS=	check.obj file.obj index.obj search.obj error.obj methods.obj\
	hash.obj list.obj mem.obj string.obj merge.obj swish2.obj stemmer.obj \
	soundex.obj docprop.obj compress.obj xml.obj txt.obj \
	metanames.obj result_sort.obj html.obj search_alt.obj \
	filter.obj parse_conffile.obj result_output.obj date_time.obj \
	keychar_out.obj extprog.obj db.obj db_native.obj dump.obj \
	entities.obj no_better_place_module.obj swish_words.obj \
	proplimit.obj swish_qsort.obj ramdisk.obj \
	xmlparse.obj xmltok.obj xmlrole.obj \
	$(FILESYSTEM_OBJS) $(HTTP_OBJS) $(VMS_OBJS)

all :	acconfig.h $(NAME) swish-search.exe ! testlib
	!

xmlparse.obj : [.expat.xmlparse]xmlparse.c

xmltok.obj : [.expat.xmltok]xmltok.c

xmlrole.obj : [.expat.xmltok]xmlrole.c

$(NAME) : $(OBJS) libswish-e.olb swish.obj
        link/exe=$(MMS$TARGET) $(LINKFLAGS) swish.obj, libswish-e.olb/lib

testlib : testlib.exe
	!

testlib.exe : testlib.obj libswish-e.olb swish.obj
        link/exe=$(MMS$TARGET) $(LINKFLAGS) testlib.obj, libswish-e.olb/lib

libswish-e.olb : $(OBJS)
	library/create $(MMS$TARGET) $(MMS$SOURCE_LIST)

swish-search.exe : $(NAME)
	copy $(NAME) swish-search.exe

regex.obj : [.vms]regex.c [.vms]descrip.mms

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


$(OBJS) :	[.vms]descrip.mms config.h swish.h acconfig.h

swish.obj :	[.vms]descrip.mms config.h swish.h acconfig.h

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
 error.h search.h string.h docprop.h compress.h
error.obj : error.c swish.h config.h error.h
file.obj : file.c swish.h config.h file.h mem.h string.h error.h list.h \
 hash.h index.h
fs.obj : fs.c swish.h config.h index.h hash.h mem.h file.h string.h \
 list.h
hash.obj : hash.c swish.h config.h hash.h mem.h string.h
http.obj : http.c swish.h config.h index.h hash.h string.h mem.h file.h \
 http.h httpserver.h
httpserver.obj : httpserver.c swish.h config.h string.h mem.h http.h \
 httpserver.h
index.obj : index.c swish.h config.h index.h hash.h mem.h string.h \
 check.h search.h docprop.h stemmer.h compress.h
list.obj : list.c swish.h config.h list.h mem.h string.h
mem.obj : mem.c swish.h config.h mem.h error.h
merge.obj : merge.c swish.h config.h merge.h error.h search.h index.h \
 string.h hash.h mem.h docprop.h compress.h
methods.obj : methods.c swish.h config.h
search.obj : search.c swish.h config.h search.h file.h list.h string.h \
 merge.h hash.h mem.h docprop.h stemmer.h compress.h
stemmer.obj : stemmer.c swish.h config.h stemmer.h
soundex.obj : soundex.c swish.h config.h stemmer.h
string.obj : string.c swish.h config.h string.h mem.h
swish2.obj : swish2.c swish.h config.h error.h list.h search.h index.h \
 string.h file.h merge.h docprop.h
swish.obj : swish.c swish.h config.h error.h list.h search.h index.h \
 string.h file.h merge.h docprop.h
testlib.obj : testlib.c swish.h config.h error.h list.h search.h index.h \
 string.h file.h merge.h docprop.h
txt.obj : txt.c txt.h swish.h mem.h string.h index.h
xml.obj : xml.c txt.h swish.h mem.h string.h index.h
proplimi.obj : swish.h string.h mem.h merge.h docprop.h index.h metanames.h \
 compress.h error.h db.h result_sort.h swish_qsort.h proplimit.h

