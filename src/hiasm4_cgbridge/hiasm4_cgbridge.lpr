{ 32-bit host for HiAsm 4 Delphi code generators.

  hiasm4_cgbridge --codegen <CodeGen.dll> --make <make_xxx.dll>
                  --scheme <snapshot.json> --work <dir> --project <file.dpr>
                  [--rc <allres.rc> --default-icon <main.ico>] [--code <pack code dir>]
  hiasm4_cgbridge --codegen <CodeGen.dll> --scheme <snapshot.json> --code <dir> --work <dir> --result <file>
  Without --make (packages with no project maker, such as web) the codegen text is written to --result.
  hiasm4_cgbridge --complete --make <make_xxx.dll> --project <file.dpr> --app <file.exe>

  The first form loads the scheme written by src/SdkSerializer.cpp, runs
  buildPrepareProc and buildProcessProc of CodeGen.dll and passes the result
  to buildMakePrj of the project maker in the same process: the result is a
  Delphi record only that maker can read. Units and resource files are
  generated in --work; --rc writes the resource script there. The second form
  calls buildCompliteProc after the compiler has run. Output lines use the
  cgt_on_debug prefixes; exit code 0 means success. }
program hiasm4_cgbridge;

{$mode delphi}
{$pointermath on}

uses Windows, SysUtils, Classes, Cgt, Model, Tools;

const
  LOAD_WITH_ALTERED_SEARCH_PATH = $00000008;

function CommandLineToArgvW(lpCmdLine: PWideChar; out pNumArgs: integer): PPWideChar;
  stdcall; external 'shell32.dll';

var
  Args: array of UnicodeString;

procedure ReadArgs;
var
  P: PPWideChar;
  N, I: integer;
begin
  P := CommandLineToArgvW(GetCommandLineW, N);
  if P = nil then raise Exception.Create('cannot read the command line');
  SetLength(Args, N);
  for I := 0 to N - 1 do Args[I] := P[I];
  LocalFree(HLOCAL(P));
end;

function Arg(const Name: UnicodeString): UnicodeString;
var I: integer;
begin
  for I := 1 to High(Args) - 1 do
    if Args[I] = '--' + Name then Exit(Args[I + 1]);
  raise Exception.Create('missing argument --' + UTF8Encode(Name));
end;

function HasArg(const Name: UnicodeString): boolean;
var I: integer;
begin
  for I := 1 to High(Args) do
    if Args[I] = '--' + Name then Exit(true);
  Result := false;
end;
// KOL in the Delphi DLLs opens files through the ANSI API.
function AnsiPath(const Path: UnicodeString): RawByteString;
var A: AnsiString;
begin
  A := AnsiString(Path);
  if UnicodeString(A) <> Path then
    raise Exception.Create('path is not representable in the ANSI code page: ' + UTF8Encode(Path));
  Result := A;
end;

