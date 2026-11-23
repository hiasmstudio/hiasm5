{ The 85 CGT functions over a loaded TBScheme. Behaviour follows the IDE's own
  table in src/CodeGen.cpp; where the IDE has a stub, so does the bridge. }
unit Tools;

{$mode delphi}

interface

uses Windows, SysUtils, Classes, zstream, Cgt, Model;

// ACodePath is returned for PARAM_CODE_PATH (ANSI code page, trailing
// delimiter); CodeGen writes the generated units there. Resource files are
// written to AWorkDir, the same directory as a UTF-16 path.
procedure InitTools(var T: TCodeGenTools; AScheme: TBScheme; const ACodePath: RawByteString;
  const AWorkDir: UnicodeString);
// Writes one UTF-8 line to stdout; the IDE reads the same prefixes as cgt_on_debug.
procedure Say(const Line: UnicodeString);
// True when a resource could not be exported; CGT cannot abort CodeGen itself.
function ResourcesFailed: boolean;
// Writes allres.rc in the work directory, as HiAsm 4 writes compiler\allres.rc.
// DefaultIcon is copied as main.ico when the main element has no icon.
function WriteResourceScript(const RcFile, DefaultIcon: UnicodeString): boolean;

implementation

var
  Scheme: TBScheme;
  CodePath: RawByteString;
  WorkDir: UnicodeString;
  Failed: boolean;
  // Resource script lines and the names returned to CodeGen (kept alive here).
  RcLines, ResNames: array of RawByteString;
  ResPref: RawByteString;
  IconCount, StrCount, StreamCount, WaveCount, BitmapCount: integer;

procedure Say(const Line: UnicodeString);
var
  U: UTF8String;
  Done: DWORD;
begin
  U := UTF8Encode(Line) + #13#10;
  WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), U[1], Length(U), Done, nil);
end;

// CodeGen strings are Windows-1251 whatever the system code page is.
function Raw1251(P: PAnsiChar): Str1251;
var R: RawByteString;
begin
  Result := '';
  if P = nil then Exit;
  SetString(R, P, StrLen(P));
  SetCodePage(R, 1251, false);
  Result := R;
end;

function FromAnsi(P: PAnsiChar): UnicodeString;
begin
  Result := UnicodeString(Raw1251(P));
end;
//~~~~~~~~~~~~~~~~~~~~~~~~ SDK ~~~~~~~~~~~~~~~~~~~~~~~~~~

function sdkGetCount(SDK: id_sdk): integer; stdcall;
begin
  if SDK = nil then Result := 0 else Result := Length(TBSdk(SDK).Elements);
end;

function sdkGetElement(SDK: id_sdk; Index: integer): id_element; stdcall;
begin
  Result := nil;
  if (SDK <> nil) and (Index >= 0) and (Index < Length(TBSdk(SDK).Elements)) then
    Result := TBSdk(SDK).Elements[Index];
end;

function sdkGetElementName(SDK: id_sdk; Name: PAnsiChar): id_element; stdcall;
var I: integer;
begin
  Result := nil;
  if (SDK = nil) or (Name = nil) then Exit;
  for I := 0 to High(TBSdk(SDK).Elements) do
    if StrComp(PAnsiChar(TBSdk(SDK).Elements[I].ElClass), Name) = 0 then
      Exit(TBSdk(SDK).Elements[I]);
end;

function sdkGetParent(SDK: id_sdk): id_element; stdcall;
begin
  Result := TBSdk(SDK).Parent;
end;

//~~~~~~~~~~~~~~~~~~~~~~~~ Element ~~~~~~~~~~~~~~~~~~~~~~~~~~

function elGetFlag(e: id_element): cardinal; stdcall;
begin
  Result := TBElement(e).Flags;
end;

function elGetPropCount(e: id_element): cardinal; stdcall;
begin
  Result := Length(TBElement(e).Props);
end;

function elGetProperty(e: id_element; Index: integer): id_prop; stdcall;
begin
  Result := nil;
  if (Index >= 0) and (Index < Length(TBElement(e).Props)) then Result := TBElement(e).Props[Index];
end;

function elIsDefProp(e: id_element; Index: integer): boolean; stdcall;
begin
  Result := (Index >= 0) and (Index < Length(TBElement(e).Props)) and TBElement(e).Props[Index].IsDefault;
