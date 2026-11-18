#ifndef _CGTSHARE_H_
#define _CGTSHARE_H_

#include "share.h"

#if defined(BUILDING_DLL) && !defined(HIASM5_ABI)
#define ELEMENT_FLG_IS_SELECT	0x100
#define ELEMENT_FLG_IS_NOMOUSE	0x2000
#define ELEMENT_FLG_IS_PARENT	0x200
#define ELEMENT_FLG_IS_CORE		0x400
#define ELEMENT_FLG_IS_FREEZE	0x0
#define ELEMENT_FLG_IS_NODELETE	0x2
#define ELEMENT_FLG_ONE_WIDGET	0x1
#define ELEMENT_FLG_IS_SYSTEM	0x400
#else
#define ELEMENT_FLG_IS_SELECT	0x01
#define ELEMENT_FLG_IS_NOMOUSE	0x02
#define ELEMENT_FLG_IS_PARENT	0x04
#define ELEMENT_FLG_IS_CORE		0x08
#define ELEMENT_FLG_IS_FREEZE	0x10
#define ELEMENT_FLG_IS_NODELETE	0x20
#define ELEMENT_FLG_ONE_WIDGET	0x80
#define ELEMENT_FLG_IS_SYSTEM	0x400
#endif

#define  pt_Work  1
#define  pt_Event 2
#define  pt_Var   3
#define  pt_Data  4

#define  data_null  0
#define  data_int   1
#define  data_str   2
#define  data_data  3
#define  data_combo 4
#define  data_list  5
#define  data_icon  6
#define  data_real  7
#define  data_color 8
#define  data_script  9
#define  data_stream  10
#define  data_bitmap  11
#define  data_wave  12
#define  data_array  13
#define  data_comboEx  14
#define  data_font    15
#define  data_matr  16
#define  data_jpeg  17
#define  data_menu  18
#define  data_code  19
#define  data_element 20
#define  data_flags   21
#define  data_stock   22
#define  data_pixbuf  23

// ������ �������� (elGetClassIndex)
#define  CI_DPElement     1
#define  CI_MultiElement  2
#define  CI_EditMulti     3
#define  CI_EditMultiEx   4
#define  CI_InlineCode    5
#define  CI_DrawElement   6
#define  CI_AS_Special    7
#define  CI_DPLElement    8
#define  CI_UseHiDLL      9
#define  CI_WinElement    10
#define  CI_PointHint     11
#define  CI_PointElement  12
#define  CI_LineBreak     13
#define  CI_LineBreakEx   14
#define  CI_UserElement   15

// ����� ����� ��-�� ��������
//extern char *DataNames[];

// ������� ���������� �����
#define  PARAM_CODE_PATH          0
#define  PARAM_DEBUG_MODE         1
#define  PARAM_DEBUG_SERVER_PORT  2
#define  PARAM_DEBUG_CLIENT_PORT  3
#define  PARAM_PROJECT_PATH       4
#define  PARAM_HIASM_VERSION      5
#define  PARAM_USER_NAME          6
#define  PARAM_USER_MAIL          7
#define  PARAM_PROJECT_NAME       8
#define  PARAM_SDE_WIDTH          9
#define  PARAM_SDE_HEIGHT         10
#define  PARAM_COMPILER           11
#define  PARAM_BUILD_CONTER       12

// ������ ��� ������ � ����������� ��������������
#define  CG_SUCCESS          0
#define  CG_NOT_FOUND        1  // ������������� �� ������
#define  CG_INVALID_VERSION  2  // ��� ������ HiAsm ������� ����� ������� ������ ��������������
#define  CG_ENTRY_POINT_NF   3  // ����� ����� � ������������� �� �������
// ������ ��� ���������� �������
#define  CG_BUILD_FAILED     10 // ����� ������ ��� ������ �������
// ������ �� ����� ��������� ���������� ����������
#define  CG_APP_NOT_FOUND    20 // ��������� ���������� �� ������

// ��������� �������
#define  CGMP_COMPRESSED     0x01 // ������������ ������
#define  CGMP_RUN            0x02 // ������������ ������ �� �����
#define  CGMP_RUN_DEBUG      0x04 // ������������ ������ �� ����� � ���������� ������
#define  CGMP_FORM_EDIT      0x08

