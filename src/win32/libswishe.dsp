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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "../../../pcre/include" /I "../../../zlib/include" /I "..\replace" /D "HAVE_PCRE" /D "HAVE_CONFIG_H" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_ZLIB" /D "HAVE_LIBXML2" /YX /FD /c
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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "." /I "../../../pcre/include" /I "../../../zlib/include" /I "..\replace" /D "HAVE_PCRE" /D "HAVE_CONFIG_H" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_ZLIB" /D "HAVE_LIBXML2" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"tmp/libswishe_Debug\libswish-e.lib"

!ENDIF 

# Begin Target

# Name "libswishe - Win32 Release"
# Name "libswishe - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\snowball\api.c
# End Source File
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

SOURCE=..\db_native.c
# End Source File
# Begin Source File

SOURCE=..\db_read.c
# End Source File
# Begin Source File

SOURCE=..\docprop.c
# End Source File
# Begin Source File

SOURCE=..\double_metaphone.c
# End Source File
# Begin Source File

SOURCE=..\error.c
# End Source File
# Begin Source File

SOURCE=..\hash.c
# End Source File
# Begin Source File

SOURCE=..\headers.c
# End Source File
# Begin Source File

SOURCE=..\list.c
# End Source File
# Begin Source File

SOURCE=..\mem.c
# End Source File
# Begin Source File

SOURCE=..\metanames.c
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

SOURCE=..\result_sort.c
# End Source File
# Begin Source File

SOURCE=..\search.c
# End Source File
# Begin Source File

SOURCE=..\soundex.c
# End Source File
# Begin Source File

SOURCE=..\snowball\stem_de.c
# End Source File
# Begin Source File

SOURCE=..\snowball\stem_dk.c
# End Source File
# Begin Source File

SOURCE=..\snowball\stem_en1.c
# End Source File
# Begin Source File

SOURCE=..\snowball\stem_en2.c
# End Source File
# Begin Source File

SOURCE=..\snowball\stem_es.c
# End Source File
# Begin Source File

SOURCE=..\snowball\stem_fi.c
# End Source File
# Begin Source File

SOURCE=..\snowball\stem_fr.c
# End Source File
# Begin Source File

SOURCE=..\snowball\stem_it.c
# End Source File
# Begin Source File

SOURCE=..\snowball\stem_nl.c
# End Source File
# Begin Source File

SOURCE=..\snowball\stem_no.c
# End Source File
# Begin Source File

SOURCE=..\snowball\stem_pt.c
# End Source File
# Begin Source File

SOURCE=..\snowball\stem_ru.c
# End Source File
# Begin Source File

SOURCE=..\snowball\stem_se.c
# End Source File
# Begin Source File

SOURCE=..\stemmer.c
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

SOURCE=..\swstring.c
# End Source File
# Begin Source File

SOURCE=..\snowball\utilities.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
