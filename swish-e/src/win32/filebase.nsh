
Section "Required Components" SecProgram
    SectionIn 1 RO
    
    ; System Files
    SetOutPath "$SYSDIR"
    ; Expat, LibXML2, and ZLib
    File ..\..\..\libxml2\lib\*.dll
    File ..\..\..\zlib\bin\*.dll
    File ..\..\..\pcre\bin\*.dll
    File ..\expat\bin\*.dll
    
    ; Local Files
    SetOutPath "$INSTDIR"
    File COPYING.txt
    File swish-e.exe
    File fixperl.pl
    Delete /REBOOTOK "$INSTDIR\*.dll"
    
    ; Local Helper Files
    SetOutPath "$INSTDIR\lib\swish-e"
    File ..\swishspider
    File ..\..\prog-bin\spider.pl.in
    
    ; Create shorcuts on the Start Menu
    SetOutPath "$SMPROGRAMS\SWISH-E\"
    CreateShortcut "$SMPROGRAMS\SWISH-E\Browse Files.lnk" "$INSTDIR\"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Website.url" "InternetShortcut" "URL" "http://swish-e.org/"
    CreateShortcut "$SMPROGRAMS\SWISH-E\License.lnk" "$INSTDIR\COPYING.txt"
    SetOutPath "$SMPROGRAMS\SWISH-E\PERL_Resources"
    WriteINIStr "$SMPROGRAMS\SWISH-E\PERL_Resources\Install_ActivePerl.url" "InternetShortcut" "URL" "http://www.activestate.com/Products/Download/Download.plex?id=ActivePerl"
    WriteINIStr "$SMPROGRAMS\SWISH-E\PERL_Resources\CPAN_PERL_Modules.url" "InternetShortcut" "URL" "http://search.cpan.org/"
SectionEnd ; end of default section

Section "Documentation" SecDocs
    SectionIn 1
    SetOutPath "$INSTDIR\share\doc\swish-e\examples"
    RMDIR /r "$INSTDIR\html"
    File /r ..\..\html
    
    ; Create shorcuts on the Start Menu
    SetOutPath "$SMPROGRAMS\SWISH-E\Documentation\"
    ;WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\Docs.url" "InternetShortcut" "URL" "file://$INSTDIR\share\doc\swish-e\html\index.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\Readme.url" "InternetShortcut" "URL" "file://$INSTDIR\share\doc\swish-e\html\README.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\CONFIG-Config_File_Directives.url" "InternetShortcut" "URL" "file://$INSTDIR\share\doc\swish-e\html\SWISH-CONFIG.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\RUN-Command_Line_Switches.url" "InternetShortcut" "URL" "file://$INSTDIR\share\doc\swish-e\html\SWISH-RUN.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\SEARCH-Searching_Instructions.url" "InternetShortcut" "URL" "file://$INSTDIR\share\doc\swish-e\html\SWISH-SEARCH.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\FAQ-Frequently_Asked_Questions.url" "InternetShortcut" "URL" "file://$INSTDIR\share\doc\swish-e\html\SWISH-FAQ.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\CGI-Web_Interfacing.url" "InternetShortcut" "URL" "file://$INSTDIR\share\doc\swish-e\html\swish.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\Spidering_Remote_Websites.url" "InternetShortcut" "URL" "file://$INSTDIR\share\doc\swish-e\html\spider.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\Filtering_Documents.url" "InternetShortcut" "URL" "file://$INSTDIR\share\doc\swish-e\html\Filter.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\Known_Bugs.url" "InternetShortcut" "URL" "file://$INSTDIR\share\doc\swish-e\html\SWISH-BUGS.html"
    ;WriteINIStr "$SMPROGRAMS\SWISH-E\Questions_and_Troubleshooting.url" "InternetShortcut" "URL" "file://$INSTDIR\share\doc\swish-e\html\INSTALL.html#QUESTIONS_AND_TROUBLESHOOTING"
SectionEnd ; end of section 'Documentation'

Section "ActiveX Control" SecSwishCtl
    ; System Files
    SetOutPath "$SYSDIR"
    ; SWISH-E Control
    File ..\..\..\swishctl\swishctl.dll
    RegDLL $SYSDIR\swishctl.dll
    
    ; Local Files
    SetOutPath "$INSTDIR"
    ; JavaScript ActiveX Search Example
    File /r ..\..\..\swishctl\example
    
    ; Create shorcuts on the Start Menu
    SetOutPath "$SMPROGRAMS\SWISH-E\"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Search_Documentation.url" "InternetShortcut" "URL" "file://$INSTDIR\example\index.htm"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SwishCtl\Options" "DLLVersion" "1007"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SwishCtl\Options" "IndexLocation" "$INSTDIR\example"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SwishCtl\Options" "swishdocs" "docs.idx"
SectionEnd ; end of ActiveX section

