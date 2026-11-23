{$MODESWITCH ANSISTRINGS+}
{$MODESWITCH AUTODEREF+}
{$MODESWITCH PCHARTOSTRING+}
{$MODESWITCH POINTERTOPROCVAR-}

library CodeGen;

uses
  Classes, SysUtils, Math,
  //MD5 in '.\FTCG\MD5.pas',
  CGTShare in 'CGTShare.pas';

{$define debug}
{define called_func}      //    -
{define called_func_sec}  //    -,
{define processed_func}   //     -
{define print_lexem}      //   print
{define call_func}        //   -
{define event_call}       //
{define processed_unit}   //    unit-
{define init_section}     //   ,  init
{define _blocks_}         //

{define OP_CALL}          //   

{define _damped_}         //     

(*
**   todo
**     -    data_array
**
**
*)

const
   ER_SYNTAX            = 'Syntax error';

   ER_FUNC_ARGS         = 'Error reading argumets %s: ';
   ER_FUNC_ARGS_OPEN    = ER_FUNC_ARGS + 'expected symbol (';
   ER_FUNC_ARGS_CLOSE   = ER_FUNC_ARGS + 'expected symbol )';
   ER_FUNC_ARGS_INVALID = ER_FUNC_ARGS + 'expected variable or expression';
   ER_FUNC_ARGS_SYNTAX  = ER_FUNC_ARGS + 'syntax error';
   ER_FUNC_ARGS_COUNT   = ER_FUNC_ARGS + 'incorrect count. Expected number of arguments: %d';

   ER_FOR               = 'Error syntax operator for: ';
   ER_FOR_EXP_EXISTS    =  ER_FOR + 'expected expression';
   ER_FOR_VAR_LOC       =  ER_FOR + 'expected name of local variable';

   ER_LEX_NOT_FOUND     = 'Lexem %s not found';
   ER_SYMBOL_EXISTS     = 'Expected symbol %s';

   ER_OBJ_FIELD_ACCESS  = 'Error access in object fields %s: expected operator .';
   ER_OBJ_METHOD_BAD    = 'Method %s for object %s not supported';

   ER_VAR_EXISTS        = 'Variable with name %s already exists';
   ER_VAR_NOT_FOUND     = 'Variable with name %s not found';

   ER_TOK_DNT_SUP       = 'Symbol with code 0x%s at position %d not supported';

type
  TLangRec = record
    entry:PChar;
    name:PChar;
    str_del_o:PChar;
    str_del_c:PChar;
    op_del:PChar;
    var_mask:PChar;
    tostr_proc:procedure(var s:string);
  end;

type
  TCGrec = record
    Code:TStringList;
  end;
  PCGrec = ^TCGrec;

{$ifdef CPU64}
  DATA_INTEGER = Int64;
{$else}
  DATA_INTEGER = Integer;
{$endif}

  PSubType = ^TSubType;
  TData = record
    //  
    data_type:byte;
    //  
    idata:DATA_INTEGER;
    sdata:string;
    rdata:real;
    //  
    lang:byte;
    level:integer; //   
    //   code
    sub_type:PSubType;
    // 
    flags:cardinal;
  end;
  TSubType = record
    //  
    data_type:byte;
    // 
    sub_type:PSubType;
  end;
  PScData = ^TScData;
  PScArray = ^TScArray;
  TScData = object
    private
     value:TData;
     ldata:PScData;
     procedure Clear;
    public
     procedure Create;
     procedure SetValue(const val:string); overload;
     procedure SetValue(val:PChar); overload;
     procedure SetValue(val:DATA_INTEGER); overload;
     procedure SetValue(val:real); overload;
     procedure SetValue(val:boolean); overload;
     procedure SetValue(const val:TData); overload;
     procedure SetValue(const val:string; tp:byte); overload;
     procedure SetValue(val:pointer); overload;

     procedure AddValue(val:PScData; wo_op:boolean = false);
     procedure AddNormalize(BaseLang:byte; BaseLevel:integer; data:PScData);
     procedure GetValue(var val:TData);

     procedure mtAttach(dt:PScData);
     function mtGetCount:integer;
     function mtGetItem(ind:integer):PScData;
     function mtPop:PScData;
     procedure SetAsMT;

     function GetType:byte;
     procedure SetType(tp:byte);
     function GetLang:byte;
     procedure SetLang(lang, level:byte);
     function GetLevel:byte;
     function GetSubType:byte;
     procedure SetSubType(stype:byte);

     function printDebug:string;

     function toStr:string;
     function toInt:DATA_INTEGER;
     function toReal:real;
     function toCode:string;
     function toBool:boolean;
     function toArray:PScArray;
     function toPointer:pointer;

     function isEmpty:boolean;
     function isValue(const val:string):boolean;

     function ReadItem(ind:integer):PScData;

     procedure BuildArray;

     //____ MATH _____
     procedure mathAdd(val:PScData); overload;
     procedure mathAdd(val:integer); overload;
     procedure mathAdd(val:real); overload;
     procedure mathAdd(const val:string); overload;
     procedure mathSub(val:PScData);
     procedure mathMul(val:PScData);
     procedure mathDiv(val:PScData);
     //function isStr:boolean;
     //function isInt:boolean;
     //function isReal:boolean;
  end;
  TScArray = record
    Items:array of PScData;
    Count:integer;
  end;

  TBlockStack = class
    private
      Items:array of record
         LType:word;
         skip:boolean;
         userdata:pointer;
      end;
      Count:word;
    public
      procedure Push(_skip:boolean; _Type:word; _userdata:pointer);
      function Pop(var _Skip:boolean;var _Type:word; var _userdata:pointer):boolean;
      procedure Clear;
  end;
  TCodeBlockRec = record
    StrList:TList;
    level:integer;
    FLevelEn:boolean;
  end;
  PCodeBlockRec = ^TCodeBlockRec;
  TCodeBlock = class
   private
    FItems:TStringList;
    CurItem:PCodeBlockRec;
    Current:TList;
    FCurName:string;
    FGen:integer;

    function Find(const Name:string):PCodeBlockRec;
    function GetItems(Index:integer):PCodeBlockRec;
    function GetCount:integer;
    function GetLevel:integer;
    procedure SetLevel(Value:integer);
    function GenSpaces:string;
   public
    lsize:integer;

    constructor Create;
    destructor Destroy; override;
    procedure LevelEnabled(value:boolean);
    procedure Reg(const Name:string);
    function RegGen:string;
    function Select(const Name:string):string;
    procedure Delete(const Name:string);
    procedure CopyTo(const Source, Dest:string);
    procedure Merge(List:TStringList);
    procedure Add(text:PScData);
    procedure Paste(text:PScData);
    procedure AddStrings(list:TList);
    procedure MergeFromFile(const FileName:string);
    function Empty:boolean;
    function isEmpty(const Name:string):boolean;

    function AsText:TScData;  //    
    function AsCode:string;   //      Code

    procedure SaveToFile(const fname, name:string; lang:byte; _asCode:boolean = false; checkHash:boolean = false);

    property Items[Index:integer]:PCodeBlockRec read GetItems;
    property Count:integer read GetCount;
    property CurBlockName:string read FCurName;
    property Level:integer read GetLevel write SetLevel;
    property CurList:TList read Current;
  end;
  TArgs = class;
  TFuncData = record
    name:string;
    Line:integer;
    Skeep,fskeep:boolean;
    inFunc:boolean;
    fsection:integer;
    _args_:TArgs;
    _vars_:TArgs;   //     -
    _data_saved_:TScData;
  end;
  PFuncData = ^TFuncData;

  TFuncState = record
    Line:string;
    LineIndex,LPos:integer;
    Token,RToken:string;
    TokType:byte;
    fData:PFuncData;
  end;
  PFuncState = ^TFuncState;
  TArgs = class
   private
    FItems:TStringList;

    function GetValues(index:integer):PScData;
    function GetNames(index:integer):string;
    procedure SetNames(index:integer; const Value:string);
    function GetCount:integer;
    procedure SetValue(index:integer; value:PScData);
   public
    FIndex:integer;

    constructor Create;
    destructor Destroy; override;

    function AddArg(const Name:string):PScData;
    function find(const name:string):PScData;
    procedure Remove(const name:string);

    property Names[Index:integer]:string read GetNames write SetNames;
    property Values[Index:integer]:PScData read GetValues write SetValue;
    property Count:integer read GetCount;
  end;
  TWhileData = record
    index:integer;
    for_cond:string;
  end;
  PWhileData = ^TWhileData;
  TElementData = record
    VarList:TArgs;
    LangList:TArgs;
  end;
  PElementData = ^TElementData;
  TParser = class
    private
     VarList:TArgs;          //   
     LangList:TArgs;         //    
     funcList:TStringList;

     _for_cond:string;      // ...

     _state_stack:TList;

     function regGVar(const name:string):PScData;

     procedure Debug(const text:string; color:integer = 0);
     function ToSection(var sec:integer):boolean;
     procedure SaveState;
     procedure LoadState;
     function _points_count_(ptype:byte):integer;
     function findProperty(const name:string):integer;

     procedure LineInsert(index:integer; const line:string);

     function ToType(const token:string; var tp:integer):boolean;

     function CheckSkipped:boolean;
     function CheckOpenArgs(const op:string):boolean;
     function CheckCloseArgs(const op:string):boolean;
     function CheckSymbol(const sym:string):boolean;

     procedure Print(const Text:string);
     procedure PrintLine;
    protected
     fData:PFuncData;
     _data_return_:TScData;

     Token,RToken:string;
     TokType:byte;
     uname:string;
     Line:string;
     Lines:TStringList;
     LineIndex,LPos:integer;
     oldLineIndex,oldLPos:integer;
     cgt:PCodeGenTools;
     el:id_element;
     BlockStack:TBlockStack;
     codeb:TCodeBlock;
     _err_:boolean;
     section:integer;    //    

     function ReadLine:boolean;
     procedure AddError(const Error:string);
     procedure Start(const fname:string; Args:TArgs);
     function ReadPoint(lp:id_point):PScData;
     function ReadID:string;

     procedure ReadValue(const Name:string; var _var_:PScData; null_data:boolean = true);
     function Level1(var _var_:PScData; flag:integer = 0):boolean;
     function Level2(var _var_:PScData; flag:integer):boolean; // ^
     function Level2a(var _var_:PScData; flag:integer):boolean; // &, &&
     function Level3(var _var_:PScData; flag:integer):boolean; // or, and
     function Level4a(var _var_:PScData; flag:integer):boolean; // _or_, _and_
     function Level4(var _var_:PScData; flag:integer):boolean; // =,<,>,<=,>=,!=
     function Level5(var _var_:PScData; flag:integer):boolean; // - +
     function Level6(var _var_:PScData; flag:integer):boolean; // * /
     function Level7(var _var_:PScData; flag:integer):boolean; // ?
     function Level8(var _var_:PScData; flag:integer):boolean; // !, not, -
     function Level9(var _var_:PScData; flag:integer):boolean; // ++, --
     function Level10(var _var_:PScData; flag:integer):boolean; // []
     function Level11(var _var_:PScData; flag:integer):boolean; // @
     function Level12(var _var_:PScData; flag:integer):boolean;

     procedure FuncLexem(const fname:string; Args:TArgs);
     function EndLexem:boolean;
     procedure IncludeLexem;
     procedure InlineLexem;
     procedure SubLexem;
     procedure SectionLexem;
     procedure FVarLexem;
     procedure VarLexem;
     procedure GVarLexem;
     procedure LangLexem;
     procedure FreeLexem;
     procedure ReturnLexem;
     procedure ForLexem;
     procedure WhileLexem;
     procedure IfLexem;
     procedure ElseIfLexem;
     procedure ElseLexem;
     procedure SwitchLexem;
     procedure CaseLexem;
     procedure IncSecLexem;
     procedure DecSecLexem;
     function EventLexem:PScData;
     function mtEventLexem:PScData;
     procedure PrintLexem;
     procedure PrintLnLexem;
     procedure VarOpLexem(var_int:PScData);
     function ReadCustomValue:PScData;

     function getInternalVar(const name:string; var int_var:PScData):boolean;
     function getElementVar(const name:string; var el_var:PScData):boolean;
     function getGlobalVar(const name:string; var gbl_var:PScData):boolean;
     function getLangVar(const name:string; var lng_var:PScData):boolean;
     function isFunction(const name:string; var ind:integer):boolean;
     function isInternalFunction(const name:string; var ind:integer):boolean;
     function isObject(const name:string; var ind:integer):boolean;

     function LinkedLexem:boolean;
     function IsDefLexem:boolean;
     function IsSetLexem:boolean;
     function IsPropLexem:boolean;
     function StrLexem:PScData;
     function CodeLexem:PScData;
     function lCodeLexem:PScData;
     function CvtLexem(toTypeaa:byte):PScData;
     function CountLexem:PScData;
     function IsSecLexem:boolean;
     function TypeOfLexem:PScData;
     function ExpOfLexem:PScData;
     function SubLexem_res:PScData;
     function CallFunc(ind:integer):PScData;
     function CallIntFunc(ind:integer):PScData;
     function CallObject(ind:integer):PScData;
     function CallTypeConvertion(ind:integer):PScData;

     function CallEvent(const event:string; data:TScData):PScData;
     function CallEventMt(const event:string; args:TArgs):PScData;
    public
     constructor Create;
     destructor Destroy; override;
     procedure SetElement(e:id_element);
     function GetToken:boolean;
     procedure PutToken;
     procedure Parse(_cgt:PCodeGenTools; e:id_element; const fname:string; _codeb:TCodeBlock; Args:TArgs; ret_data:PScData);
     procedure ReadFuncList(list:TStringList; _cgt:PCodeGenTools; e:id_element; const fname:string);
     function readSection(const s:string):integer;
  end;

 TMapProc = function (parser:TParser; args:TArgs):TScData;
 TObjProc = function (parser:TParser; obj:pointer; index:integer; args:TArgs):TScData;
 TFuncMap = record
   name:string;
   count:integer;   //  -1    
   proc:TMapProc;
   ainfo:string;
 end;
 TObjField = record
   name:string;
 end;
 TObjMethod = record
   name:string;
   count:integer;   //  -1    
   ainfo:string;
 end;
 TAObjMethod = array of TObjMethod;
 TObjMap = record
   name:string;
   obj:pointer;
   fields_count:integer;   //   
   methods_count:integer;  //   
   fields:array of TObjField;
   methods:TAObjMethod;
   method_proc:TObjProc;
 end;

const
  data_object = 100; //    

  //  ,   GetTok
  TokName   = 1;
  TokNumber = 2;
  TokReal   = 3;
  TokString = 4;
  TokSymbol = 5;
  TokMath   = 6;
  TokCode   = 7;
  TokProp   = 8;

  //   
  BLK_FUNC   = 0;
  BLK_IF     = 1;
  BLK_ELSEIF = 2;
  BLK_WHILE  = 3;
  BLK_SWITCH = 4;
  BLK_CASE   = 5;

  MATH_ASSIGN = $01; //    

  DT_FLG_DIRECT = $01;

var el_cnt:integer;
    nsection:integer;    //  
    lang_level:integer;  //   
    GVarList:TArgs;      //   

    LngUserTypes:TStringList; //     
    LngTypeCounter:integer;  //  ID   

    hnt_point:id_point;
    hnt_this:boolean;
    hnt_message:string;

    readCustomProperty:procedure(Result:PScData; e:id_element; cgt:PCodeGenTools; prop:id_prop);

    {$ifdef _damped_}
    list:PStrList;
    {$endif}

procedure initAllFreeElements(cgt:PCodeGenTools; sdk:id_sdk); forward;
procedure call_init(cgt:PCodeGenTools; e:id_element; const proc:string); forward;

function compareTwoLists(l1, l2:TStringList):boolean;
var i:integer;
begin
  l1.text := l1.text;
  Result := false;
  if l1.count <> l2.count then exit;
 
  for i := 0 to l1.count-1 do
    if l1.Strings[i] <> l2.Strings[i] then
       exit;

  result := true;
end;

function StrIComp(s1,s2:PChar):integer;
begin
   if LowerCase(s1) = LowerCase(s2) then
     Result := 0
   else
     Result := 1;
end;


function MakeData(const stra:string; code:boolean = false):PScData; overload;
begin
   New(Result);
   FillChar(Result^, sizeof(TScData), 0);
   if code then Result^.SetValue(stra, data_code)
   else Result^.SetValue(stra);
end;

function MakeData(num:integer):PScData; overload;
begin
   New(Result);
   FillChar(Result^, sizeof(TScData), 0);
   Result^.SetValue(num);
end;

function MakeData(num:real):PScData; overload;
begin
   New(Result);
   FillChar(Result^, sizeof(TScData), 0);
   Result^.SetValue(num);
end;

function MakeData(value:boolean):PScData; overload;
begin
   New(Result);
   FillChar(Result^, sizeof(TScData), 0);
   Result^.SetValue(value);
end;

function MakeData(const value:TData):PScData; overload;
begin
   New(Result);
   FillChar(Result^, sizeof(TScData), 0);
   Result^.SetValue(value);
end;

function MakeData(data:PScData):PScData; overload;
var i:integer;
    arr:PScArray;
begin
   New(Result);
   FillChar(Result^, sizeof(TScData), 0);
   Result.setValue(data.value);
   if data.GetType = data_array then
    begin
      new(arr);
      arr.Count := data.toArray().Count;
      SetLength(arr.items, arr.count);
      for i := 0 to arr.Count-1 do
        arr.Items[i] := MakeData(data.toArray().Items[i]);
      Result.value.idata := DATA_INTEGER(arr);
      Result.value.data_type := data_array;
    end;
   if data.ldata <> nil then
    Result.ldata := MakeData(data.ldata);
end;

function MakeMethod(const name:string; count:integer; const ainfo:string):TObjMethod;
begin
   Result.name := name;
   Result.count := count;
   Result.ainfo := ainfo;
end;

//____________________

function RegisterUserType(const Name:string):integer;
begin
  Result := 0;
  if LngUserTypes = nil then exit;

  inc(LngTypeCounter);
  LngUserTypes.AddObject(Name, TObject(LngTypeCounter));
  Result := LngTypeCounter;
end;

function FindUserType(const Name:string; var ind:integer):boolean;
var i:integer;
begin
  for i := 0 to LngUserTypes.Count-1 do
   if StrIComp(PChar(Name), PChar(LngUserTypes[i])) = 0 then
    begin
      ind := integer(LngUserTypes.Objects[i]);
      result := true;
      exit;
    end;
  Result := false;
end;

//*******************************************************

{$i direct.inc}

//*******************************************************

function map_replace(parser:TParser; args:TArgs):TScData;
var s:string;
    tp:byte;
begin
  s := args.Values[0].toStr();
//  parser.cgt._Debug(PChar('Replace...' + args.Values[0].toStr() + ':' + args.Values[1].toStr()),clred);
  Replace(s, args.Values[1].toStr(), args.Values[2].toStr());
  tp := args.Values[0].GetType;
  if tp <> data_code then
    tp := data_str;
  args.Values[0].SetValue(s, tp);
  Result := args.Values[0]^;
end;

function map_lower(parser:TParser; args:TArgs):TScData;
begin
   Result.Create;
   Result.SetValue(LowerCase(args.Values[0].toStr()), args.Values[0].GetType);
end;

function map_upper(parser:TParser; args:TArgs):TScData;
begin
   Result.Create;
   Result.SetValue(UpperCase(args.Values[0].toStr()),args.Values[0].GetType);
end;

function map_pos(parser:TParser; args:TArgs):TScData;
begin
   Result.Create;
   Result.SetValue(pos(args.Values[0].toStr(), args.Values[1].toStr()));
end;

