Name "SWISH-E"
OutFile "swishsetup.exe"

!define VERSION "2.4.0-pr4"

; Some default compiler settings (uncomment and change at will):
SetCompress auto ; (can be off or force)
SetDatablockOptimize on ; (can be off)
CRCCheck on ; (can be off)
AutoCloseWindow false ; (can be true for the window go away automatically at end)
ShowInstDetails hide ; (can be show to have them shown, or nevershow to disable)
SetDateSave on ; (can be on to have files restored to their orginal date)

LicenseText "You may redistribute SWISH-E under the following terms:"
LicenseData "COPYING.txt"

InstallDir "$PROGRAMFILES\SWISH-E"
InstallDirRegKey HKEY_LOCAL_MACHINE "SOFTWARE\SWISH-E Team\SWISH-E\${VERSION}" ""
DirShow show ; (make this hide to not let the user change it)
DirText "Select location where to install SWISH-E:"

ComponentText "Which components do you require?"

# defines SF_*, SECTION_OFF and some macros
!include Sections.nsh

Page license
Page directory
# Page custom InstallActivePerl ": ActivePerl Detection"
Page components
Page instfiles


;--------------------------------
; Installer Sections

; Basic Sections
!include filebase.nsh