function LoadDll(const Path: UnicodeString): HMODULE;
var Error: DWORD;
begin
  Result := LoadLibraryExW(PWideChar(Path), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
  if Result = 0 then begin
    Error := GetLastError;
    raise Exception.Create('cannot load ' + UTF8Encode(Path) + ': ' + SysErrorMessage(Error));
  end;
end;

function Proc(Module: HMODULE; const Name: AnsiString): pointer;
begin
  Result := GetProcAddress(Module, PAnsiChar(Name));
  if Result = nil then raise Exception.Create('entry point ' + Name + ' not found');
end;

type
  TFreeBuildResult = procedure(value: pointer); cdecl;

// Web codegen: the generated text is the whole result; written as bytes, without a BOM.
procedure WriteResultText(const FileName: UnicodeString; Text: pointer);
var
  Stream: TFileStream;
begin
  if Text = nil then raise Exception.Create('buildProcessProc returned no text');
  Stream := TFileStream.Create(string(AnsiPath(FileName)), fmCreate);
  try
    Stream.WriteBuffer(PByte(Text)^, StrLen(PAnsiChar(Text)));
  finally
    Stream.Free;
  end;
end;

procedure Generate;
var
  CgtTable: TCodeGenTools;
  Scheme: TBScheme;
  Work: UnicodeString;
  CodeDir: UnicodeString;
  CodeGen, Maker: HMODULE;
  Process: TBuildProcessRec;
  MakePrj: TBuildMakePrjRec;
  ProjectFile: RawByteString;
  FreeResult: pointer;
  Code: integer;
begin
  Work := Arg('work');
  if (Work <> '') and (Work[Length(Work)] <> '\') and (Work[Length(Work)] <> '/') then Work := Work + '\';
  Scheme := LoadScheme(Arg('scheme'));
  // PARAM_CODE_PATH: the Delphi codegen writes its units to the work directory; the web codegen
  // reads its hi<Class>.hws templates from the pack's code folder given by --code.
  CodeDir := Work;
  if HasArg('code') then begin
    CodeDir := Arg('code');
    if (CodeDir <> '') and (CodeDir[Length(CodeDir)] <> #92) and (CodeDir[Length(CodeDir)] <> #47) then
      CodeDir := CodeDir + #92;
  end;
  InitTools(CgtTable, Scheme, AnsiPath(CodeDir), Work);
  CodeGen := LoadDll(Arg('codegen'));
  Code := TbuildPrepareProc(Proc(CodeGen, 'buildPrepareProc'))(nil);
  if Code <> CG_SUCCESS then raise Exception.CreateFmt('buildPrepareProc failed: %d', [Code]);
  Process.cgt := @CgtTable;
  Process.sdk := Scheme.Root;
  Process.result := nil;
  Code := TbuildProcessProc(Proc(CodeGen, 'buildProcessProc'))(Process);
  if Code <> CG_SUCCESS then raise Exception.CreateFmt('buildProcessProc failed: %d', [Code]);
  if Process.result = nil then raise Exception.Create('buildProcessProc returned no result');
  if ResourcesFailed then raise Exception.Create('resources could not be exported');
  if not HasArg('make') then begin
    WriteResultText(Arg('result'), Process.result);
    FreeResult := GetProcAddress(CodeGen, 'freeBuildResult');
    if FreeResult <> nil then TFreeBuildResult(FreeResult)(Process.result);
    Exit;
  end;
  ProjectFile := AnsiPath(Arg('project'));
  Maker := LoadDll(Arg('make'));
  // The IDE runs project makers from the directory with the generated units.
  if not SetCurrentDirectoryW(PWideChar(Work)) then raise Exception.Create('cannot enter the work directory');
  MakePrj.result := Process.result;
  MakePrj.prjFilename := PAnsiChar(ProjectFile);
  Code := TbuildMakePrj(Proc(Maker, 'buildMakePrj'))(@MakePrj);
  if Code <> CG_SUCCESS then raise Exception.CreateFmt('buildMakePrj failed: %d', [Code]);
  if HasArg('rc') and not WriteResourceScript(Arg('rc'), Arg('default-icon')) then
    raise Exception.Create('resource script was not written');
end;

procedure Complete;
var
  Rec: TBuildCompliteRec;
  ProjectFile, AppFile: RawByteString;
  Code: integer;
begin
  ProjectFile := AnsiPath(Arg('project'));
  AppFile := AnsiPath(Arg('app'));
  Rec.prjFilename := PAnsiChar(ProjectFile);
  Rec.appFilename := PAnsiChar(AppFile);
  Code := TbuildCompliteProc(Proc(LoadDll(Arg('make')), 'buildCompliteProc'))(@Rec);
  if Code <> CG_SUCCESS then raise Exception.CreateFmt('buildCompliteProc failed: %d', [Code]);
end;

begin
  try
    ReadArgs;
    if HasArg('complete') then Complete else Generate;
  except
    on E: Exception do begin
      Say('!' + UnicodeString(E.Message));
      Halt(1);
    end;
  end;
end.