function map_copy(parser:TParser; args:TArgs):TScData;
var tp:byte;
begin
  tp := args.Values[0].GetType;
  if tp <> data_code then
    tp := data_str;
  Result.Create;
  Result.SetValue(copy(args.Values[0].toStr(), args.Values[1].toInt(), args.Values[2].toInt()), tp);
end;

function map_delete(parser:TParser; args:TArgs):TScData;
var s:string;
    tp:byte;
begin
   s := args.Values[0].toStr();
   delete(s, args.Values[1].toInt(), args.Values[2].toInt());
   tp := args.Values[0].GetType;
   if tp <> data_code then
     tp := data_str;
   args.Values[0].SetValue(s, tp);
   Result := args.Values[0]^;
end;

function map_trace(parser:TParser; args:TArgs):TScData;
begin
  Parser.Debug(Args.Values[0].toStr(), $00FF00);
  Result.Create;
  Result.SetValue('');
end;

function map_fopen(parser:TParser; args:TArgs):TScData;
var fs:TFileStream;
    md,fname:string;
begin
    fname := args.Values[0].toStr();
    md := args.Values[1].toCode();
    if md = 'r' then
      fs := TFileStream.Create(fname, fmOpenRead)
    else fs := TFileStream.Create(fname, fmOpenWrite);
    Result.Create;
    Result.SetValue(DATA_INTEGER(fs));
end;

