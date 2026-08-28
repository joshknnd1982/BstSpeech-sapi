; Inno Setup script for the BestSpeech SAPI5 voices.
;
; Compiled directly rather than through CPack: the engine has to register two COM
; servers into two different registry views, stop a running worker process first, and
; roll all of that back cleanly on uninstall, none of which the generated script
; expressed well.
;
; Languages and character voices are separate component branches, and what is ticked in
; each is crossed to produce the voice list: five languages and three voices is fifteen
; voices, not one lump. The choice is written to {app}\voices.ini, which the engine reads
; when it registers its tokens -- see src\install_selection.hpp -- so the wizard and the
; registry can never drift apart.
;
; Build with build_all.bat, which stages output\ and then invokes ISCC on this file.

#define AppName "BestSpeech SAPI5 Voices"
#define AppVersion "3.1.0"
#define AppPublisher "Gozaltech"
#define AppURL "http://gozaltech.org"
#define OutputDir "..\output"

[Setup]
AppId={{8B5E2A41-6C3D-4F17-9E28-1A7B4D9C5E30}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
DefaultDirName={autopf}\BestSpeech
DefaultGroupName=BestSpeech
DisableProgramGroupPage=yes
DisableDirPage=no
OutputDir={#OutputDir}
OutputBaseFilename=BestSpeechSAPI_Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern

; The component page is the point of this installer, so it is always shown -- even when
; the user picked a ready-made type -- and the Ready page lists what was chosen.
AlwaysShowComponentsList=yes
ShowComponentSizes=yes

; Voice tokens and COM classes live in HKLM, so the installer needs elevation.
PrivilegesRequired=admin

; In 64-bit install mode {sys} is the real System32 and {syswow64} is SysWOW64, which is
; what lets each COM server be registered into the registry view its own hosts read.
ArchitecturesInstallIn64BitMode=x64compatible

; Every engine dll is 32-bit, so the package itself is architecture neutral and installs
; on 32-bit Windows too, just without the 64-bit bridge.
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\BestspeechSAPI.dll

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
; The stock wording talks about program features; this page is choosing voices, and the
; cross of the two branches is the one thing a first-time user will not guess.
WizardSelectComponents=Choose languages and voices
SelectComponentsDesc=Which BestSpeech voices should be installed?
SelectComponentsLabel2=Tick the languages you want, then tick the character voices you want. Every ticked character voice is created in every ticked language, so three voices across five languages gives you fifteen voices. Clear a tick to leave that language or voice out. Click Next when you are ready.

[Types]
Name: "full";    Description: "Everything: all 13 languages and all 14 character voices"
Name: "english"; Description: "English only: both English engines, all 14 character voices"
Name: "compact"; Description: "Compact: English with four character voices"
Name: "custom";  Description: "Custom: choose the languages and voices yourself"; Flags: iscustom

[Components]
; --- languages -----------------------------------------------------------------------
; Greek, Japanese and Polish have no character voices to cross with: their frontends
; ignore every voice command, so each publishes exactly one voice under its own name.
Name: "lang";         Description: "Languages"; Types: full english compact custom
Name: "lang\eng";     Description: "English"; Types: full english compact custom
Name: "lang\classic"; Description: "English (Classic 1994) - the original 1994 engine, a different sound"; Types: full english
Name: "lang\dut";     Description: "Dutch"; Types: full
Name: "lang\fre";     Description: "French"; Types: full
Name: "lang\ger";     Description: "German"; Types: full
Name: "lang\gre";     Description: "Greek (one voice, no character voices)"; Types: full
Name: "lang\heb";     Description: "Hebrew"; Types: full
Name: "lang\ita";     Description: "Italian"; Types: full
Name: "lang\jpn";     Description: "Japanese (one voice, no character voices)"; Types: full
Name: "lang\pol";     Description: "Polish (one voice, no character voices)"; Types: full
Name: "lang\por";     Description: "Portuguese"; Types: full
Name: "lang\rus";     Description: "Russian"; Types: full
Name: "lang\spa";     Description: "Spanish"; Types: full

; --- character voices ----------------------------------------------------------------
; No files of their own: a character voice is a set of engine parameters, so ticking one
; costs no disk space and only adds a voice per language to the list. Pitches are the
; measured defaults, which is the quickest way to tell them apart on paper.
Name: "voice";         Description: "Character voices"; Types: full english compact custom
Name: "voice\fred";    Description: "Fred - male, 80 Hz, the usual default"; Types: full english compact custom
Name: "voice\sara";    Description: "Sara - female, 175 Hz"; Types: full english compact
Name: "voice\hary";    Description: "Hary - male, 65 Hz"; Types: full english compact
Name: "voice\wendy";   Description: "Wendy - female, 150 Hz"; Types: full english compact
Name: "voice\dexter";  Description: "Dexter - male, 90 Hz, large head"; Types: full english
Name: "voice\alien";   Description: "Alien - male, 115 Hz, flat inflection"; Types: full english
Name: "voice\kit";     Description: "Kit - female, 230 Hz"; Types: full english
Name: "voice\bruno";   Description: "Bruno - male, 60 Hz"; Types: full english
Name: "voice\ghost";   Description: "Ghost - male, 60 Hz, whispery"; Types: full english
Name: "voice\peeper";  Description: "Peeper - male, 80 Hz, breathy"; Types: full english
Name: "voice\dracula"; Description: "Dracula - male, 47 Hz, the lowest"; Types: full english
Name: "voice\granny";  Description: "Granny - female, 350 Hz"; Types: full english
Name: "voice\martha";  Description: "Martha - female, 300 Hz"; Types: full english
Name: "voice\tim";     Description: "Tim - male, 60 Hz"; Types: full english

; --- the sculpted voice --------------------------------------------------------------
Name: "customvoice"; Description: "Custom Voice - a voice you shape yourself in the configuration utility, plus one per language"; Types: full english compact custom

[Tasks]
; Optional desktop shortcut to the configuration utility, offered on its own
; wizard page so the choice is reachable with a screen reader.
Name: "desktopicon"; Description: "Create a &desktop icon for the BestSpeech configuration utility"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
; --- 32-bit SAPI engine, the worker process, and the engine shim ---
Source: "..\output\BestspeechSAPI.dll";   DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\BestspeechServer.exe"; DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\b32_wrapper.dll";      DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\b32_helper.exe";       DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\BestSpeechDiagnostics.exe"; DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\BestSpeechConfig.exe";      DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete

; --- the speech engines themselves, one file per language ---
Source: "..\output\b32_tts.dll"; DestDir: "{app}"; Components: lang\classic; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\dll_eng.dll"; DestDir: "{app}"; Components: lang\eng;     Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\dll_dut.dll"; DestDir: "{app}"; Components: lang\dut;     Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\dll_fre.dll"; DestDir: "{app}"; Components: lang\fre;     Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\dll_ger.dll"; DestDir: "{app}"; Components: lang\ger;     Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\dll_gre.dll"; DestDir: "{app}"; Components: lang\gre;     Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\dll_heb.dll"; DestDir: "{app}"; Components: lang\heb;     Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\dll_ita.dll"; DestDir: "{app}"; Components: lang\ita;     Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\dll_jpn.dll"; DestDir: "{app}"; Components: lang\jpn;     Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\dll_pol.dll"; DestDir: "{app}"; Components: lang\pol;     Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\dll_por.dll"; DestDir: "{app}"; Components: lang\por;     Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\dll_rus.dll"; DestDir: "{app}"; Components: lang\rus;     Flags: ignoreversion restartreplace uninsrestartdelete
Source: "..\output\dll_spa.dll"; DestDir: "{app}"; Components: lang\spa;     Flags: ignoreversion restartreplace uninsrestartdelete

; --- 64-bit SAPI engine, for Narrator and other 64-bit hosts ---
Source: "..\output\x64\BestspeechSAPI.dll"; DestDir: "{app}\x64"; \
    Flags: ignoreversion restartreplace uninsrestartdelete; Check: Is64BitInstallMode
Source: "..\output\x64\BestSpeechDiagnostics.exe"; DestDir: "{app}\x64"; \
    Flags: ignoreversion restartreplace uninsrestartdelete; Check: Is64BitInstallMode

[Icons]
Name: "{group}\BestSpeech configuration"; Filename: "{app}\BestSpeechConfig.exe"
Name: "{group}\Check BestSpeech voices"; Filename: "{app}\BestSpeechDiagnostics.exe"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\BestSpeech configuration"; Filename: "{app}\BestSpeechConfig.exe"; Tasks: desktopicon

[UninstallDelete]
; Written from [Code] rather than copied, so the uninstaller is told about it by hand.
Type: files; Name: "{app}\voices.ini"

[Code]
var
  DonatePage: TWizardPage;
  DonateLabel: TNewStaticText;
  DonateLabel2: TNewStaticText;
  PayPalButton: TNewButton;

  // The language and voice tables, in the same order as engines.hpp and its voices[].
  // LangDll is what gets deleted when a language is unticked on a second run, and
  // LangMulti says whether a language crosses with the character voices at all.
  LangId: array[0..12] of String;
  LangDll: array[0..12] of String;
  LangMulti: array[0..12] of Boolean;
  VoiceName: array[0..13] of String;

procedure InitTables;
begin
  LangId[0]  := 'classic'; LangDll[0]  := 'b32_tts.dll'; LangMulti[0]  := True;
  LangId[1]  := 'eng';     LangDll[1]  := 'dll_eng.dll'; LangMulti[1]  := True;
  LangId[2]  := 'dut';     LangDll[2]  := 'dll_dut.dll'; LangMulti[2]  := True;
  LangId[3]  := 'fre';     LangDll[3]  := 'dll_fre.dll'; LangMulti[3]  := True;
  LangId[4]  := 'ger';     LangDll[4]  := 'dll_ger.dll'; LangMulti[4]  := True;
  LangId[5]  := 'gre';     LangDll[5]  := 'dll_gre.dll'; LangMulti[5]  := False;
  LangId[6]  := 'heb';     LangDll[6]  := 'dll_heb.dll'; LangMulti[6]  := True;
  LangId[7]  := 'ita';     LangDll[7]  := 'dll_ita.dll'; LangMulti[7]  := True;
  LangId[8]  := 'jpn';     LangDll[8]  := 'dll_jpn.dll'; LangMulti[8]  := False;
  LangId[9]  := 'pol';     LangDll[9]  := 'dll_pol.dll'; LangMulti[9]  := False;
  LangId[10] := 'por';     LangDll[10] := 'dll_por.dll'; LangMulti[10] := True;
  LangId[11] := 'rus';     LangDll[11] := 'dll_rus.dll'; LangMulti[11] := True;
  LangId[12] := 'spa';     LangDll[12] := 'dll_spa.dll'; LangMulti[12] := True;

  VoiceName[0]  := 'Fred';    VoiceName[1]  := 'Sara';
  VoiceName[2]  := 'Hary';    VoiceName[3]  := 'Wendy';
  VoiceName[4]  := 'Dexter';  VoiceName[5]  := 'Alien';
  VoiceName[6]  := 'Kit';     VoiceName[7]  := 'Bruno';
  VoiceName[8]  := 'Ghost';   VoiceName[9]  := 'Peeper';
  VoiceName[10] := 'Dracula'; VoiceName[11] := 'Granny';
  VoiceName[12] := 'Martha';  VoiceName[13] := 'Tim';
end;

function InitializeSetup(): Boolean;
begin
  InitTables;
  Result := True;
end;

function LanguagePicked(Index: Integer): Boolean;
begin
  Result := WizardIsComponentSelected('lang\' + LangId[Index]);
end;

function VoicePicked(Index: Integer): Boolean;
begin
  Result := WizardIsComponentSelected('voice\' + Lowercase(VoiceName[Index]));
end;

function PickedVoiceCount(): Integer;
var
  I: Integer;
begin
  Result := 0;
  for I := 0 to 13 do
    if VoicePicked(I) then
      Result := Result + 1;
end;

// What the voice list will actually contain: a multi-voice language contributes one
// voice per ticked character, a single-voice language contributes exactly one, and the
// Custom Voice adds the plain token plus one per multi-voice language.
function PickedTokenCount(): Integer;
var
  I, Voices: Integer;
begin
  Result := 0;
  Voices := PickedVoiceCount;
  for I := 0 to 12 do
    if LanguagePicked(I) then
    begin
      if LangMulti[I] then
        Result := Result + Voices
      else
        Result := Result + 1;
    end;

  if WizardIsComponentSelected('customvoice') then
  begin
    Result := Result + 1;
    for I := 0 to 12 do
      if LanguagePicked(I) and LangMulti[I] then
        Result := Result + 1;
  end;
end;

// The two lists the engine reads back out of {app}\voices.ini.
function PickedLanguageList(): String;
var
  I: Integer;
begin
  Result := '';
  for I := 0 to 12 do
    if LanguagePicked(I) then
    begin
      if Result <> '' then
        Result := Result + ',';
      Result := Result + LangId[I];
    end;
end;

function PickedVoiceList(): String;
var
  I: Integer;
begin
  Result := '';
  for I := 0 to 13 do
    if VoicePicked(I) then
    begin
      if Result <> '' then
        Result := Result + ',';
      Result := Result + VoiceName[I];
    end;
end;

// Nothing to speak with, or nothing to speak as: either way the install would finish
// with an empty voice list, so it is caught here where it can still be fixed.
function NextButtonClick(PageID: Integer): Boolean;
var
  I, Langs, Multi: Integer;
begin
  Result := True;
  if PageID <> wpSelectComponents then
    Exit;

  Langs := 0;
  Multi := 0;
  for I := 0 to 12 do
    if LanguagePicked(I) then
    begin
      Langs := Langs + 1;
      if LangMulti[I] then
        Multi := Multi + 1;
    end;

  if Langs = 0 then
  begin
    MsgBox('Please tick at least one language.' + #13#10 + #13#10 +
           'Without a language there is no engine to speak with, and BestSpeech ' +
           'would add no voices at all.', mbError, MB_OK);
    Result := False;
  end
  else if (Multi > 0) and (PickedVoiceCount = 0) then
  begin
    MsgBox('Please tick at least one character voice.' + #13#10 + #13#10 +
           'The languages you chose speak in a character voice, so with none ticked ' +
           'they would add no voices at all. Fred is the usual choice.', mbError, MB_OK);
    Result := False;
  end;
end;

function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo, MemoTypeInfo,
                         MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
begin
  Result := MemoDirInfo + NewLine + NewLine + MemoTypeInfo + NewLine + NewLine +
            MemoComponentsInfo + NewLine + NewLine;
  if MemoTasksInfo <> '' then
    Result := Result + MemoTasksInfo + NewLine + NewLine;
  Result := Result + 'Voices that will appear in your speech settings:' + NewLine +
            Space + IntToStr(PickedTokenCount) + ' voices';
end;

procedure PayPalButtonClick(Sender: TObject);
var
  ErrorCode: Integer;
begin
  ShellExec('open', 'https://paypal.me/gozaltech', '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode);
end;

procedure CreateDonatePage;
begin
  DonatePage := CreateCustomPage(wpWelcome, 'Support Development', 'Help improve BestSpeech SAPI');

  DonateLabel := TNewStaticText.Create(DonatePage);
  DonateLabel.Parent := DonatePage.Surface;
  DonateLabel.Caption := 'Thank you for installing BestSpeech SAPI!';
  DonateLabel.Left := 0;
  DonateLabel.Top := 0;
  DonateLabel.Width := DonatePage.SurfaceWidth;
  DonateLabel.Height := 40;
  DonateLabel.Font.Size := 12;
  DonateLabel.Font.Style := [fsBold];

  DonateLabel2 := TNewStaticText.Create(DonatePage);
  DonateLabel2.Parent := DonatePage.Surface;
  DonateLabel2.Caption :=
    'This software is provided free of charge. If you find it useful, please consider ' +
    'supporting development with a donation.' + #13#10 + #13#10 +
    'Your support helps us continue improving this project.';
  DonateLabel2.Left := 0;
  DonateLabel2.Top := 50;
  DonateLabel2.Width := DonatePage.SurfaceWidth;
  DonateLabel2.Height := 80;
  DonateLabel2.AutoSize := False;
  DonateLabel2.WordWrap := True;

  PayPalButton := TNewButton.Create(DonatePage);
  PayPalButton.Parent := DonatePage.Surface;
  PayPalButton.Caption := 'Donate via PayPal';
  PayPalButton.Left := (DonatePage.SurfaceWidth - 150) div 2;
  PayPalButton.Top := 140;
  PayPalButton.Width := 150;
  PayPalButton.Height := 30;
  PayPalButton.OnClick := @PayPalButtonClick;
end;

procedure InitializeWizard();
begin
  CreateDonatePage;
end;

// The 32-bit worker keeps the engine dlls open on behalf of 64-bit hosts, so it has to
// be gone before any of them can be replaced or deleted.
procedure StopWorker;
var
  ResultCode: Integer;
begin
  Exec(ExpandConstant('{sys}\taskkill.exe'), '/F /IM BestspeechServer.exe',
       '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Sleep(500);
end;

// The record the engine reads when it registers its voice tokens. Written before
// registration, and deliberately plain text so a user can see what they chose.
procedure WriteSelectionFile;
var
  Contents: String;
begin
  Contents :=
    '; Which BestSpeech languages and character voices were installed.' + #13#10 +
    '; Written by the installer and read by the engine when its voices are' + #13#10 +
    '; registered, so editing it by hand changes nothing until then. Run the' + #13#10 +
    '; installer again to change what is installed.' + #13#10 +
    '[Selection]' + #13#10 +
    'Languages=' + PickedLanguageList + #13#10 +
    'Voices=' + PickedVoiceList + #13#10 +
    'CustomVoice=';
  if WizardIsComponentSelected('customvoice') then
    Contents := Contents + '1'
  else
    Contents := Contents + '0';
  Contents := Contents + #13#10;

  if not SaveStringToFile(ExpandConstant('{app}\voices.ini'), Contents, False) then
    MsgBox('The list of installed voices could not be written to ' +
           ExpandConstant('{app}\voices.ini') + '.' + #13#10 + #13#10 +
           'Every BestSpeech voice will be registered instead of only the ones you ' +
           'chose.', mbInformation, MB_OK);
end;

// A second run with fewer languages ticked leaves the earlier engine dlls behind,
// because Inno only ever adds files. They are dead weight once their voices are gone,
// so they go too. Safe here: the worker was killed at ssInstall and nothing has spoken
// since, so no engine dll is open.
procedure RemoveUnpickedEngines;
var
  I: Integer;
begin
  for I := 0 to 12 do
    if not LanguagePicked(I) then
      DeleteFile(ExpandConstant('{app}\') + LangDll[I]);
end;

// Each COM server must be registered by a regsvr32 of its own bitness: the 32-bit one
// writes the voice tokens under WOW6432Node where 32-bit SAPI hosts look, and the
// 64-bit one writes them to the native view where Narrator and other 64-bit hosts look.
function RegisterServer(const Dll: String; Use64: Boolean; Unregister: Boolean): Boolean;
var
  Exe, Args: String;
  ResultCode: Integer;
begin
  if Use64 then
    Exe := ExpandConstant('{sys}\regsvr32.exe')
  else if Is64BitInstallMode then
    Exe := ExpandConstant('{syswow64}\regsvr32.exe')
  else
    Exe := ExpandConstant('{sys}\regsvr32.exe');

  if Unregister then
    Args := '/s /u "' + Dll + '"'
  else
    Args := '/s "' + Dll + '"';

  Result := Exec(Exe, Args, '', SW_HIDE, ewWaitUntilTerminated, ResultCode) and (ResultCode = 0);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Failed: String;
begin
  if CurStep = ssInstall then
  begin
    StopWorker;
    // Drop any previous registration first so a rename or removal of a voice between
    // versions -- or a language unticked on this run -- cannot leave an orphaned token
    // pointing at this engine.
    RegisterServer(ExpandConstant('{app}\BestspeechSAPI.dll'), False, True);
    if Is64BitInstallMode then
      RegisterServer(ExpandConstant('{app}\x64\BestspeechSAPI.dll'), True, True);
  end
  else if CurStep = ssPostInstall then
  begin
    // Order matters: the selection has to be on disk before regsvr32 runs, because that
    // is what decides which tokens get written.
    WriteSelectionFile;
    RemoveUnpickedEngines;

    Failed := '';
    if not RegisterServer(ExpandConstant('{app}\BestspeechSAPI.dll'), False, False) then
      Failed := '32-bit';

    if Is64BitInstallMode then
    begin
      if not RegisterServer(ExpandConstant('{app}\x64\BestspeechSAPI.dll'), True, False) then
      begin
        if Failed <> '' then
          Failed := Failed + ' and 64-bit'
        else
          Failed := '64-bit';
      end;
    end;

    if Failed <> '' then
      MsgBox('The ' + Failed + ' speech engine could not be registered. ' +
             'The BestSpeech voices may not appear in your applications.' + #13#10 + #13#10 +
             'Try running the installer again as an administrator.', mbError, MB_OK);
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
  begin
    StopWorker;
    if Is64BitInstallMode then
      RegisterServer(ExpandConstant('{app}\x64\BestspeechSAPI.dll'), True, True);
    RegisterServer(ExpandConstant('{app}\BestspeechSAPI.dll'), False, True);
  end;
end;
