# Microsoft Developer Studio Project File - Name="libswishe" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libswishe - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libswishe.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libswishe.mak" CFG="libswishe - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libswishe - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libswishe - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libswishe - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "tmp/libswishe_Release"
# PROP Intermediate_Dir "tmp/libswishe_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "..\expat\xmlparse" /I "..\expat\xmltok" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\..\zlib" /I "..\expat\xmlparse" /I "..\expat\xmltok" /I "../../../libxml2/include" /I "../../../pcre/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_ZLIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"libswish-e.lib"

!ELSEIF  "$(CFG)" == "libswishe - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "tmp/libswishe_Debug"
# PROP Intermediate_Dir "tmp/libswishe_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\expat\xmlparse" /I "..\expat/xmltok" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\expat\xmlparse" /I "../../../libxml2/include" /I "../../../pcre/include" /I "..\expat/xmltok" /D "_LIB" /D "_DEBUG" /D "_MBCS" /D "WIN32" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"libswish-e.lib"

!ENDIF 

# Begin Target

# Name "libswishe - Win32 Release"
# Name "libswishe - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\check.c
# End Source File
# Begin Source File

SOURCE=..\compress.c
# End Source File
# Begin Source File

SOURCE=..\date_time.c
# End Source File
# Begin Source File

SOURCE=..\db.c
# End Source File
# Begin Source File

SOURCE=..\db_native.c
# End Source File
# Begin Source File

SOURCE=.\dirent.c
# End Source File
# Begin Source File

SOURCE=..\docprop.c
# End Source File
# Begin Source File

SOURCE=..\dump.c
# End Source File
# Begin Source File

SOURCE=..\entities.c
# End Source File
# Begin Source File

SOURCE=..\error.c
# End Source File
# Begin Source File

SOURCE=..\extprog.c
# End Source File
# Begin Source File

SOURCE=..\file.c
# End Source File
# Begin Source File

SOURCE=..\filter.c
# End Source File
# Begin Source File

SOURCE=..\fs.c
# End Source File
# Begin Source File

SOURCE=..\hash.c
# End Source File
# Begin Source File

SOURCE=..\html.c
# End Source File
# Begin Source File

SOURCE=..\http.c
# End Source File
# Begin Source File

SOURCE=..\httpserver.c
# End Source File
# Begin Source File

SOURCE=..\index.c
# End Source File
# Begin Source File

SOURCE=..\keychar_out.c
# End Source File
# Begin Source File

SOURCE=..\list.c
# End Source File
# Begin Source File

SOURCE=..\mem.c
# End Source File
# Begin Source File

SOURCE=..\merge.c
# End Source File
# Begin Source File

SOURCE=..\metanames.c
# End Source File
# Begin Source File

SOURCE=..\methods.c
# End Source File
# Begin Source File

SOURCE=.\mkstemp.c
# End Source File
# Begin Source File

SOURCE=..\no_better_place_module.c
# End Source File
# Begin Source File

SOURCE=..\parse_conffile.c
# End Source File
# Begin Source File

SOURCE=..\parser.c
# End Source File
# Begin Source File

SOURCE=..\proplimit.c
# End Source File
# Begin Source File

SOURCE=..\ramdisk.c
# End Source File
# Begin Source File

SOURCE=..\rank.c
# End Source File
# Begin Source File

SOURCE=..\result_output.c
# End Source File
# Begin Source File

SOURCE=..\result_sort.c
# End Source File
# Begin Source File

SOURCE=..\search.c
# End Source File
# Begin Source File

SOURCE=..\search_alt.c
# End Source File
# Begin Source File

SOURCE=..\soundex.c
# End Source File
# Begin Source File

SOURCE=..\stemmer.c
# End Source File
# Begin Source File

SOURCE=..\string.c
# End Source File
# Begin Source File

SOURCE=..\swish2.c
# End Source File
# Begin Source File

SOURCE=..\swish_qsort.c
# End Source File
# Begin Source File