end;
// Native CGT returns the element itself.
function elSetCodeName(e: id_element; Name: PAnsiChar): cardinal; stdcall;
begin
  TBElement(e).CodeName := Raw1251(Name);
  Result := cardinal(e);
end;

function elGetCodeName(e: id_element): PAnsiChar; stdcall;
begin
  Result := PAnsiChar(TBElement(e).CodeName);
end;

function elGetClassName(e: id_element): PAnsiChar; stdcall;
begin
  Result := PAnsiChar(TBElement(e).ElClass);
end;

function elGetInfSub(e: id_element): PAnsiChar; stdcall;
begin
  Result := PAnsiChar(TBElement(e).Sub);
end;

function elGetInterface(e: id_element): PAnsiChar; stdcall;
begin
  Result := PAnsiChar(TBElement(e).Interfaces);
end;

function elGetInherit(e: id_element): PAnsiChar; stdcall;
begin
  Result := PAnsiChar(TBElement(e).Inherit);
end;

function elGetPtCount(e: id_element): cardinal; stdcall;
begin
  Result := Length(TBElement(e).Points);
end;

function elGetPt(e: id_element; i: integer): id_point; stdcall;
begin
  Result := nil;
  if (i >= 0) and (i < Length(TBElement(e).Points)) then Result := TBElement(e).Points[i];
end;

function elGetPtName(e: id_element; Name: PAnsiChar): id_point; stdcall;
var I: integer;
begin
  Result := nil;
  if Name = nil then Exit;
  for I := 0 to High(TBElement(e).Points) do
    if StrIComp(PAnsiChar(TBElement(e).Points[I].Name), Name) = 0 then Exit(TBElement(e).Points[I]);
end;
function elGetClassIndex(e: id_element): byte; stdcall;
begin
  Result := TBElement(e).ClassIndex;
end;

// Container: its inner SDK; otherwise the container that owns this SDK.
function elGetSDK(e: id_element): id_sdk; stdcall;
begin
  if Assigned(TBElement(e).Inner) then Result := TBElement(e).Inner
  else if Assigned(TBElement(e).ParentSdk) then Result := TBElement(e).ParentSdk.Parent
  else Result := nil;
end;

function elGetParent(e: id_element): id_sdk; stdcall;
begin
  Result := TBElement(e).ParentSdk;
end;

function elLinkIs(e: id_element): boolean; stdcall;
begin
  Result := TBElement(e).LinkIs;
end;

function elLinkMain(e: id_element): id_element; stdcall;
begin
  if TBElement(e).LinkIs then Result := TBElement(e).LinkMain else Result := nil;
end;

procedure elGetPos(e: id_element; var X, Y: integer); stdcall;
begin
  X := TBElement(e).X; Y := TBElement(e).Y;
end;

procedure elGetSize(e: id_element; var W, H: integer); stdcall;
begin
  W := TBElement(e).W; H := TBElement(e).H;
end;

function elGetEID(e: id_element): integer; stdcall;
begin
  Result := TBElement(e).EID;
end;

function elGetGroup(e: id_element): integer; stdcall;
begin
  if e = nil then Result := 0 else Result := TBElement(e).Group;
end;

function elGetData(e: id_element): pointer; stdcall;
begin
  Result := TBElement(e).UserData;
end;

procedure elSetData(e: id_element; data: pointer); stdcall;
begin
  TBElement(e).UserData := data;
end;
// Polymorphic containers are not supported by the IDE either.
function elGetSDKByIndex(e: id_element; index: integer): id_sdk; stdcall;
begin
  Result := nil;
end;

function elGetSDKCount(e: id_element): integer; stdcall;
begin
  if TBElement(e).ClassIndex = CI_MultiElement then Result := 1 else Result := 0;
end;

function elGetSDKName(e: id_element; index: integer): PAnsiChar; stdcall;
begin
  Result := nil;
end;

//~~~~~~~~~~~~~~~~~~~~~~~~ Point ~~~~~~~~~~~~~~~~~~~~~~~~~~

function ptGetLinkPoint(p: id_point): id_point; stdcall;
begin
  Result := TBPoint(p).Link;
end;

function ptGetRLinkPoint(p: id_point): id_point; stdcall;
begin
  Result := TBPoint(p).RLink;
