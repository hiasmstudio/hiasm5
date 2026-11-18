{ HiAsm 4 code generator interface as seen by 32-bit Delphi CodeGen DLLs.
  Field order and signatures follow Elements/CGTShare.pas of HiAsm 4; CGT
  functions are stdcall, CodeGen and make entry points are cdecl. Handles are
  pointers to the objects of unit Model. }
unit Cgt;

{$mode delphi}

interface

const
  CI_MultiElement = 2;

  PARAM_CODE_PATH         = 0;
  PARAM_DEBUG_MODE        = 1;
  PARAM_DEBUG_SERVER_PORT = 2;
  PARAM_DEBUG_CLIENT_PORT = 3;
  PARAM_PROJECT_PATH      = 4;
  PARAM_HIASM_VERSION     = 5;
  PARAM_USER_NAME         = 6;
  PARAM_USER_MAIL         = 7;
  PARAM_PROJECT_NAME      = 8;
  PARAM_SDE_WIDTH         = 9;
  PARAM_SDE_HEIGHT        = 10;
  PARAM_COMPILER          = 11;

  CG_SUCCESS = 0;

type
  id_sdk = pointer;
  id_element = pointer;
  id_point = pointer;
  id_prop = pointer;
  id_array = pointer;
  id_data = pointer;
  id_font = pointer;
  id_proplist = pointer;

  // 85 procedure pointers; Tools.InitTools checks that every slot is assigned.
  TCodeGenTools = record
    sdkGetCount: function(SDK: id_sdk): integer; stdcall;
    sdkGetElement: function(SDK: id_sdk; Index: integer): id_element; stdcall;
    sdkGetElementName: function(SDK: id_sdk; Name: PAnsiChar): id_element; stdcall;
    elGetFlag: function(e: id_element): cardinal; stdcall;
    elGetPropCount: function(e: id_element): cardinal; stdcall;
    elGetProperty: function(e: id_element; Index: integer): id_prop; stdcall;
    elIsDefProp: function(e: id_element; Index: integer): boolean; stdcall;
    elSetCodeName: function(e: id_element; Name: PAnsiChar): cardinal; stdcall;
    elGetCodeName: function(e: id_element): PAnsiChar; stdcall;
    elGetClassName: function(e: id_element): PAnsiChar; stdcall;
    elGetInfSub: function(e: id_element): PAnsiChar; stdcall;
    elGetPtCount: function(e: id_element): cardinal; stdcall;
    elGetPt: function(e: id_element; i: integer): id_point; stdcall;
    elGetPtName: function(e: id_element; Name: PAnsiChar): id_point; stdcall;
    elGetClassIndex: function(e: id_element): byte; stdcall;
    elGetSDK: function(e: id_element): id_sdk; stdcall;
    elLinkIs: function(e: id_element): boolean; stdcall;
    elLinkMain: function(e: id_element): id_element; stdcall;
    elGetPos: procedure(e: id_element; var X, Y: integer); stdcall;
    elGetSize: procedure(e: id_element; var W, H: integer); stdcall;
    elGetEID: function(e: id_element): integer; stdcall;
    ptGetLinkPoint: function(p: id_point): id_point; stdcall;
    ptGetRLinkPoint: function(p: id_point): id_point; stdcall;
    ptGetType: function(p: id_point): cardinal; stdcall;
    ptGetName: function(p: id_point): PAnsiChar; stdcall;
    ptGetParent: function(p: id_point): id_element; stdcall;
    ptGetIndex: function(p: id_point): cardinal; stdcall;
    pt_dpeGetName: function(p: id_point): PAnsiChar; stdcall;
    propGetType: function(prop: id_prop): integer; stdcall;
    propGetName: function(prop: id_prop): PAnsiChar; stdcall;
    propGetValue: function(prop: id_prop): pointer; stdcall;
    propToByte: function(prop: id_prop): byte; stdcall;
    propToInteger: function(prop: id_prop): integer; stdcall;
    propToReal: function(prop: id_prop): double; stdcall;
    propToString: function(prop: id_prop): PAnsiChar; stdcall;
    resAddFile: function(Name: PAnsiChar): cardinal; stdcall;
    resAddIcon: function(P: id_prop): PAnsiChar; stdcall;
    resAddStr: function(P: PAnsiChar): PAnsiChar; stdcall;
    resAddStream: function(P: id_prop): PAnsiChar; stdcall;
    resAddWave: function(P: id_prop): PAnsiChar; stdcall;
    resAddBitmap: function(P: id_prop): PAnsiChar; stdcall;
    resAddMenu: function(P: id_prop): PAnsiChar; stdcall;
    _Debug: function(Text: PAnsiChar; Color: cardinal): cardinal; stdcall;
    GetParam: function(index: word; value: pointer): cardinal; stdcall;
    arrCount: function(a: id_array): integer; stdcall;
    arrType: function(a: id_array): integer; stdcall;
    arrItemName: function(a: id_array; Index: integer): PAnsiChar; stdcall;
    arrItemData: function(a: id_array; Index: integer): pointer; stdcall;
    arrGetItem: function(a: id_array; Index: integer): id_prop; stdcall;
    isDebug: function(e: id_sdk): longbool; stdcall;
    dtType: function(d: id_data): byte; stdcall;
    dtStr: function(d: id_data): PAnsiChar; stdcall;
    dtInt: function(d: id_data): integer; stdcall;
    dtReal: function(d: id_data): double; stdcall;
    fntName: function(f: id_font): PAnsiChar; stdcall;
    fntSize: function(f: id_font): integer; stdcall;
    fntStyle: function(f: id_font): byte; stdcall;
    fntColor: function(f: id_font): cardinal; stdcall;
    fntCharSet: function(f: id_font): byte; stdcall;
    elGetData: function(e: id_element): pointer; stdcall;
    elSetData: procedure(e: id_element; data: pointer); stdcall;
    ptGetDataType: function(p: id_point): integer; stdcall;
    elGetParent: function(e: id_element): id_sdk; stdcall;
    elGetPropertyListCount: function(e: id_element): cardinal; stdcall;
    elGetPropertyListItem: function(e: id_element; i: integer): id_proplist; stdcall;
    plGetName: function(p: id_proplist): PAnsiChar; stdcall;
    plGetInfo: function(p: id_proplist): PAnsiChar; stdcall;
    plGetGroup: function(p: id_proplist): PAnsiChar; stdcall;
    plGetProperty: function(p: id_proplist): id_prop; stdcall;
    plGetOwner: function(p: id_proplist): id_element; stdcall;
    ptGetInfo: function(p: id_point): PAnsiChar; stdcall;
    propGetLinkedElement: function(e: id_element; propName: PAnsiChar): id_element; stdcall;
    propIsTranslate: function(e: id_element; p: id_prop): integer; stdcall;
    propGetLinkedElementInfo: function(e: id_element; prop: id_prop; _int: PAnsiChar): id_element; stdcall;
    elGetSDKByIndex: function(e: id_element; index: integer): id_sdk; stdcall;
    elGetSDKCount: function(e: id_element): integer; stdcall;
    elGetSDKName: function(e: id_element; index: integer): PAnsiChar; stdcall;
    sdkGetParent: function(SDK: id_sdk): id_element; stdcall;
    elGetInterface: function(e: id_element): PAnsiChar; stdcall;
    elGetInherit: function(e: id_element): PAnsiChar; stdcall;
    resEmpty: function: integer; stdcall;
    resSetPref: function(pref: PAnsiChar): integer; stdcall;
    _Error: function(Line: integer; e: id_element; Text: PAnsiChar): cardinal; stdcall;
    elGetGroup: function(e: id_element): integer; stdcall;
    propSaveToFile: function(p: id_prop; fileName: PAnsiChar): integer; stdcall;
  end;
  PCodeGenTools = ^TCodeGenTools;

  TBuildProcessRec = record
    cgt: PCodeGenTools;
    sdk: id_sdk;
    result: pointer;
  end;
  TBuildMakePrjRec = record
    result: pointer;
    prjFilename: PAnsiChar;
  end;
  PBuildMakePrjRec = ^TBuildMakePrjRec;
  TBuildCompliteRec = record
    prjFilename: PAnsiChar;
    appFilename: PAnsiChar;
  end;
  PBuildCompliteRec = ^TBuildCompliteRec;

  // Delphi passes these const/var records by reference (checked in make_exe.dll);
  // FPC would copy a const record under cdecl, so pointers are explicit.
  TbuildPrepareProc = function(params: pointer): integer; cdecl;
  TbuildProcessProc = function(var params: TBuildProcessRec): integer; cdecl;
  TbuildMakePrj = function(params: PBuildMakePrjRec): integer; cdecl;
  TbuildCompliteProc = function(params: PBuildCompliteRec): integer; cdecl;

implementation

end.