typedef void* id_sdk;
typedef void* id_element;
typedef void* id_point;
typedef void* id_prop;
typedef void* id_array;
typedef void* id_data;
typedef void* id_font;
typedef void* id_proplist;

struct TCodeGenTools;

typedef TCodeGenTools *PCodeGenTools;

// CodeGen types and entry point interfaces

typedef struct {
	short int major;
	short int minor;
	short int build;
} THiAsmVersion;

typedef struct {
	// none
} TBuildPrepareRec;

typedef struct {
	PCodeGenTools cgt;
	id_sdk sdk;
	void *result;
} TBuildProcessRec;

#if defined(__cplusplus)
static_assert(sizeof(TBuildProcessRec) == sizeof(void *) * 3,
              "TBuildProcessRec ABI must contain three pointer-sized fields");
#endif

typedef struct {
	void *result;
	char *prjFilename;
	char *compiler;
} TBuildMakePrjRec;

typedef struct {
	char *prjFilename;
	char *appFilename;
} TBuildCompliteRec;

typedef struct {
	int flags;
} TBuildParams;

typedef struct {
	char *FileName;
	int Mode;
	int ServerPort;
	int ClientPort;
	void *data;
} TBuildRunRec;

#if !defined(BUILDING_DLL) || defined(HIASM5_ABI) || !defined(_WIN32)
#ifdef __stdcall
#undef __stdcall
#endif
#define __stdcall
#endif

struct TCodeGenTools {
	//~~~~~~~~~~~~~~~~~~~~~~~~ SDK ~~~~~~~~~~~~~~~~~~~~~~~~~~
	int __stdcall(* sdkGetCount)(id_sdk SDK);
	//возвращает количество элементов в схеме
	id_element __stdcall(*sdkGetElement)(id_sdk SDK, int Index);
	//возвращает идент элемента по его Z-координате(индексу)
	id_element __stdcall(*sdkGetElementName)(id_sdk SDK, char *Name);
	//возвращает идент элемента по имени его класса

	//~~~~~~~~~~~~~~~~~~~~~~~~ Element ~~~~~~~~~~~~~~~~~~~~~~~~~~
	int __stdcall(*elGetFlag)(id_element e);
	//возвращает "хитрые" особенности элемента по его иденту
	int __stdcall(*elGetPropCount)(id_element e);
	//возвращает кол-во св-в элемента
	id_prop __stdcall(*elGetProperty)(id_element e, int Index);
	//возвращает целую структуру для конкретного св-ва с порядковым номером из INI
	bool __stdcall(*elIsDefProp)(id_element e, int Index);
	//возвращает True, если значение св-ва совпадает с заданным в INI файле, иначе False
	id_element __stdcall(*elSetCodeName)(id_element e, const char *Name);
	//даем элементу свое любимое(уникальное) имя
	char* __stdcall(*elGetCodeName)(id_element e);
	//и получаем его обратно (если забыли)
	char* __stdcall(*elGetClassName)(id_element e);
	//а вот имя класса не мы давали - можем только узнать
	char* __stdcall(*elGetInfSub)(id_element e);
	//просто содержимое поля Sub из INI-файла элемента
	int __stdcall(*elGetPtCount)(id_element e);
	//получаем количество точек у элемента
	id_point __stdcall(*elGetPt)(id_element e, int i);
	//получаем идент точки по её индексу
	id_point __stdcall(*elGetPtName)(id_element e, const char *Name);
	//получаем идент точки по её имени
	unsigned char __stdcall(*elGetClassIndex)(id_element e);
	//получаем подкласс элемента(константы типа CI_ХХХ)
	id_sdk __stdcall(*elGetSDK)(id_element e);
	//получаем идент внутренней схемы для контейнеров и идент родителя(id_element) для редактора контейнера
	bool __stdcall(*elLinkIs)(id_element e);
	//возвращает True, если данный элемент является ссылкой либо на него ссылаются
	id_element __stdcall(*elLinkMain)(id_element e);
	//возвращает идент главного элемента(тот, на который ссылаются другие)
	void __stdcall(*elGetPos)(id_element e, int &X, int &Y);
	//возвращает позицию элемента в редакторе схем
	void __stdcall(*elGetSize)(id_element e, int &W, int &H);
	//возвращает размеры элемента в редакторе схем
	int __stdcall(*elGetEID)(id_element e);