end;

function ptGetType(p: id_point): cardinal; stdcall;
begin
  Result := TBPoint(p).PointType;
end;

function ptGetName(p: id_point): PAnsiChar; stdcall;
begin
  if p = nil then Result := nil else Result := PAnsiChar(TBPoint(p).Name);
end;

function ptGetParent(p: id_point): id_element; stdcall;
begin
  Result := TBPoint(p).Owner;
end;

function ptGetIndex(p: id_point): cardinal; stdcall;
begin
  Result := TBPoint(p).Index;
end;
function pt_dpeGetName(p: id_point): PAnsiChar; stdcall;
begin
  Result := PAnsiChar(TBPoint(p).DpeName);
end;

function ptGetDataType(p: id_point): integer; stdcall;
begin
  Result := TBPoint(p).DataType;
end;

function ptGetInfo(p: id_point): PAnsiChar; stdcall;
begin
  Result := PAnsiChar(TBPoint(p).Info);
end;

//~~~~~~~~~~~~~~~~~~~~~~~~ Property ~~~~~~~~~~~~~~~~~~~~~~~~~~

function propGetType(prop: id_prop): integer; stdcall;
begin
  Result := TBProp(prop).PropType;
end;

function propGetName(prop: id_prop): PAnsiChar; stdcall;
begin
  Result := PAnsiChar(TBProp(prop).Name);
end;

// Typed handle for data/array/font, otherwise a pointer to the scalar value.
// Numbers and strings are returned by address: CodeGen reads string(propGetValue(p)^)
// for data_str, so a string property gives the address of its string variable.
function propGetValue(prop: id_prop): pointer; stdcall;
var P: TBProp;
begin
  P := TBProp(prop);
  if Assigned(P.Data) then Result := P.Data
  else if Assigned(P.Arr) then Result := P.Arr
  else if Assigned(P.Font) then Result := P.Font
  else if P.HasReal then Result := @P.RealValue
  else if P.HasInt then Result := @P.IntValue
  else if P.HasStr then Result := @P.StrValue
  else Result := nil;
end;

function propToByte(prop: id_prop): byte; stdcall;
begin
  Result := byte(TBProp(prop).IntValue);
end;

function propToInteger(prop: id_prop): integer; stdcall;
begin
  Result := TBProp(prop).IntValue;
end;
function propToReal(prop: id_prop): double; stdcall;
begin
  Result := TBProp(prop).RealValue;
end;

// nil for types the IDE has no string form for, like propToString in CodeGen.cpp.
function propToString(prop: id_prop): PAnsiChar; stdcall;
begin
  if TBProp(prop).HasStr then Result := PAnsiChar(TBProp(prop).StrValue) else Result := nil;
end;

// The following property queries are stubs in the IDE as well.
function propGetLinkedElement(e: id_element; propName: PAnsiChar): id_element; stdcall;
begin
  Result := nil;
end;

function propGetLinkedElementInfo(e: id_element; prop: id_prop; _int: PAnsiChar): id_element; stdcall;
begin
  Result := nil;
end;

function propIsTranslate(e: id_element; p: id_prop): integer; stdcall;
begin
  Result := 0;
end;

function propSaveToFile(p: id_prop; fileName: PAnsiChar): integer; stdcall;
begin
  Result := 0;
end;

//~~~~~~~~~~~~~~~~~~~~~~~~ Res ~~~~~~~~~~~~~~~~~~~~~~~~~~

// Generated files stay in the work directory; the caller removes it.
function resAddFile(Name: PAnsiChar): cardinal; stdcall;
begin
  Result := 0;
end;

function Fail(const Message: UnicodeString): PAnsiChar;
begin
  Say('!' + Message);
  Failed := true;
  Result := nil;
end;

function HexValue(C: AnsiChar): integer;
begin
  case C of
    '0'..'9': Result := Ord(C) - Ord('0');
    'A'..'F': Result := Ord(C) - Ord('A') + 10;
    'a'..'f': Result := Ord(C) - Ord('a') + 10;
  else Result := -1;
  end;
end;