SubSection "Document Filters" SubSecFilters
    Section "Word Doc Filter" SecDocFilter
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}\Options" "catdoc" "1"
        SetOutPath "$INSTDIR\lib\swish-e"
        File ..\..\..\catdoc\win32\catdoc.exe
        File /r ..\..\..\catdoc\charsets
    SectionEnd
    Section "PDF Filter" SecPDFFilter
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}\Options" "xpdf" "1"
        SetOutPath "$INSTDIR\lib\swish-e"
        File ..\..\..\xpdf\pdfinfo.*
        File ..\..\..\xpdf\pdftotext.*
        File ..\..\..\xpdf\sample-xpdfrc
        File ..\..\..\xpdf\xpdfrc.txt
    SectionEnd
SubSectionEnd        
    
SubSection "PERL Support" SubSecPerlSupport
    Section /o "PERL API" SecPerlApi
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}\Options" "Perl" "1"
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}\Options" "PerlApi" "1"
        
        ; SWISH::API Scripts
        SetOutPath "$INSTDIR\lib\swish-e\perl\SWISH"
        ; File /r ..\..\perl\blib\lib\SWISH
        File ..\..\perl\API.pm
        
        ; SWISH::API Binaries go into $PERL\lib\auto\SWISH\API
        Call ActivePerlLocation
        Pop $R1
        SetOutPath "$R1\lib"
        File /r ..\..\perl\blib\arch\auto
    SectionEnd
    
    Section /o "PERL Filters" SecPerlFilter
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}\Options" "Perl" "1"
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}\Options" "PerlFilters" "1"
        SetOutPath "$INSTDIR"
        File ..\..\filters\swish-filter-test.in"
        
        SetOutPath "$INSTDIR\share\doc\swish-e\examples\filter-bin"
        ; Example Filter Scripts
        File ..\..\filter-bin\_binfilter.sh
        File ..\..\filter-bin\_pdf2html.pl
        
	# Generic SWISH::Filter Filter
	SetOutPath "$INSTDIR\lib\swish-e"
	File ..\..\filter-bin\swish_filter.pl.in 

	# SWISH::Filter API
        SetOutPath "$INSTDIR\lib\swish-e\perl"
        File /r ..\..\filters\SWISH
    SectionEnd
    
    Section /o "PERL -S prog Examples" SecPerlMethod
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}\Options" "Perl" "1"
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}\Options" "PerlMethods" "1"
        SetOutPath "$INSTDIR\share\doc\swish-e\examples\prog-bin"
        File ..\..\prog-bin\*.pl
        File ..\..\prog-bin\*.in
        File ..\..\prog-bin\*.pm
    SectionEnd
    
Section "Examples" SecExample
    SetOutPath "$INSTDIR\share\doc\swish-e\examples\"
    File /r ..\..\conf
    File /r ..\..\example

    ; Rename text files so Windows has a clue
    Rename "$INSTDIR\share\doc\swish-e\examples\conf\README" "$INSTDIR\conf\README.txt"
SectionEnd ; end of section 'Examples'

Section "-post" ; (post install section, happens last after any optional sections)
    ; add any commands that need to happen after any optional sections here
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}" "" "$INSTDIR"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E" "CurrentVersion" "${VERSION}"
    
    ; fixperl needs these values:  installdir libexecdir perlmoduledir pkgdatadir swishbinary
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}" "installdir" "$INSTDIR"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}" "libexecdir" "$INSTDIR\lib\swish-e"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}" "mylibexecdir" "$INSTDIR\lib\swish-e"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}" "perlmoduledir" "$INSTDIR\lib\swish-e\perl"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}" "pkgdatadir" "$INSTDIR\lib\swish-e"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}" "swishbinary" "$INSTDIR\swish-e.exe"
    
    ; Uninstaller information
    WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\SWISH-E" "DisplayName" "SWISH-E (remove only)"
    WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\SWISH-E" "UninstallString" '"$INSTDIR\uninst.exe"'
    
    ; Add swish-e.exe to the Win32 Path
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\AppPaths\swish-e.exe" "" "$INSTDIR\swish-e.exe"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\AppPaths\swish-e.exe" "Path" "$INSTDIR;$INSTDIR\lib\swish-e"
    
    WriteUninstaller "uninst.exe"
    
    ; Is ActivePerl Installed?
    Call IsActivePerlInstalled
    Pop $R0
    StrCmp $R0 0 endofpost
    
    ; Did SWISH-E Install any Perl modules?
    Call IsSwishPerlInstalled
    Pop $R0
    StrCmp $R0 0 endofpost
    
    ; If both were true we'll run fixperl.pl
    Call ActivePerlLocation
    Pop $R1
    Exec "$R1bin\perl.exe $INSTDIR\fixperl.pl"
    
    endofpost:
SectionEnd ; end of -post section

