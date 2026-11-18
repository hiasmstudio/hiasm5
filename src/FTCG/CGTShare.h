#ifndef _CGTSHARE_H_
#define _CGTSHARE_H_

  // параметры элемента, возвращаемые elGetFlag
#define  ONE_COPY    0x1   // одна копия компонента в пределах SDK этого же типа(???)
#define  NO_DELETE   0x2   // не удаляемый компонент
#define  IS_EXCLUZIV 0x4   // одна копия компонента в пределах SDK с таким флагом(???)
#define  IS_EDIT     0x8   // определяет компоненты редактирования
#define  IS_CONTAIN  0x10  // контейнер может содержать все
#define  IS_BACK     0x20  // для элементов всегда находящихся внизу( EditMulti )
#define  IS_MULTI    0x40  // элемент контейнер
#define  ADD_LAST    0x80  // элемент, добавляемый на форму последним( Splitter )
#define  IS_SELECT   0x100 // выбранный элемент
#define  IS_PARENT   0x200 // родитель для элементов текущей SDK
#define  IS_HIDE     0x400 // not build
#define  IS_LINK     0x800
#define  PSTORE      0x1000 // генерировать карту св-тв(not used)

  // типы точек элемента
#define  pt_Work  1
#define  pt_Event 2
#define  pt_Var   3
#define  pt_Data  4

  // типы данных
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

  // классы элемента (elGetClassIndex)
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

  // имена типов св-тв элемента
//extern char *DataNames[];

  // индексы параметров среды
#define  PARAM_CODE_PATH          0
#define  PARAM_DEBUG_MODE         1
#define  PARAM_DEBUG_SERVER_PORT  2
#define  PARAM_DEBUG_CLIENT_PORT  3
#define  PARAM_PROJECT_PATH       4
#define  PARAM_HIASM_VERSION      5

  // ошибки при работе с библиотекой кодогенератора
#define  CG_SUCCESS          0
#define  CG_NOT_FOUND        1  // кодогенератор не найден
#define  CG_INVALID_VERSION  2  // эта версия HiAsm требует более позднюю версию кодогенератора
#define  CG_ENTRY_POINT_NF   3  // точка входа в кодогенератор не найдена
  // ошибки при компиляции проекта
#define  CG_BUILD_FAILED     10 // общая ошибка при сборке проекта
  // ошибки на этапе обработки результата компиляции
#define  CG_APP_NOT_FOUND    20 // результат компиляции не найден

  // параметры проекта
#define  CGMP_COMPRESSED     0x01 // поддерживает сжатие
#define  CGMP_RUN            0x02 // поддерживает запуск из среды
#define  CGMP_RUN_DEBUG      0x04 // поддерживает запуск из среды в отладочном режиме

typedef unsigned short WORD;
typedef unsigned int DWORD;

typedef void* id_point;
typedef void* id_sdk;

typedef void* PCodeGenTools;

// CodeGen types and entry point interfaces
typedef struct {
   WORD major;
   WORD minor;
   WORD build;
} THiAsmVersion;

typedef struct {
     // none
} TBuildPrepareRec;

typedef struct {
    PCodeGenTools cgt;
    // id_sdk is a pointer in CGTShare.pas. Keep this field pointer-sized
    // on both Win64 and Unix; DWORD truncates the SDK handle on 64-bit hosts.
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
    DWORD flags;
} TBuildParams;

typedef struct {
    char *FileName;
    int Mode;
    int ServerPort;
    int ClientPort;
    void *data;
} TBuildRunRec;

typedef struct {
   char *elName;     // имя текущего элемента
   char *objName;    // имя объекта
   char *inst_list;  // список методов и полей для вставки в редактор
   char *disp_list;  // список, отображаемый во всплыывающей подсказке
} TSynParams;

typedef struct {
    id_point point;
    id_sdk sdk;
    PCodeGenTools cgt;
    char *hint;
} THintParams;

#endif