function HexToBytes(const Hex: RawByteString; From: integer; out Bytes: RawByteString): boolean;
var I, Hi, Lo: integer;
begin
  Result := (Length(Hex) - From + 1) mod 2 = 0;
  if not Result then Exit;
  SetLength(Bytes, (Length(Hex) - From + 1) div 2);
  for I := 1 to Length(Bytes) do begin
    Hi := HexValue(Hex[From + 2 * I - 2]); Lo := HexValue(Hex[From + 2 * I - 1]);
    if (Hi < 0) or (Lo < 0) then Exit(false);
    Bytes[I] := AnsiChar(Hi * 16 + Lo);
  end;
end;
// HiAsm 4 scheme text of a resource: "[<hex>]" or "[ZIP<size LE><zlib>]".
function DecodeResource(const Text: RawByteString; out Data: RawByteString): boolean;
var
  Raw: RawByteString;
  Size: cardinal;
  Input: TMemoryStream;
  Unzip: TDecompressionStream;
begin
  Data := '';
  if Text = '' then Exit(true);
  Result := false;
  if (Length(Text) < 2) or (Text[1] <> '[') or (Text[Length(Text)] <> ']') then Exit;
  if Copy(Text, 2, 3) <> 'ZIP' then Exit(HexToBytes(Copy(Text, 1, Length(Text) - 1), 2, Data));
  if not HexToBytes(Copy(Text, 1, Length(Text) - 1), 5, Raw) or (Length(Raw) < 4) then Exit;
  Move(Raw[1], Size, 4);
  // A memory stream keeps the bytes as they are; string streams may convert code pages.
  Input := TMemoryStream.Create;
  try
    if Length(Raw) > 4 then Input.WriteBuffer(Raw[5], Length(Raw) - 4);
    Input.Position := 0;
    Unzip := TDecompressionStream.Create(Input);
    try
      SetLength(Data, Size);
      if Size > 0 then Unzip.ReadBuffer(Data[1], Size);
      Result := true;
    finally
      Unzip.Free;
    end;
  except
    Data := '';
  end;
  Input.Free;
end;

function WriteBytes(const FileName: UnicodeString; const Data: RawByteString): boolean;
var
  H: THandle;
  Done: DWORD;
begin
  H := CreateFileW(PWideChar(FileName), GENERIC_WRITE, 0, nil, CREATE_ALWAYS, 0, 0);
  if H = INVALID_HANDLE_VALUE then Exit(false);
  Result := (Length(Data) = 0) or (WriteFile(H, Data[1], Length(Data), Done, nil) and (Done = DWORD(Length(Data))));
  CloseHandle(H);
end;
// Names and script lines follow TResource in HiAsm.exe: "<pref><kind><n>" and
// "<name> <type> <name><ext>"; the files are written next to allres.rc.
function AddResource(const Kind, RcType, Ext: RawByteString; Index: integer;
  const Data: RawByteString): PAnsiChar;
var Name: RawByteString;
begin
  Name := ResPref + Kind + RawByteString(IntToStr(Index));
  if not WriteBytes(WorkDir + UnicodeString(Name + Ext), Data) then
    Exit(Fail('cannot write resource ' + UnicodeString(Name + Ext)));
  SetLength(RcLines, Length(RcLines) + 1);
  RcLines[High(RcLines)] := Name + ' ' + RcType + ' ' + Name + Ext;
  SetLength(ResNames, Length(ResNames) + 1);
  ResNames[High(ResNames)] := Name;
  Result := PAnsiChar(ResNames[High(ResNames)]);
end;

const
  // HiAsm 4 returns the application icon for an empty Icon property.
  AsmaName: PAnsiChar = 'ASMA';

function resAddIcon(P: id_prop): PAnsiChar; stdcall;
var Data: RawByteString;
begin
  if not DecodeResource(TBProp(P).Resource, Data) then Exit(Fail('bad icon data in ' + UnicodeString(TBProp(P).Name)));
  if Data = '' then Exit(AsmaName);
  Result := AddResource('ICON', 'ICON', '.ico', IconCount, Data);
  Inc(IconCount);
end;

