# Microsoft Developer Studio Project File - Name="dllswish" - Package Owner=<4>


# Microsoft Developer Studio Generated Build File, Format Version 6.00


# ** DO NOT EDIT **





# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102





CFG=dllswish - Win32 Debug


!MESSAGE This is not a valid makefile. To build this project using NMAKE,


!MESSAGE use the Export Makefile command and run


!MESSAGE 


!MESSAGE NMAKE /f "dllswishe.mak".


!MESSAGE 


!MESSAGE You can specify a configuration when running NMAKE


!MESSAGE by defining the macro CFG on the command line. For example:


!MESSAGE 


!MESSAGE NMAKE /f "dllswishe.mak" CFG="dllswish - Win32 Debug"


!MESSAGE 


!MESSAGE Possible choices for configuration are:


!MESSAGE 


!MESSAGE "dllswish - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")


!MESSAGE "dllswish - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")


!MESSAGE 





# Begin Project


# PROP AllowPerConfigDependencies 0


# PROP Scc_ProjName ""


# PROP Scc_LocalPath ""


CPP=cl.exe


MTL=midl.exe


RSC=rc.exe





!IF  "$(CFG)" == "dllswish - Win32 Release"





# PROP BASE Use_MFC 0


# PROP BASE Use_Debug_Libraries 0


# PROP BASE Output_Dir "Release"


# PROP BASE Intermediate_Dir "Release"


# PROP BASE Target_Dir ""


# PROP Use_MFC 0


# PROP Use_Debug_Libraries 0


# PROP Output_Dir "tmp/dllswisheRelease"


# PROP Intermediate_Dir "tmp/dllswisheRelease"


# PROP Ignore_Export_Lib 0


# PROP Target_Dir ""


# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DLLSWISH_EXPORTS" /Yu"stdafx.h" /FD /c


# ADD CPP /nologo /MT /W3 /GX /O2 /I "." /I "..\..\..\expat\xmlparse" /I "../../../libxml2/include" /I "..\..\..\expat\xmltok" /I "../../../pcre/include" /I "../../../zlib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DLLSWISH_EXPORTS" /D "HAVE_PCRE" /D "HAVE_CONFIG_H" /D "HAVE_ZLIB" /FD /c


# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32


# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32


# ADD BASE RSC /l 0x409 /d "NDEBUG"


# ADD RSC /l 0x409 /d "NDEBUG"


BSC32=bscmake.exe


# ADD BASE BSC32 /nologo


# ADD BSC32 /nologo


LINK32=link.exe


# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386


# ADD LINK32 kernel32.lib ../../../expat/xmltok/Debug/xmltok.lib ../../../expat/xmlparse/Debug/xmlparse.lib ../../../libxml2/win32/dsp/libxml2/libxml2.lib ../../../pcre/lib/libpcre.lib ../../../zlib/zlib.lib /nologo /dll /machine:I386 /out:"dllswish-e.dll" /implib:"dllswish-e.lib"


# SUBTRACT LINK32 /pdb:none





!ELSEIF  "$(CFG)" == "dllswish - Win32 Debug"





# PROP BASE Use_MFC 0


# PROP BASE Use_Debug_Libraries 1


# PROP BASE Output_Dir "Debug"


# PROP BASE Intermediate_Dir "Debug"


# PROP BASE Target_Dir ""


# PROP Use_MFC 0


# PROP Use_Debug_Libraries 1


# PROP Output_Dir "tmp/dllswisheDebug"


# PROP Intermediate_Dir "tmp/dllswisheDebug"


# PROP Ignore_Export_Lib 0


# PROP Target_Dir ""


# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DLLSWISH_EXPORTS" /Yu"stdafx.h" /FD /GZ /c


# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "." /I "..\..\..\expat\xmlparse" /I "../../../libxml2/include" /I "..\..\..\expat\xmltok" /I "../../../pcre/include" /I "../../../zlib" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DLLSWISH_EXPORTS" /D "HAVE_PCRE" /D "HAVE_CONFIG_H" /D "HAVE_ZLIB" /FD /GZ /c


# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32


# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32


# ADD BASE RSC /l 0x409 /d "_DEBUG"


# ADD RSC /l 0x409 /d "_DEBUG"


BSC32=bscmake.exe


# ADD BASE BSC32 /nologo


# ADD BSC32 /nologo


LINK32=link.exe


# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept


# ADD LINK32 kernel32.lib ../../../expat/xmltok/Debug/xmltok.lib ../../../expat/xmlparse/Debug/xmlparse.lib ../../../libxml2/win32/dsp/libxml2/libxml2.lib ../../../pcre/lib/libpcre.lib ../../../zlib/zlib.lib /nologo /dll /debug /machine:I386 /out:"tmp/dllswisheDebug/dllswish-e.dll" /pdbtype:sept





!ENDIF 





# Begin Target





# Name "dllswish - Win32 Release"


# Name "dllswish - Win32 Debug"


# Begin Group "Source Files"





# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"


# Begin Source File





SOURCE=..\array.c


# End Source File


# Begin Source File





SOURCE=..\btree.c


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





SOURCE=..\db_write.c


# End Source File


# Begin Source File





SOURCE=.\dirent.c


# End Source File


# Begin Source File





SOURCE=".\dllswish-e.def"


# End Source File


# Begin Source File





SOURCE=..\docprop.c


# End Source File


# Begin Source File





SOURCE=..\docprop_write.c


# End Source File


# Begin Source File





SOURCE=..\double_metaphone.c


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





SOURCE=..\fhash.c


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





SOURCE=..\headers.c


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





SOURCE=..\mkstemp.c


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





SOURCE=..\pre_sort.c


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





SOURCE=..\worddata.c


# End Source File


# Begin Source File





SOURCE=..\xml.c


# End Source File


# End Group


# Begin Group "Header Files"





# PROP Default_Filter "h;hpp;hxx;hm;inl"


# Begin Source File





SOURCE=..\acconfig.h


# End Source File


# Begin Source File





SOURCE=..\array.h


# End Source File


# Begin Source File





SOURCE=..\btree.h


# End Source File


# Begin Source File





SOURCE=..\check.h


# End Source File


# Begin Source File





SOURCE=..\compress.h


# End Source File


# Begin Source File





SOURCE=..\config.h


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





SOURCE=..\fhash.h


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





SOURCE=..\headers.h


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





SOURCE=..\worddata.h


# End Source File


# Begin Source File





SOURCE=..\xml.h


# End Source File


# End Group


# Begin Group "Resource Files"





# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"


# End Group


# End Target


# End Project


