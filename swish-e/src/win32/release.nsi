;NSIS Modern User Interface version 1.63
;SWISH-E Installer based on:
;  Basic Example Script
;  Written by Joost Verburg

!define MUI_PRODUCT "SWISH-E" ;Define your own software name here
!define MUI_VERSION "2.4.0" ;Define your own software version here

; !define MUI_SPECIALBITMAP "${NSISDIR}\Contrib\Icons\modern-wizard.bmp"
!include "MUI.nsh"

;--------------------------------
;Configuration

  ; Windows XP Manifest?
  XPStyle on

  ;General
  OutFile "${MUI_PRODUCT}-${MUI_VERSION}.exe"

  ;Folder selection page
  InstallDir "$PROGRAMFILES\${MUI_PRODUCT}"
  
  ;Remember install folder
  InstallDirRegKey HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\${MUI_PRODUCT}" ""

;--------------------------------
;Modern UI Configuration
  ; Configure page settings
  !define MUI_COMPONENTSPAGE_SMALLDESC
  ; Show these installer pages
  !define MUI_LICENSEPAGE
  !define MUI_COMPONENTSPAGE
  !define MUI_DIRECTORYPAGE
  
  !define MUI_ABORTWARNING
  
  ; Show these uninstaller pages
  !define MUI_UNINSTALLER
  !define MUI_UNCONFIRMPAGE
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"
  
;--------------------------------
;Language Strings

  ;Description
  LangString DESC_SecProgram ${LANG_ENGLISH} "SWISH-E application and support files."
  LangString DESC_SecDocs ${LANG_ENGLISH} "SWISH-E Documentation."
  LangString DESC_SecExample ${LANG_ENGLISH} "SWISH-E example scripts and configuration files."
  LangString DEST_SecSwishCtl ${LANG_ENGLISH} "SWISH-E ActiveX control and Searchable Documentation."
;--------------------------------
;Data
  
  LicenseData "COPYING.txt"

; Basic file sections
!addincludedir nsi
!include filebase.nsh

Section "SWISH-E ActiveX Control" SecSwishCtl
SectionIn 1
SetOutPath "$INSTDIR"

; SWISH-E Control
File ..\..\..\swishctl\swishctl.dll

; Misc SWISH-E Support Files
File /r ..\..\..\swishctl\example

; Create shorcuts on the Start Menu
SetOutPath "$SMPROGRAMS\SWISH-E\"
CreateShortcut "$SMPROGRAMS\SWISH-E\Search_Documentation.lnk" "$INSTDIR\example\index.htm"
WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SwishCtl\Options" "IndexLocation" "$INSTDIR\example"
WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SwishCtl\Options" "swishdoc" "docs.idx"
SectionEnd ; end of ActiveX section

Section "-post" ; (post install section, happens last after any optional sections)
; add any commands that need to happen after any optional sections here
WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E" "" "$INSTDIR"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\SWISH-E" "DisplayName" "SWISH-E (remove only)"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\SWISH-E" "UninstallString" '"$INSTDIR\uninst.exe"'

WriteUninstaller "uninst.exe"
SectionEnd ; end of -post section


;Display the Finish header
;Insert this macro after the sections if you are not using a finish page
!insertmacro MUI_SECTIONS_FINISHHEADER

;--------------------------------
;Descriptions

!insertmacro MUI_FUNCTIONS_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecProgram} $(DESC_SecProgram)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecExample} $(DESC_SecExample)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDocs} $(DESC_SecDocs)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSwishCtl} $(DESC_SecSwishCtl)
!insertmacro MUI_FUNCTIONS_DESCRIPTION_END
 
;--------------------------------
;Uninstaller Section

Section Uninstall
  Delete "$INSTDIR\uninst.exe"
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E"
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\SWISH-E"
  RMDir /r "$INSTDIR"
  RMDir /r "$SMPROGRAMS\SWISH-E\"
SectionEnd ; end of uninstall section