	//~~~~~~~~~~~~~~~~~~~~~~~~ Point ~~~~~~~~~~~~~~~~~~~~~~~~~~
	id_point __stdcall(*ptGetLinkPoint)(id_point p);
	//возвращает идент точки, с которой соеденена данная
	id_point __stdcall(*ptGetRLinkPoint)(id_point p);
	//возвращает идент точки, с которой соеденена данная без учета точек разрыва и хабов
	int __stdcall(*ptGetType)(id_point p);
	//возвращает тип точек(константы pt_XXX)
	char* __stdcall(*ptGetName)(id_point p);
	//возвращает имя точки
	id_element __stdcall(*ptGetParent)(id_point p);
	//возвращает идент элемента, которому принадлежит точка
	int __stdcall(*ptGetIndex)(id_point p);
	//возвращает относительный индекс точки по принадлежности к одной из 4х групп
	char* __stdcall(*pt_dpeGetName)(id_point p);
	//возвращает базовую часть имени динамических точек(для CI_DPElement)

	//~~~~~~~~~~~~~~~~~~~~~~~~ Property ~~~~~~~~~~~~~~~~~~~~~~~~~~
	int __stdcall(*propGetType)(id_prop prop);
	//возвращает тип параметра
	char* __stdcall(*propGetName)(id_prop prop);
	//возвращает имя параметра
	void* __stdcall(*propGetValue)(id_prop prop);
	//возвращает значение параметра
	unsigned char __stdcall(*propToByte)(id_prop prop);
	//
	int __stdcall(*propToInteger)(id_prop prop);
	//
	double __stdcall(*propToReal)(id_prop prop);
	//
	char* __stdcall(*propToString)(id_prop prop);
	//

	//~~~~~~~~~~~~~~~~~~~~~~~~ Res ~~~~~~~~~~~~~~~~~~~~~~~~~~
	int __stdcall(*resAddFile)(const char *Name);
	//добавляет имя файла в общий список временных файлов для последующего удаления
	char* __stdcall(*resAddIcon)(id_prop p);
	//добавляет иконку в ресурсы и в список временных файлов
	char* __stdcall(*resAddStr)(const char *p);
	//добавляет строку в ресурсы и в список временных файлов
	char* __stdcall(*resAddStream)(id_prop p);
	//добавляет поток в ресурсы и в список временных файлов
	char* __stdcall(*resAddWave)(id_prop p);
	//добавляет звук в ресурсы и в список временных файлов
	char* __stdcall(*resAddBitmap)(id_prop p);
	//добавляет картинку в ресурсы и в список временных файлов
	char* __stdcall(*resAddMenu)(id_prop p);
	//добавляет меню в ресурсы и в список временных файлов

	//~~~~~~~~~~~~~~~~~~~~~~~~ Other ~~~~~~~~~~~~~~~~~~~~~~~~~~
	int __stdcall(*_Debug)(const char *Text, int Color);
	//выводит строку Text в окно Отладка цветом Color
	int __stdcall(*GetParam)(short int index, void *value);
	//возвращает значение параметра среды по его индексу

	//~~~~~~~~~~~~~~~~~~~~~~~~ Arrays ~~~~~~~~~~~~~~~~~~~~~~~~~~
	int __stdcall(*arrCount)(id_array a);
	//возвращает кол-во элементов в массиве а
	int __stdcall(*arrType)(id_array a);
	//возвращает тип элементов в массиве а
	char* __stdcall(*arrItemName)(id_array s, int Index);
	//возвращает имя элемента с индексом Index
	void* __stdcall(*arrItemData)(id_array a, int Index);
	//возвращает значение элемента с индексом Index
	id_prop __stdcall(*arrGetItem)(id_array a, int Index);
	//
	bool __stdcall(*isDebug)(id_sdk e);

	//~~~~~~~~~~~~~~~~~~~~~~~~ Data ~~~~~~~~~~~~~~~~~~~~~~~~~~
	unsigned char __stdcall(*dtType)(id_data d);
	char* __stdcall(*dtStr)(id_data d);
	int __stdcall(*dtInt)(id_data d);
	double __stdcall(*dtReal)(id_data d);