SOURCE=..\swish_words.c
# End Source File
# Begin Source File

SOURCE=..\swregex.c
# End Source File
# Begin Source File

SOURCE=..\txt.c
# End Source File
# Begin Source File

SOURCE=..\xml.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\_datatypes.h
# End Source File
# Begin Source File

SOURCE=..\_module_example.h
# End Source File
# Begin Source File

SOURCE=..\acconfig.h
# End Source File
# Begin Source File

SOURCE=..\check.h
# End Source File
# Begin Source File

SOURCE=..\compress.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=..\date_time.h
# End Source File
# Begin Source File

SOURCE=..\db.h
# End Source File
# Begin Source File

SOURCE=..\db_berkeley_db.h
# End Source File
# Begin Source File

SOURCE=..\db_gdbm.h
# End Source File
# Begin Source File

SOURCE=..\db_native.h
# End Source File
# Begin Source File

SOURCE=..\deflate.h
# End Source File
# Begin Source File

SOURCE=.\dirent.h
# End Source File
# Begin Source File

SOURCE=..\docprop.h
# End Source File
# Begin Source File

SOURCE=..\dump.h
# End Source File
# Begin Source File

SOURCE=..\entities.h
# End Source File
# Begin Source File

SOURCE=..\error.h
# End Source File
# Begin Source File

SOURCE=..\extprog.h
# End Source File
# Begin Source File

SOURCE=..\file.h
# End Source File
# Begin Source File

SOURCE=..\filter.h
# End Source File
# Begin Source File

SOURCE=..\fs.h
# End Source File
# Begin Source File

SOURCE=..\hash.h
# End Source File
# Begin Source File

SOURCE=..\html.h
# End Source File
# Begin Source File

SOURCE=..\http.h
# End Source File
# Begin Source File

SOURCE=..\httpserver.h
# End Source File
# Begin Source File

SOURCE=..\index.h
# End Source File
# Begin Source File

SOURCE=..\keychar_out.h
# End Source File
# Begin Source File

SOURCE=..\list.h
# End Source File
# Begin Source File

SOURCE=..\mem.h
# End Source File
# Begin Source File

SOURCE=..\merge.h
# End Source File
# Begin Source File

SOURCE=..\metanames.h
# End Source File
# Begin Source File

SOURCE=.\mkstemp.h
# End Source File
# Begin Source File

SOURCE=..\modules.h
# End Source File
# Begin Source File

SOURCE=..\no_better_place_module.h
# End Source File
# Begin Source File

SOURCE=..\parse_conffile.h
# End Source File
# Begin Source File

SOURCE=..\parser.h
# End Source File
# Begin Source File

SOURCE=..\proplimit.h
# End Source File
# Begin Source File

SOURCE=..\ramdisk.h
# End Source File
# Begin Source File

SOURCE=..\rank.h
# End Source File
# Begin Source File

SOURCE=..\result_output.h
# End Source File
# Begin Source File

SOURCE=..\result_sort.h
# End Source File
# Begin Source File

SOURCE=..\search.h
# End Source File
# Begin Source File

SOURCE=..\search_alt.h
# End Source File
# Begin Source File

SOURCE=..\soundex.h
# End Source File
# Begin Source File

SOURCE=..\stemmer.h
# End Source File
# Begin Source File

SOURCE=..\string.h
# End Source File
# Begin Source File

SOURCE=..\swish.h
# End Source File
# Begin Source File

SOURCE=..\swish_qsort.h
# End Source File
# Begin Source File

SOURCE=..\swish_words.h
# End Source File
# Begin Source File

SOURCE=..\swregex.h
# End Source File
# Begin Source File

SOURCE=..\txt.h
# End Source File
# Begin Source File

SOURCE=..\xml.h
# End Source File
# Begin Source File

SOURCE=..\expat\xmlparse\xmlparse.h
# End Source File
# End Group
# End Target
# End Project