Section Uninstall
    ; add delete commands to delete whatever files/registry keys/etc you installed here.
    ; UninstallText "This will remove SWISH-E from your system"
    Delete "$INSTDIR\uninst.exe"
    DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}"
    DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E" "CurrentVersion"
    DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\SWISH-E"
    DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\AppPaths\swish-e.exe"
    
    ; SWISH::API Binaries go into $PERL\lib\auto\SWISH\API
    Call un.ActivePerlLocation
    Pop $R1
    RMDir /r "$R1\lib\auto\SWISH"
    
    UnRegDLL $SYSDIR\swishctl.dll
    
    ; Other files
    RMDir /r "$INSTDIR"
    RMDir /r "$SMPROGRAMS\SWISH-E\"
SectionEnd ; end of uninstall section


Function .onInit
    Call IsActivePerlInstalled
    Pop $R0
    StrCmp $R0 1 endselect
    
    ; Do Not Select Perl Modules
    MessageBox MB_OK "ActivePerl 5.8 is not installed!  SWISH-E Perl support will not work and has been deselected."
    Goto end
    
    ; Select Perl Modules
    endselect:
    !insertmacro SelectSection ${SecPerlFilter}
    !insertmacro SelectSection ${SecPerlApi}
    !insertmacro SelectSection ${SecPerlMethod}
    !insertmacro SelectSection ${SecPerlCgi}
    
    end:
FunctionEnd                                                                                                                                                                                                 

Function DownloadActivePerl
; http://downloads.activestate.com/ActivePerl/Windows/5.8/ActivePerl-5.8.0.806-MSWin32-x86.msi    
    GetTempFileName $R0
    File /oname=$R0 perlpage.ini
    InstallOptions::dialog $R0
    Pop $R1
    StrCmp $R1 "cancel" done
    StrCmp $R1 "back" done
    StrCmp $R1 "success" done
    error: MessageBox MB_OK|MB_ICONSTOP "InstallOptions error:$\r$\n$R1"
    done:

    ; Is ActivePerl Installed?
    Call IsActivePerlInstalled
    Pop $R0
    StrCmp $R0 1 end
    MessageBox MB_YESNO "Would you like to install ActivePerl 5.8?" IDNO end
    
    ; Download ActivePerl to SWISH-E Install Directory
    NSISdl::download http://downloads.activestate.com/ActivePerl/Windows/5.8/ActivePerl-5.8.0.806-MSWin32-x86.msi "$INSTDIR\ActivePerl-5.8.0.806.msi"
    Pop $R0 ;Get the return value
    StrCmp $R0 "success" succeed
    MessageBox MB_OK "ActivePerl Download Failed: $R0"
    Goto end
    
    ; Attempt to install ActivePerl
    succeed:
    ExecShell open "$INSTDIR\ActivePerl-5.8.0.806.msi"
    end:
FunctionEnd

;--------------------------------
 ; IsActivePerlInstalled
 ; Based on IsFlashInstalled
 ; By Yazno, http://yazno.tripod.com/powerpimpit/
 ; Returns on top of stack
 ; 0 (ActivePerl is not installed)
 ; or
 ; 1 (ActivePerl is installed)
 ;
 ; Usage:
 ;   Call IsActivePerlInstalled
 ;   Pop $R0
 ;   ; $R0 at this point is "1" or "0"

 Function IsActivePerlInstalled
  Push $R0
  ClearErrors
  ReadRegStr $R0 HKEY_LOCAL_MACHINE "Software\ActiveState\ActivePerl\" "CurrentVersion"
  IfErrors lbl_na
    StrCpy $R0 1
  Goto lbl_end
  lbl_na:
    StrCpy $R0 0
  lbl_end:
  Exch $R0
 FunctionEnd
 
 Function ActivePerlLocation
  Push $R0
  ClearErrors
  ReadRegStr $R0 HKEY_LOCAL_MACHINE "Software\ActiveState\ActivePerl\" "CurrentVersion"
  ReadRegStr $R1 HKEY_LOCAL_MACHINE "Software\ActiveState\ActivePerl\$R0" ""
  IfErrors lbl_na
  Goto lbl_end
  lbl_na:
    StrCpy $R0 0
  lbl_end:
  Exch $R1
 FunctionEnd
 
 Function un.ActivePerlLocation
  Push $R0
  ClearErrors
  ReadRegStr $R0 HKEY_LOCAL_MACHINE "Software\ActiveState\ActivePerl\" "CurrentVersion"
  ReadRegStr $R1 HKEY_LOCAL_MACHINE "Software\ActiveState\ActivePerl\$R0" ""
  IfErrors lbl_na
  Goto lbl_end
  lbl_na:
    StrCpy $R0 0
  lbl_end:
  Exch $R1
 FunctionEnd
 Function IsSwishPerlInstalled
  Push $R0
  ClearErrors
  ReadRegStr $R0 HKEY_LOCAL_MACHINE "Software\SWISH-E Team\SWISH-E\${VERSION}\Options" "Perl"
  IfErrors lbl_na
    StrCpy $R0 1
  Goto lbl_end
  lbl_na:
    StrCpy $R0 0
  lbl_end:
  Exch $R0
 FunctionEnd