function resAddStr(P: PAnsiChar): PAnsiChar; stdcall;
var Data: RawByteString;
begin
  if (P = nil) or (P^ = #0) then Exit(nil);
  SetString(Data, P, StrLen(P) + 1); // the terminating zero is part of the resource
  Inc(StrCount);
  Result := AddResource('Str', '100', '.txt', StrCount, Data);
end;
// TOOLS_RES_4

// Stream, wave and bitmap payloads are complete files (BMP header included).
function AddPayload(P: id_prop; const Kind, RcType, Ext: RawByteString; var Count: integer): PAnsiChar;
var Data: RawByteString;
begin
  if not DecodeResource(TBProp(P).Resource, Data) then
    Exit(Fail('bad ' + UnicodeString(LowerCase(Kind)) + ' data in ' + UnicodeString(TBProp(P).Name)));
  if Data = '' then Exit(nil);
  Result := AddResource(Kind, RcType, Ext, Count, Data);
  Inc(Count);
end;

function resAddStream(P: id_prop): PAnsiChar; stdcall;
begin
  Result := AddPayload(P, 'STREAM', '100', '.dat', StreamCount);
end;

function resAddWave(P: id_prop): PAnsiChar; stdcall;
begin
  Result := AddPayload(P, 'WAVE', '100 LOADONCALL', '.wav', WaveCount);
end;

function resAddBitmap(P: id_prop): PAnsiChar; stdcall;
begin
  Result := AddPayload(P, 'BITMAP', 'BITMAP', '.bmp', BitmapCount);
end;

// The Delphi package never calls it; other packages would need MENU scripts.
function resAddMenu(P: id_prop): PAnsiChar; stdcall;
begin
  if TBProp(P).HasResource then Result := Fail('menu resources are not supported by the HiAsm 4 bridge')
  else Result := nil;
end;

function resEmpty: integer; stdcall;
begin
  if Length(RcLines) = 0 then Result := 1 else Result := 0;
end;

function resSetPref(pref: PAnsiChar): integer; stdcall;
begin
  if pref = nil then ResPref := '' else SetString(ResPref, pref, StrLen(pref));
  Result := 0;
end;

function ResourcesFailed: boolean;
begin
  Result := Failed;
end;

// HiAsm 4 adds ASMA only when the main element has no icon ("[]" counts as none).
function MainIconEmpty: boolean;
const IS_PARENT = $200;
var
  E: TBElement;
  P: TBProp;
  Data: RawByteString;
begin
  for E in Scheme.Root.Elements do
    if E.Flags and IS_PARENT <> 0 then begin
      for P in E.Props do
        if StrIComp(PAnsiChar(P.Name), 'Icon') = 0 then
          Exit(not DecodeResource(P.Resource, Data) or (Data = ''));
      Exit(true);
    end;
  Result := true;
end;

function WriteResourceScript(const RcFile, DefaultIcon: UnicodeString): boolean;
var
  Text: RawByteString;
  Line: RawByteString;
begin
  Text := '';
  if MainIconEmpty then begin
    if not CopyFileW(PWideChar(DefaultIcon), PWideChar(WorkDir + 'main.ico'), false) then begin
      Say('!cannot copy the default application icon: ' + DefaultIcon);
      Exit(false);
    end;
    Text := 'ASMA ICON main.ico'#13#10;
  end;
  for Line in RcLines do Text := Text + Line + #13#10;
  Result := WriteBytes(RcFile, Text);
  if not Result then Say('!cannot write ' + RcFile);
end;

//~~~~~~~~~~~~~~~~~~~~~~~~ Other ~~~~~~~~~~~~~~~~~~~~~~~~~~

// Same colour-to-prefix mapping as _Debug in CodeGen.cpp.
function _Debug(Text: PAnsiChar; Color: cardinal): cardinal; stdcall;
begin
  case Color of
    0: Say(FromAnsi(Text));
    $0000FF: Say('#' + FromAnsi(Text));
    $FF0000: Say('!' + FromAnsi(Text));
  else Say('@' + FromAnsi(Text));
  end;
  Result := 0;
end;
function _Error(Line: integer; e: id_element; Text: PAnsiChar): cardinal; stdcall;
begin
  Say('!' + FromAnsi(Text));
  Result := 0;
end;

function isDebug(e: id_sdk): longbool; stdcall;
begin
  Result := false;
end;

// Legacy GetParam has no length argument; callers reserve 256 bytes.
function CopyParam(value: pointer; const S: RawByteString): cardinal;
begin
  if Length(S) >= 256 then begin
    PAnsiChar(value)^ := #0;
    Exit(1);
  end;
  if Length(S) > 0 then Move(S[1], value^, Length(S));
  PAnsiChar(value)[Length(S)] := #0;
  Result := 0;
end;

// Values match GetParam in CodeGen.cpp. PARAM_HIASM_VERSION and
// PARAM_COMPILER are not used by the Delphi package and are not provided.
function GetParam(index: word; value: pointer): cardinal; stdcall;
begin
  Result := 0;
  if value = nil then Exit(1);
  case index of
    PARAM_CODE_PATH: Result := CopyParam(value, CodePath);
    PARAM_DEBUG_MODE: PInteger(value)^ := 1;
    PARAM_DEBUG_SERVER_PORT: PInteger(value)^ := 100;
    PARAM_DEBUG_CLIENT_PORT: PInteger(value)^ := 101;
    PARAM_PROJECT_PATH: Result := CopyParam(value, Scheme.ProjectPath);
    PARAM_PROJECT_NAME: Result := CopyParam(value, Scheme.ProjectName);
    PARAM_USER_NAME, PARAM_USER_MAIL: PAnsiChar(value)^ := #0;
    PARAM_SDE_WIDTH, PARAM_SDE_HEIGHT: ;
  else Result := 1;
  end;
end;
//~~~~~~~~~~~~~~~~~~~~~~~~ Arrays ~~~~~~~~~~~~~~~~~~~~~~~~~~

function arrCount(a: id_array): integer; stdcall;
begin
  if a = nil then Result := 0 else Result := Length(TBArray(a).Items);
end;

function arrType(a: id_array): integer; stdcall;
begin
  Result := TBArray(a).ItemType;
end;

function arrItemName(a: id_array; Index: integer): PAnsiChar; stdcall;
begin
  Result := PAnsiChar(TBArray(a).Items[Index].Name);
end;

function arrItemData(a: id_array; Index: integer): pointer; stdcall;
begin
  Result := propGetValue(TBArray(a).Items[Index]);
end;

function arrGetItem(a: id_array; Index: integer): id_prop; stdcall;
begin
  Result := nil;
  if (a <> nil) and (Index >= 0) and (Index < Length(TBArray(a).Items)) then
    Result := TBArray(a).Items[Index];
end;

//~~~~~~~~~~~~~~~~~~~~~~~~ Data ~~~~~~~~~~~~~~~~~~~~~~~~~~

function dtType(d: id_data): byte; stdcall;
begin
  Result := TBData(d).DataType;
end;

function dtStr(d: id_data): PAnsiChar; stdcall;
begin
  Result := PAnsiChar(TBData(d).StrValue);
end;

function dtInt(d: id_data): integer; stdcall;
begin
  Result := TBData(d).IntValue;
end;

function dtReal(d: id_data): double; stdcall;
begin
  Result := TBData(d).RealValue;
end;
//~~~~~~~~~~~~~~~~~~~~~~~~ Font ~~~~~~~~~~~~~~~~~~~~~~~~~~

function fntName(f: id_font): PAnsiChar; stdcall;
begin
  Result := PAnsiChar(TBFont(f).Name);
end;

function fntSize(f: id_font): integer; stdcall;
begin
  Result := TBFont(f).Size;
end;

function fntStyle(f: id_font): byte; stdcall;
begin
  Result := TBFont(f).Style;
end;

function fntColor(f: id_font): cardinal; stdcall;
begin
  Result := TBFont(f).Color;
end;

function fntCharSet(f: id_font): byte; stdcall;
begin
  Result := TBFont(f).Charset;
end;

//~~~~~~~~~~~~~~~~~~~~~~~~ PropertyList ~~~~~~~~~~~~~~~~~~~~~~~~~~
// The IDE reports an empty property list, so no item handle is ever valid.

function elGetPropertyListCount(e: id_element): cardinal; stdcall;
begin
  Result := 0;
end;

function elGetPropertyListItem(e: id_element; i: integer): id_proplist; stdcall;
begin
  Result := nil;
end;

function plGetString(p: id_proplist): PAnsiChar; stdcall;
begin
  Result := nil;
end;

function plGetProperty(p: id_proplist): id_prop; stdcall;
begin
  Result := nil;
end;

function plGetOwner(p: id_proplist): id_element; stdcall;
begin
  Result := nil;
end;
procedure InitTools(var T: TCodeGenTools; AScheme: TBScheme; const ACodePath: RawByteString;
  const AWorkDir: UnicodeString);
type TSlots = array[0..SizeOf(TCodeGenTools) div SizeOf(Pointer) - 1] of pointer;
var I: integer;
begin
  Scheme := AScheme;
  CodePath := ACodePath;
  WorkDir := AWorkDir;
  FillChar(T, SizeOf(T), 0);
  T.sdkGetCount := sdkGetCount; T.sdkGetElement := sdkGetElement;
  T.sdkGetElementName := sdkGetElementName;
  T.elGetFlag := elGetFlag; T.elGetPropCount := elGetPropCount; T.elGetProperty := elGetProperty;
  T.elIsDefProp := elIsDefProp; T.elSetCodeName := elSetCodeName; T.elGetCodeName := elGetCodeName;
  T.elGetClassName := elGetClassName; T.elGetInfSub := elGetInfSub; T.elGetPtCount := elGetPtCount;
  T.elGetPt := elGetPt; T.elGetPtName := elGetPtName; T.elGetClassIndex := elGetClassIndex;
  T.elGetSDK := elGetSDK; T.elLinkIs := elLinkIs; T.elLinkMain := elLinkMain;
  T.elGetPos := elGetPos; T.elGetSize := elGetSize; T.elGetEID := elGetEID;
  T.ptGetLinkPoint := ptGetLinkPoint; T.ptGetRLinkPoint := ptGetRLinkPoint; T.ptGetType := ptGetType;
  T.ptGetName := ptGetName; T.ptGetParent := ptGetParent; T.ptGetIndex := ptGetIndex;
  T.pt_dpeGetName := pt_dpeGetName;
  T.propGetType := propGetType; T.propGetName := propGetName; T.propGetValue := propGetValue;
  T.propToByte := propToByte; T.propToInteger := propToInteger; T.propToReal := propToReal;
  T.propToString := propToString;
  T.resAddFile := resAddFile; T.resAddIcon := resAddIcon; T.resAddStr := resAddStr;
  T.resAddStream := resAddStream; T.resAddWave := resAddWave; T.resAddBitmap := resAddBitmap;
  T.resAddMenu := resAddMenu;
  T._Debug := _Debug; T.GetParam := GetParam;
  T.arrCount := arrCount; T.arrType := arrType; T.arrItemName := arrItemName;
  T.arrItemData := arrItemData; T.arrGetItem := arrGetItem; T.isDebug := isDebug;
  T.dtType := dtType; T.dtStr := dtStr; T.dtInt := dtInt; T.dtReal := dtReal;
  T.fntName := fntName; T.fntSize := fntSize; T.fntStyle := fntStyle; T.fntColor := fntColor;
  T.fntCharSet := fntCharSet;
  T.elGetData := elGetData; T.elSetData := elSetData;
  T.ptGetDataType := ptGetDataType; T.elGetParent := elGetParent;
  T.elGetPropertyListCount := elGetPropertyListCount; T.elGetPropertyListItem := elGetPropertyListItem;
  T.plGetName := plGetString; T.plGetInfo := plGetString; T.plGetGroup := plGetString;
  T.plGetProperty := plGetProperty; T.plGetOwner := plGetOwner;
  T.ptGetInfo := ptGetInfo;
  T.propGetLinkedElement := propGetLinkedElement; T.propIsTranslate := propIsTranslate;
  T.propGetLinkedElementInfo := propGetLinkedElementInfo;
  T.elGetSDKByIndex := elGetSDKByIndex; T.elGetSDKCount := elGetSDKCount;
  T.elGetSDKName := elGetSDKName;
  T.sdkGetParent := sdkGetParent;
  T.elGetInterface := elGetInterface; T.elGetInherit := elGetInherit;
  T.resEmpty := resEmpty; T.resSetPref := resSetPref;
  T._Error := _Error;
  T.elGetGroup := elGetGroup;
  T.propSaveToFile := propSaveToFile;
  // A missed slot would crash inside CodeGen far from its cause.
  for I := Low(TSlots) to High(TSlots) do
    if TSlots(T)[I] = nil then raise Exception.CreateFmt('CGT slot %d is not assigned', [I]);
end;

end.