	//~~~~~~~~~~~~~~~~~~~~~~~~ Font ~~~~~~~~~~~~~~~~~~~~~~~~~~
	char* __stdcall(*fntName)(id_font f);
	int __stdcall(*fntSize)(id_font f);
	unsigned char __stdcall(*fntStyle)(id_font f);
	int __stdcall(*fntColor)(id_font f);
	unsigned char __stdcall(*fntCharSet)(id_font f);

	//~~~~~~~~~~~~~~~~~~~~~~~~ Element ~~~~~~~~~~~~~~~~~~~~~~~~~~
	void* __stdcall(*elGetData)(id_element e);
	// получает пользовательские данные элемента
	void __stdcall(*elSetData)(id_element e, void *data);
	// устанавливает пользовательские данные элемента

	//~~~~~~~~~~~~~~~~~~~~~~~~ Point ~~~~~~~~~~~~~~~~~~~~~~~~~~
	int __stdcall(*ptGetDataType)(id_point p);
	// возвращает тип данных точки

	//~~~~~~~~~~~~~~~~~~~~~~~~ Element ~~~~~~~~~~~~~~~~~~~~~~~~~~
	id_sdk __stdcall(*elGetParent)(id_element e);
	// возвращает родителя
	int __stdcall(*elGetPropertyListCount)(id_element e);
	// возвращает количество элементов в списке св-тв(из панели св-ва)
	id_proplist __stdcall(*elGetPropertyListItem)(id_element e, int i);
	// возвращает элемент списка св-тв

	//~~~~~~~~~~~~~~~~~~~~~~~~ PropertyList ~~~~~~~~~~~~~~~~~~~~~~~~~~
	char* __stdcall(*plGetName)(id_proplist p);
	// возвращает имя св-ва
	char* __stdcall(*plGetInfo)(id_proplist p);
	// возвращает описание св-ва
	char* __stdcall(*plGetGroup)(id_proplist p);
	// возвращает группу св-ва
	id_prop __stdcall(*plGetProperty)(id_proplist p);
	// возвращает значение св-ва
	id_element __stdcall(*plGetOwner)(id_proplist p);
	// возвращает родительский элемент даного св-ва

	//~~~~~~~~~~~~~~~~~~~~~~~~ Point ~~~~~~~~~~~~~~~~~~~~~~~~~~
	char* __stdcall(*ptGetInfo)(id_point p);
	// Возвращает описание точки

	//~~~~~~~~~~~~~~~~~~~~~~~~ Property ~~~~~~~~~~~~~~~~~~~~~~~~~~
	id_element __stdcall(*propGetLinkedElement)(id_element e, const char *propName);
	// Возвращает id элемента, прилинкованного к указанному св-ву
	int __stdcall(*propIsTranslate)(id_element e, id_prop p);
	// Возвращает 1 если св-во помечено на перевод

	id_element __stdcall(*propGetLinkedElementInfo)(id_element e, id_prop prop, const char *_int);

	//~~~~~~~~~~~~~~~~~~~~~~~~ Element ~~~~~~~~~~~~~~~~~~~~~~~~~~
	id_sdk __stdcall(*elGetSDKByIndex)(id_element e, int index);
	// Возвращает SDK полиморфного контейнера по его индексу
	int __stdcall(*elGetSDKCount)(id_element e);
	// Возвращает количаство сабклассов полиморфного контейнера
	char* __stdcall(*elGetSDKName)(id_element e, int index);

	//~~~~~~~~~~~~~~~~~~~~~~~~ SDK ~~~~~~~~~~~~~~~~~~~~~~~~~~
	id_element __stdcall(*sdkGetParent)(id_sdk SDK);
	// Возвращает элемент родитель для данного SDK

	//~~~~~~~~~~~~~~~~~~~~~~~~ Element ~~~~~~~~~~~~~~~~~~~~~~~~~~
	char* __stdcall(*elGetInterface)(id_element e);
	// Возвращает интерфейсы, предоставляемые элементом
	char* __stdcall(*elGetInherit)(id_element e);
	// Возвращает классы, от которых наследуется элемент

