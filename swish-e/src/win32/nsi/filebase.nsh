
Section "Program" SecProgram
SectionIn 1
SetOutPath "$INSTDIR"
File COPYING.txt

; SWISH-E executable
File ..\swish-e.exe

; Expat, LibXML2, and ZLib
File ..\..\..\libxml2\lib\*.dll
File ..\..\..\zlib\bin\*.dll
File ..\..\..\pcre\bin\*.dll

; Misc SWISH-E Support Files
File ..\swishspider
File /r ..\..\filter-bin
File /r ..\..\prog-bin
File /r ..\..\filters

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

Section "Examples" SecExample
SectionIn 1
SetOutPath "$INSTDIR"
RMDIR /r "$INSTDIR\example"
RMDIR /r "$INSTDIR\conf"
File /r ..\..\example
File /r ..\..\conf

; Rename text files so Windows has a clue
Rename "$INSTDIR\conf\README" "$INSTDIR\conf\README.txt"
Rename "$INSTDIR\example\README" "$INSTDIR\example\README.txt"
SectionEnd ; end of section 'Examples'
