# Microsoft Developer Studio Project File - Name="libswishindex" - Package Owner=<4>

# Microsoft Developer Studio Generated Build File, Format Version 6.00

# ** DO NOT EDIT **



# TARGTYPE "Win32 (x86) Static Library" 0x0104



CFG=libswishindex - Win32 Debug

!MESSAGE This is not a valid makefile. To build this project using NMAKE,

!MESSAGE use the Export Makefile command and run

!MESSAGE 

!MESSAGE NMAKE /f "libswishindex.mak".

!MESSAGE 

!MESSAGE You can specify a configuration when running NMAKE

!MESSAGE by defining the macro CFG on the command line. For example:

!MESSAGE 

!MESSAGE NMAKE /f "libswishindex.mak" CFG="libswishindex - Win32 Debug"

!MESSAGE 

!MESSAGE Possible choices for configuration are:

!MESSAGE 

!MESSAGE "libswishindex - Win32 Release" (based on "Win32 (x86) Static Library")

!MESSAGE "libswishindex - Win32 Debug" (based on "Win32 (x86) Static Library")

!MESSAGE 



# Begin Project

# PROP AllowPerConfigDependencies 0

# PROP Scc_ProjName ""

# PROP Scc_LocalPath ""

CPP=cl.exe

RSC=rc.exe



!IF  "$(CFG)" == "libswishindex - Win32 Release"



# PROP BASE Use_MFC 0

# PROP BASE Use_Debug_Libraries 0

# PROP BASE Output_Dir "libswishindex___Win32_Release"

# PROP BASE Intermediate_Dir "libswishindex___Win32_Release"

# PROP BASE Target_Dir ""

# PROP Use_MFC 0

# PROP Use_Debug_Libraries 0

# PROP Output_Dir "tmp/libswishindex_Release"

# PROP Intermediate_Dir "tmp/libswishindex_Release"

# PROP Target_Dir ""

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c

# ADD CPP /nologo /W3 /GX /O2 /I "." /I "../replace" /I "../../../pcre/include" /I "../../../libxml2/include" /I "../../../zlib/include" /I ".." /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_PCRE" /D "HAVE_LIBXML2" /D "HAVE_ZLIB" /D "HAVE_CONFIG_H" /YX /FD /c

# ADD BASE RSC /l 0x409 /d "NDEBUG"

# ADD RSC /l 0x409 /d "NDEBUG"

BSC32=bscmake.exe

# ADD BASE BSC32 /nologo

# ADD BSC32 /nologo

LIB32=link.exe -lib

# ADD BASE LIB32 /nologo

# ADD LIB32 /nologo /out:"libswishindex.lib"



!ELSEIF  "$(CFG)" == "libswishindex - Win32 Debug"



# PROP BASE Use_MFC 0

# PROP BASE Use_Debug_Libraries 1

# PROP BASE Output_Dir "libswishindex___Win32_Debug"

# PROP BASE Intermediate_Dir "libswishindex___Win32_Debug"

# PROP BASE Target_Dir ""

# PROP Use_MFC 0

# PROP Use_Debug_Libraries 1

# PROP Output_Dir "libswishindex___Win32_Debug"

# PROP Intermediate_Dir "libswishindex___Win32_Debug"

# PROP Target_Dir ""

# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c

# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "." /I "../replace" /I "../../../pcre/include" /I "../../../libxml2/include" /I "../../../zlib/include" /I ".." /D "WIN32" /D "_MBCS" /D "_LIB" /D "HAVE_PCRE" /D "HAVE_LIBXML2" /D "HAVE_ZLIB" /D "HAVE_CONFIG_H" /D "_DEBUG" /YX /FD /GZ /c

# ADD BASE RSC /l 0x409 /d "_DEBUG"

# ADD RSC /l 0x409 /d "_DEBUG"

BSC32=bscmake.exe

# ADD BASE BSC32 /nologo

# ADD BSC32 /nologo /o"tmp/libswishindex___Win32_Debug/libswishindex.bsc"

LIB32=link.exe -lib

# ADD BASE LIB32 /nologo

# ADD LIB32 /nologo



!ENDIF 



# Begin Target



# Name "libswishindex - Win32 Release"

# Name "libswishindex - Win32 Debug"

# Begin Group "Source Files"



# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"

# Begin Source File



SOURCE=..\db_write.c

# End Source File

# Begin Source File



SOURCE=.\dirent.c

# End Source File

# Begin Source File



SOURCE=..\docprop_write.c

# End Source File

# Begin Source File



SOURCE=..\entities.c

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



SOURCE=..\getruntime.c

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



SOURCE=..\merge.c

# End Source File

# Begin Source File



SOURCE=..\methods.c

# End Source File

# Begin Source File



SOURCE=..\replace\mkstemp.c

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

# End Group

# End Target

# End Project