	//~~~~~~~~~~~~~~~~~~~~~~~~ Resource ~~~~~~~~~~~~~~~~~~~~~~~~~~
	int __stdcall(*resEmpty)();
	// Возвращает 1 если список ресурсов пуст, и 0 в противном случае
	int __stdcall(*resSetPref)(const char *pref);
	// Устанавливает префикс для имен ресурсов

	//~~~~~~~~~~~~~~~~~~~~~~~~ SDE ~~~~~~~~~~~~~~~~~~~~~~~~~~
	int __stdcall(*_Error)(int line, id_element e, const char *text);
	// Добавляет информацию в панель ошибок

	//~~~~~~~~~~~~~~~~~~~~~~~~ Element ~~~~~~~~~~~~~~~~~~~~~~~~~~
	int __stdcall(*elGetGroup)(id_element e);
	// возвращает ID группы, к которой принадлежит элемент и 0 если группа отсутствует

	//~~~~~~~~~~~~~~~~~~~~~~~~ Property ~~~~~~~~~~~~~~~~~~~~~~~~~~
	int __stdcall(*propSaveToFile)(id_prop p, const char *fileName);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// пользовательские ф-ции, не входящие в интерфейс

	int ReadIntParam(int Index) {
		int i = 0;
		GetParam(Index, &i);
		return i;
	}

	char *ReadStrParam(int Index, id_element e = NULL) {
		char *c;
		c = new char[256]();
		*((id_element*)c) = e;
		if(GetParam(Index, c)) c[0] = 0;
		return c;
	}

	//------------------------- DEBUG TOOLS ----------------------

	inline void trace(const char *text) {
		_Debug(text, 0x0000FF);
	}

	void trace(int value) {
		char text[32];
		sprintf(text, "%d", value);
		_Debug(text, 0x0000FF);
	}
	
	void error(const char *text) {
		_Debug(text, 0xFF0000);
	}
	
	id_prop getPropByName(id_element e, const char *name) {
		for(int i = 0; i < elGetPropCount(e); i++) {
			id_prop p = elGetProperty(e, i);
			if(strcasecmp(name, propGetName(p)) == 0)
				return p;
		}
		
		return NULL;
	}
	bool isDefProp(id_element e, id_prop p) {
		for(int i = 0; i < elGetPropCount(e); i++) {
			if(elGetProperty(e, i) == p)
				return elIsDefProp(e, i);
		}
		
		return false;
	}
	int getBuildCounter(id_element e) {
		return GetParam(PARAM_BUILD_CONTER, &e);
	}
};

typedef struct {
	char *elName; // ��� �������� ��������
	char *objName; // ��� �������
	char *inst_list; // ������ ������� � ����� ��� ������� � ��������
	char *disp_list; // ������, ������������ �� ������������ ���������
} TSynParams;

typedef struct {
	id_point point;
	id_sdk sdk;
	PCodeGenTools cgt;
	char *hint;
} THintParams;

/*
function PosEx(const SubStr, S: string; Offset: Cardinal = 1): Integer;
var
  I,X: Integer;
  Len, LenSubStr: Integer;
begin
  if Offset = 1 then
	Result := Pos(SubStr, S)
  else begin
	I := Offset;
	LenSubStr := Length(SubStr);
	Len := Length(S) - LenSubStr + 1;
	while I <= Len do begin
	  if S[I] = SubStr[1] then
	  begin
		X := 1;
		while (X < LenSubStr) and (S[I + X] = SubStr[X + 1]) do
		  Inc(X);
		if (X = LenSubStr) then begin
		  Result := I;
		  exit;
		end;
	  end;
	  Inc(I);
	end;
	Result := 0;
  end;
end;


procedure Replace(var Str:string;const substr,dest:string );
var p:integer;
begin
  p := Pos(substr,str);
  while p > 0 do begin
	Delete(str,p,length(substr));
	Insert(dest,Str,p);
	p := PosEx(substr,str,p + Length(dest));
  end;
end;

function GetTok(var s:string; const c:char):string;
var p:integer;
begin
  p := Pos(c,s);
  if p > 0 then begin
	Result := Copy(s,1,p-1);
	if p = Length(s) then
	  s := ''
	else Delete(s,1,p);
  end else Result := s;
end;
 */

#endif
