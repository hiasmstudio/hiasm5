{ Scheme snapshot written by src/SdkSerializer.cpp. Object addresses are the
  CGT handles given to CodeGen; strings are kept in CP1251 because the HiAsm 4
  Delphi package generates Windows-1251 sources. }
unit Model;

{$mode delphi}

interface

uses Windows, SysUtils, fpjson, jsonparser;

type
  Str1251 = type AnsiString(1251);

  TBElement = class;
  TBSdk = class
    Parent: TBElement;
    Elements: array of TBElement;
  end;

  TBData = class
    DataType, IntValue: integer;
    RealValue: double;
    StrValue: Str1251;
  end;
  TBFont = class
    Name: Str1251;
    Size, Style, Color, Charset: integer;
  end;
  TBArray = class;
  TBProp = class
    Name, StrValue, Resource: Str1251;
    PropType, IntValue: integer;
    RealValue: double;
    IsDefault, HasInt, HasReal, HasStr, HasResource: boolean;
    Data: TBData;
    Arr: TBArray;
    Font: TBFont;
  end;
  TBArray = class
    ItemType: integer;
    Items: array of TBProp;
  end;

  TBPoint = class
    Owner: TBElement;
    Name, Info, DpeName: Str1251;
    PointType, DataType, Index: integer;
    Link, RLink: TBPoint;
  end;
  TBElement = class
    ElClass, Sub, Interfaces, Inherit, CodeName: Str1251;
    Flags, ClassIndex, EID, Group, X, Y, W, H: integer;
    ParentSdk, Inner: TBSdk;
    LinkIs: boolean;
    LinkMain: TBElement;
    Props: array of TBProp;
    Points: array of TBPoint;
    UserData: pointer;
  end;

  TBScheme = class
    Root: TBSdk;
    ProjectPath, ProjectName: Str1251;
  end;

// Reads a UTF-8 snapshot; raises an exception with a reason on malformed input.
function LoadScheme(const FileName: UnicodeString): TBScheme;
function ToAnsi(const S: UnicodeString): Str1251;

implementation

function ToAnsi(const S: UnicodeString): Str1251;
begin
  Result := Str1251(S); // converted to code page 1251 by the RTL
end;

function ReadUtf8File(const FileName: UnicodeString): UTF8String;
var
  H: THandle;
  Size, Done: DWORD;
begin
  H := CreateFileW(PWideChar(FileName), GENERIC_READ, FILE_SHARE_READ, nil, OPEN_EXISTING, 0, 0);
  if H = INVALID_HANDLE_VALUE then
    raise Exception.Create('cannot open scheme snapshot: ' + SysErrorMessage(GetLastError));
  try
    Size := GetFileSize(H, nil);
    SetLength(Result, Size);
    if (Size > 0) and (not ReadFile(H, Result[1], Size, Done, nil) or (Done <> Size)) then
      raise Exception.Create('cannot read scheme snapshot');
  finally
    CloseHandle(H);
  end;
end;

function JStr(O: TJSONObject; const Key: string): Str1251;
var D: TJSONData;
begin
  D := O.Find(Key);
  if Assigned(D) and (D.JSONType = jtString) then Result := ToAnsi(D.AsUnicodeString) else Result := '';
end;

function JInt(O: TJSONObject; const Key: string; Default: integer): integer;
var D: TJSONData;
begin
  D := O.Find(Key);
  if Assigned(D) and (D.JSONType = jtNumber) then Result := D.AsInteger else Result := Default;
end;
function LoadProp(O: TJSONObject): TBProp;
var
  D: TJSONData;
  A: TJSONObject;
  Items: TJSONArray;
  I: integer;
begin
  Result := TBProp.Create;
  Result.Name := JStr(O, 'name');
  Result.PropType := JInt(O, 'type', 0);
  Result.IsDefault := O.Get('default', false);
  Result.Resource := JStr(O, 'resource');
  Result.HasResource := Result.Resource <> '';
  Result.HasInt := Assigned(O.Find('int'));
  Result.IntValue := JInt(O, 'int', 0);
  D := O.Find('real');
  Result.HasReal := Assigned(D) and (D.JSONType = jtNumber);
  if Result.HasReal then Result.RealValue := D.AsFloat;
  Result.HasStr := Assigned(O.Find('str'));
  Result.StrValue := JStr(O, 'str');
  if O.Find('data', A) then begin
    Result.Data := TBData.Create;
    Result.Data.DataType := JInt(A, 'type', 0);
    Result.Data.StrValue := JStr(A, 'str');
    Result.Data.IntValue := JInt(A, 'int', 0);
    D := A.Find('real');
    if Assigned(D) and (D.JSONType = jtNumber) then Result.Data.RealValue := D.AsFloat;
  end;
  if O.Find('font', A) then begin
    Result.Font := TBFont.Create;
    Result.Font.Name := JStr(A, 'name');
    Result.Font.Size := JInt(A, 'size', 0);
    Result.Font.Style := JInt(A, 'style', 0);
    Result.Font.Color := JInt(A, 'color', 0);
    Result.Font.Charset := JInt(A, 'charset', 0);
  end;
  if O.Find('array', A) then begin
    Result.Arr := TBArray.Create;
    Result.Arr.ItemType := JInt(A, 'type', 0);
    Items := A.Arrays['items'];
    SetLength(Result.Arr.Items, Items.Count);
    for I := 0 to Items.Count - 1 do
      Result.Arr.Items[I] := LoadProp(Items.Objects[I]);
  end;