function map_fputs(parser:TParser; args:TArgs):TScData;
var fs:TFileStream;
begin
  fs := TFileStream(args.Values[0].toInt());
  fs.WriteAnsiString(args.Values[1].toCode() + #13#10);
  Result.Create;
  Result.SetValue('');
end;

function map_fgets(parser:TParser; args:TArgs):TScData;
var fs:TFileStream;
    s:string;
    b:char;
begin
   fs := TFileStream(args.Values[0].toInt());
   s := '';
   while fs.Position < fs.Size do
     begin
      fs.Read(b,1);
      if b = #13 then
       begin
         fs.Read(b,1);
         break;
       end
      else s := s + b;
     end;
  Result.Create;
  Result.SetValue(s);
end;

function map_fclose(parser:TParser; args:TArgs):TScData;
var fs:TFileStream;
begin
   fs := TFileStream(args.Values[0].toInt());
   fs.Free;
   Result.Create;
   Result.SetValue('');
end;

function map_project_dir(parser:TParser; args:TArgs):TScData;
var ppath:array[0..255] of char;
begin
   pointer(pointer(@ppath)^) := Parser.el;
   Parser.cgt.GetParam(PARAM_PROJECT_PATH, @ppath);
   Result.Create;
   Result.SetValue(PChar(@ppath));
end;

function map_project_name(parser:TParser; args:TArgs):TScData;
var ppath:array[0..255] of char;
begin
   pointer(pointer(@ppath)^) := Parser.el;
   Parser.cgt.GetParam(PARAM_PROJECT_NAME, @ppath);
   Result.Create;
   Result.SetValue(ExtractFileName(PChar(@ppath)));
end;

function map_error(parser:TParser; args:TArgs):TScData;
begin
   Parser.AddError(args.Values[0].toStr());
   Result.Create;
   Result.SetValue('');
end;

function map_lng_lvl(parser:TParser; args:TArgs):TScData;
begin
   Result.Create;
   Result.SetValue(lang_level);
   lang_level := lang_level + args.Values[0].toInt();
end;

const
  //   - 
  func_map_size = 15;
  func_map:array[0..func_map_size-1] of TFuncMap = (
    // ------------ STRINGS ----------------------------
    (name: 'replace';     count:3;  proc:map_replace;  ainfo: 'str, dst, src'),
    (name: 'lower';       count:1;  proc:map_lower;    ainfo: 'str'),
    (name: 'upper';       count:1;  proc:map_upper;    ainfo: 'str'),
    (name: 'copy';        count:3;  proc:map_copy;     ainfo: 'str, position, length'),
    (name: 'pos';         count:2;  proc:map_pos;      ainfo: 'substr, str'),
    (name: 'delete';      count:3;  proc:map_delete;   ainfo: 'str, position, length'),

    // ------------ SYSTEM -----------------------------
    (name: 'trace';       count:1;  proc:map_trace;    ainfo: 'text'),
    (name: 'error';       count:1;  proc:map_error;    ainfo: 'text'),
    (name: 'lng_lvl';     count:1;  proc:map_lng_lvl;  ainfo: 'increment'),

    // ------------ FILES ------------------------------
    (name: 'fopen';       count:2;  proc:map_fopen;    ainfo: 'filename, mode'),
    (name: 'fputs';       count:2;  proc:map_fputs;    ainfo: 'id, str'),
    (name: 'fgets';       count:1;  proc:map_fgets;    ainfo: 'id'),
    (name: 'fclose';      count:1;  proc:map_fclose;   ainfo: 'id'),

    // ------------ ENVEROUMENT ------------------------
    (name: 'project_dir'; count:0;  proc:map_project_dir; ainfo: ''),
    (name: 'project_name'; count:0;  proc:map_project_name; ainfo: '')
  );
var
  //    
  obj_map_size:integer;
  obj_map:array of TObjMap;

function block_proc(parser:TParser; obj:TCodeBlock; index:integer; args:TArgs):TScData;
var s:string;
    i:integer;
begin
   Result.Create;
   Result.SetValue('');
   case index of
    0: obj.Reg(args.Values[0].toStr());
    1: Result.SetValue(obj.Select(args.Values[0].toStr()));
    2: obj.Delete(args.Values[0].toStr());
    3: obj.CopyTo(obj.CurBlockName,args.Values[0].toStr());
    4: Result := obj.AsText();
    5: if hnt_point = nil then obj.SaveToFile(args.Values[0].toStr(),obj.CurBlockName, parser.section);
    6: Result.SetValue(obj.CurBlockName);
    7: Result.SetValue(obj.RegGen());
    8: Result.SetValue(obj.empty());
    9: obj.level := obj.level + 1;
    10: obj.level := obj.level - 1;
    11: Result.SetValue(obj.level);
    12: obj.LevelEnabled(true);
    13: obj.LevelEnabled(false);
    14: Result.SetValue(obj.CurItem.FLevelEn);
    15: obj.CopyTo(args.Values[0].toStr(),obj.CurBlockName);
    16: Result.SetValue(obj.isEmpty(args.Values[0].toStr()));
    17:
      begin
        s := obj.AsCode();
        lngs[nsection].tostr_proc(s);
        Result.SetValue(s);
      end;
    18:
      begin
        Result.SetValue(false);
        s := args.Values[0].toStr();
        for i := 0 to obj.CurList.Count-1 do
         if s = PScData(obj.CurList[i]).value.sdata then
           begin
             Result.SetValue(true);
             break;
           end;
      end;
     19: obj.SaveToFile(args.Values[0].toStr(),obj.CurBlockName, parser.section, true, true);
     20: obj.MergeFromFile(args.Values[0].toStr());
   end;
end;

function ReadPointByTypeIndex(parser:TParser; ind:integer; ptype:byte):id_point;
var i,c:integer;
begin
  Result := 0;
  c := 0;
  with parser do
   for i := 0 to cgt.elGetPtCount(el)-1 do
    if cgt.ptGetType(cgt.elGetPt(el, i)) = ptype then
     if c = ind then
      begin
        Result := cgt.elGetPt(el, i);
        exit;
      end
     else inc(c)
end;

const
  CGT_pt_get_parent     = 0;
  CGT_pt_get_link_point = CGT_pt_get_parent + 1;
  CGT_pt_get_rlink_point= CGT_pt_get_link_point + 1;
  CGT_pt_get_name       = CGT_pt_get_rlink_point + 1;
  CGT_pt_get_name_byid  = CGT_pt_get_name + 1;
  CGT_pt_arr_work       = CGT_pt_get_name_byid + 1;
  CGT_pt_arr_event      = CGT_pt_arr_work + 1;
  CGT_pt_arr_var        = CGT_pt_arr_event + 1;
  CGT_pt_arr_data       = CGT_pt_arr_var + 1;
  CGT_pt_get_type       = CGT_pt_arr_data + 1;
  CGT_pt_get_data_type  = CGT_pt_get_type + 1;
  CGT_el_get_class_name = CGT_pt_get_data_type + 1;
  CGT_el_get_code_name  = CGT_el_get_class_name + 1;
  CGT_el_get_child_id   = CGT_el_get_code_name + 1;
  CGT_el_get_parent_id  = CGT_el_get_child_id + 1;
  CGT_el_get_point_name = CGT_el_get_parent_id + 1;
  CGT_el_link_is        = CGT_el_get_point_name + 1;
  CGT_el_link_main      = CGT_el_link_is + 1;
  CGT_el_get_parent     = CGT_el_link_main + 1;
  CGT_get_point_id      = CGT_el_get_parent + 1;
  CGT_sdk_get_count     = CGT_get_point_id + 1;
  CGT_sdk_get_element   = CGT_sdk_get_count + 1;
  CGT_sdk_get_element_name = CGT_sdk_get_element + 1;
  CGT_res_add_file      = CGT_sdk_get_element_name + 1;

function cgt_proc(parser:TParser; obj:pointer; index:integer; args:TArgs):TScData;
begin
  Result.Create;
  with parser do
   case index of
    CGT_pt_get_parent: Result.SetValue(cgt.ptGetParent(args.Values[0].toPointer()));
    CGT_pt_get_link_point: Result.SetValue(cgt.ptGetLinkPoint(args.Values[0].toPointer()));
    CGT_pt_get_rlink_point: Result.SetValue(cgt.ptGetRLinkPoint(args.Values[0].toPointer()));
    CGT_pt_get_name: Result.SetValue(cgt.ptGetName(cgt.elGetPt(Parser.el, args.Values[0].toInt())));
    CGT_pt_get_name_byid: Result.SetValue(cgt.ptGetName(args.Values[0].toPointer()));
    CGT_pt_arr_work..CGT_pt_arr_data: Result.SetValue(ReadPointByTypeIndex(parser,args.Values[0].toInt(), index - CGT_pt_arr_work + 1));
    CGT_pt_get_type: Result.SetValue(cgt.ptGetType(args.Values[0].toPointer()));
    CGT_pt_get_data_type: Result.SetValue(cgt.ptGetDataType(args.Values[0].toPointer()));

    CGT_el_get_class_name: Result.SetValue(cgt.elGetClassName(args.Values[0].toPointer()));
    CGT_el_get_code_name: Result.SetValue(cgt.elGetCodeName(args.Values[0].toPointer()));
    CGT_el_get_child_id: Result.SetValue(cgt.sdkGetElement(cgt.elGetSDK(el),args.Values[0].toInt()));
    CGT_el_get_parent_id: Result.SetValue(cgt.elGetSDK(parser.el));
    CGT_el_get_point_name: Result.SetValue(cgt.elGetPtName(args.Values[0].toPointer(), PChar(args.Values[1].toStr())));
    CGT_el_link_is: Result.SetValue(cgt.elLinkIs(parser.el));
    CGT_el_link_main: Result.SetValue(cgt.elLinkMain(parser.el));

    CGT_el_get_parent: Result.SetValue(cgt.elGetParent(args.Values[0].toPointer()));
    CGT_get_point_id: Result.SetValue(cgt.elGetPtName(el,PChar(args.Values[0].toStr())));

    CGT_sdk_get_count: Result.SetValue(cgt.sdkGetCount(args.Values[0].toPointer()));
    CGT_sdk_get_element: Result.SetValue(cgt.sdkGetElement(args.Values[0].toPointer(), args.Values[1].toInt()));
    CGT_sdk_get_element_name: Result.SetValue(cgt.sdkGetElementName(args.Values[0].toPointer(), PChar(args.Values[1].toStr)));
    CGT_res_add_file:
     begin
        if hnt_point = nil then Result.SetValue(cgt.resAddFile(PChar(args.Values[0].toStr())));
     end;
   end;
end;

function TimeToStr(const Format:string; const t:TSystemTime):string;
const namstr:string = 'YMWDhms';
type TTimeValue = array[0..6] of WORD;
     PTimeValue = ^TTimeValue;
var
  i:byte;
  function TwoDigit(value:integer):string;
  begin
    if Value < 10 then
      Result := '0' + IntToStr(value)
    else Result := IntToStr(value);
  end;
begin
   Result := Format;
   for i := 0 to 6 do
     Replace(Result,namstr[i+1],TwoDigit(PTimeValue(@t)^[i]));
end;

function sys_proc(parser:TParser; obj:pointer; index:integer; args:TArgs):TScData;
var ind:integer;
    buf:array[0..128] of char;
    SystemTime: TSystemTime;
    e:id_element;
    pars:TParser;
    pargs:TArgs;
begin
   Result.Create;
   case index of
    0:
      begin
        Result.SetValue(parser.el);
        e := args.Values[0].toPointer();
        if parser.cgt.elGetData(e) = nil then
          call_init(parser.cgt, e, 'initFree');
        parser.SetElement(e);
      end;
     1: Result.SetValue(GVarList.Count);
     2:
       if GVarList.find(args.Values[0].toStr()) <> nil then
         Result.SetValue(GVarList.FIndex)
       else Result.SetValue(-1);
     3,4,5,6:
      begin
         ind := args.Values[0].toInt();
         if(ind >= 0)and(ind < GVarList.Count)then
          case index of
            3: Result.SetValue(GVarList.Names[ind]);
            4: Result := GVarList.Values[ind]^;
            5: GVarList.Values[ind]^ := args.Values[1]^;
            6: GVarList.Remove(GVarList.Names[ind]);
          end;
      end;
     7: Result.SetValue(obj_map_size);
     8:
       begin
         ind := args.Values[0].toInt;
         if(ind >= 0)and(ind < obj_map_size)then
           Result.SetValue(obj_map[ind].name)
         else Result.SetValue('Unknown');
       end;
     9: Result.SetValue(Parser.el);
     10:
      begin
        StrCopy(buf, PChar(args.Values[0].toStr()));
        Parser.cgt.GetParam(PARAM_HIASM_VERSION, @buf);
        Result.SetValue(PChar(@buf[0]));
      end;
     11:
      begin
        GetLocalTime(SystemTime);
        Result.SetValue(TimeToStr(args.Values[0].toStr(),SystemTime));
      end;
     12:
      begin
        Result.SetValue(el_cnt);
        el_cnt := args.Values[0].toInt();
      end;
     13: initAllFreeElements(parser.cgt, parser.cgt.elGetParent(parser.el));
     14: Result.SetValue(parser.cgt.ReadCodeDir(parser.el));
     15:
      begin
        pars := TParser.create;
        pargs:= TArgs.create;
        for ind := 2 to args.count-1 do
         begin
          pargs.AddArg('');
          pargs.Values [ind-2] := args.Values [ind];
         end;
        pars.Parse(parser.cgt, args.Values[0].toPointer(), args.Values[1].toStr(), parser.codeb, pargs, @result);
        pars.destroy;
        pargs.destroy;
      end;
   end;
end;

function array_proc(parser:TParser; obj:pointer; index:integer; args:TArgs):TScData;
var i:integer;
    s,d,nv:string;
    dt:PScData;
begin
   Result.Create;
   case index of
     0: // count
      if args.Values[0].GetType = data_array then
        Result.SetValue(args.Values[0].toArray().count)
      else Result.SetValue(1);
     1: // join
      if args.Values[0].GetType = data_array then
       begin
        s := '';
        d := args.Values[1].toStr;
        for i := 0 to args.Values[0].toArray().count-1 do
         begin
           case args.Values[1].GetType of
            data_str: nv := args.Values[0].toArray().Items[i].toStr;
            data_code: nv := args.Values[0].toArray().Items[i].toCode;
            else nv := args.Values[0].toArray().Items[i].toStr;
           end;
           if i = 0 then
            s := nv
           else s := s + d + nv;
         end;
        Result.SetValue(s, args.Values[1].GetType);
       end
      else Result := args.Values[0]^;
     2: Result.SetValue(args.Values[0].mtGetCount());
     3:
      begin
        i := args.Values[1].toInt();
        if i = 0 then
          Result := args.Values[0]^
        else
         begin
            dt := args.Values[0].mtGetItem(i);
            if dt = nil then
              result := makedata('undefined')^
            else Result := dt^;
         end;
      end;
     4: Result := args.Values[0].mtPop()^;
   end;
end;

procedure CreateObjMap;
begin
  obj_map_size := 5;
  SetLength(obj_map, obj_map_size);
  with obj_map[0] do
   begin
     name := 'block';
     obj := TCodeBlock.Create;
     fields_count := 0;
     methods_count := 21;
     SetLength(fields,fields_count);
     SetLength(methods,methods_count);
     methods[0] := MakeMethod('reg',    1, 'blockName');
     methods[1] := MakeMethod('select', 1, 'blockName');
     methods[2] := MakeMethod('delete', 1, 'blockName');
     methods[3] := MakeMethod('copyto', 1, 'destBlockName');
     methods[4] := MakeMethod('astext', 0, '');
     methods[5] := MakeMethod('save',   1, 'fileName');
     methods[6] := MakeMethod('cur',    0, '');
     methods[7] := MakeMethod('reggen', 0, '');
     methods[8] := MakeMethod('empty',  0, '');
     methods[9] := MakeMethod('inclvl', 0, '');
     methods[10] := MakeMethod('declvl',0, '');
     methods[11] := MakeMethod('getlvl',0, '');
     methods[12] := MakeMethod('lvlon', 0, '');
     methods[13] := MakeMethod('lvloff',0, '');
     methods[14] := MakeMethod('lvlenabled',0, '');
     methods[15] := MakeMethod('copyhere',1, 'blockName');
     methods[16] := MakeMethod('isempty',1, 'blockName');
     methods[17] := MakeMethod('ascode', 0, '');
     methods[18] := MakeMethod('intext', 1, 'text');
     methods[19] := MakeMethod('savecode', 1, 'fileName');
     methods[20] := MakeMethod('mergefrom', 1, 'fileName');
     method_proc := TObjProc(@block_proc);
   end;
  with obj_map[1] do
   begin
     name := 'cgt';
     obj := nil;
     fields_count := 0;
     methods_count := 24;
     SetLength(fields,fields_count);
     SetLength(methods,methods_count);
     methods[CGT_pt_get_parent]     := MakeMethod('pt_get_parent',      1,  'id_point');
     methods[CGT_pt_get_link_point] := MakeMethod('pt_get_link_point',  1,  'id_point');
     methods[CGT_pt_get_rlink_point] := MakeMethod('pt_get_rlink_point',1,  'id_point');
     methods[CGT_pt_get_name]       := MakeMethod('pt_get_name',        1,  'absIndex');
     methods[CGT_pt_get_name_byid]  := MakeMethod('pt_get_name_byid',   1,  'id_point');
     methods[CGT_pt_arr_work]       := MakeMethod('pt_arr_work',        1,  'workIndex');
     methods[CGT_pt_arr_event]      := MakeMethod('pt_arr_event',       1,  'eventIndex');
     methods[CGT_pt_arr_var]        := MakeMethod('pt_arr_var',         1,  'varIndex');
     methods[CGT_pt_arr_data]       := MakeMethod('pt_arr_data',        1,  'dataIndex');
     methods[CGT_pt_get_type]       := MakeMethod('pt_get_type',        1,  'id_point');
     methods[CGT_pt_get_data_type]  := MakeMethod('pt_get_data_type',   1,  'id_point');

     methods[CGT_el_get_class_name] := MakeMethod('el_get_class_name',  1,  'id_element');
     methods[CGT_el_get_code_name]  := MakeMethod('el_get_code_name',   1,  'id_element');
     methods[CGT_el_get_child_id]   := MakeMethod('el_get_child_id',    1,  'index');
     methods[CGT_el_get_parent_id]  := MakeMethod('el_get_parent_id',   0,  '');
     methods[CGT_el_get_point_name] := MakeMethod('el_get_point_name',  2,  'id_element, name');
     methods[CGT_el_link_is]        := MakeMethod('el_link_is',         0,  '');
     methods[CGT_el_link_main]      := MakeMethod('el_link_main',       0,  '');
     methods[CGT_el_get_parent]     := MakeMethod('el_get_parent',      1,  'id_element');
     methods[CGT_get_point_id]      := MakeMethod('get_point_id',       1,  'pointName');

     methods[CGT_sdk_get_count]     := MakeMethod('sdk_get_count',      1,  'id_sdk');
     methods[CGT_sdk_get_element]   := MakeMethod('sdk_get_element',    2,  'id_sdk, index');
     methods[CGT_sdk_get_element_name] := MakeMethod('sdk_get_element_name', 2, 'id_sdk, name');
     methods[CGT_res_add_file]      := MakeMethod('res_add_file',       1,  'fileName');

     method_proc := cgt_proc;
   end;
  with obj_map[2] do
   begin
     name := 'sys';
     obj := nil;
     fields_count := 0;
     methods_count := 16;
     SetLength(fields,fields_count);
     SetLength(methods,methods_count);
     methods[0] := MakeMethod('selectelement',     1, 'id_element');

     methods[1] := MakeMethod('gvarcount',         0, '');
     methods[2] := MakeMethod('gvarfind',          1, 'name');
     methods[3] := MakeMethod('gvarname',          1, 'index');
     methods[4] := MakeMethod('gvargetvalue',      1, 'index');
     methods[5] := MakeMethod('gvarsetvalue',      2, 'index, value');
     methods[6] := MakeMethod('gvardestroy',       1, 'index');

     methods[7] := MakeMethod('obj_count',         0, '');
     methods[8] := MakeMethod('obj_names',         1, 'index');

     methods[9] := MakeMethod('curelement',        0, '');

     methods[10] := MakeMethod('hi_version',       1, 'mask');
     methods[11] := MakeMethod('time',             1, 'mask');

     methods[12] := MakeMethod('setindex',         1, 'index');
     methods[13] := MakeMethod('initall',          0, '');
     methods[14] := MakeMethod('codedir',          0, '');
     methods[15] := MakeMethod('event',           -1, 'id_element, point[, args]');
     method_proc := sys_proc;
   end;
  with obj_map[3] do
   begin
     name := '_arr';
     obj := nil;
     fields_count := 0;
     methods_count := 5;
     SetLength(fields,fields_count);
     SetLength(methods,methods_count);
     methods[0] := MakeMethod('count',     1, 'array');
     methods[1] := MakeMethod('join',      2, 'array, splitter');

     methods[2] := MakeMethod('mt_count',  1, 'mt array');
     methods[3] := MakeMethod('mt_item',   2, 'mt array, index');
     methods[4] := MakeMethod('mt_pop',    1, 'mt array');

     method_proc := array_proc;
   end;
  with obj_map[4] do
   begin
     name := 'lng';
     obj := nil;
     fields_count := 0;
     fill_lng_object(methods, methods_count);
     method_proc := lng_proc;
   end;
end;

procedure DestroyObjMap;
begin
  TCodeBlock(obj_map[0].Obj).Destroy;
end;

//*******************************************************

procedure TScData.Create;
begin
  ldata := nil;
  FillChar(Value, sizeof(TData), 0);
end;

procedure TScData.Clear;
begin
   if ldata <> nil then
     dispose(ldata);
end;

procedure TScData.SetValue(val:PChar);
begin
   Value.data_type := data_str;
   Value.sdata := val;
   SetLang(nsection,lang_level);
end;
procedure TScData.SetValue(const val:string);
begin
   Value.data_type := data_str;
   Value.sdata := val;
   SetLang(nsection,lang_level);
end;

procedure TScData.SetValue(val:DATA_INTEGER);
begin
   Value.data_type := data_int;
   Value.idata := val;
   SetLang(nsection,lang_level);
end;

procedure TScData.SetValue(val:real);
begin
   Value.data_type := data_real;
   Value.rdata := val;
   SetLang(nsection,lang_level);
end;

procedure TScData.SetValue(val:boolean);
begin
   Value.data_type := data_int;
   Value.idata := integer(val);
   SetLang(nsection,lang_level);
end;

procedure TScData.SetValue(const val:TData);
begin
   Value := val;
end;

procedure TScData.SetValue(const val:string; tp:byte);
begin
   SetValue(val);
   Value.data_type := tp;
end;

procedure TScData.SetValue(val:pointer);
begin
   SetValue(DATA_INTEGER(val));
end;

procedure TScData.mtAttach;
var l:^PScData;
begin
   l := @ldata;
   while l^ <> nil do
     l := @l^.ldata;
   l^ := dt;
end;

function TScData.mtGetCount;
var l:PScData;
begin
   l := ldata;
   Result := 1;
   while l <> nil do
    begin
      inc(Result);
      l := l.ldata;
    end;
end;

function TScData.mtGetItem;
var l:PScData;
    i:integer;
begin
   l := ldata;
   i := 0;
   while l <> nil do
    begin
     inc(i);
     if i = ind then
      begin
         Result := l;
         exit;
      end;
      l := l.ldata;
    end;
  Result := nil;
end;

function TScData.mtPop;
begin
   Result := MakeData(value);
   if ldata <> nil then
    begin
      value := ldata.value;
      ldata := ldata.ldata;
    end;
end;

procedure TScData.SetAsMT;
begin
   if ldata = nil then
     SetValue('')
   else
    begin
      value := ldata.value;
      ldata := ldata.ldata;
    end;
end;

procedure TScData.AddValue;
var arr:PScArray;
    i,wop:integer;
begin
   if Value.data_type <> data_array then
    begin
      new(arr);
      FillChar(arr^,sizeof(TScArray),0);
      if not isEmpty then
       begin
        inc(arr.count);
        SetLength(arr.items,arr.count);
        arr.items[0] := MakeData(value);
       end;
      Value.idata := DATA_INTEGER(arr);
      Value.data_type := data_array;
      value.sub_type := nil;
    end
   else arr := PScArray(value.idata);

   if val.GetType = data_array then
    begin
     SetLength(arr.items,arr.count + val.toArray.Count);
     for i := 0 to val.toArray.Count-1 do
       arr.items[arr.count + i] := MakeData(val.toArray.Items[i]);
     wop := arr.count;
     inc(arr.count,val.toArray.Count);
    end
   else
    begin
     inc(arr.count);
     SetLength(arr.items,arr.count);
     arr.items[arr.count-1] := MakeData(val);
     wop := arr.count-1;
    end;
   if wo_op and(wop <> -1) then
    with arr.items[wop].value do
     flags := flags or DT_FLG_DIRECT;
end;

var str_open:boolean;

function StrLngCat(data,dt:PScData; const str_del_o, str_del_c, str_op:string):string;
var op:string;
begin
    if dt.value.flags and DT_FLG_DIRECT > 0 then
      op := ''
    else op := str_op;

    if data.GetType = data_str then
      begin
        Result := str_del_c + op + dt.toStr;
        str_open := false;
      end
    else if dt.GetType = data_str then
      begin
        Result := op + str_del_o + dt.toStr;
        str_open := true;
      end
    else Result := op + dt.toStr;
end;

//     LangOther   static 
procedure TScData.AddNormalize;
var arr:PScArray;
    i:integer;
    s:string;
    cur,prev:PScData;
   procedure normalize(val:PScData);
   begin
      inc(arr.count);
      SetLength(arr.items,arr.count);
      arr.items[arr.count-1] := MakeData(val);
      val := arr.items[arr.count-1];
      {$ifdef _damped_}
      list.add(Format('valLang: %d, valLevel: %d,nsec: %d, lnglvl: %d', [val.GetLang, val.GetLevel, nsection, lang_level]));
      {$endif}
      if (val.GetLevel-1 > lang_level) then
        dec(val.value.level)
      else if (val.GetLang <> nsection)or(val.GetLevel-1 = lang_level)then
        begin
          case val.GetType of
           data_str, data_code:
             begin
               val.SetValue(val.toCode);
               lngs[nsection].tostr_proc(val.value.sdata);
             end;
           data_int: val.SetValue(inttostr(val.toInt));
           data_real: val.SetValue(FloatToStr(val.toReal));
          end;
        end;
   end;
   procedure finalize;
   var tmp:TScData;
   begin
      tmp.Create;
      tmp.SetValue(s, data_code);
      tmp.SetLang(BaseLang, BaseLevel);
      normalize(@tmp);
      s := '';
   end;
   function isBaseItem(dt:PScData):boolean;
   begin
      Result := (dt.GetLang <> nsection)and(dt.GetLevel-1 = lang_level);
   end;
begin
   str_open := false;
   cur := nil;
   if Value.data_type <> data_array then
     BuildArray;
   arr := PScArray(value.idata);

   if data.GetType = data_array then
    begin
     s := '';
     for i := 0 to data.toArray.Count-1 do
      begin
       cur := data.toArray.Items[i];
       if i > 0 then
         prev := data.toArray.Items[i-1]
       else prev := nil;

       if prev = nil then
        if isBaseItem(cur) then
          begin
           if cur.GetType = data_str then
            begin
             s := lngs[cur.GetLang].str_del_o;
             str_open := true;
            end;
           s := s + cur.toStr;
          end
        else
         begin
           normalize(cur);
         end
       else
        if isBaseItem(cur) then
         begin
          if not isBaseItem(prev) then
            begin
              if (cur.GetType <> data_str)and(cur.value.flags and DT_FLG_DIRECT = 0) then
                s := s + lngs[cur.GetLang].op_del;
              s := s + cur.toStr;
            end
          else if (prev.GetType = data_code) or (cur.GetType = data_code) then
            s := s + StrLngCat(prev, cur, lngs[cur.GetLang].str_del_o, lngs[cur.GetLang].str_del_c, lngs[cur.GetLang].op_del)
          else s := s + cur.toStr;
         end
        else
         begin
           if isBaseItem(prev) then
            begin
              if (prev.GetType <> data_str)and(cur.value.flags and DT_FLG_DIRECT = 0) then
                 s := s + lngs[cur.GetLang].op_del;
              finalize;
              cur := MakeData(cur);
              cur.value.flags := cur.value.flags and not DT_FLG_DIRECT;
            end;
           normalize(cur);
         end;
      end;

     if isBaseItem(cur) then
      begin
       if cur.GetType = data_str then
         s := s + lngs[BaseLang].str_del_c;
       finalize;
      end;
    end
   else normalize(data);

   value.lang := nsection;
   value.level := lang_level;
end;

{
//   
procedure TScData.AddNormalize;
var arr:PScArray;
    i:integer;
    s:string;
    dt:PScData;
    str_open:boolean;
   procedure normalize(val:PScData);
   begin
      inc(arr.count);
      SetLength(arr.items,arr.count);
      arr.items[arr.count-1] := MakeData(val);
      val := arr.items[arr.count-1];
      if (val.GetLevel-1 > lang_level) then
        dec(val.value.level)
      else if (val.GetLang <> nsection)then
        begin
          case val.GetType of
           data_str, data_code:
             begin
               val.SetValue(val.toCode);
               if nsection in [sec_php, sec_java] then
                 replace(val.value.sdata, '"', '\"');
             end;
           data_int: val.SetValue(int2str(val.toInt));
           data_real: val.SetValue(double2str(val.toReal));
          end;
        end;
   end;
   procedure finalize(data:PScData);
   var tmp:TScData;
   begin
      if str_open then
       case data.GetLang of
         sec_php, sec_java: s := s + '"';
       end;
      FillChar(tmp, sizeof(tmp), 0);
      tmp.SetValue(s, data_code);
      with data^ do
        tmp.SetLang(GetLang, GetLevel);
      normalize(@tmp);
   end;
begin
   str_open := false;
   if Value.data_type <> data_array then
     BuildArray;
   arr := PScArray(value.idata);

   if data.GetType = data_array then
    begin
     s := '';
     for i := 0 to data.toArray.Count-1 do
      if (data.toArray.Items[i].GetLang <> nsection)and(data.toArray.Items[i].GetLevel-1 = lang_level) then
       begin
          dt := data.toArray.Items[i];
          if (i = 0)or(data.toArray.Items[i-1].GetLang = nsection) then
           begin
            case dt.GetLang of
             sec_php,sec_java:
               if dt.GetType = data_str then
                begin
                 str_open := true;
                 s := '"' + dt.toStr
                end
               else s := dt.toStr;
             sec_html: s := dt.toStr;
            end
           end
          else if (data.toArray.Items[i-1].GetType = data_code) or (dt.GetType = data_code) then
            case dt.GetLang of
             sec_php : s := s + StrLngCat(data.toArray.Items[i-1], dt, '"', '.');
             sec_java: s := s + StrLngCat(data.toArray.Items[i-1], dt, '"', ' + ');
             sec_html: s := s + dt.toStr;
            end
          else s := s + dt.toStr;
       end
      else
       begin
         if(i > 0)and(data.toArray.Items[i-1].GetLang <> nsection)and(data.toArray.Items[i-1].GetLevel-1 = lang_level)then
          begin
            if(data.toArray.Items[i].value.flags and DT_FLG_DIRECT = 0)
               and(data.toArray.Items[i-1].GetType = data_code)then
             case data.toArray.Items[i-1].GetLang of
              sec_php : s := s + '.';
              sec_java: s := s + ' + ';
             end;
            finalize(data.toArray.Items[i-1]);
            s := '';
            dt := MakeData(data.toArray.Items[i]);
            dt.value.flags := dt.value.flags and not DT_FLG_DIRECT;
          end
         else dt := data.toArray.Items[i];
         normalize(dt);
       end;

     i := data.toArray.Count-1;
     if(data.toArray.Items[i].GetLang <> nsection)and(data.toArray.Items[i].GetLevel-1 = lang_level)then
       finalize(data.toArray.Items[i]);
    end
   else normalize(data);
//   SetLang(nsection, value.level);
   value.lang := nsection;
   value.level := lang_level;
end;
}

{
//       LangOther  
procedure TScData.AddNormalize;
var arr:PScArray;
    i:integer;
    s:string;
    dt:PScData;
   procedure normalize(val:PScData);
   begin
      inc(arr.count);
      SetLength(arr.items,arr.count);
      arr.items[arr.count-1] := MakeData(val);
      val := arr.items[arr.count-1];
      if (val.GetLevel-1 > lang_level) then
        dec(val.value.level)
      else if (val.GetLang <> nsection)then
        begin
          case val.GetType of
           data_str, data_code:
             begin
               val.SetValue(val.toCode);
               if nsection in [sec_php, sec_java] then
                 replace(val.value.sdata, '"', '\"');
             end;
           data_int: val.SetValue(int2str(val.toInt));
           data_real: val.SetValue(double2str(val.toReal));
          end;
        end;
   end;
   procedure finalize; //(data:PScData);
   var tmp:TScData;
   begin
//      if str_open then
//       case data.GetLang of
//         sec_php, sec_java: s := s + '"';
//       end;
//      str_open := false;

      FillChar(tmp, sizeof(tmp), 0);
      tmp.SetValue(s, data_code);
      //with data^ do
      tmp.SetLang(BaseLang, BaseLevel);
      normalize(@tmp);
      s := '';
   end;
begin
   if Value.data_type <> data_array then
     BuildArray;
   arr := PScArray(value.idata);

   if data.GetType = data_array then
    begin
     s := '';
     str_open := false;
     for i := 0 to data.toArray.Count-1 do
      if (data.toArray.Items[i].GetLang <> nsection)and(data.toArray.Items[i].GetLevel-1 = lang_level) then
       begin
          dt := data.toArray.Items[i];
          if (i = 0)or(data.toArray.Items[i-1].GetLang = nsection)or(data.toArray.Items[i-1].GetLevel-1 <> lang_level) then
           begin
            case dt.GetLang of
             sec_php,sec_java:
               if(dt.GetType = data_str)and not str_open then
                begin
                 str_open := true;
                 s := '"' + dt.toStr
                end
               else
                begin
                 if str_open then
                  begin
                    str_open := false;
                    case dt.GetLang of
                     sec_php,sec_java: s := '"';
                    end;
                  end;
                 s := s + dt.toStr;
                end;
             sec_html: s := dt.toStr;
            end
           end
          else if (data.toArray.Items[i-1].GetType = data_code) or (dt.GetType = data_code) then
            case dt.GetLang of
             sec_php : s := s + StrLngCat(data.toArray.Items[i-1], dt, '"', '.');
             sec_java: s := s + StrLngCat(data.toArray.Items[i-1], dt, '"', ' + ');
             sec_html: s := s + dt.toStr;
            end
          else s := s + dt.toStr;
       end
      else
       begin
         if i = 0 then
           case BaseLang of
             sec_php,sec_java: begin s := '"'; str_open := true; end;
             sec_html: s := '';
           end;

         //if(data.toArray.Items[i-1].GetLang <> nsection)and(data.toArray.Items[i-1].GetLevel-1 = lang_level)then
         if s <> '' then
          begin
            if(i > 0)and(data.toArray.Items[i-1].GetType = data_code)then
             begin
               if data.toArray.Items[i].value.flags and DT_FLG_DIRECT = 0 then
                case data.toArray.Items[i-1].GetLang of
                 sec_php : s := s + '.';
                 sec_java: s := s + ' + ';
                end;

               str_open := true;
               case data.toArray.Items[i-1].GetLang of
                sec_php,sec_java: s := s + '"';
               end;
             end;
            finalize; //(data.toArray.Items[i-1]);
            dt := MakeData(data.toArray.Items[i]);
            dt.value.flags := dt.value.flags and not DT_FLG_DIRECT;
          end
         else dt := data.toArray.Items[i];
         normalize(dt);
       end;

     if str_open then
       s := s + '"';
     //i := data.toArray.Count-1;
     //if(data.toArray.Items[i].GetLang <> nsection)and(data.toArray.Items[i].GetLevel-1 = lang_level)then
     if s <> '' then
       finalize; //(data.toArray.Items[i]);
    end
   else normalize(data);
//   SetLang(nsection, value.level);
   value.lang := nsection;
   value.level := lang_level;
end;
}

function TScData.GetType;
begin
   Result := Value.data_type;
end;

procedure TScData.SetType;
begin
   Value.data_type := tp;
end;

function TScData.GetLang;
begin
   Result := Value.lang;
end;

procedure TScData.SetLang;
var i:integer;
begin
   if value.data_type = data_array then
     for i := 0 to toArray.Count-1 do
      toArray.Items[i].SetLang(Lang, level);

   //if Value.lang = 0 then
   Value.lang := lang;
   Value.level := level;
end;

function TScData.GetLevel;
begin
   Result := Value.level;
end;

procedure TScData.SetSubType;
begin
   if value.sub_type = nil then
    begin
     new(value.sub_type);
     value.sub_type.sub_type := nil;
    end;
   value.sub_type.data_type := stype;
end;

function TScData.GetSubType;
begin
   if value.sub_type = nil then
     Result := data_null
   else  Result := value.sub_type.data_type;
end;

function TScData.printDebug;
var i:integer;
begin
   if value.data_type = data_array then
    begin
     Result := 'Array(';
     for i := 0 to toArray.Count-1 do
      begin
       if i > 0 then Result := Result + ',';
       with toArray.Items[i]^ do
        Result := Result + format('Item([%s],%d,%d,%s)',[toCode,GetLang,GetLevel,inttohex(value.flags,2)]);
      end;
     Result := Result + ')';
    end
   else
     Result := format('Item([%s],%d,%d,%s)',[toCode,GetLang,GetLevel,inttohex(value.flags,2)]);
end;

procedure TScData.GetValue;
begin
   val := value;
end;

function TScData.toStr;
var i:integer;
    arr:PScArray;
begin
   case Value.data_type of
    data_int: Result := inttostr(Value.idata);
    data_str,data_code: Result := Value.sdata;
    data_real: Result := FloatToStr(Value.rdata);
    data_array:
     begin
      arr := PScArray(value.idata);
      Result := '';
      for i := 0 to arr.count-1 do
        Result := Result + arr.items[i].toStr;
     end;
    data_null: Result := '';
   end;
end;

function TScData.toInt;
begin
   case Value.data_type of
    data_int: Result := Value.idata;
    data_str: Result := strtoint(Value.sdata);
    data_real: Result := Round(Value.rdata);
    else Result := 0;
   end;
end;

function TScData.toReal;
begin
   case Value.data_type of
    data_int: Result := Value.idata;
    data_str: Result := StrToFloat(Value.sdata);
    data_real: Result := Value.rdata;
    else Result := 0;
   end;
end;

function TScData.toBool;
begin
   case Value.data_type of
    data_int: Result := Value.idata <> 0;
    data_str,data_code: Result := Value.sdata <> '';
    data_real: Result := Value.rdata <> 0;
    data_array: Result := PScArray(value.idata).Count > 0;
    else Result := false;
   end;
end;

function TScData.toPointer:pointer;
begin
   case Value.data_type of
    data_int: Result := pointer(Value.idata);
    else Result := nil;
   end;
end;

function TScData.toCode;
var i:integer;
    arr:PScArray;
begin
   case Value.data_type of
    data_int: Result := inttostr(Value.idata);
    data_str: Result := lngs[value.lang].str_del_o + Value.sdata + lngs[value.lang].str_del_c;
    data_real: Result := FloatToStr(Value.rdata);
    data_code: Result := Value.sdata;
    data_array:
     begin
      arr := PScArray(value.idata);
      if arr.items[0].GetType <> data_str then
        Result := ''
      else Result := lngs[value.lang].str_del_o;
      str_open := Result = lngs[value.lang].str_del_o;
      Result := Result + arr.items[0].toStr;

      for i := 1 to arr.count-1 do
       if (arr.items[i-1].GetType = data_code)or(arr.items[i].GetType = data_code) then
        Result := Result + StrLngCat(arr.items[i-1], arr.items[i], lngs[value.lang].str_del_o, lngs[value.lang].str_del_c, lngs[value.lang].op_del)
       else Result := Result + arr.items[i].toStr;

      if str_open then Result := Result + lngs[value.lang].str_del_c;
     end;
    else Result := string(lngs[value.lang].str_del_o) + lngs[value.lang].str_del_c;
   end;
   {
   case value.lang of
    sec_php:
       case Value.data_type of
        data_int: Result := int2str(Value.idata);
        data_str: Result := '"' + Value.sdata + '"';
        data_real: Result := double2str(Value.rdata);
        data_code: Result := Value.sdata;
        data_array:
         begin
          arr := PScArray(value.idata);
          if arr.items[0].GetType = data_code then
            Result := ''
          else Result := '"';
          str_open := Result = '"';
          Result := Result + arr.items[0].toStr;

          for i := 1 to arr.count-1 do
           if (arr.items[i-1].GetType = data_code)or(arr.items[i].GetType = data_code) then
            Result := Result + StrLngCat(arr.items[i-1], arr.items[i], '"', '.')
           else Result := Result + arr.items[i].toStr;

          if str_open then Result := Result + '"';
         end;
        else Result := '""';
       end;
    sec_html:
       case Value.data_type of
        data_int: Result := int2str(Value.idata);
        data_str,data_code: Result := Value.sdata;
        data_real: Result := double2str(Value.rdata);
        data_array:
         begin
          arr := PScArray(value.idata);
          Result := '';
          for i := 0 to arr.count-1 do
            Result := Result + arr.items[i].toStr
         end;
        else Result := '';
       end;
    sec_java:
       case Value.data_type of
        data_int: Result := int2str(Value.idata);
        data_str: Result := '"' + Value.sdata + '"';
        data_real: Result := double2str(Value.rdata);
        data_code: Result := Value.sdata;
        data_array:
         begin
          arr := PScArray(value.idata);
          if arr.items[0].GetType = data_code then
            Result := ''
          else Result := '"';
          str_open := Result = '"';
          Result := Result + arr.items[0].toStr;

          for i := 1 to arr.count-1 do
           if (arr.items[i-1].GetType = data_code)or(arr.items[i].GetType = data_code) then
             Result := Result + StrLngCat(arr.items[i-1], arr.items[i], '"', ' + ')
           else Result := Result + arr.items[i].toStr;

          if str_open then Result := Result + '"';
         end;
        else Result := '""';
       end;
    else ;
   end;
   }
end;

function TScData.isEmpty;
begin
   case Value.data_type of
//    data_int: Result := Value.idata = 0;
    data_str,data_code: Result := Value.sdata = '';
//    data_real: Result := Value.rdata = 0;
    data_int,data_real,data_array: Result := false;
    else Result := true;
   end;
end;

function TScData.isValue;
begin
   case Value.data_type of
    data_str,data_code: Result := Value.sdata = val;
    else Result := false;
   end;
end;

function TScData.ReadItem;
begin
  if (Value.data_type = data_array)and(ind >= 0)and(ind < toArray().Count) then
    Result := toArray().Items[ind]
  else Result := nil;
end;

procedure TScData.BuildArray;
var arr:PScArray;
begin
   new(arr);
   fillchar(arr^, sizeof(TScArray), 0);
   value.data_type := data_array;
   value.idata := DATA_INTEGER(arr);
   value.sub_type := nil;
end;

function TScData.toArray;
begin
   Result := PScArray(value.idata);
end;

procedure TScData.mathAdd(val:PScData);
begin
   case value.data_type of
     data_int: inc(value.idata, val.toInt());
     data_str: value.sdata := value.sdata + val.toStr();
     data_code: value.sdata := value.sdata + val.toCode();
     data_real: value.rdata := value.rdata + val.toReal();
     else ;
   end
end;

procedure TScData.mathAdd(val:integer);
begin
   case value.data_type of
     data_int: inc(value.idata, val);
     data_str,data_code: value.sdata := value.sdata + inttostr(val);
     data_real: value.rdata := value.rdata + val;
     else ;
   end
end;

procedure TScData.mathAdd(val:real);
begin
   case value.data_type of
     data_int: inc(value.idata, Round(val));
     data_str,data_code: value.sdata := value.sdata + FloatToStr(val);
     data_real: value.rdata := value.rdata + val;
     else ;
   end
end;

procedure TScData.mathAdd(const val:string);
begin
   case value.data_type of
     data_int: inc(value.idata, StrToInt(val));
     data_str,data_code: value.sdata := value.sdata + val;
     data_real: value.rdata := value.rdata + StrToFloat(val);
     else ;
   end
end;

procedure TScData.mathSub;
begin
   case value.data_type of
     data_int: dec(value.idata, val.toInt());
//     data_str: value.sdata := value.sdata + val.toStr();
//     data_code: value.sdata := value.sdata + val.toCode();
     data_real: value.rdata := value.rdata - val.toReal();
     else ;
   end
end;

procedure TScData.mathMul;
begin
   case value.data_type of
     data_int: value.idata := value.idata * val.toInt();
//     data_str: value.sdata := value.sdata + val.toStr();
//     data_code: value.sdata := value.sdata + val.toCode();
     data_real: value.rdata := value.rdata * val.toReal();
     else ;
   end
end;

procedure TScData.mathDiv;
begin
   case value.data_type of
     data_int: value.idata := value.idata div val.toInt();
//     data_str: value.sdata := value.sdata + val.toStr();
//     data_code: value.sdata := value.sdata + val.toCode();
     data_real: value.rdata := value.rdata / val.toReal();
     else ;
   end
end;

// ----------------------------

function MakeSpaces(level:integer):string;
var i:integer;
begin
  Result := '';
  for i := 1 to level do
    Result := Result + ' ';
end;

function TParser.findProperty(const name:string):integer;
var i:integer;
begin
   Result := -1;
   for i := 0 to cgt.elGetPropCount(el)-1 do
     if stricomp(cgt.propGetName(cgt.elGetProperty(el, i)), PChar(name)) = 0 then
      begin
        Result := i;
        Exit;
      end
end;

procedure TParser.LineInsert;
var i:integer;
begin
   Lines.Insert(Index, Line);
   if _state_stack <> nil then
    for i := 0 to _state_stack.Count-1 do
     with PFuncState(_state_stack[i])^ do
      if LineIndex > Index then
       inc(LineIndex);
end;

procedure DoData(Result:PScData; cgt:PCodeGenTools; dt:id_data);
begin
     case cgt.dtType(Dt) of
       data_null: Result.SetValue(''{$ifndef NULL_TO_STR}, data_code{$endif});
       data_int:  Result.SetValue(cgt.dtInt(Dt));
       data_str:  Result.SetValue(cgt.dtStr(Dt));
       data_real: Result.SetValue(cgt.dtReal(Dt));
     end;
end;

function readProperty(e:id_element; cgt:PCodeGenTools; prop:id_prop):PScData;
var list:TStringList;
    i:integer;
    dt,item:TScData;
    arr:id_array;
    f:id_font;
begin
  dt.Create;
  Result := MakeData('');
  case cgt.propGetType(prop) of
    data_int, data_color, data_flags: Result.SetValue(cgt.propToInteger(prop));
    data_str: Result.SetValue(cgt.propToString(prop));
    data_real: Result.SetValue(cgt.propToReal(prop));
    data_data: DoData(Result, cgt,id_data(cgt.propGetValue(prop)));
    data_combo: Result.SetValue(cgt.propToByte(prop));
    data_array:
     begin
      arr := id_array(cgt.propGetValue(prop));
      Result.BuildArray;
      item.Create;
      for i := 0 to cgt.arrCount(arr)-1 do
       begin
         dt.BuildArray;
         case cgt.arrType(arr) of
          data_int: item.SetValue(cgt.propToInteger(cgt.arrGetItem(arr, i)));
          data_str: item.SetValue(cgt.propToString(cgt.arrGetItem(arr, i)));
         end;
         dt.AddValue(@item);
         item.SetValue(string(cgt.arrItemName(arr, i)));
         dt.AddValue(@item);
         Result.AddValue(@item);
         Result.toArray.Items[i]^ := dt;
       end;
     end;
    data_list:
     begin
      list := TStringList.Create;
      list.Text := cgt.propToString(prop);
      Result.BuildArray;
      for i := 0 to list.count-1 do
       begin
         dt.SetValue(List[i]);
         Result.AddValue(@dt);
       end;
      list.Free;
     end;
    data_comboEx: Result.SetValue(cgt.propToString(prop), data_code);
    data_element: Result.SetValue(cgt.propGetLinkedElement(e, cgt.propGetName(prop)));
    data_font:
      begin
        Result.BuildArray;
        f := id_font(cgt.propGetValue(prop));
        dt.SetValue(cgt.fntName(f));
        Result.AddValue(@dt);
        dt.SetValue(cgt.fntSize(f));
        Result.AddValue(@dt);
        dt.SetValue(cgt.fntStyle(f));
        Result.AddValue(@dt);
        dt.SetValue(cgt.fntColor(f));
        Result.AddValue(@dt);
        dt.SetValue(cgt.fntCharSet(f));
        Result.AddValue(@dt);
      end
    else
      Result.SetValue('');
  end;
 if assigned(readCustomProperty) then
   readCustomProperty(Result, e, cgt, prop);
end;

procedure ResetState(sdk:id_sdk; cgt:PCodeGenTools; free:boolean = true);
var i:integer;
    e:id_element;
    p:PElementData;
begin
   for i := 0 to cgt.sdkGetCount(sdk)-1 do
    begin
      e := cgt.sdkGetElement(sdk,i);
      cgt.elSetCodeName(e,'');
      p := cgt.elGetData(e);
      if(p <> nil)and free then
       begin
         p.VarList.Destroy;
         p.LangList.Destroy;
         dispose(p);
       end;
      cgt.elSetData(e,nil);
      if(cgt.elGetClassIndex(e) = CI_MultiElement)and(not cgt.elLinkIs(e) or (cgt.elLinkMain(e) = e)) then
        ResetState(cgt.elGetSDK(e),cgt,free);
    end;
end;

function CheckVersionProc(const params:THiAsmVersion):integer;
begin
  if(params.build >= 162)then
    Result := CG_SUCCESS
  else Result := CG_INVALID_VERSION;
end;

function buildPrepareProc(const params:TBuildPrepareRec):integer; cdecl;
begin
  Result := CG_SUCCESS;
end;

procedure call_init(cgt:PCodeGenTools; e:id_element; const proc:string);
var
    p:TParser;
    Args:TArgs;
begin
    Args := TArgs.Create;
    p := TParser.Create;
    p.Parse(cgt, e, proc, TCodeBlock(obj_map[0].obj), Args, nil);
    p.destroy;
    Args.Destroy;
end;

procedure initAllFreeElements(cgt:PCodeGenTools; sdk:id_sdk);
var j:integer;
    e:id_element;
    s:string;
begin
   for j := 0 to cgt.sdkGetCount(sdk)-1 do
    begin
      e := cgt.sdkGetElement(sdk,j);
      if cgt.elGetFlag(e) and IS_HIDE > 0 then continue;

      if cgt.elLinkIs(e) then
         s := cgt.elGetCodeName(cgt.elLinkMain(e))
      else s := cgt.elGetCodeName(e);

      if length(s) = 0 then
       begin
//         cgt._debug(cgt.elGetClassName(e), clBlue);
         nsection := 0;
         call_init(cgt, e, 'initFree');
       end;
    end;
end;

function buildProcessProc(var params:TBuildProcessRec):integer; cdecl;
var
    i,j:integer;
    e:id_element;
    Parser:TParser;
    codeb:TCodeBlock;
    Args:TArgs;
    lst:TStringList;
    search:boolean;
begin
   params.result := nil;
   if (params.cgt = nil) or (params.sdk = nil) then
   begin
     Result := CG_BUILD_FAILED;
     Exit;
   end;
   search := false;
   LngUserTypes := TStringList.Create;
   LngTypeCounter := data_object;

   CreateObjMap;

   lst := TStringList.Create;
   codeb := TCodeBlock(obj_map[0].obj);

   with params do
     ResetState(SDK,cgt,false);

   el_cnt := 0;
   Parser := TParser.Create;
   GVarList := TArgs.Create;
   lang_level := 0;
   Args := TArgs.Create;
   for i := 0 to params.cgt.sdkGetCount(params.sdk)-1 do
    begin
     e := params.cgt.sdkGetElement(params.sdk,i);
     search := false;
     for j := 0 to lng_count-1 do
      if StrIComp(params.cgt.elGetClassName(e), lngs[j].entry) = 0 then
       begin
         nsection := Parser.readSection(lngs[j].name);
         Parser.Parse(params.cgt, e, 'dostart', codeb, Args, nil);
         search := true;
         break;
       end;
     if search then break;
    end;
  Args.Destroy;

   if Parser._err_ or not search then
    begin
      Result := -1;
      params.result := nil;
    end
   else
    begin
      codeb.Merge(lst);
      GetMem(params.result, length(lst.Text)+1);
      StrLCopy(params.result, PChar(lst.text), length(lst.Text));
      Result := CG_SUCCESS;
    end;
   lst.Free;
   Parser.Destroy;
   GVarList.Destroy;
   LngUserTypes.Free;

   with params do
     ResetState(SDK,cgt);

   DestroyObjMap;
end;

procedure freeBuildResult(value:pointer); cdecl;
begin
   if value <> nil then FreeMem(value);
end;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
procedure TBlockStack.Push;
begin
   inc(Count);
   SetLength(Items,Count);
   Items[Count-1].skip := _skip;
   Items[Count-1].LType := _Type;
   Items[Count-1].userdata := _userdata;
end;

function TBlockStack.Pop;
begin
   if Count > 0 then
    begin
     _Skip := Items[Count-1].skip;
     _Type := Items[Count-1].LType;
     _userdata := Items[Count-1].userdata;
     dec(Count);
     Result := false;
    end
   else Result := true;
end;

procedure TBlockStack.Clear;
begin
   Count := 0;
   SetLength(Items,Count);
end;

//**************************************************************************************

constructor TCodeBlock.Create;
begin
   inherited;
   FItems := TStringList.Create;
   lsize := 2;
end;

destructor TCodeBlock.Destroy;
var i:integer;
begin
   for i := 0 to Count-1 do
    begin
      Items[i].StrList.Free;
      dispose(Items[i]);
    end;
   inherited;
end;

procedure TCodeBlock.LevelEnabled;
begin
   CurItem.FLevelEn := Value;
end;

procedure TCodeBlock.Reg;
var cbr:PCodeBlockRec;
begin
   if Find(name) <> nil then exit;

   new(cbr);
   cbr.StrList := TList.Create;
   cbr.level := 0;
   cbr.FLevelEn := true;
   FItems.AddObject(LowerCase(name), TObject(cbr));
end;

function TCodeBlock.RegGen;
begin
   Result := 'block_' + IntToStr(FGen);
   inc(FGen);
   Reg(Result);
end;

function TCodeBlock.Select;
var l:PCodeBlockRec;
begin
   l := Find(name);
   if l <> nil then
     begin
      CurItem := l;
      Current := l.StrList;
      Result := FCurName;
      FCurName := name;
     end
   else Result := '';
end;

procedure TCodeBlock.Delete;
var i,j:integer;
begin
   for i := 0 to Count-1 do
    if StrIComp(PChar(FItems[i]), PChar(name)) = 0 then
     begin
       if Current = Items[i].StrList then
        Current := nil;
       with Items[i].StrList do
         for j := 0 to Count-1 do
          Dispose(PScData(Items[j]));
       Items[i].StrList.Free;
       dispose(Items[i]);
       FItems.Delete(i);
       break;
     end;
end;

procedure TCodeBlock.CopyTo;
var src,dst:PCodeBlockRec;
    i:integer;
    sp:string;
begin
    src := Find(Source);
    dst := Find(Dest);
    if(src <> nil)and(dst <> nil)then
      if dst.FLevelEn then
       begin
        sp := MakeSpaces(dst.level*lsize);
        for i := 0 to src.StrList.Count-1 do
         begin
          if(i = 0)or(PScData(src.StrList[i-1]).isValue(#13#10))then
            dst.StrList.Add(MakeData(sp, true));
          dst.StrList.Add(MakeData(src.StrList.Items[i]));
         end;
       end
      else
        for i := 0 to src.StrList.Count-1 do
         dst.StrList.Add(MakeData(src.StrList.Items[i]));
end;

procedure TCodeBlock.Merge;
var i:integer;
    s:string;
    cur:TList;
begin
   s := '';
   cur := Current;
   for i := 0 to Count-1 do
    begin
      current := Items[i].StrList;
      if s <> '' then
       s := s + #13#10;
      s := s + asCode
    end;
   Current := cur;
   List.Text := s;
end;

procedure TCodeBlock.Add;
begin
   if Current <> nil then
    if CurItem.FLevelEn and not Text.isEmpty then
     begin
      Current.Add(Makedata(MakeSpaces(level*lsize), true));
      Current.Add(MakeData(Text));
      Current.Add(MakeData(#13#10, true));
     end
    else
      if CurItem.FLevelEn or (Current.Count = 0) then
        begin
         Current.Add(MakeData(Text));
         Current.Add(MakeData(#13#10, true));
        end
      else Current.Add(MakeData(Text));
end;

procedure TCodeBlock.Paste;
begin
   if Current <> nil then
    begin
       if CurItem.FLevelEn and((Current.Count = 0) or PScData(Current.Items[Current.Count-1]).isValue(#13#10)) then
         begin
          Current.Add(MakeData(MakeSpaces(level*lsize), true));
          Current.Add(MakeData(Text));
         end
       else Current.Add(MakeData(Text));
    end;
end;

procedure TCodeBlock.AddStrings;
begin
   if Current <> nil then
     Current.AddList(list);
end;

procedure TCodeBlock.MergeFromFile;
var list:TStringList;
    i:integer;
    dt:TScData;
begin
  dt.Create;
  list := TStringList.Create;
  list.loadfromfile(FileName);
  if Current <> nil then
    for i := 0 to list.count-1 do
      begin
         dt.SetValue(list[i], data_code);
         add(@dt);
      end;
  list.Free;
end;

function TCodeBlock.Empty;
begin
   Result := true;
   if Current <> nil then
    Result := Current.Count = 0;
end;

function TCodeBlock.isEmpty;
var l:PCodeBlockRec;
begin
   l := Find(name);
   Result := (l = nil) or (l.StrList.Count = 0);
end;

function test:string;
begin
end;

function TCodeBlock.AsText;
var
   j:integer;
begin
   FillChar(Result, sizeof(Result),0);
   {$ifdef _damped_}
   list := NewStrList;
   list.loadfromfile(GetStartDir + 'asText.txt');
   {$endif}
    if(Current = nil)or(Current.Count = 0)then exit;

    for j := 0 to Current.Count-1 do
     begin
       {$ifdef _damped_} list.add('After norm: ' + PScData(Current.Items[j]).printDebug); {$endif}
       with PScData(Current.Items[0])^ do
         Result.AddNormalize(GetLang, GetLevel, PScData(Current.Items[j]));
     end;
   {$ifdef _damped_} list.add('Damp: ' + Result.printDebug); {$endif}

   {$ifdef _damped_}
   list.savetofile(GetStartDir + 'asText.txt');
   list.free;
   {$endif}
end;

function TCodeBlock.AsCode;
var i:integer;
begin
   Result := '';
   for i := 0 to Current.Count-1 do
     Result := Result + PScData(Current.Items[i]).toCode();
end;

procedure TCodeBlock.SaveToFile;
var lst:PCodeBlockRec;
    list,tmp:TStringList;
    cur:TList;
    s:string;
//    h1,h2:TMD5Digest;
begin
   lst := Find(name);
   if lst <> nil then
    begin
     cur := Current;
     Current := lst.StrList;
     list := TStringList.Create;
     if _asCode then
       s := asCode()
     else
       s := astext().toStr();
     list.text := s;
     if checkHash and fileexists(fname) then
      begin
        tmp := TStringList.Create; 
        tmp.LoadFromFile(fname);
        if not compareTwoLists(list, tmp) then
          List.SaveToFile(fname);
        tmp.Free;
      end
     else List.SaveToFile(fname);
     List.free;
     current := cur;
    end;
end;

function TCodeBlock.Find;
var i:integer;
begin
   for i := 0 to Count-1 do
    if StrIComp(PChar(FItems[i]), PChar(name)) = 0 then
     begin
       Result := Items[i];
       exit;
     end;
   Result := nil;
end;

function TCodeBlock.GetItems;
begin
   if(Index >= 0)and(Index < Count)then
     Result := PCodeBlockRec(FItems.Objects[Index])
   else Result := nil;
end;

function TCodeBlock.GetCount;
begin
  Result := FItems.Count;
end;

function TCodeBlock.GetLevel;
begin
   if CurItem <> nil then
     Result := CurItem.level
   else Result := -1;
end;

procedure TCodeBlock.SetLevel;
begin
   if CurItem <> nil then
     CurItem.level := value;
end;

function TCodeBlock.GenSpaces;
begin
   Result := MakeSpaces(level*lsize);
end;

//**************************************************************************************

constructor TArgs.Create;
begin
   inherited;
   FItems := TStringList.Create;
end;

destructor TArgs.Destroy;
var i:integer;
begin
   for i := 0 to Count-1 do
    begin
     //Values[i].value.sdata := '';
     //dispose(Values[i]);
    end;
   FItems.Free;
   inherited;
end;

function TArgs.GetValues;
begin
   if(index >= 0)and(index < Count)then
     Result := PScData(FItems.objects[index])
   else Result := nil;
end;

procedure TArgs.SetValue;
begin
   if(index >= 0)and(index < Count)then
     FItems.objects[index] := TObject(Value)
   else ;
end;

function TArgs.GetNames;
begin
   if(index >= 0)and(index < Count)then
     Result := FItems[index]
   else Result := '';
end;

procedure TArgs.SetNames;
begin
   if(index >= 0)and(index < Count)then
     FItems[index] := lowercase(value);
end;

function TArgs.GetCount;
begin
   Result := FItems.Count;
end;

function TArgs.find;
var i:integer;
begin
   for i := 0 to COunt-1 do
    if name = names[i] then
     begin
       FIndex := i;
       Result := Values[i];
       exit;
     end;
   FIndex := -1;
   Result := nil;
end;

procedure TArgs.Remove;
begin
  if find(name) <> nil then
   begin
     dispose(Values[FIndex]);
     FItems.Delete(FIndex);
   end;
end;

function TArgs.AddArg;
begin
   new(Result);
   FillChar(Result^,sizeof(TScData),0);
   FItems.AddObject(lowercase(Name), TObject(Result));
end;

//**************************************************************************************

constructor TParser.Create;
begin
   inherited Create;
   BlockStack := TBlockStack.Create;
   Lines := TStringList.Create;
   funcList := TStringList.Create;
   fData := nil;
   _data_return_.Create;
end;

destructor TParser.Destroy;
var i:integer;
begin
   BlockStack.Destroy;
   Lines.Free;
   for i := 0 to funcList.Count-1 do
     begin
      PFuncData(funcList.Objects[i])._args_.Destroy;
      PFuncData(funcList.Objects[i])._vars_.Destroy;
      dispose(PFuncData(funcList.Objects[i]));
     end;
   funcList.Free;
   inherited;
end;

procedure TParser.Debug;
begin
   {$ifdef debug}
   cgt._Debug(PChar(Text), color);
   {$endif}
end;

function TParser.ReadLine;
begin
   while LineIndex < Lines.Count do
    begin
       Line := Lines[LineIndex] + #1;
       inc(LineIndex);
       if Line <> #1 then break;
    end;
   Result := LineIndex = Lines.Count;
end;

procedure TParser.AddError;
begin
   //cgt._Debug(PChar('Error ' + uname + '[' + int2str(LineIndex) + ']: ' + Error), clRed);
   cgt._Error(LineIndex, el, PChar(Error));
   ReadLine;
   LPos := 1;
   _err_ := true;
end;

function TParser.CheckSkipped;
begin
   if (fData <> nil) and(fData.fSkeep or fData.Skeep) then
    begin
      ReadLine;
      LPos := 1;
      Result := true;
    end
   else Result := false;
end;

function TParser.CheckOpenArgs;
begin
   Result := true;
   if GetToken then exit;
   if Token <> '(' then
     begin
       AddError(Format(ER_FUNC_ARGS_OPEN,[op]));
       exit;
     end;
   Result := false;
end;

function TParser.CheckCloseArgs;
begin
   Result := true;
   if GetToken then exit;
   if Token <> ')' then
     begin
       AddError(Format(ER_FUNC_ARGS_CLOSE,[op]));
       exit;
     end;
   Result := false;
end;

function TParser.CheckSymbol;
begin
   Result := true;
   if GetToken then exit;
   if Token <> sym then
     begin
       AddError(Format(ER_SYMBOL_EXISTS,[sym]));
       exit;
     end;
   Result := false;
end;

procedure TParser.PutToken;
begin
   LineIndex := oldLineIndex;
   Line := Lines[lineindex-1] + #1;
   Lpos := oldLPos;
   Token := '';
end;

function Hex2Int( const Value : AnsiString) : Integer;
var I : Integer;
begin
  Result := 0;
  I := 1;
  if Value = '' then Exit;
  if Value[ 1 ] = '$' then Inc( I );
  while I <= Length( Value ) do
  begin
    if Value[ I ] in [ '0'..'9' ] then
       Result := (Result shl 4) or (Ord(Value[I]) - Ord('0'))
    else
    if Value[ I ] in [ 'A'..'F' ] then
       Result := (Result shl 4) or (Ord(Value[I]) - Ord('A') + 10)
    else
    if Value[ I ] in [ 'a'..'f' ] then
       Result := (Result shl 4) or (Ord(Value[I]) - Ord('a') + 10)
    else
      break;
    Inc( I );
  end;
end;


function TParser.GetToken;
var err:string;
    l:char;
begin
   oldLineIndex := LineIndex;
   oldLPos := LPos;
   Token := '';
   RToken := '';
   TokType := 0;
   err := '';
   Result := false;
   repeat
    case Line[LPos] of
     ' ',#9: inc(LPos);
     'a'..'z','A'..'Z','_':
       begin
         repeat
           RToken := RToken + Line[LPos];
           inc(LPos);
         until not(Line[LPos] in ['a'..'z','A'..'Z','_','0'..'9']);
         Token := LowerCase(RToken);
         TokType := TokName;
       end;
     '0'..'9':
         if(Line[LPos] = '0') and (Line[LPos+1] = 'x') then
           begin
             inc(LPos,2);
             while (Line[LPos] in ['0'..'9']) or (Line[LPos] in ['A'..'F','a'..'f']) do
               begin
                 RToken := RToken + Line[LPos];
                 inc(LPos);
               end;
             Token := IntToStr(hex2int(RToken));
             TokType := TokNumber;
           end
         else
           begin
             repeat
               Token := Token + Line[LPos];
               inc(LPos);
             until not(Line[LPos] in ['0'..'9']);
             if (Line[LPos] = '.')and(Line[LPos+1] <> '.') then
              begin
               repeat
                Token := Token + Line[LPos];
                inc(LPos);
               until not(Line[LPos] in ['0'..'9']);
               if Token[Length(Token)] = '.' then
                 AddError('Syntax error. Please write correct real number.');
               TokType := TokReal;
              end
             else TokType := TokNumber;
           end;
     '"':
       begin
         inc(LPos);
         while (Line[LPos] <> #1) do
          begin
           case Line[LPos] of
            '"': break;
            '\':
              case Line[LPos+1] of
                '\','"': inc(LPos);
                'r':
                  begin
                    Token := Token + #13;
                    inc(LPos,2);
                    continue;
                  end;
                'n':
                  begin
                    Token := Token + #10;
                    inc(LPos,2);
                    continue;
                  end;
              end;
           end;
           Token := Token + Line[LPos];
           inc(LPos);
          end;
         TokType := TokString;
         if Line[LPos] = #1 then
           err := Format(ER_LEX_NOT_FOUND, [' " '])
         else inc(LPos);
       end;
     '''':
       begin
         inc(LPos);
         while (Line[LPos] <> #1) do
          begin
           case Line[LPos] of
            '''': break;
            '\':
              if Line[LPos+1] in ['\',''''] then
                 inc(LPos);
           end;
           Token := Token + Line[LPos];
           inc(LPos);
          end;
         TokType := TokCode;
         if Line[LPos] = #1 then
           err := Format(ER_LEX_NOT_FOUND, [' '' '])
         else inc(LPos);
       end;
     '`':
       begin
         inc(LPos);
         while (Line[LPos] <> #1)and(Line[LPos] <> '`') do
           begin
             RToken := RToken + Line[LPos];
             inc(LPos);
           end;
         Token := LowerCase(RToken);
         TokType := TokProp;
         if Line[LPos] = #1 then
           err := Format(ER_LEX_NOT_FOUND, [' ` '])
         else inc(LPos);
       end;
     '=':
       begin
         Token := Line[LPos];
         inc(LPos);
         if Line[LPos] = '=' then inc(LPos);  // c++ style support
         TokType := TokSymbol;
       end;
     '(',')',',',':','.','?','[',']','@','^':
        begin Token := Line[LPos]; inc(LPos); TokType := TokSymbol; end;
     '&':
        begin
          Token := Line[LPos];
          inc(LPos);
          if (Line[LPos] = '&')and(Line[LPos+1] = '=') then
            begin
               Token := '&&=';
               inc(LPos,2);
            end
          else if (Line[LPos] = '=')or(Line[LPos] = '&') then
            begin
               Token := Token + Line[LPos];
               inc(LPos);
            end;
          TokType := TokSymbol
        end;
     '+','-','/','*':
        begin
          l := Line[LPos];
          Token := Line[LPos];
          inc(LPos);
          if (Line[LPos] = '/')and(l = '/') then
            while Line[LPos] <> #1 do inc(LPos)
          else if ((Line[LPos] = '=')or(Line[LPos] = '+')and(l = '+'))or
                  ((Line[LPos] = '=')or(Line[LPos] = '-')and(l = '-'))or
                  ((Line[LPos] = '=')and(l = '*'))or
                  ((Line[LPos] = '=')and(l = '/'))then
            begin
               Token := Token + Line[LPos];
               inc(LPos);
               TokType := TokSymbol
            end
          else TokType := TokMath;
        end;
     '>','<','!':
        begin
          Token := Line[LPos];
          inc(LPos);
            if Line[LPos] = '=' then
             begin
                Token := Token + Line[LPos];
                inc(LPos);
             end
            else if ( Token = '<' )and(Line[LPos] = '>')then
             begin
                Token := '!=';
                inc(LPos);
             end;
          TokType := TokMath;
        end;
     '#':
        begin
          TokType := TokString;
          inc(LPos);
          Token := '';
          while (Line[LPos] in ['0'..'9'])and(Line[LPos] <> #1) do
           begin
             Token := Token + Line[LPos];
             inc(LPos);
           end;
          if Token = '' then
           Token := #0
          else Token := chr(StrToInt(Token));
        end;
     #1: begin ReadLine; LPos := 1; end;
     #2: Result := true;
     else
      begin
       err := Format(ER_TOK_DNT_SUP,[inttohex(ord(Line[LPos]),2), LPos]);
       inc(LPos);
      end;
    end;
   until (TokType <> 0)or(err <> '')or(Line[LPos] = #2);

   if err <> '' then
    begin
     Result := true;
     AddError(err);
    end;
end;

function TParser.ReadPoint(lp:id_point):PScData;
var
    Parser:TParser;
    ret:TScData;
    fname:string;
    Args:TArgs;
    el:id_element;
begin
  Parser := TParser.Create;
  el := cgt.ptGetParent(lp);
  Args := TArgs.Create;
  // cgt.ptGetType(lp) = pt_var -  ...
  args.AddArg(''); // todo:     
  if (cgt.elGetClassIndex(el) = CI_DPElement)and(cgt.ptGetType(lp) = pt_var)and(length(cgt.pt_dpeGetName(lp)) > 0)then
    begin
     fname := lowercase(cgt.pt_dpeGetName(lp));
     args.AddArg('').SetValue(cgt.ptGetIndex(lp));
    end
  else if (cgt.elGetClassIndex(el) in [CI_EditMulti,CI_EditMultiEx])then
    begin
     fname := lowercase(cgt.pt_dpeGetName(lp));
     args.AddArg('').SetValue(cgt.ptGetIndex(lp));
    end
  else if (cgt.elGetClassIndex(el) = CI_MultiElement){and(cgt.ptGetType(lp) = pt_var)}then
    begin
     fname := lowercase(cgt.pt_dpeGetName(lp));
     args.AddArg('').SetValue(cgt.ptGetIndex(lp));
    end
  else fname := lowercase(cgt.ptGetName(lp));

//  Debug('Try to read: ' + fname);

  ret.Create;
  Parser.Parse(cgt, el, fname, codeb, args, @ret);
  Result := MakeData(@ret);
  Args.Destroy;
  Parser.Destroy;
end;

function TParser.ReadID;
begin
  if cgt.elLinkIs(el) then
    Result := cgt.elGetCodeName(cgt.elLinkMain(el))
  else Result := cgt.elGetCodeName(el);
end;

procedure TParser.SetElement;
var ed:PElementData;
begin
   el := e;
   if cgt.elGetData(e) = nil then
     begin
      new(ed);
      ed.VarList := TArgs.Create;
      ed.LangList := TArgs.Create;
      cgt.elSetData(e,ed)
     end
   else ed := cgt.elGetData(e);

   VarList := ed.VarList;
   LangList := ed.LangList;
end;

procedure TParser.ReadFuncList;
var
  _codeb:TCodeBlock;
  Args:TArgs;
  ret_data:TScData;
begin
   Args := TArgs.Create;
   _codeb := TCodeBlock.Create;
   ret_data.Create;
   Parse(_cgt, e, '##', _codeb, Args, @ret_data);
   Args.Destroy;
   _codeb.Destroy;
   List.Assign(funcList);
end;

function TParser.readSection;
begin
  Token := s;
  ToSection(Result);
end;

procedure TParser.Parse;
var UnitName:string;
    _init:boolean;
begin
   cgt := _cgt;
   codeb := _codeb;
//   section := sec_php;
   section := 0;
   _init := cgt.elGetData(e) = nil;
   SetElement(e);

   if _init then
    if cgt.elLinkIs(el)and(cgt.elLinkMain(el) <> el) then
     begin
      if (Length(ReadID) = 0) then
       begin
         cgt.elSetCodeName(cgt.elLinkMain(el), PChar(IntToStr(el_cnt)));
         inc(el_cnt);
       end;
     end
    else
    begin
     if not cgt.elLinkIs(el) or(Length(ReadID) = 0) then
      begin
       cgt.elSetCodeName(el, PChar(IntToStr(el_cnt)));
       inc(el_cnt);
      end;

     {$ifdef init_section}
     Debug('Call init section for ' + cgt.elGetClassName(e));
     {$endif}
     call_init(cgt,el,'init');
     if fname = 'init' then exit;
    end;

   uname := 'hi' + cgt.elGetClassName(e) + '.hws';
   UnitName := cgt.ReadCodeDir(el) + uname;
   // A missing template gives an empty unit, as in the HiAsm 4 web codegen; without the check
   // the FPC x64 build raises an unhandled exception here.
   if FileExists(UnitName) then Lines.LoadFromFile(UnitName);
   Lines.Add(#2);
   {$ifdef processed_unit}
   Debug('Load unit: ' + uname);
   {$endif}

   case cgt.elGetClassIndex(e) of
    CI_DPElement: ;
    CI_MultiElement: ;
   end;

   LineIndex := 0;
   Line := '';
   LPos := 1;
   Start(fname, args);
   if ret_data <> nil then
     ret_data^ := _data_return_;
end;

procedure TParser.FuncLexem;
var sec,i:integer;
begin
   if GetToken then exit;

   {$ifdef processed_func}
   Debug('Process func: ' + RToken);
   {$endif}

   funcList.Add(token);
   new(fData);
   FillChar(fData^,sizeof(TFuncData),0);
   funcList.Objects[funcList.Count-1] := TObject(fdata);
   fData.Line := LineIndex;
   fData.Skeep := false;
   fData.name := RToken;
   //Debug('Add function to list: ' + RToken + ':' + fname);
   fData._args_ := TArgs.Create;
   fData._vars_ := TArgs.Create;
   fData._data_saved_.SetValue('');

   fData.fSkeep := not (fname = Token);
   {$ifdef called_func}
   if not fData.fSkeep then
     Debug('Called func: ' + RToken);
   {$endif}

   fData.fsection := 1 shl section;
   if GetToken then exit;
   if Token = '(' then
    begin
      if GetToken then exit;
      while token <> ')' do
       begin
         if token = ',' then
          if GetToken then exit;

         if toktype <> tokname then
          begin
            AddError('Name expected');
            Exit;
          end;

         fData._args_.AddArg(token);

         if GetToken then exit;
         if(token <> ',')and(token <> ')')then
          begin
            AddError('Syntax error for arguments');
            Exit;
          end;
       end;
      if GetToken then exit;
    end;

   if Token = ':' then
    begin
     repeat
      if GetToken then exit;
      if token = 'any' then
        fData.fSection := $FF
      else
       begin
        if ToSection(sec) then exit;
        fData.fSection := fData.fSection or (1 shl sec);
       end;
      if GetToken then exit;
     until token <> ',';
    end;
   PutToken;

   //Debug('Need section: ' + IntToStr(fsection));

   fData.fSkeep := fData.fSkeep or (fData.fsection and (1 shl nsection) = 0);
   if not fData.fSkeep then
    begin
      {$ifdef called_func_sec}
      Debug(' Called func: ' + RToken + '(' + IntToStr(fData._args_.Count) + ') for section ' + IntToStr(nsection));
      {$endif}
      for i := 0 to min(args.Count,fData._args_.Count)-1 do
       fData._args_.values[i]^ := args.Values[i]^;
      if args.count > 0 then
         fData._data_saved_ := args.Values[0]^;
    end;

   fData.Skeep := false;
   BlockStack.Push(fData.fSkeep, BLK_FUNC, nil);
end;

function TParser.EndLexem;
var
    tp:word;
    p:pointer;
    wdt:PWhileData;
begin
  Result := false;
  if BlockStack.Count = 0 then
    begin
      AddError('Syntax error: END lexem');
      Exit;
    end;

  BlockStack.Pop(fData.Skeep, tp, p);

  case tp of
   BLK_FUNC:
    begin
     fData.fSkeep := fData.Skeep;
     Result := not fData.fSkeep;
     fData.Skeep := false;
     fData := nil;
    end;
   BLK_WHILE:
    if p <> nil then
     begin
      wdt := PWhileData(p);
      if wdt.for_cond = '' then
       begin
        LineIndex := wdt.index;
        LPos := 1;
        ReadLine;
       end
      else
       begin
         LineInsert(LineIndex-1, wdt.for_cond);
         dec(LineIndex);
         LPos := 1;
         ReadLine;
         wdt.for_cond := '';
         BlockStack.Push(fData.Skeep, tp, p);
//         debug(lines.text);
       end;
     end;
  end;
end;

procedure TParser.IncludeLexem;
var fname:PScData;
    s:string;
    procedure InsertStrings;
    var list:TStringList;
        i:integer;
    begin
       list := TStringList.Create;
       list.LoadFromFile(s);
       for i := 0 to list.count-1 do
         LineInsert(LineIndex+i, list[i]);
       list.Free;
    end;
begin
   if (fData <> nil)and(fData.fSkeep) then
     if CheckSkipped then exit;

   if CheckOpenArgs('include') then exit;

   if level1(fname) then exit;
//   debug('fname: ' + f + 'inc: ' + fname.toStr );
   s := cgt.ReadCodeDir(el) + fname.toStr + '.hws';
   if FileExists(s) then
     InsertStrings
   else
     AddError('File ' + s + ' not found');

   if CheckCloseArgs('include') then exit;
end;

procedure TParser.InlineLexem;
var data:PScData;
    procedure InsertStrings;
    var list:TStringList;
        i:integer;
    begin
       list := TStringList.Create;
       list.Text := data.toStr();
       for i := 0 to list.count-1 do
         LineInsert(LineIndex+i, list[i]);
       list.Free;
    end;
begin
   if CheckSkipped then exit;

   if CheckOpenArgs('inline') then exit;

   if level1(data) then exit;
   InsertStrings;

   if CheckCloseArgs('inline') then exit;
end;

procedure TParser.SubLexem;
var lvar, data:PScData;
    arr:PScArray;
    st,old:^PSubType;
    i:integer;
begin
   if CheckSkipped then exit;

   if CheckOpenArgs('sub') then exit;

   if GetToken then exit;

   lvar := LangList.find(token);
   if lvar = nil then
     lvar := fdata._vars_.find(token);

   if lvar = nil then
    begin
      AddError('Variable ' + token + ' not found');
      Exit
    end;
   if GetToken then exit;
   if token <> ',' then exit;
   if level1(data) then exit;

   if data.GetType = data_array then
    begin
     arr := data.toArray();

     st := @lvar.value.sub_type;
     if st^ <> nil then
       dispose(st^);
     old := nil;
     for i := 0 to arr.Count-1 do
      begin
        new(st^);
        if old <> nil then
          old^.sub_type := st^;
        st^.data_type := arr.items[i].toInt();
        st^.sub_type := nil;
        old := st;
        st := @st^.sub_type;
      end;
    end
   else
    begin
      st := @lvar.value.sub_type;
      new(st^);
      st^.data_type := data.toInt();
      st^.sub_type := nil;
    end;

   if CheckCloseArgs('sub') then exit;
end;

function TParser.ToSection;
var i:integer;
begin
   for i := 0 to lng_count-1 do
    if token = lngs[i].name then
     begin
       sec := i;
       Result := false;
       exit;
     end;
   AddError('Unknown section ' + RToken);
   Result := true;
end;

function TParser.ToType;
var i:integer;
begin
     if Token = 'int' then
       tp := data_int
     else if Token = 'str' then
       tp := data_str
     else if Token = 'real' then
       tp := data_real
     else if Token = 'array' then
       tp := data_array
     else if FindUserType(Token, i) then
       tp := i
     else tp := -1;
     Result := tp <> -1;
end;

procedure TParser.SaveState;
var state:PFuncState;
begin
  if _state_stack = nil then
    _state_stack := TList.Create;
  new(state);
  fillchar(state^,sizeof(TFuncState),0);
  state.Line := Line;
  state.LineIndex := LineIndex;
  state.LPos := LPos;
  state.Token := Token;
  state.RToken := RToken;
  state.TokType := TokType;
  state.fData := fData;
  _state_stack.Add(state);
end;

procedure TParser.LoadState;
var state:PFuncState;
begin
  state := _state_stack.Items[_state_stack.Count-1];
  _state_stack.Delete(_state_stack.Count-1);
  Line := state.Line;
  LineIndex := state.LineIndex;
  LPos := state.LPos;
  Token := state.Token;
  RToken := state.RToken;
  TokType := state.TokType;
  fData := state.fData;
  dispose(state);

  if _state_stack.count = 0 then
     begin
        _state_stack.Free;
        _state_stack := nil;
     end;
end;

function TParser._points_count_;
var i:integer;
begin
   Result := 0;
   for i := 0 to cgt.elGetPtCount(el)-1 do
    if cgt.ptGetType(cgt.elGetPt(el,i)) = ptype then
      inc(Result);
end;

procedure TParser.SectionLexem;
begin
   if GetToken then exit;
   ToSection(section);
end;

procedure TParser.FVarLexem;
begin
   if CheckSkipped then exit;

   if CheckOpenArgs('fvar') then exit;

   if GetToken then exit;
   repeat
     if VarList.find(token) <> nil then
      begin
        AddError(format(ER_VAR_EXISTS,[RToken]));
        Exit;
      end;
     with fdata._vars_.AddArg(Token)^ do
      begin
        SetValue('');
        SetLang(nsection,lang_level);
      end;
     if GetToken then exit;

     if token = ',' then
       if GetToken then exit;
   until (Token = ')')or(toktype <> tokname);
   if TokType <> TokSymbol then
     begin
       AddError('Syntax error for fvar: )');
       Exit;
     end;
end;

procedure TParser.VarLexem;
begin
   if CheckSkipped then exit;

   if CheckOpenArgs('var') then exit;

   if GetToken then exit;
   repeat
     if VarList.find(token) <> nil then
      begin
        AddError(format(ER_VAR_EXISTS,[RToken]));
        Exit;
      end;
     with VarList.AddArg(Token)^ do
      begin
        SetValue('');
        SetLang(nsection,lang_level);
      end;
     if GetToken then exit;

     if token = ',' then
       if GetToken then exit;
   until (Token = ')')or(toktype <> tokname);
   if TokType <> TokSymbol then
     begin
       AddError('Syntax error for var: )');
       Exit;
     end;
end;

procedure TParser.GVarLexem;
begin
   if CheckSkipped then exit;

   if CheckOpenArgs('gvar') then exit;

   if GetToken then exit;
   repeat
     if GVarList.find(token) <> nil then
      begin
        AddError(format(ER_VAR_EXISTS,[RToken]));
        Exit;
      end;

     with GVarList.AddArg(Token)^ do
      begin
        SetValue('');
        SetLang(nsection,lang_level);
      end;
     if GetToken then exit;

     if token = ',' then
       if GetToken then exit;
   until (Token = ')')or(toktype <> tokname);
   if TokType <> TokSymbol then
     begin
       AddError('Syntax error for gvar: )');
       Exit;
     end;
end;

function TParser.regGVar;
begin
     Result := LangList.AddArg(Name);
     with Result^ do
      begin
        SetValue(lngs[nsection].var_mask, data_code);
        SetLang(nsection,lang_level);
        replace(value.sdata,'%n',Name);
        replace(value.sdata,'%i',cgt.elGetCodeName(el));
      end;
end;

procedure TParser.LangLexem;
var ldt:PScData;
    i:integer;
begin
   if CheckSkipped then exit;

   if CheckOpenArgs('lang') then exit;

   if GetToken then exit;
   repeat
     if LangList.find(token) <> nil then
      begin
        AddError(format(ER_VAR_EXISTS,[RToken]));
        Exit;
      end;
     ldt := regGVar(Token);
     if GetToken then exit;
     if Token <> ':' then exit;
     if GetToken then exit;
     if ToType(Token, i) then
       ldt.SetSubType(i)
     else
       AddError('Type ' + Token + ' not found');

     if GetToken then exit;
     if token = ',' then
       if GetToken then exit;
   until (Token = ')')or(toktype <> tokname);
   if TokType <> TokSymbol then
     begin
       AddError('Syntax error for lang: )');
       Exit;
     end;
end;

procedure TParser.FreeLexem;
begin
   if CheckSkipped then exit;

   if CheckOpenArgs('free') then exit;

   if GetToken then exit;
   repeat
     if GVarList.find(token) = nil then
      begin
        AddError(format(ER_VAR_NOT_FOUND,[RToken]));
        Exit;
      end;
     GVarList.Remove(token);

     if GetToken then exit;

     if token = ',' then
       if GetToken then exit;
   until (Token = ')')or(toktype <> tokname);
   if TokType <> TokSymbol then
     begin
       AddError('Syntax error for free: )');
       Exit;
     end;
end;

procedure TParser.ReturnLexem;
var data:PScData;
    i:integer;
    pt:id_point;
begin
   if CheckSkipped then exit;

   if CheckOpenArgs('return') then exit;

   _data_return_.SetValue('');
   if Level1(data) then exit;
   _data_return_ := data^;

   if data.gettype in [data_code, data_array] then
     begin
      pt := cgt.elGetPtName(el, PChar(fData.name));
      if pt <> nil then
       begin
        i := cgt.ptGetDataType(cgt.elGetPtName(el, PChar(fData.name)));
        if i <> data_null then _data_return_.SetSubType(i);
       end;
     end;
   fdata.fskeep := true;
//   debug('ret : ' + IntToStr(_data_return_.value.level));

   if CheckCloseArgs('return') then exit;
end;

procedure TParser.ForLexem;
var
    s,p:string;
begin
   if CheckSkipped then
    begin
      BlockStack.Push(true, BLK_WHILE, nil);
      exit;
    end;

   if CheckOpenArgs('for') then exit;  //TODO:  ,    ...   .
   s := Line;
   delete(s,1,LPos-1);
   p := getTok(s, ';');
   inc(LPos, length(p));
   LineInsert(LineIndex, p);
   p := getTok(s, ';');
   inc(LPos, length(p)+1);
   LineInsert(LineIndex+1, 'while(' + p + ')');
   inc(LPos,pos(')',s)+1);
   _for_cond := getTok(s, ')');
//   if CheckCloseArgs('for') then exit;
end;

procedure TParser.WhileLexem;
var data:PScData;
    wdt:PWhileData;
begin
   if CheckSkipped then
    begin
      BlockStack.Push(true, BLK_WHILE, nil);
      exit;
    end;

   if CheckOpenArgs('while') then exit;

   if level1(data) then exit;
   fData.Skeep := data.toInt = 0;
   new(wdt);
   FillChar(wdt^,sizeof(TWhileData),0);
   wdt.index := lineindex-1;
   wdt.for_cond := _for_cond;
   _for_cond := '';
   if fData.Skeep then
     BlockStack.Push(false, BLK_WHILE, nil)
   else BlockStack.Push(false, BLK_WHILE, wdt);

   if CheckCloseArgs('while') then exit;
end;

procedure TParser.IfLexem;
var tp:word;
    sk:boolean;
    data:PScData;
begin
   if CheckSkipped then
    begin
      BlockStack.Push(true, BLK_IF, nil);
      exit;
    end;

   if CheckOpenArgs('if') then exit;

   if Level1(data) then exit;
   sk := fData.Skeep;
   fData.Skeep := fData.Skeep or not data.toBool();
   if fData.Skeep then tp := BLK_IF else tp := BLK_ELSEIF;
   BlockStack.Push(Sk, tp, nil);

   if CheckCloseArgs('if') then exit;
end;

procedure TParser.ElseIfLexem;
var _sk:boolean;
    _tp:word;
    data:PScData;
    p:pointer;
begin
   if fData.fSkeep then
    begin
      ReadLine;
      LPos := 1;
      exit;
    end;

   if CheckOpenArgs('elseif') then exit;

   if Level1(data) then exit;
   BlockStack.Pop(_sk, _tp, p);
   if(_tp <> BLK_ELSEIF) and fData.Skeep and not _sk then
     begin
      fData.Skeep := data.toInt = 0;
      if not fData.Skeep then
       _tp := BLK_ELSEIF;
     end
   else fData.Skeep := true;
   BlockStack.Push(_sk, _tp, p);

   if not checkSkipped then
     CheckCloseArgs('elseif');
end;

procedure TParser.ElseLexem;
var _sk:boolean;
    _tp:word;
    p:pointer;
begin
   if fData.fSkeep then
    begin
      ReadLine;
      LPos := 1;
      exit;
    end;

   BlockStack.Pop(_sk, _tp, p);
   if ((_tp = BLK_IF)or(_tp = BLK_SWITCH))and(not _sk) then
     fData.Skeep := not fData.Skeep
   else fData.Skeep := true;
   BlockStack.Push(_sk, _tp, p);
end;

procedure TParser.SwitchLexem;
var
    data:PScData;
begin
   if CheckSkipped then
    begin
      BlockStack.Push(true, BLK_SWITCH, nil);
      exit;
    end;

   if CheckOpenArgs('switch') then exit;

   if Level1(data) then exit;
   BlockStack.Push(fData.Skeep, BLK_SWITCH, data);

   if CheckCloseArgs('switch') then exit;
end;

procedure TParser.CaseLexem;
var swt:PScData;
    data:PScData;
    _sk:boolean;
    _tp:word;
    function CheckRange:boolean;
    begin
       Result := false;
       if Level1(data) then exit;
       if gettoken then exit;
       if token <> ':' then
        begin
           AddError(ER_SYNTAX + 'lexem : expected');
           exit;
        end;
       if swt <> nil then
        case swt.GetType of
         data_int: Result := data.toInt = swt.toInt;
         data_real: Result := data.toReal = swt.toReal;
         data_str,data_code,data_array:
              Result := data.toStr = swt.toStr;
         else Result := false;
        end
       else Result := false;
    end;
begin
   if fData.fSkeep then
    begin
      ReadLine;
      LPos := 1;
      exit;
    end;

   BlockStack.Pop(_sk,_tp,pointer(swt));
   case _tp of
    BLK_SWITCH:
     begin
       fData.Skeep := not CheckRange;
       if fData.Skeep then
         BlockStack.Push(_sk, BLK_SWITCH, swt)
       else BlockStack.Push(_sk, BLK_CASE, swt);
     end;
    BLK_CASE:
     begin
       CheckRange;
       fData.Skeep := true;
       BlockStack.Push(_sk, _tp, swt);
     end;
    else
     begin
       AddError('Need switch() lexem');
       exit;
     end;
   end;
end;

procedure TParser.IncSecLexem;
var _var_:PScData;
begin
   if CheckSkipped then exit;

   if CheckOpenArgs('inc_sec') then exit;

   if level1(_var_) then exit;

   token := _var_.toStr();
   if ToSection(nsection) then exit;
   inc(lang_level);

   if CheckCloseArgs('inc_sec') then exit;
end;

procedure TParser.DecSecLexem;
var _var_:PScData;
begin
   if CheckSkipped then exit;

   if CheckOpenArgs('dec_sec') then exit;

   if level1(_var_) then exit;

   token := _var_.toStr();
   if ToSection(nsection) then exit;
   dec(lang_level);

   if CheckCloseArgs('dec_sec') then exit;
end;

function TParser.EventLexem;
var ename:string;
    data:PScData;
    oldsec:integer;
    p:id_point;
    setup:boolean;
begin
   Result := nil;
   if CheckSkipped then exit;

   if CheckOpenArgs('event') then exit;

   if level1(data) then exit;
   ename := data.toStr;
   oldsec := nsection;
   if GetToken then exit;
   if Token = ':' then
    begin
      setup := true;
      if GetToken then exit;
      if ToSection(nsection) then exit;
      if GetToken then exit;
    end
   else setup := false;

   if setup then inc(lang_level);
   if Token = ',' then
     begin
       if level1(data) then exit;
       if GetToken then exit;
     end
   else data := makedata(''{$ifndef NULL_TO_STR}, true{$endif});
   {$ifdef event_call}
   Debug('Call event with ' + ename);
   {$endif}
   if hnt_this then
    begin
      p := cgt.elGetPtName(el, PChar(ename));
      if (p <> nil)and(cgt.ptGetRLinkPoint(p) <> nil) then
        hnt_message := hnt_message + codeb.GenSpaces + '[' + cgt.ptGetName(p) + '(' + data^.toCode + ')]'#13#10 + codeb.GenSpaces
    end
   else Result := CallEvent(ename, MakeData(data)^);
   if setup then  dec(lang_level);
   nsection := oldsec;

   if Token <> ')' then
     begin
       AddError('Syntax error: no argument found');
       Exit;
     end;
end;

function TParser.mtEventLexem;
var ename:string;
    data:PScData;
    oldsec:integer;
    p:id_point;
    setup:boolean;
    args:TArgs;
begin
   Result := nil;
   if CheckSkipped then exit;

   if CheckOpenArgs('mtevent') then exit;

   if level1(data) then exit;
   ename := data.toStr;
   oldsec := nsection;
   if GetToken then exit;
   if Token = ':' then
    begin
      setup := true;
      if GetToken then exit;
      if ToSection(nsection) then exit;
      if GetToken then exit;
    end
   else setup := false;

   if setup then inc(lang_level);
   args := TArgs.Create;
   while Token = ',' do
     begin
       if level1(data) then exit;
       args.AddArg('')^ := data^;
       if GetToken then exit;
     end;

   {$ifndef NULL_TO_STR}
   if args.Count = 0 then
     args.AddArg('').SetValue('');
   {$endif}

   {$ifdef event_call}
   Debug('Call mtevent with ' + ename);
   {$endif}
   if hnt_this then
    begin
      p := cgt.elGetPtName(el, PChar(ename));
      if (p <> nil)and(cgt.ptGetRLinkPoint(p) <> nil) then
        hnt_message := hnt_message + codeb.GenSpaces + '[' + cgt.ptGetName(p) + '(' + data^.toCode + ')]'#13#10 + codeb.GenSpaces
    end
   else Result := CallEventMt(ename, args);
   if setup then dec(lang_level);
   nsection := oldsec;

   args.destroy;

   if Token <> ')' then
     begin
       AddError('Syntax error: no argument found');
       Exit;
     end;
end;

procedure TParser.Print(const Text:string);
begin
   codeb.Paste(MakeData(Text, true));
   if hnt_this then
     hnt_message := hnt_message + Text;
end;

procedure TParser.PrintLine;
begin
   codeb.Add(makedata('', true));
   if hnt_this then
     hnt_message := hnt_message + #13#10 + codeb.GenSpaces;
end;

procedure TParser.PrintLexem;
var
    data:array of PScData;
    cnt,i:integer;
begin
   if CheckSkipped then exit;

   if CheckOpenArgs('print') then exit;

   cnt := 0;
   repeat
     inc(cnt);
     SetLength(data, cnt);
     if Level1(data[cnt-1]) then exit;
     if GetToken then exit;

     if (Token <> ')') and(Token <> ',')  then
       begin
         AddError('Syntax error: no argument found');
         Exit;
       end;

//     if data.GetLang = 0 then
//      begin
//        data.SetLang(nsection);
//        data.level := lang_level;
//        Debug('print: ' + data.printDebug)
//      end;

   until Token = ')';

   for i := 0 to cnt-1 do
    begin
      if hnt_this then
        hnt_message := hnt_message + data[i].toCode;
      codeb.Paste(data[i]);
    end;

   {$ifdef print_lexem}
   Debug('Print[' + codeb.CurBlockName + ']: ');
   {$endif}
end;

procedure TParser.PrintLnLexem;
begin
   if CheckSkipped then exit;

   PrintLexem;
   codeb.Add(makedata('', true));
   if hnt_this then
     hnt_message := hnt_message + #13#10 + codeb.GenSpaces;
end;

function TParser.CountLexem;
var _var_:PScData;
begin
   Result := nil;
   if CheckSkipped then exit;

   if CheckOpenArgs('count') then exit;

   if level1(_var_) then exit;

   if _var_.GetType() = data_array then
     Result := MakeData(_var_.toArray().Count)
   else Result := MakeData(1);

   if CheckCloseArgs('count') then exit;
end;

procedure tparser.VarOpLexem;
var _var_:PScData;
begin
   if CheckSkipped then exit;

   PutToken;

//   debug('parse ' + line + ' line ' + IntToStr(LineIndex) + ' LPos ' + IntToStr(LPos));
   if level1(var_int, MATH_ASSIGN) then exit;
//   debug('level ' + line + ' line ' + IntToStr(LineIndex));
   if GetToken then exit;
   if token = '=' then
     begin
       if Level1(_var_) then exit;
       var_int^ := MakeData(_var_)^;
     end
   else if token = '&=' then
     begin
       if Level1(_var_) then exit;
       var_int.AddValue(_var_);
     end
   else if token = '&&=' then
     begin
       if Level1(_var_) then exit;
       var_int.AddValue(_var_, true);
     end
   else if token = '+=' then
     begin
       if Level1(_var_) then exit;
       var_int.mathAdd(_var_);
     end
   else if token = '-=' then
     begin
       if Level1(_var_) then exit;
       var_int.mathSub(_var_);
     end
   else if token = '*=' then
     begin
       if Level1(_var_) then exit;
       var_int.mathMul(_var_);
     end
   else if token = '/=' then
     begin
       if Level1(_var_) then exit;
       var_int.mathDiv(_var_);
     end
   else if token = '++' then
     begin
       var_int.mathAdd(1);
     end
   else if token = '--' then
     begin
       var_int.mathAdd(-1);
     end
   else
    begin
      PutToken;
      //AddError('Syntax error: uncnown operator ' + token);
    end;
end;

function TParser.ReadCustomValue;
var dt:PScData;
    oldsec:integer;
    setup:boolean;
begin
   if CheckOpenArgs('point') then exit;

   if level1(dt) then exit;

   if CheckCloseArgs('point') then exit;
   if GetToken then exit;

   oldsec := nsection;
   setup := false;
   if token = ':' then
    begin
      if GetToken then exit;
      if ToSection(nsection) then exit;
      setup := true;
    end
   else PutToken;

   if setup then
    begin
     //debug('inc lvl from read value');
     inc(lang_level);
    end;
   ReadValue(dt.toStr, Result);
   if setup then
    begin
     //debug('dec lvl from event');
     dec(lang_level);
    end;
   nsection := oldsec;
end;

function TParser.LinkedLexem;
var lp:id_point;
    parser:TParser;
    lst:TStringList;
begin
   Result := false;
   if CheckOpenArgs('linked') then exit;

   if GetToken then exit;
   lp := cgt.elGetPtName(el, PChar(RToken));
   if lp <> nil then
    begin
     lp := cgt.ptGetRLinkPoint(lp);
     Result := lp <> nil;
     if Result and(cgt.elGetClassIndex(cgt.ptGetParent(lp)) = CI_EditMulti) then
       if cgt.ptGetRLinkPoint(cgt.elGetPt(cgt.elGetSDK(cgt.ptGetParent(lp)), cgt.ptGetIndex(lp))) <> nil then
        // ok
       else
        begin
           parser := TParser.Create;
           lst := TStringList.Create;
           parser.ReadFuncList(lst, cgt, cgt.ptGetParent(lp), '');
           Result := lst.IndexOfName(RToken) <> -1;
           lst.Free;
           parser.Destroy;
        end;
    end;
   if CheckCloseArgs('linked') then exit;
end;

function TParser.IsDefLexem;
var pr:integer;
begin
   Result := false;

   if CheckOpenArgs('isdef') then exit;

   if GetToken then exit;
   pr := findProperty(RToken);
   if pr <> -1 then
     Result := cgt.elIsDefProp(el, pr)
   else
    {do nothing};
    //AddError('Property ' + RToken + ' not found!');

   if CheckCloseArgs('isdef') then exit;
end;

function TParser.IsSetLexem;
var _var_:PScData;
    int_var:PScData;
begin
   Result := false;
   if CheckOpenArgs('isset') then exit;

   if GetToken then exit;

   if getElementVar(token,int_var) or getGlobalVar(token,int_var) then
     Result := true
   else
    begin
      ReadValue(RToken, _var_, false);
      Result := not _var_.isEmpty;
    end;

   if CheckCloseArgs('isset') then exit;
end;

function TParser.IsPropLexem;
begin
   Result := false;
   if CheckOpenArgs('isprop') then exit;

   if GetToken then exit;

   Result := findProperty(token) <> -1;

   if CheckCloseArgs('isprop') then exit;
end;

function TParser.IsSecLexem;
var
    sec:integer;
begin
   Result := false;
   if CheckOpenArgs('issec') then exit;

   if GetToken then exit;

   if ToSection(sec) then
     Result := false
   else Result := nsection = sec;

   if CheckCloseArgs('issec') then exit;
end;

function TParser.TypeOfLexem;
var
    _data:PScData;
begin
   Result := nil;
   if CheckOpenArgs('typeof') then exit;

   if Level1(_data) then exit;
   Result := MakeData(_data.getType);

   if CheckCloseArgs('typeof') then exit;
end;

function TParser.ExpOfLexem;
var
    _data:PScData;
begin
   Result := nil;
   if CheckOpenArgs('expof') then exit;

   if Level1(_data) then exit;
   Result := MakeData(_data.GetSubType);

   if CheckCloseArgs('expof') then exit;
end;

function TParser.SubLexem_res:PScData;
var lvar:PScData;
    sc:PSubType;
    dt:TScData;
begin
   Result := nil;
   if CheckOpenArgs('sub') then exit;

   if GetToken then exit;

   lvar := LangList.find(token);
   if lvar = nil then
     lvar := fdata._vars_.find(token);

   if lvar = nil then
    begin
      AddError('Variable ' + token + ' not found');
      Exit
    end;
   new(Result);
   Result.BuildArray;
   dt.Create;
   sc := lvar.value.sub_type;
   while sc <> nil do
    begin
     dt.SetValue(sc.data_type);
     Result.AddValue(@dt);
     sc := sc.sub_type;
    end;

   if CheckCloseArgs('sub') then exit;
end;

function TParser.StrLexem;
var data:PScData;
begin
   Result := nil;
   if CheckOpenArgs('str') then exit;

   if level1(data) then exit;
   Result := MakeData(data.toStr);

   if CheckCloseArgs('str') then exit;
end;

function TParser.CodeLexem;
var data:PScData;
begin
   Result := nil;
   if CheckOpenArgs('code') then exit;

   if level1(data) then exit;
   Result := MakeData(data.toStr);
   Result.SetType(data_code);

   if CheckCloseArgs('code') then exit;
end;

function TParser.lCodeLexem;
var data:PScData;
begin
   Result := nil;
   if CheckOpenArgs('lcode') then exit;

   if level1(data) then exit;
   Result := MakeData(data.toCode, true);

   if CheckCloseArgs('lcode') then exit;
end;


function TParser.CvtLexem;
var data:PScData;
begin
   Result := nil;
   if CheckOpenArgs('X_int') then exit;

   if level1(data) then exit;
   Result := _toCode(data, toTypeaa);

   if CheckCloseArgs('X_int') then exit;
end;

function TParser.CallFunc;
var
    fname:string;
    i:integer;
    fd:PFuncData;
    _arg:PScData;
begin
   Result := nil;
   if CheckSkipped then exit;

   {$ifdef call_func}
   Debug('Call user function ' + RToken);
   {$endif}
   fname := Token;

   if CheckOpenArgs('func') then exit;

   fd := PFuncData(funcList.Objects[ind]);
   for i := 0 to fd._args_.Count-1 do // todo: check errors
    begin
      if i > 0 then
       begin
         if GetToken then exit;
         if Token <> ',' then
           begin
             AddError('Syntax error after arg ' + fd._args_.Names[i]);
             Exit;
           end;
       end;
      if level1(_arg) then exit;
      fd._args_.values[i] := _arg;
    end;
   SaveState;
   fData := fd;
   LineIndex := fData.Line;
   LPos := 1;

   if fd._args_.count > 0 then
     fData._data_saved_ := fd._args_.Values[0]^;

//   if fData._args_.count > 0 then
//     fData._data_saved_ := fData._args_.Values[0]^
//   else fData._data_saved_.SetValue('');

   fData.Skeep := false;
   fData.fSkeep := false;

   BlockStack.Push(false, 0, nil);
   Start(fname, nil);
   Result := MakeData(@_data_return_);
   LoadState;
//   debug('line: ' + {lines.items[}IntToStr(lineindex){]});

   {$ifdef call_func}
   Debug('Call user function complite.');
   {$endif}

   if CheckCloseArgs('func') then exit;
end;

function TParser.CallIntFunc;
label TO_ERROR;
var i:integer;
    args:TArgs;
    _arg:PScData;
begin
   Result := nil;
   if CheckSkipped then exit;

   args := TArgs.Create;
   Result := MakeData('');

   if GetToken then goto TO_ERROR;
   if Token <> '(' then
     begin
       AddError(Format(ER_FUNC_ARGS_OPEN,[func_map[ind].name]));
       goto TO_ERROR;
     end;

   i := 0;
   if GetToken then goto TO_ERROR;
   while ((i < func_map[ind].count)or(func_map[ind].count = -1))and(Token <> ')') do
    begin
      if i = 0 then
        PutToken;

      if level1(_arg) then
       begin
        AddError(Format(ER_FUNC_ARGS_INVALID,[func_map[ind].name]));
        goto TO_ERROR;
       end;
      args.AddArg('');
      args.Values[args.Count-1] := _arg;

      if GetToken then goto TO_ERROR;
      if (Token <> ',')and(Token <> ')') then
        begin
          AddError(Format(ER_FUNC_ARGS_SYNTAX,[func_map[ind].name]));
          goto TO_ERROR;
        end;
      //if GetToken then goto TO_ERROR;
      inc(i);
    end;
   if(func_map[ind].count <> -1)and(i < func_map[ind].count)then
     begin
       AddError(Format(ER_FUNC_ARGS_COUNT,[func_map[ind].name, func_map[ind].count]));
       goto TO_ERROR;
     end;

   Result^ := func_map[ind].proc(Self, args);

   if Token <> ')' then
     begin
       AddError(Format(ER_FUNC_ARGS_CLOSE,[func_map[ind].name]));
       goto TO_ERROR;
     end;

TO_ERROR:
  args.Destroy;
end;

function TParser.CallObject;
label TO_ERROR;
var i,mt_ind,count:integer;
    args:TArgs;
    mt:string;
    _arg:PScData;
begin
   args := TArgs.Create;
   Result := nil;
   if CheckSkipped then exit;

   if GetToken then goto TO_ERROR;
   if Token <> '.' then
     begin
       AddError(format(ER_OBJ_FIELD_ACCESS, [obj_map[ind].name]));
       Exit;
     end;

   if GetToken then exit;
   mt := Token;

//   for i := 0 to obj_map[ind].fields_count-1 do  // TODO
//    ;
   mt_ind := -1;
   for i := 0 to obj_map[ind].methods_count-1 do
     if mt = obj_map[ind].methods[i].name then
      begin
       mt_ind := i;
       break;
      end;

   if mt_ind = -1 then
     begin
       AddError(format(ER_OBJ_METHOD_BAD, [RToken, obj_map[ind].name]));
       Exit;
     end
   else count := obj_map[ind].methods[mt_ind].count;

   Result := MakeData('');

   if GetToken then goto TO_ERROR;
   if Token <> '(' then
     begin
       AddError(Format(ER_FUNC_ARGS_OPEN,[mt]));
       goto TO_ERROR;
     end;

   i := 0;
   if GetToken then goto TO_ERROR;
   while ((i < count)or(count = -1))and(Token <> ')') do
    begin
      if i = 0 then
        PutToken;

      if level1(_arg) then
       begin
        AddError(Format(ER_FUNC_ARGS_INVALID,[mt]));
        goto TO_ERROR;
       end;
      args.AddArg('');
      args.Values[args.Count-1] := _arg;

      if GetToken then goto TO_ERROR;
      if (Token <> ',')and(Token <> ')') then
        begin
          AddError(Format(ER_FUNC_ARGS_SYNTAX,[mt]));
          goto TO_ERROR;
        end;
      inc(i);
    end;
   if(count <> -1)and(i < count)then
     begin
       AddError(Format(ER_FUNC_ARGS_COUNT,[mt, count]));
       goto TO_ERROR;
     end;

   Result^ := obj_map[ind].method_proc(self, obj_map[ind].obj, mt_ind, args);

   if Token <> ')' then
     begin
       AddError(Format(ER_FUNC_ARGS_CLOSE,[mt]));
       goto TO_ERROR;
     end;

TO_ERROR:
  args.Destroy;
end;

function TParser.CallTypeConvertion;
var s:string;
    data:PScData;
begin
   Result := nil;
   if CheckSkipped then exit;
   s := RToken;
   if gettoken then exit;
   if token <> '(' then
    begin
      puttoken;
      Result := MakeData(ind);
    end
   else
    begin
     if level1(data) then exit;
     Result := _toCode(data, ind);

     if CheckCloseArgs(s) then exit;
    end;
end;

function TParser.Level1;
begin
   if GetToken then
    begin
     Result := true;
     exit;
    end;

   Result := level2(_var_,flag);
   PutToken;
end;

function TParser.Level2;
var
  var2:PScData;
begin
  Result := Level2a(_var_,flag);
  while (Token = '^') do
   begin
    if GetToken then exit;
    Result := Level2a(var2,flag);
    _var_ := MakeData(_var_);
    _var_.mtAttach(var2);
   end;
end;

function TParser.Level2a;
var
  var2:PScData;
  i:byte;
begin
  Result := Level3(_var_,flag);
  while (Token = '&')or(Token = '&&') do
   begin
    i := length(token);
    if GetToken then exit;
    Result := Level3(var2,flag);
    _var_ := MakeData(_var_);
    _var_.AddValue(var2, i = 2);
   end;
end;

function TParser.Level3;
var
  var2:PScData;
  s:string;
begin
  Result := Level4a(_var_,flag);
  while (Token = 'or')or(Token = 'and') do
   begin
    s := Token;
    if GetToken then exit;
    Result := Level4a(var2,flag);
    if s = 'or' then
      _var_.SetValue(_var_.toBool or Var2.toBool)
    else
      _var_.SetValue(_var_.toBool and Var2.toBool);
   end;
end;

function TParser.Level4a;
var
  var2:PScData;
  s:string;
begin
  Result := Level4(_var_,flag);
  while (Token = '_or_')or(Token = '_and_') do
   begin
    s := Token;
    if GetToken then exit;
    Result := Level4(var2,flag);
    if s = '_or_' then
      _var_.SetValue(_var_.toInt or Var2.toInt)
    else
      _var_.SetValue(_var_.toInt and Var2.toInt);
   end;
end;

function TParser.Level4;
var
  Var1,var2:PScData;
  s:string;
  v:boolean;
begin
  Result := Level5(_var_,flag);
  var1 := _var_;
  v := false;
  if flag and MATH_ASSIGN = 0 then
  while (Token = '=')or(Token = '>')or(Token = '<')or(Token = '>=')or(Token = '<=')or(Token = '!=')do
   begin
    s := Token;
    if GetToken then exit;
    Result := Level5(var2,flag);
    if s = '=' then
       case Var1.GetType of
        data_int: v := (Var1.toInt = Var2.toInt);
        data_str: v := (Var1.toStr = Var2.toStr);
        data_real: v := (Var1.toReal = Var2.toReal);
        data_code: v := (Var1.toCode = Var2.toCode);
        else v := false;
       end
    else if s = '!=' then
       case Var1.GetType of
        data_int: v := (Var1.toInt <> Var2.toInt);
        data_str: v := (Var1.toStr <> Var2.toStr);
        data_real: v := (Var1.toReal <> Var2.toReal);
        data_code: v := (Var1.toCode <> Var2.toCode);
        else v := false;
       end
    else if s = '>' then
       case Var1.GetType of
        data_int: v := (Var1.toInt > Var2.toInt);
        //data_str: _var_.SetValue(Var1.toStr = Var2.toStr);
        data_real: v := (Var1.toReal > Var2.toReal);
        else v := false;
       end
    else if s = '<' then
       case Var1.GetType of
        data_int: v := (Var1.toInt < Var2.toInt);
        //data_str: _var_.SetValue(Var1.toStr = Var2.toStr);
        data_real: v := (Var1.toReal > Var2.toReal);
        else v := false;
       end
    else if s = '>=' then
       case Var1.GetType of
        data_int: v := (Var1.toInt >= Var2.toInt);
        //data_str: _var_.SetValue(Var1.toStr = Var2.toStr);
        data_real: v := (Var1.toReal >= Var2.toReal);
        else v := false;
       end
    else if s = '<=' then
       case Var1.GetType of
        data_int: v := (Var1.toInt <= Var2.toInt);
        //data_str: _var_.SetValue(Var1.toStr = Var2.toStr);
        data_real: v := (Var1.toReal >= Var2.toReal);
        else v := false;
       end;
    _var_ := MakeData(v);
   end;
end;

function TParser.Level5;
var
  op:byte;
  var1,var2:PScData;
begin
  Result := Level6(_var_,flag);
  if flag and MATH_ASSIGN = 0 then
  while (Token = '-')or(Token = '+') do
   begin
    if Token = '+' then op := 0 else op := 1;
    if GetToken then exit;
    Result := Level6(var2,flag);
    var1 := MakeData(_var_);
    if op = 0 then
      var1.mathAdd(var2)
    else
      var1.mathSub(var2);
    _var_ := var1;
   end;
end;

function TParser.Level6;
var
  op:byte;
  var1,var2:PScData;
begin
  Result := Level7(_var_,flag);
  if flag and MATH_ASSIGN = 0 then
  while (Token = '*')or(Token = '/') do
   begin
    if Token = '*' then op := 0 else op := 1;
    if GetToken then exit;
    Result := Level7(var2,flag);
    var1 := MakeData(_var_);
    if op = 0 then
      var1.mathMul(var2)
    else
      var1.mathDiv(var2);
    _var_ := var1;
   end;
end;

function TParser.Level7;
var
  var2,var3:PScData;
begin
  Result := Level8(_var_,flag);
  while Token = '?' do
   begin
    if GetToken then exit;
    Result := Level11(var2,flag);
    if token <> ':' then
     begin
       AddError('Syntax error for <exp ? exp1 : exp2>' + token);
       Result := true;
       Exit;
     end;
    if GetToken then exit;
    Result := Level11(var3,flag);
    if _var_.toBool then
      _var_ := var2
    else _var_ := var3;
   end;
end;

function TParser.Level8;
var c:char;
    r:real;
begin
   Result := false;
   if(TokType <> tokCode)and(TokType <> tokString)and((token = '!')or(Token = 'not')or(Token = '-'))then
    begin
      c := token[1];
      if GetToken then exit;
      Result := Level9(_var_,flag);
      _var_ := MakeData(_var_);
      if c = '-' then
        case _var_.GetType of
         data_int: _var_.SetValue(-_var_.toInt);
         data_real: begin r := -_var_.toReal; _var_.SetValue(r); end;
         else _var_.SetValue('');
        end
      else
        _var_.SetValue(not _var_.toBool);
    end
   else
    Result := Level9(_var_,flag);
end;

function TParser.Level9;
var
    item:PScData;
begin
  Result := Level10(_var_,flag);
  while (Token = '++')or(Token = '--') do
   begin
     item := MakeData(_var_);
     if Token = '++' then _var_.mathAdd(1) else _var_.mathAdd(-1);
     _var_ := item;
     if GetToken then exit;
   end;
end;

function TParser.Level10;
var ind:PScData;
    item:PScData;
begin
  Result := Level11(_var_,flag);
  while Token = '[' do
   begin
    Result := Level1(ind,flag and not MATH_ASSIGN);
    if gettoken then exit;
    if token <> ']' then
     begin
       AddError('Syntax error for arr[]' + token);
       Result := true;
       Exit;
     end;
    if GetToken then exit;
    item := _var_.ReadItem(ind.toInt());
    if item = nil then
      _var_ := MakeData('undefined')
    else _var_ := item;
   end;
end;

function TParser.Level11;
var i:integer;
begin
  Result := Level12(_var_,flag);
  while (Token = '@') do
   begin
    if GetToken then exit;
    if ToType(Token, i) then
      _var_.SetSubType(i)
    else AddError('Type ' + token + ' not found');
    if GetToken then exit;
   end;
end;

function TParser.Level12;
var
    int_var,arg:PScData;
    ind:integer;
    user_name:boolean;
begin
   Result := false;
   _var_ := nil;
   if TokType = TokCode then
    begin
      _var_ := MakeData(token);
      _var_.SetType(data_code);
    end
   else if TokType = TokString then
    begin
      _var_ := MakeData(token);
    end
   else if TokType = TokNumber then
      _var_ := MakeData(StrToInt(Token))
   else if TokType = TokReal then
      _var_ := MakeData(StrToFloat(Token))
   else if TokType = TokSymbol then
    begin
      if Token <> '(' then exit;
      if level1(_var_, flag and not MATH_ASSIGN) then exit;
      if GetToken then exit;
      if Token <> ')' then exit;
    end
   else if TokType = TokProp then
     ReadValue(RToken,_var_)
   else if TokType = TokName then
      begin
        user_name := false;
        if Token[1] = '_' then
         begin
            if Token = '_data_' then
              if fData._args_.count > 0 then
                _var_ := fData._args_.values[0]
              else _var_ := MakeData('')
            else if Token = '_line_index_' then
              _var_ := MakeData(LineIndex)
            else if Token = '_line_' then
              _var_ := MakeData(Line)
            else if Token = '_text_' then
              _var_ := MakeData(Lines.Text)
            else if Token = '_id_' then
              _var_ := MakeData(ReadID)
            else if Token = '_event_count_' then
              _var_ := MakeData(_points_count_(pt_event))
            else if Token = '_work_count_' then
              _var_ := MakeData(_points_count_(pt_work))
            else if Token = '_data_count_' then
              _var_ := MakeData(_points_count_(pt_data))
            else if Token = '_var_count_' then
              _var_ := MakeData(_points_count_(pt_var))
            else user_name := true;
         end
        else if Token = 'linked' then
          _var_ := MakeData(LinkedLexem)
        else if Token = 'str' then
          _var_ := StrLexem
        else if Token = 'code' then
          _var_ := CodeLexem
        else if Token = 'lcode' then
          _var_ := lCodeLexem
        else if Token = 'e_int' then
          _var_ := CvtLexem(data_int)
        else if Token = 'e_str' then
          _var_ := CvtLexem(data_str)
        else if Token = 'e_real' then
          _var_ := CvtLexem(data_real)
        else if Token = 'count' then
          _var_ := CountLexem
        else if Token = 'typeof' then
          _var_ := TypeOfLexem
        else if Token = 'expof' then
          _var_ := ExpOfLexem
        else if Token = 'sub' then
          _var_ := SubLexem_res
        else if Token = 'event' then
          _var_ := EventLexem
        else if Token[1] = 'i' then
         begin
            if Token = 'isdef' then
              _var_ := MakeData(IsDefLexem)
            else if Token = 'isset' then
              _var_ := MakeData(IsSetLexem)
            else if Token = 'isndef' then
              _var_ := MakeData(not IsDefLexem)
            else if Token = 'isnset' then
              _var_ := MakeData(not IsSetLexem)
            else if Token = 'issec' then
              _var_ := MakeData(IsSecLexem)
            else if Token = 'isprop' then
              _var_ := MakeData(IsPropLexem)
            else user_name := true;
         end
        else if Token = 'point' then
          _var_ := ReadCustomValue
        else user_name := true;

        if user_name then
          if getInternalVar(token,int_var) then
            _var_ := int_var
          else if getElementVar(token,int_var) then
            _var_ := int_var
          else if getGlobalVar(token,int_var) then
            _var_ := int_var
          else if getLangVar(token,int_var) then
            _var_ := int_var
          else if isInternalFunction(token, ind) then
            _var_ := CallIntFunc(ind)
          else if isFunction(token, ind) then
            _var_ := CallFunc(ind)
          else if isObject(token, ind) then
            _var_ := CallObject(ind)
          else if FindUserType(Token, ind) then
            _var_ := CallTypeConvertion(ind)
          else
           begin
             arg := fData._args_.find(token);
             if arg <> nil then
               _var_ := arg
             else ReadValue(RToken,_var_);
           end;
      end
   else
    begin
      _var_ := MakeData(Token);
    end;

   if _var_ = nil then
    _var_ := MakeData(0);
   Result := GetToken;
end;

procedure TParser.ReadValue;
var
    pt,lp:id_point;
    prop:integer;
    ntype:byte;
begin
    _var_ := MakeData('');
    pt := cgt.elGetPtName(el, PChar(Name));
    lp := 0;
    if pt <> nil then
        case cgt.ptGetType(pt) of
          pt_event:
            begin
               //Debug('Event name found ' + RToken);
               _var_.SetValue(token, data_code);
               exit;
            end;
          pt_data:
            begin
               lp := cgt.ptGetRLinkPoint(pt);
               if not null_data and (lp <> nil) then
                 begin
                   _var_.SetValue(1);
                   exit;
                 end;
            end;
          pt_work, pt_var: ;
        end;

    if (lp = nil){and(pt = 0)} then
       prop := findProperty(RToken)
    else prop := -1;

    if(pt = nil)and(lp = nil)and(prop = -1)then
     begin
        if null_data then
         begin
           _var_.SetValue(lngs[nsection].var_mask,data_code);
           replace(_var_.value.sdata,'%n',Token);
           replace(_var_.value.sdata,'%i',ReadID);
         end
        else _var_.SetValue('');
     end
    else if lp <> nil then
     begin
       _var_ := ReadPoint(lp);
       if pt <> nil then
        begin
         //Debug('def data: ' + _var_.toCode);
         ntype := cgt.ptGetDataType(pt);
         _var_^ := _toCode(_var_, ntype)^;
        end;
     end
    else if (prop <> -1)and((pt = nil) or fData._data_saved_.isEmpty or not cgt.elIsDefProp(el, prop) or (cgt.propGetType(cgt.elGetProperty(el, prop)) = data_comboEx)) then
      begin
       _var_ := readProperty(el, cgt, cgt.elGetProperty(el, prop));
       //Debug('Try to read ' + RToken + ' ' + _var_.toCode);
      end
    else
     begin
      //Debug('def data: ' + fData._data_.toCode);
      if (prop <> -1) {and(fData._data_saved_.GetSubType <> data_null)} then
       begin
         ntype := cgt.propGetType(cgt.elGetProperty(el, prop));
         _var_^ := _toCode(@fData._data_saved_, ntype)^;
       end
      else if pt <> nil then
       begin
         ntype := cgt.ptGetDataType(pt);
//         Debug('i ' + IntToStr(ntype));
         _var_^ := _toCode(@fData._data_saved_, ntype)^;
       end
      else _var_^ := fData._data_saved_;
      if null_data then
        fData._data_saved_.SetAsMT();
     end;
end;

function TParser.getInternalVar;
begin
   int_var := fData._vars_.find(name);
   Result := int_var <> nil;
end;

function TParser.getElementVar;
begin
   el_var := VarList.find(name);
   Result := el_var <> nil;
end;

function TParser.getGlobalVar;
begin
   gbl_var := GVarList.find(name);
   Result := gbl_var <> nil;
end;

function TParser.getLangVar;
begin
   lng_var := LangList.find(name);
   Result := lng_var <> nil;
end;

function TParser.isFunction;
var i:integer;
begin
//   debug('cn: ' + name + ':' + IntToStr(funcList.count));
   for i := 0 to funcList.Count-1 do
    if StrIComp(pchar(name), PChar(funclist[i])) = 0 then
     begin
       ind := i;
       Result := true;
       exit;
     end;
   Result := false;
end;

function TParser.isInternalFunction;
var i:integer;
begin
   for i := 0 to func_map_size-1 do
    if StrIComp(pchar(name), PChar(func_map[i].name)) = 0 then
     begin
       ind := i;
       Result := true;
       exit;
     end;
   Result := false;
end;

function TParser.isObject;
var i:integer;
begin
   for i := 0 to obj_map_size-1 do
    if StrIComp(pchar(name), PChar(obj_map[i].name)) = 0 then
     begin
       ind := i;
       Result := true;
       exit;
     end;
   Result := false;
end;

{$ifdef OP_CALL}
var ind:integer;
{$endif}

procedure TParser.Start;
var var_:PScData;
    ind:integer;
begin
   _err_ := false;
   if ReadLine then exit;

{$ifdef OP_CALL}
  ind := 0;
{$endif}

   while not _err_ and not GetToken do
    case TokType of
      TokSymbol:
         AddError(ER_SYNTAX + ': symbol unknown ' + token);
      TokName:   {$ifdef OP_CALL} begin {debug('try ' + token);} inc(ind); {$endif}
          if Token = 'func' then
             FuncLexem(fname,args)
          else if Token = 'end' then
             begin
               if EndLexem then exit;
             end
          else if Token = 'include' then
             IncludeLexem
          else if Token = 'inline' then
             InlineLexem
          else if Token = 'fvar' then
             FvarLexem
          else if Token = 'var' then
             VarLexem
          else if Token = 'gvar' then
             GVarLexem
          else if Token = 'lang' then
             LangLexem
          else if Token = 'free' then
             FreeLexem
          else if Token = 'return' then
             ReturnLexem
          else if Token = 'for' then
             ForLexem
          else if Token = 'while' then
             WhileLexem
          else if Token = 'if' then
             IfLexem
          else if Token = 'elseif' then
             ElseIfLexem
          else if Token = 'else' then
             ElseLexem
          else if Token = 'print' then
             PrintLexem
          else if Token = 'println' then
             PrintLnLexem
          else if Token = 'event' then
             EventLexem
          else if Token = 'section' then
             SectionLexem
          else if Token = 'switch' then
             SwitchLexem
          else if Token = 'case' then
             CaseLexem
          else if Token = 'inc_sec' then
             IncSecLexem
          else if Token = 'dec_sec' then
             DecSecLexem
          else if Token = 'sub' then
             SubLexem
          else if getInternalVar(token, var_) then
             VarOpLexem(var_)
          else if getElementVar(token, var_) then
             VarOpLexem(var_)
          else if getGlobalVar(token, var_) then
             VarOpLexem(var_)
          else if getLangVar(token, var_) then
             VarOpLexem(var_)
          else if isInternalFunction(token, ind) then
             CallIntFunc(ind)
          else if isObject(token, ind) then
             CallObject(ind)
          else if isFunction(token, ind) then
             CallFunc(ind)
          else
           if not CheckSkipped then
             AddError('Undeclared identifier: ' + RToken);
           {$ifdef OP_CALL}
           //debug('after;');
           if ind > 120 then begin _err_ := true; exit; end; end;
           {$endif}
    end;
end;

function TParser.CallEvent;
var Parser:TParser;
    lp:id_point;
    Args:TArgs;
    e:id_element;
    pt_name:string;
    i:integer;
begin
  Result := MakeData('');
  lp := cgt.elGetPtName(el, PChar(event));
  if(lp <> nil)and(cgt.ptGetType(lp) in [pt_event, pt_data]) then
   begin
    if data.gettype in [data_code, data_array] then
     begin
      i := cgt.ptGetDataType(lp);
      if i <> data_null then data.SetSubType(i);
     end;

//    debug('Call point for event:' + event);
    lp := cgt.ptGetRLinkPoint(lp);
    if lp <> nil then
      begin
        Parser := TParser.Create;
        Args := TArgs.Create;
        Args.AddArg('data')^  := data;
        e := cgt.ptGetParent(lp);
        pt_name := cgt.pt_dpeGetName(lp);
        if length(pt_name) > 0 then   //   ... !!!
           Args.AddArg('index').SetValue(cgt.ptGetIndex(lp))
        else pt_name := cgt.ptGetName(lp);

        {
        if (cgt.elGetClassIndex(e) = CI_MultiElement)or(cgt.elGetClassIndex(e) = CI_EditMulti)or(cgt.elGetClassIndex(e) = CI_EditMultiEx) then
         begin
           Args.AddArg('index').SetValue(cgt.ptGetIndex(lp));
           pt_name := cgt.pt_dpeGetName(lp);
         end
        else pt_name := cgt.ptGetName(lp);
        }
        if lp = hnt_point then
          begin
            hnt_this := true;
            hnt_message := codeb.GenSpaces;
          end;
        Parser.Parse(cgt, e, lowercase(pt_name), codeb, args, Result);
        hnt_this := false;

        Args.Destroy;
        Parser.Destroy;
      end;
   end
  else if event <> '' then ; //AddError('Event ' + event + ' not found');
end;

function TParser.CallEventMt;
var Parser:TParser;
    lp:id_point;
    e:id_element;
    pt_name:string;
begin
  Result := MakeData('');
  lp := cgt.elGetPtName(el, PChar(event));
  if(lp <> nil)and(cgt.ptGetType(lp) in [pt_event, pt_data]) then
   begin
    lp := cgt.ptGetRLinkPoint(lp);
    if lp <> nil then
      begin
        Parser := TParser.Create;
        Args := TArgs.Create;
        //Args.AddArg('data')^  := data;
        e := cgt.ptGetParent(lp);
        pt_name := cgt.pt_dpeGetName(lp);
        if length(pt_name) > 0 then   //   ... !!!
           Args.AddArg('index').SetValue(cgt.ptGetIndex(lp))
        else pt_name := cgt.ptGetName(lp);

        if lp = hnt_point then
          begin
            hnt_this := true;
            hnt_message := codeb.GenSpaces;
          end;
        Parser.Parse(cgt, e, lowercase(pt_name), codeb, args, Result);
        hnt_this := false;

        Args.Destroy;
        Parser.Destroy;
      end;
   end
  else if event <> '' then ;
end;

//**************************************************************************************

procedure ConfToCode(const Pack,UName:string);
var
   State:integer;
   List,Body:TStringList;
   i:integer;
   s,MName:string;
begin
   if FileExists(Pack + 'code\hi' + UName + '.hws') then exit;

   List := TStringList.Create;
   Body := TStringList.Create;

   State := 0;

   List.LoadFromFile(Pack + 'conf\' + UName + '.ini');
   for i := 0 to List.Count-1 do
    if List[i] <> '' then
     if List[i] = '[Edit]' then
      State := 1
     else if List[i] = '[Property]' then
      State := 2
     else if List[i] = '[Methods]' then
      State := 3
     else if List[i] = '[Type]' then
      State := 4
     else
      case State of
        0:;
        1:;
        2:;
        3:
          begin
             s := List[i];
             MName := getTok(s,'=');
             if MName[1] = '*' then delete(mname,1,1);
             GetTok(s,'|');
             case StrToInt(s) of
               1:
                 begin
                   Body.Add('func ' + MName + '(_data)');
                   Body.Add('  // TODO');
                   Body.Add('end');
                   Body.Add('');
                 end;
               2: ;
               3:
                 begin
                   Body.Add('func ' + MName + '()');
                   Body.Add('  return()');
                   Body.Add('end');
                   Body.Add('');
                 end;
               4: ;
             end;
          end;
        4: ;
      end;
   Body.SaveToFile(Pack + 'code\hi' + UName + '.hws');
   List.free;
   Body.free;
end;

//**************************************************************************************

procedure hintForElement(var p:THintParams); cdecl;
var
    pb:TBuildProcessRec;
    list:TStringList;
begin
   pb.cgt := p.cgt;
   pb.sdk := p.sdk;
   hnt_point := p.point;
   hnt_this := false;
   p.hint := nil;
   if buildProcessProc(pb) = CG_SUCCESS then
   else hnt_message := 'Build error';
   freeBuildResult(pb.result);

     list := TStringList.Create;
     list.text := hnt_message;
     if length(list.text) > 1 then
      begin
       GetMem(p.hint, length(list.text)-1);
       StrLCopy(p.hint, PChar(list.text), length(list.text)-2);
      end;
     list.free;

   hnt_point := 0;
end;

//**************************************************************************************

procedure synReadFuncList(var p:TSynParams); cdecl;
var
    ilist,dlist:TStringList;
    i,j:integer;
begin
   ilist := TStringList.Create;
   dlist := TStringList.Create;
   if length(p.objName) = 0 then
    begin
      for i := 0 to func_map_size-1 do
       with func_map[i] do
        begin
         ilist.Add(name + '()');
         dlist.Add('func \column{}\style{+B}' + name + '\style{-B} (' + ainfo + ')');
        end;
    end
   else
    begin
      CreateObjMap;
      for i := 0 to obj_map_size-1 do
       if StrIComp(PChar(obj_map[i].name), p.objName) = 0 then
        begin
          for j := 0 to obj_map[i].methods_count-1 do
           with obj_map[i].methods[j] do
            begin
             ilist.Add(name + '()');
             dlist.Add('func \column{}\style{+B}' + name + '\style{-B} (' + ainfo + ')');
            end;
          break;
        end;
    end;

   if length(ilist.Text) > 0 then
    begin
     GetMem(p.inst_list, length(ilist.Text)+1);
     GetMem(p.disp_list, length(dlist.Text)+1);
     StrCopy(p.inst_list, PChar(iList.Text));
     StrCopy(p.disp_list, PChar(dList.Text));
    end
   else
    begin
     p.inst_list := nil;
     p.disp_list := nil;
    end;
   iList.free;
   dList.free;
end;

exports
  buildPrepareProc,
  buildProcessProc,
  freeBuildResult,
  CheckVersionProc,
  ConfToCode,
  synReadFuncList,
  hintForElement;

end.

