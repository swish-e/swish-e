
Section "Required Components" SecProgram
    SectionIn 1 RO
    
    ; System Files
    SetOutPath "$SYSDIR"
    ; Expat, LibXML2, and ZLib
    File ..\..\..\libxml2\lib\*.dll
    File ..\..\..\zlib\bin\*.dll
    File ..\..\..\pcre\bin\*.dll
    
    ; Local Files
    SetOutPath "$INSTDIR"
    File COPYING.txt
    File ..\swish-e.exe
    File fixperl.pl
    Delete /REBOOTOK "$INSTDIR\*.dll"
    
    ; Local Helper Files
    SetOutPath "$INSTDIR\lib\swish-e"
    File ..\swishspider
    ;File /r ..\..\filter-bin
    ;File /r ..\..\prog-bin
    ;File /r ..\..\filters
    
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
    SetOutPath "$INSTDIR"
    RMDIR /r "$INSTDIR\html"
    File /r ..\..\html
    
    ; Create shorcuts on the Start Menu
    SetOutPath "$SMPROGRAMS\SWISH-E\Documentation\"
    ;WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\Docs.url" "InternetShortcut" "URL" "file://$INSTDIR\html\index.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\Readme.url" "InternetShortcut" "URL" "file://$INSTDIR\html\README.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\CONFIG-Config_File_Directives.url" "InternetShortcut" "URL" "file://$INSTDIR\html\SWISH-CONFIG.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\RUN-Command_Line_Switches.url" "InternetShortcut" "URL" "file://$INSTDIR\html\SWISH-RUN.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\SEARCH-Searching_Instructions.url" "InternetShortcut" "URL" "file://$INSTDIR\html\SWISH-SEARCH.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\FAQ-Frequently_Asked_Questions.url" "InternetShortcut" "URL" "file://$INSTDIR\html\SWISH-FAQ.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\CGI-Web_Interfacing.url" "InternetShortcut" "URL" "file://$INSTDIR\html\swish.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\Spidering_Remote_Websites.url" "InternetShortcut" "URL" "file://$INSTDIR\html\spider.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\Filtering_Documents.url" "InternetShortcut" "URL" "file://$INSTDIR\html\Filter.html"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Documentation\Known_Bugs.url" "InternetShortcut" "URL" "file://$INSTDIR\html\SWISH-BUGS.html"
    ;WriteINIStr "$SMPROGRAMS\SWISH-E\Questions_and_Troubleshooting.url" "InternetShortcut" "URL" "file://$INSTDIR\html\INSTALL.html#QUESTIONS_AND_TROUBLESHOOTING"
SectionEnd ; end of section 'Documentation'

Section "ActiveX Control" SecSwishCtl
    ; System Files
    SetOutPath "$SYSDIR"
    ; SWISH-E Control
    File ..\..\..\swishctl\swishctl.dll
    Exec "regsvr32 /s $SYSDIR\swishctl.dll"
    
    ; Local Files
    SetOutPath "$INSTDIR"
    ; JavaScript ActiveX Search Example
    RmDir /r example
    File /r ..\..\..\swishctl\example
    RmDir /r search
    Rename example search
    
    ; Create shorcuts on the Start Menu
    SetOutPath "$SMPROGRAMS\SWISH-E\"
    WriteINIStr "$SMPROGRAMS\SWISH-E\Search_Documentation.url" "InternetShortcut" "URL" "file://$INSTDIR\search\index.htm"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SwishCtl\Options" "IndexLocation" "$INSTDIR\search"
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SwishCtl\Options" "swishdocs" "docs.idx"
SectionEnd ; end of ActiveX section

SubSection "Document Filters" SubSecFilters
    Section "Word Doc Filter" SecDocFilter
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\Options" "catdoc" "1"
        SetOutPath "$INSTDIR\lib\swish-e"
        File ..\..\..\catdoc\win32\catdoc.exe
        File /r ..\..\..\catdoc\charsets
    SectionEnd
    Section "PDF Filter" SecPDFFilter
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\Options" "xpdf" "1"
        SetOutPath "$INSTDIR\lib\swish-e"
        File ..\..\..\xpdf\pdfinfo.*
        File ..\..\..\xpdf\pdftotext.*
        File ..\..\..\xpdf\sample-xpdfrc
        File ..\..\..\xpdf\xpdfrc.txt
    SectionEnd
