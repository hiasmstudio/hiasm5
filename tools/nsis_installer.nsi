; -*- coding: utf-8 -*-
; Stage a Qt Release install with cmake --install, then:
;   makensis /DBUILD_DIR="D:\path\install" [/DOUTPUT_FILE="D:\path\HiAsm_Setup.exe"] tools\nsis_installer.nsi
; Relative build/output paths are resolved against the repository root.
Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
!include "FileFunc.nsh"
; makensis enters the script's directory before preprocessing (do not use /NOCD).
; __FILEDIR__ may be relative to the original caller, so do not append it again.
!cd ".."
!ifndef BUILD_DIR
  !error "BUILD_DIR is required: pass /DBUILD_DIR=<staged-install-prefix> containing qt/hiasm.exe."
!endif
!if ! /FileExists "${BUILD_DIR}\qt\hiasm.exe"
  !error "BUILD_DIR/qt/hiasm.exe is missing. Run cmake --install first."
!endif
!if ! /FileExists "${BUILD_DIR}\int\hiasm.db"
  !error "HiAsm runtime database is missing from BUILD_DIR/int."
!endif
!if ! /FileExists "${BUILD_DIR}\packs\cpp\elements.db"
  !error "C++ element pack is missing from BUILD_DIR/packs/cpp."
!endif
!if ! /FileExists "${BUILD_DIR}\qt\Qt6Widgets.dll"
  !error "Qt Widgets runtime is missing."
!endif
!if ! /FileExists "${BUILD_DIR}\qt\platforms\qwindows.dll"
  !error "Qt Windows platform plugin is missing."
!endif
!if ! /FileExists "resources\hiasm.ico"
  !error "resources/hiasm.ico is missing. Run tools\build_windows_installer.bat."
!endif
!ifndef OUTPUT_FILE
  !define OUTPUT_FILE "${BUILD_DIR}\..\HiAsm_Setup.exe"
!endif
!ifndef PRODUCT_VERSION
  !define PRODUCT_VERSION "6.0.0.0"
!endif
!define PRODUCT_NAME "HiAsm Studio Qt"
!define PRODUCT_PUBLISHER "Dilma"
!define PRODUCT_WEB_SITE "http://hiasm.com"
!define APP_KEY "Software\HiAsm Studio Qt"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

; Inventory includes runtime DLLs, Qt plugins, packs and the C++ package database.
; Uninstall removes only packaged files and directories that are empty.
!tempfile HIASM_PAYLOAD_INCLUDE
!system 'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "tools\nsis_payload.ps1" -Source "${BUILD_DIR}" -Output "${HIASM_PAYLOAD_INCLUDE}"' = 0
!include /CHARSET=UTF8 "${HIASM_PAYLOAD_INCLUDE}"
!delfile "${HIASM_PAYLOAD_INCLUDE}"

Name "${PRODUCT_NAME}"
OutFile "${OUTPUT_FILE}"
InstallDir "$PROGRAMFILES64\HiAsm Studio Qt"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
!define MUI_ICON "resources\hiasm.ico"
!define MUI_UNICON "resources\hiasm.ico"
!define MUI_ABORTWARNING
!define MUI_LANGDLL_REGISTRY_ROOT HKLM
!define MUI_LANGDLL_REGISTRY_KEY "${APP_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "InstallerLanguage"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "English"

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP "${PRODUCT_NAME} requires 64-bit Windows."
    Abort
  ${EndIf}
  SetRegView 64
  SetShellVarContext all
  ; Preserve a custom /D= directory supplied on the command line.
  ${If} $INSTDIR == "$PROGRAMFILES64\HiAsm Studio Qt"
    ReadRegStr $0 HKLM "${APP_KEY}" "InstallPath"
    ${If} $0 != ""
      StrCpy $INSTDIR $0
    ${EndIf}
  ${EndIf}
  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

Function ValidateInstallDirectory
  GetFullPathName $0 "$INSTDIR"
  ${GetRoot} "$0" $1
  ${If} $0 == "$1\"
  ${OrIf} $0 == "$PROGRAMFILES64"
  ${OrIf} $0 == "$PROGRAMFILES32"
  ${OrIf} $0 == "$WINDIR"
  ${OrIf} $0 == "$SYSDIR"
    MessageBox MB_OK|MB_ICONSTOP "Choose a dedicated application folder."
    Abort
  ${EndIf}
FunctionEnd
Function .onVerifyInstDir
  Call ValidateInstallDirectory
FunctionEnd

Section "${PRODUCT_NAME}" SEC_APP
  SectionIn RO
  Call ValidateInstallDirectory
  SetOutPath "$INSTDIR"
  SetOverwrite on
  !insertmacro HIASM_INSTALL_PAYLOAD
  ReadRegStr $0 HKLM "${APP_KEY}" "InstallPath"
  ${If} $0 == "$INSTDIR"
    Delete "$INSTDIR\qt\hiasm-project-manager.exe"
    Delete "$INSTDIR\qt\hiasm-qt.exe"
    Delete "$INSTDIR\qt\hiasm-build.exe"
  ${EndIf}
  ; Reset the working directory after copying nested Qt/pack folders.
  SetOutPath "$INSTDIR"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "${APP_KEY}" "InstallPath" "$INSTDIR"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "UninstallString" '$\"$INSTDIR\Uninstall.exe$\"'
  WriteRegStr HKLM "${UNINSTALL_KEY}" "QuietUninstallString" '$\"$INSTDIR\Uninstall.exe$\" /S'
  WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayIcon" '$\"$INSTDIR\qt\hiasm.exe$\",0'
  WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "${UNINSTALL_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair" 1
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\qt\hiasm.exe"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Function un.onInit
  SetRegView 64
  SetShellVarContext all
  !insertmacro MUI_UNGETLANGUAGE
  ReadRegStr $0 HKLM "${APP_KEY}" "InstallPath"
  GetFullPathName $1 "$INSTDIR"
  GetFullPathName $2 "$0"
  ${If} $0 == ""
  ${OrIf} $1 != $2
    MessageBox MB_OK|MB_ICONSTOP "Run the uninstaller from the registered application folder."
    Abort
  ${EndIf}
FunctionEnd

Section "Uninstall"
  !insertmacro HIASM_UNINSTALL_PAYLOAD
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"
  DeleteRegKey HKLM "${UNINSTALL_KEY}"
  DeleteRegKey HKLM "${APP_KEY}"
SectionEnd