end;
function LoadScheme(const FileName: UnicodeString): TBScheme;
var
  Root, O, P: TJSONObject;
  Sdks, Els, List, Ref: TJSONArray;
  S: array of TBSdk;
  E: array of TBElement;
  I, J: integer;

  function SdkAt(Index: integer): TBSdk;
  begin
    if (Index >= 0) and (Index < Length(S)) then Result := S[Index] else Result := nil;
  end;
  function ElementAt(Index: integer): TBElement;
  begin
    if (Index >= 0) and (Index < Length(E)) then Result := E[Index] else Result := nil;
  end;
  function PointAt(Data: TJSONData): TBPoint;
  var
    Owner: TBElement;
    Index: integer;
  begin
    Result := nil;
    if not (Data is TJSONArray) or (TJSONArray(Data).Count <> 2) then Exit;
    Owner := ElementAt(TJSONArray(Data).Integers[0]);
    Index := TJSONArray(Data).Integers[1];
    if Assigned(Owner) and (Index >= 0) and (Index < Length(Owner.Points)) then
      Result := Owner.Points[Index];
  end;
begin
  Root := GetJSON(ReadUtf8File(FileName)) as TJSONObject;
  try
    if JInt(Root, 'version', 0) <> 1 then raise Exception.Create('unsupported scheme snapshot version');
    Sdks := Root.Arrays['sdks'];
    Els := Root.Arrays['elements'];
    if Sdks.Count = 0 then raise Exception.Create('scheme snapshot has no root SDK');
    SetLength(S, Sdks.Count);
    for I := 0 to High(S) do S[I] := TBSdk.Create;
    SetLength(E, Els.Count);
    for I := 0 to High(E) do E[I] := TBElement.Create;
    // Pass 1: element fields, properties and points without links.
    for I := 0 to High(E) do begin
      O := Els.Objects[I];
      with E[I] do begin
        ElClass := JStr(O, 'class'); Sub := JStr(O, 'sub');
        Interfaces := JStr(O, 'interfaces'); Inherit := JStr(O, 'inherit');
        CodeName := JStr(O, 'codename');
        Flags := JInt(O, 'flags', 0); ClassIndex := JInt(O, 'classIndex', 0);
        EID := JInt(O, 'eid', 0); Group := JInt(O, 'group', 0);
        X := JInt(O, 'x', 0); Y := JInt(O, 'y', 0); W := JInt(O, 'w', 0); H := JInt(O, 'h', 0);
        ParentSdk := SdkAt(JInt(O, 'parent', -1)); Inner := SdkAt(JInt(O, 'inner', -1));
        LinkIs := O.Get('linkIs', false); LinkMain := ElementAt(JInt(O, 'linkMain', -1));
        List := O.Arrays['props'];
        SetLength(Props, List.Count);
        for J := 0 to List.Count - 1 do Props[J] := LoadProp(List.Objects[J]);
        List := O.Arrays['points'];
        SetLength(Points, List.Count);
        for J := 0 to List.Count - 1 do begin
          P := List.Objects[J];
          Points[J] := TBPoint.Create;
          Points[J].Owner := E[I];
          Points[J].Name := JStr(P, 'name'); Points[J].Info := JStr(P, 'info');
          Points[J].DpeName := JStr(P, 'dpe');
          Points[J].PointType := JInt(P, 'type', 0); Points[J].DataType := JInt(P, 'dataType', 0);
          Points[J].Index := JInt(P, 'index', 0);
        end;
      end;
    end;
    // Pass 2: links between points now that every element exists.
    for I := 0 to High(E) do begin
      List := Els.Objects[I].Arrays['points'];
      for J := 0 to List.Count - 1 do begin
        E[I].Points[J].Link := PointAt(List.Objects[J].Find('link'));
        E[I].Points[J].RLink := PointAt(List.Objects[J].Find('rlink'));
      end;
    end;
    for I := 0 to High(S) do begin
      O := Sdks.Objects[I];
      S[I].Parent := ElementAt(JInt(O, 'parent', -1));
      Ref := O.Arrays['elements'];
      SetLength(S[I].Elements, Ref.Count);
      for J := 0 to Ref.Count - 1 do begin
        S[I].Elements[J] := ElementAt(Ref.Integers[J]);
        if not Assigned(S[I].Elements[J]) then raise Exception.Create('bad element reference in SDK');
      end;
    end;
    Result := TBScheme.Create;
    Result.Root := S[0];
    O := Root.Objects['params'];
    Result.ProjectPath := JStr(O, 'projectPath');
    Result.ProjectName := JStr(O, 'projectName');
  finally
    Root.Free;
  end;
end;

end.
