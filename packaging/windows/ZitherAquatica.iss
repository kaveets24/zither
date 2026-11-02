; Windows installer for Zither Aquatica (Inno Setup 6): the VST3 plug-in and the Standalone app.
;
;   iscc /DAppVersion=0.1.0 /DArtefacts=<repo>\build\Zither_artefacts\Release /DOutputDir=<dir> ZitherAquatica.iss

#ifndef AppVersion
  #error Pass /DAppVersion=x.y.z
#endif
#ifndef Artefacts
  #error Pass /DArtefacts=<path to build\Zither_artefacts\Release>
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif

#define Product "Zither Aquatica"

[Setup]
; Windows recognises upgrades of an existing install by AppId. It is permanent: never change it.
AppId={{42106BA6-FA0F-47BC-945E-F297C21C0067}
AppName={#Product}
AppVersion={#AppVersion}
AppVerName={#Product} {#AppVersion}
AppPublisher=Steven Kveton
AppPublisherURL=https://github.com/kaveets24/zither
AppSupportURL=https://github.com/kaveets24/zither/issues
DefaultDirName={autopf}\{#Product}
DefaultGroupName={#Product}
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputDir={#OutputDir}
OutputBaseFilename={#Product} {#AppVersion} Setup
UninstallDisplayName={#Product}
UninstallDisplayIcon={app}\{#Product}.exe
; The plug-in is 64-bit, and the system VST3 folder (Common Files) needs admin rights.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Types]
Name: "full"; Description: "VST3 plug-in and Standalone app"
Name: "custom"; Description: "Choose components"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in (Ableton Live, Cubase, Reaper, FL Studio, Bitwig, Studio One...)"; Types: full custom
Name: "standalone"; Description: "Standalone app (play without a DAW)"; Types: full

[Files]
; Hosts look for VST3 plug-ins in Common Files\VST3. A .vst3 is a folder bundle, so copy it whole.
Source: "{#Artefacts}\VST3\{#Product}.vst3\*"; DestDir: "{commoncf64}\VST3\{#Product}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\Standalone\{#Product}.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{group}\{#Product}"; Filename: "{app}\{#Product}.exe"; Components: standalone

[Run]
Filename: "{app}\{#Product}.exe"; Description: "Open {#Product}"; Components: standalone; Flags: nowait postinstall skipifsilent unchecked