SubSectionEnd        
    
SubSection "PERL Support" SubSecPerlSupport
    Section /o "PERL API" SecPerlApi
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\Options" "Perl" "1"
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\Options" "PerlApi" "1"
        SetOutPath "$INSTDIR\lib\swish-e"
        
        ; SWISH::API Scripts
        File /r ..\..\perl\blib\lib\SWISH
        
        ; SWISH::API Binaries go into $PERL\lib\auto\SWISH\API
        Call ActivePerlLocation
        Pop $R1
        SetOutPath "$R1\lib"
        File /r ..\..\perl\blib\arch\auto
    SectionEnd
    
    Section /o "PERL Filters" SecPerlFilter
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\Options" "Perl" "1"
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\Options" "PerlFilters" "1"
        SetOutPath "$INSTDIR\lib\swish-e"
        ; Filter Scripts
        File ..\..\filter-bin\swish_filter.pl.in
        File ..\..\filter-bin\_binfilter.sh
        File ..\..\filter-bin\_pdf2html.pl
        File /r ..\..\filters\SWISH
        
        ; CreateShortcut "$SMPROGRAMS\SWISH-E\Browse_Filters.lnk" "$INSTDIR\lib\swish-e"
    SectionEnd
    
    Section /o "PERL -S prog Methods" SecPerlMethod
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\Options" "Perl" "1"
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\Options" "PerlMethods" "1"
        SetOutPath "$INSTDIR\lib\swish-e"
        ; CGI Scripts
        File ..\..\prog-bin\*.pl
        File ..\..\prog-bin\*.pm
        File ..\..\prog-bin\*.in
    SectionEnd
    
    Section /o "PERL CGI" SecPerlCgi
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\Options" "Perl" "1"
        WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\Options" "PerlCgi" "1"
        SetOutPath "$INSTDIR\lib\swish-e"
        ; CGI Scripts
        File ..\..\example\swish.cgi.in
        File ..\..\example\search.tt
        File ..\..\example\swish.tmpl
        File ..\..\example\swish.gif
        File /r ..\..\example\modules\SWISH
    SectionEnd
SubSectionEnd

Section "Examples" SecExample
    SetOutPath "$INSTDIR"
    File /r ..\..\example
    File /r ..\..\conf
    
    ; Rename text files so Windows has a clue
    Rename "$INSTDIR\conf\README" "$INSTDIR\conf\README.txt"
    Rename "$INSTDIR\example\README" "$INSTDIR\example\README.txt"
SectionEnd ; end of section 'Examples'

Section "-post" ; (post install section, happens last after any optional sections)
    ; add any commands that need to happen after any optional sections here
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E" "" "$INSTDIR"
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
    ExecShell open "$INSTDIR\fixperl.pl"
    
    endofpost:
SectionEnd ; end of -post section

Section Uninstall
    ; add delete commands to delete whatever files/registry keys/etc you installed here.
    ; UninstallText "This will remove SWISH-E from your system"
    Delete "$INSTDIR\uninst.exe"
    DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E"
    DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\SWISH-E"
    DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\AppPaths\swish-e.exe"
    
    ; SWISH::API Binaries go into $PERL\lib\auto\SWISH\API
    Call un.ActivePerlLocation
    Pop $R1
    RMDir /r "$R1\lib\auto\SWISH"
    
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
  ReadRegStr $R0 HKEY_LOCAL_MACHINE "Software\SWISH-E Team\SWISH-E\Options" "Perl"
  IfErrors lbl_na
    StrCpy $R0 1
  Goto lbl_end
  lbl_na:
    StrCpy $R0 0
  lbl_end:
  Exch $R0
 FunctionEnd
