#include "scheme_codegen.h"
#include "element_config.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "scheme_diagram.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QRegularExpression>
#include <QtMath>
#include <QSet>
#include <QStringDecoder>

#include "../RTCG/CGTShare.h"
#include <deque>
#include <stdexcept>

namespace {
[[noreturn]] void fail(const QString &message) {
    throw std::runtime_error(message.toStdString());
}

struct Property {
    QByteArray name;
    int type = 0;
    QByteArray initial;
    QByteArray value;
    QList<QByteArray> choices;
    QByteArray text;
    int number = 0;
    unsigned char dataType = data_null;

    void decode() {
        text = value;
        number = value.toInt();
        if (type == data_comboEx) text = choices.value(number);
        if (type == data_str && text.startsWith('"') && text.endsWith('"'))
            text = text.mid(1, text.size() - 2);
        if ((type == data_str || type == data_list) && value.startsWith('#')) {
            QStringList lines;
            if (!decodeSchemeLines(QString::fromUtf8(value), lines))
                fail(QStringLiteral("Invalid string property: %1").arg(QString::fromUtf8(name)));
            text = lines.join(u'\n').toUtf8();
        }
        if (type == data_color && value.startsWith('#')) {
            QColor color(QString::fromUtf8(value));
            number = color.red() | (color.green() << 8) | (color.blue() << 16);
        }
        if (type == data_data && !value.isEmpty()) {
            if (value == "Null()") {
                text.clear();
                return;
            }
            const auto open = value.indexOf('(');
            const auto kind = value.left(open);
            if (open < 0 || !value.endsWith(')')) fail(QStringLiteral("Invalid Data property"));
            if (kind == "String") dataType = data_str;
            else if (kind == "Integer") dataType = data_int;
            else if (kind == "Real") dataType = data_real;
            else fail(QStringLiteral("Unsupported Data type: %1").arg(QString::fromUtf8(kind)));
            text = value.mid(open + 1, value.size() - open - 2);
            number = text.toInt();
        }
    }
};

struct Node;
struct Port {
    QByteArray name;
    Node *owner;
    int kind = 0;
    int type = 0;
    Port *link = nullptr;
    QByteArray dynamicName;
};

template<typename Item>
Item *named(std::deque<Item> &items, const QByteArray &name) {
    for (auto &item : items)
        if (item.name.compare(name, Qt::CaseInsensitive) == 0) return &item;
    return nullptr;
}

struct Sdk {
    QList<Node *> elements;
    Node *owner = nullptr;
    QByteArray hostClass;
    Node *host = nullptr;
    QByteArray pageClass;
    bool polyContainer = false;
};

struct Node {
    QByteArray name;
    int id;
    bool entry;
    std::deque<Property> props;
    std::deque<Port> ports;
    void *state = nullptr;
    Sdk *parent = nullptr;
    Sdk *child = nullptr;
    QList<Sdk *> childSdks;
    unsigned char classIndex = 0;
    bool controlManager = false;
};

struct Model {
    std::deque<Node> nodes;
    QByteArray codePath;
    QByteArray project;
    QStringList errors;
};

// The RTCG ABI has no host-context parameter. Each build has its own helper process.
thread_local Model *active = nullptr;
Node *node(void *handle) { return static_cast<Node *>(handle); }
Property *prop(void *handle) { return static_cast<Property *>(handle); }
Port *port(void *handle) { return static_cast<Port *>(handle); }

// These are the event/data endpoints implemented by the Qt branches in the package templates.
bool supportedQtPort(const Node &n, const QByteArray &name) {
    if (n.name == "ChildPanelPoly" || n.name == "ChildFormPoly" ||
        (n.name == "EditPolyMulti" && n.parent->polyContainer)) return true;
    static const QHash<QByteArray, QList<QByteArray>> ports{
        {"MST_ArrayRows", {"Strings","Count","EndIdx"}},
        {"MST_ArrayCols", {"ColumnArray","MTCols","CountCol","EndIdxCol"}},
        {"MST_Virtual", {"doNextPage","doPrevPage","doSetPage","doResetPage","doInitPage","doSetVirtIdx","onChangeVirtIdx","CurVirtIdx","CurPage","PerPage","VirtualData","Page","VirtIdx"}},
        {"ArrayEnum", {"doEnum","doStop","onItem","onEndEnum","Array","Item","Index"}},
        {"MTStrTbl", {"doClientRect","doGrid","doStyle","doInfoTip","doTrackSelect","doMultiSelect","doEnableOnClick","doChangeWidth","doNColorRow","doColor","doSetFocus","doVisible","doEnabled","doHint","onClientRect","onClick","onSelect","onColumnClick","onCheck","onLineChange","Row","Col","Select","Index","Count","CountCol","EndIdx","EndIdxCol","ItemAtPos","TopItem","PerPage","MTStrTbl"}},
        {"MST_DB", {"doAddRow","doAddCols","doClear","doClearAll","onChange","onChangeColLst","Count","EndIdx","CountCol","EndIdxCol"}},
        {"MST_Clear", {"doClear","onChange"}},
        {"MST_RowAction", {"doRowAction","onChange","Index","Count","EndIdx"}},
        {"MST_ColAction", {"doColAction","doMinColWidth","doMaxColWidth","onChange","onChangeColLst","onResult","Index","CountCol","EndIdxCol"}},
        {"MST_SelectRow", {"doSelect","doSelEndStr","doSelectOut","doSelectAll","doSelectInvert","doEnsureVisible","doSetFocus","doAutoMakeVisible","onSelectRow","IndexSelect","Select","Index","Count","EndIdx"}},
        {"MST_Selected", {"AllSelect","SelCount"}},
        {"MST_CheckBoxes", {"doCheckBox","onCheckBox","IndexRow","Mode","AllCheck","CheckCount"}},
        {"MST_Matrix", {"doEMatrix","doChkEMatrix","doSelEMatrix","onEMatrix","X","Y"}},
        {"MST_FindText", {"doFindText","doFindNext","doEnsureVisible","doSetFocus","onFindText","FindText","StartRow","StartCol","ReplaceText","Select","Index","Count","EndIdx"}},
        {"MST_SortInCol", {"doSort","doSortDigit","doMethodSort","onChange"}},
        {"MST_Load", {"doLoad","doLoadFromStream","doDelimiter","onLoad","onLoadFromStream","FileName","Stream"}},
        {"MST_Save", {"doSave","doSaveToStream","doDelimiter","onSave","onSaveToStream","FileName","Stream"}},
        {"MST_HTML", {"doTabToHTML","onTabToHTML","HTMLTab"}},
        {"MST_EditEnd", {"doEditEnd","EditEnd"}},
        {"MST_UseEditCtrl", {"doSetData","doClientRect","onCellSize","onEnterCell","onEscCell","onClientRect","SubItem","Index","Cell","Row","Col"}},
        {"MST_DrawManager", {"doIconSize","doSingleString","doTabGrid","doTabGridFrame","doGutterStyle","doLightTextSel","doBumpText","doGrid3D","doStyleGrid3D","doGradient","doColorRowSel","doGradientColor","doFrameColor","doBkFrameColor","doShadowColor","doLightTxtColor","doGutterColor","onChangeProperty"}},
        {"MST_SetColors", {"doBkColor","doTextColor","doTextBkColor","doIconColColor","onChangeProperty","GenColors"}},
        {"CommandCenter", {"doRefresh","onRefresh","onAction"}},
        {"CMD_State", {"doSetState","onSetState","Name","Enabled","Checked"}},
        {"CMD_PopupMenu", {"doPopupHere","doPopup","onRefresh","onAction"}},
        {"CMD_ToolBar", {"onRefresh","onAction"}},
        {"TreeViewTrain", {"doClear","doExpand","doExpandNode","doExpandToggle","doCollapse","doCollapseNode","doSort","onClick","onDelete","onExpand","onCollapse","onDrop","Select","NextID","DropAccept"}},
        {"TVT_AddNode", {"doAddNode","onAddNode","onAddFailed","Data"}},
        {"TVT_ChangeNode", {"doChangeNode","onChangeNode","onChangeFailed","Data"}},
        {"TVT_DeleteNode", {"doDeleteNode","onDeleteNode","onDeleteFailed","Data"}},
        {"TVT_GetNode", {"doGetNode","onGetNode","onGetFailed","Node","ID"}},
        {"TVT_SelectNode", {"doSelectNode","onSelectNode","onSelectFailed","SelectNode","ID"}},
        {"TVT_FirstNode", {"doFirstNode","onFirstNode","onEmpty","onError","ID"}},
        {"TVT_ItemPath", {"doItemPath","onItemPath","onError","ID"}},
        {"TVT_EnumNodes", {"doEnum","onEnum","onStop","Node","ID"}},
        {"TVT_LoadTree", {"doLoadTree","onLoadTree","FileName"}},
        {"TVT_SaveTree", {"doSaveTree","onSaveTree","FileName"}},
        {"TVT_DataSource", {"doLoad","doSave","onLoad","onError"}},
        {"DoData", {"doData","onEventData","Data"}},
        {"MReadWrite", {"doRead","doWrite","doColumn","onRead","Item","Count","Index","Value"}},
        {"MEnumerator", {"doEnum","doColumn","onEnum","Item","Index"}},
        {"MListBox", {"doRefresh","doCaptionField","doDataType","doDataField","onSelectChange"}},
        {"DS_UserData", {"onInit","onClose","Count","Columns","Row"}},
        {"BoxDrawManager", {"doGradient","doHorizontal","doFrame","doInversGrad","doLightColor","doDarkColor","doColorFrame"}},
        {"CtrlPalette", {"doCaption","doStrings","doAdd","doClear","doDelete","doReplace","doInsert","doLoad","doSave",
                         "onClick","onChangeState","onChange","Str","FileName","Strings","Count","EndIdx"}},
        {"HBoxLayout", {"doResize","doSort"}},
        {"VBoxLayout", {"doResize","doSort"}},
        {"GridLayout", {"doResize","doSort"}},
        {"ToolBarLayout", {"doResize","doSort"}},
        {"ToolBar", {"doEnable","doDisable","onClick"}},
        {"TreeView", {"doAdd","doClear","doDelete","doLoad","doSave","doRename","doSelect","doLoadFromText","onChange","onClick","Count","Array","Index","Str","FileName","IconID"}},
        {"StringTable", {"doAdd","doClear","doDelete","doLoad","doSave","doInsert","doAddColumn","doSort","doSortDigit",
                         "doSelect","doEnsureVisible","onClick","onSelect","onColumnClick","onLineChange",
                         "Count","Select","EndIdx","Index","PerPage","Strings","Str","FileName"}},
        {"StringTableMT", {"doAdd","doClear","doDelete","doInsert","doReplace","doSort","doSortDigit","doSelect","doEnsureVisible",
                           "doDeleteCol","doClearCol","doAutoColWidth","doRefresh","doMT_Add","doMT_Insert","doMT_Replace","doMT_AddCols","doMT_NameCol","doMT_WidthCol","onChangeColLst",
                           "doMT_AlignTxtCol","doGetCol_MT","onMT_GetCol",
                           "onClick","onSelect","onColumnClick","onLineChange","Count","Select","EndIdx","Index","PerPage",
                           "Strings","CountCol","EndIdxCol","Str"}},
        {"TreeViewEx", {"doAdd","doClear","doDelete","doLoad","doSave","doRename","doSelect","doLoadFromText",
                        "doExpand","doCollapse","doExpandToggle","doSort","doItemBold","doItemHasChild","doItemPath","doAddChild","doInsert",
                        "onChange","onClick","onExpand","onCollapse","onItemPath","Count","Array","Index","ItemParent","ItemChild",
                        "Str","FileName","IconID","Parent","Child"}},
        {"PageControl", {"##add","##select","##delete","##clear","##nselect","##count","##index","##name"}},
        {"MainForm", {"onCreate","onClose","onActivate","onDeactivate","doCaption","doClose","doMinimize","doRestore","doVisible","doEnabled","doHint","doShowModal"}},
        {"Message", {"doMessage","onMessage","onOK","onCANCEL","onABORT","onRETRY","onIGNORE","onYES","onNO","Message","Caption"}},
        {"Button", {"onClick"}}, {"BitBtn", {"onClick","doCaption"}}, {"Panel", {"onClick","doCaption"}}, {"GroupBox", {"onClick"}}, {"Label", {"Caption","doText","Text","onClick"}},
        {"CheckBox", {"doCheck","onCheck","onClick","Checked"}},
        {"RadioButton", {"doSelect","doCaption","onSelect","Selected"}},
        {"TrackBar", {"doPosition","doPosition2","doMin","doMax","doTickCount","onPosition","onStart","onStop","Position"}},
        {"TrackBarRush", {"doPosition","doPosition2","doMin","doMax","doUpdate","onPosition","onStart","onStop","Position"}},
        {"ProgressBar", {"doPosition","doMax","Position"}},
        {"ProgressBarRush", {"doPosition","doMax","doRoundWidth","doRoundHeight","doUpdate","Position"}},
        {"ButtonRush", {"onClick","doUpdate","doRoundWidth","doRoundHeight"}},
        {"PanelRush", {"onClick","doCaption","doUpdate"}},
        {"CheckBoxRush", {"doCheck","doCaption","doUpdate","Checked","onCheck","onClick"}},
        {"RadioButtonRush", {"doSelect","doCaption","doUpdate","Selected","onSelect"}},
        {"GProgressBar", {"doPosition","doMax","doLightProgress","doDarkProgress","doInvKindGrad","onClick","Position"}},
        {"Grapher", {"doAdd","doClear","doSaveToFile","doMaxValues","Min","Max","ValueY"}},
        {"Plotter", {"doClear","doSaveToFile","doMinH","doMaxH","doMinW","doMaxW","MinX","MaxX","MinY","MaxY","FileName"}},
        {"PlotLines", {"doAdd","doClear","doShow","doColor","doMaxValues","onAdd","MinX","MaxX","MinY","MaxY","ValueY","ValueX"}},
        {"PlotAxis", {"doAxis","onAxis","X","Y","Angle"}},
        {"PlotMarker", {"doAxis","onAxis","X","Y"}},
        {"PlotStream", {"doPlotStream","onPlotStream","Stream"}},
        {"PlotDiffSeries", {"doDiff","onDiff"}},
        {"PlotMouseValues", {"onMouseDown","onMouseUp","onMouseMove","Button"}},
        {"RedrawManager", {"doRedraw","doBeginUpdate","doEndUpdate"}},
        {"FontManager", {"doFont","doSetFont","FontSize","FontColor","FontName","FontStyle","FontCharset"}},
        {"UseExtCursor", {"doSetCursor","doResetCursor","onSetCursor","FileCurName"}},
        {"UseWinAnimation", {"doAnimation","doRedrawWindow","doActivationMode","doCENTER","doSLIDE","doHOR_POSITIVE","doHOR_NEGATIVE","doVER_POSITIVE","doVER_NEGATIVE"}},
        {"UserHintManager", {"onChangeHint","onShow","onHide"}},
        {"TransparentManager", {"doAlphaBlendValue","doPicture"}},
        {"Menu", {"doInit","onSelectNum","onSelectStr","Handle","Array"}},
        {"PopupMenu", {"doPopupHere","doPopup","doAddItem","doClear","onEndPopup","onSelectStr","onClick","Bitmaps","Handle"}},
        {"PopupMenuEx", {"doPopupHere","doPopup","doClear","doInit","onEndPopup","MenuItemName","MenuItemIdx",
                         "doNameItems","onNameItems","doEnabledItems","onEnabledItems","doVisibleItems","onVisibleItems",
                         "doCheckItems","onCheckItems","doIdxCheck","doIdxEnabled","doidxVisible","doCheckIdx","doEnablIdx",
                         "doVisiblIdx","Index","doInitBmp","Bitmaps","Count","Handle"}},
        {"GLabel", {"doText","doColor","doColorShadow","doColorHover","doFont","doAlignment","doVAlignment",
                     "doTextGradient","doFonGradient","doAntialiased","doUnderline","doHorizontal","doHorizFonGrad",
                     "doInversGrad","doInvFonGrad","doHSymbolGrad","doDepthShadow","doDepthGrad","doContrast","doContrastFon",
                     "onClick","Caption","Text"}},
        {"IndexManager", {"doClear","doDelete","Indexes"}},
        {"IconsManager", {"doClearIcons","doImgBkColor","IconArray","CountIcons","EndIdxIcons","ImgSize"}},
        {"WorkIconsManager", {"doClearIcons","doDeleteIcon","doGetIcon","doLoadIcon","doSaveIcon","doLoadIList","doSaveIList","doCountPakIcons","doLoadPakIcons","onChangeImgLst","onGetIcon","onCountPakIcons","IconArray","CountIcons","EndIdxIcons","ImgSize","CountPakIcons","IconFileName","IListFileName","IconsFileName"}},
        {"DropFileManager", {"doListen","doStop","doDropType","onDrop","onDropEnd","onDragEnter","onDragOver","onDragLeave"}},
        {"DropTextManager", {"doListen","doStop","doDropType","onDrop","onDragEnter","onDragOver","onDragLeave"}},
        {"VisualShape", {"doColor2","doPColor","doShapeType","doPWidth","doPStyle"}},
        {"PaintBox", {"doRefresh","doClear","doColor","onBeforeDraw","onClick","Bitmap","Width","Height"}},
        {"ScrollBox", {"doHScroll","doVScroll","onHScroll","onVScroll","HScroll","VScroll","HPos","VPos"}},
        {"ColorBox", {"doInitList","doAdd","doSelectColor","doSelectName","doFormatColor","onClick","Index","CurrentColor","ColorArray","Str","Value"}},
        {"ScrollBar", {"doPosition","doMin","doMax","doPage","onPosition","onEndScroll","Position"}},
        {"ScrollBarEx", {"doPosition","doMin","doMax","onPosition","Position"}},
        {"UpDown", {"doPosition","doMin","doMax","doStep","onPosition","Position"}},
        {"FontBox", {"doSetFont","doReReadFonts","onResult","CurrentFont","Font"}},
        {"DriveBox", {"doDisk","doLabel","onSelect","onUnreadable","Disk","Label","DefaultDisk"}},
        {"StatusBar", {"doText","doIndexText","doWidth","doVisible","onText","Handle","Count","Text","Panel","Width"}},
        {"StatusBarEx", {"doText","doIndexText","doWidth","doVisible","doPanelPBar","doVisiblePBar","doHintPBar","doMax","doPosPBar","onText","Handle","Count","PosPBar","Text","Panel","Width","Array"}},
        {"Timer", {"doTimer","doStop","doStopAll","doInterval","doAutoStop","onTimer","onStop","Enabled"}},
        {"TabControl", {"PageInsert","PageTab","PageDelete","DeleteCurrent","ReplaceIdx","onChange","CurTabIndx","CurTabCaption","TabCount","NewPageText","Index","IconIdx"}},
        {"Image", {"doLoad","doClear","doRefresh","doViewStyle","doHalfTone","onClick","Bitmap","ImageBitmap","ImageWidth","ImageHeight"}},
        {"AlphaImage", {"doLoad","doClear","doRefresh","doViewStyle","doAlphaBlendValue","doAlphaMode","onClick","AlphaBitmap","ImageBitmap","ImageWidth","ImageHeight"}},
        {"ImgBtn", {"doNormal","doSelect","doDown","doEnable","onClick","Normal"}},
        {"DatePicker", {"doSetDate","doVisible","doEnabled","doHint","onChange","onDropDown","onCloseUp","Data","DateTime","CurrentDateTime","DateInt","CurrentDateInt"}},
        {"LED", {"doOn","doOff","doChangeValue","doSendToBack","onChange","onClick","Value"}},
        {"LedNumber", {"doText","doAlignment","doShowLines","doColorHover","doFonColor","doSegColor","doNoSegColor","doFonHover","doSegHover","doNoSegHover","onClick","Caption","Text"}},
        {"LedLadder", {"doPosition","doMax","doMinMid","doMidMax","doCount","doSpacing","doGradient","doFonColor","doSegColorMin","doNoSegColorMin","doSegColorMid","doNoSegColorMid","doSegColorMax","doNoSegColorMax","onClick","Position"}},
        {"ComboBox", {"doAdd","doClear","doDelete","doText","doLoad","doSave","doSelect","doEditText","doEditTextNoEvents","doSelectString","doAddDir","doReplace","onChange","onClick","onChangeText","onSelect","String","Text","Count","EndIdx","EditText","Index","Str","FileName"}},
        {"ListBox", {"doAdd","doClear","doDelete","doText","doLoad","doSave","doSelect","doSelectAll","doSelectString","doAddDir","doReplace","doInsert","doAppend","doEnsureVisible","doUp","doDown","onChange","onClick","onSelect","Text","Count","EndIdx","String","Index","Str","Value","FileName"}},
        {"Memo", {"doAdd","doClear","doDelete","doText","doLoad","doSave","doSetSelect","doSetSelStart","doSetSelLength","doEnsureVisible","onChange","Text","Array","Count","EndIdx","SelText","Position","Str","FileName"}},
        {"WebBrowser", {"doNavigate","doRefresh","doBack","doForward","doFromText","doPrint","doPreview","doSavePage","doStop",
                        "onNavigate","onTitle","onStatus","onProgress","onQuit","CurrentURL","Page","URL","Navigate","NewWindow"}},
        {"Flash", {"doPlay","doPause","doStop","doGoToFrame","doBgColor","onPlay","onPause","onStop","CurrentFrame","FileName"}},
        {"HilightMemo", {"doAdd","doClear","doDelete","doText","doLoad","doSave","doHilight","doHilightCaseSens","doAddHilightStrings",
                         "doReadOnly","doOverwrite","doCut","doCopy","doPaste","doUndo","doRedo","doEnsureVisible","onChange",
                         "Text","Count","EndIdx","SelText","Str","FileName"}},
        {"RichEdit", {"doAdd","doFormatSel","doText","doClear","doDelete","doLoad","doSave","doSetSelect","doSetSelStart","doSetSelLength","doUndo","doRedo","onChange","Text","Count","EndIdx","Position","SelText","Str","Color","Style","FileName"}},
        {"Edit", {"onChange","onEnter","Text","Str","doText","doText2","doSelectAll","doPosition","Position"}}
    };
    if (n.name == "PlotPoints" || n.name == "PlotHistogram") return ports.value("PlotLines").contains(name);
    if (n.name == "MenuEx") {
        if (name == "doPopup" || name == "doPopupHere" || name == "doClear" || name == "doInit" || name == "onEndPopup") return false;
        return ports.value("PopupMenuEx").contains(name);
    }
    return n.classIndex == CI_MultiElement || n.classIndex == CI_EditMulti || n.name == "Hub" || ports.value(n.name).contains(name);
}

TCodeGenTools callbacks() {
    TCodeGenTools t{};
    t.sdkGetCount = [](id_sdk s) { return s ? int(static_cast<Sdk *>(s)->elements.size()) : 0; };
    t.sdkGetElement = [](id_sdk s, int i) -> id_element { return static_cast<Sdk *>(s)->elements.value(i); };
    t.sdkGetParent = [](id_sdk s) -> id_element { return static_cast<Sdk *>(s)->owner; };
    t.elGetSDK = [](id_element e) -> id_sdk { return node(e)->child ? node(e)->child : node(e)->parent; };
    t.elGetParent = [](id_element e) -> id_sdk { return node(e)->parent; };
    t.elGetFlag = [](id_element e) {
        const auto *n = node(e);
        if (n->name == "PagePanel" || n->name == "EditPolyMulti" ||
            (n->parent->polyContainer && n->parent->host == n)) return ELEMENT_FLG_IS_SYSTEM;
        return (n->entry || n->parent->host == n) ? ELEMENT_FLG_IS_PARENT : 0;
    };
    t.elGetClassIndex = [](id_element e) { return node(e)->classIndex; };
    t.elGetClassName = [](id_element e) { return node(e)->name.data(); };
    t.elGetEID = [](id_element e) { return e ? node(e)->id : 0; };
    t.elGetCodeName = [](id_element e) { return node(e)->name.data(); };
    t.elGetSDKByIndex = [](id_element e, int i) -> id_sdk { return node(e)->childSdks.value(i); };
    t.resAddFile = [](const char *file) {
        active->errors.append(QStringLiteral("External resource is not packaged: %1").arg(QString::fromUtf8(file)));
        return 0;
    };
    t.propGetLinkedElement = [](id_element e, const char *key) -> id_element {
        auto *p = named(node(e)->props,key);
        if (!p || p->value.isEmpty()) return nullptr;
        for (auto *candidate : node(e)->parent->elements) {
            auto *name = named(candidate->props,"Name");
            if (name && name->text == p->text) return candidate;
        }
        return nullptr;
    };
    t.arrCount = [](id_array a) {
        if (prop(a)->value.startsWith('[') && prop(a)->value != "[]")
            active->errors.append(QStringLiteral("Unsupported RTCG array: %1").arg(QString::fromUtf8(prop(a)->name)));
        return 0;
    };
    t.arrType = [](id_array a) { return prop(a)->initial.toInt(); };
    t.arrGetItem = [](id_array, int) -> id_prop { return nullptr; };
    t.elGetData = [](id_element e) { return node(e)->state; };
    t.elSetData = [](id_element e, void *v) { node(e)->state = v; };
    t.elGetPropCount = [](id_element e) { return int(node(e)->props.size()); };
    t.elGetProperty = [](id_element e, int i) -> id_prop { return &node(e)->props.at(i); };
    t.elIsDefProp = [](id_element e, int i) { const auto &p = node(e)->props.at(i); return p.initial == p.value; };
    t.propGetType = [](id_prop p) { return prop(p)->type; };
    t.propGetName = [](id_prop p) { return prop(p)->name.data(); };
    t.propGetValue = [](id_prop p) -> void * { return p; };
    t.propToInteger = t.dtInt = [](void *p) { return prop(p)->number; };
    t.propToReal = t.dtReal = [](void *p) { return prop(p)->text.toDouble(); };
    t.propToString = t.dtStr = [](void *p) { return prop(p)->text.data(); };
    t.dtType = [](id_data p) { return prop(p)->dataType; };
    t.elGetPtCount = [](id_element e) { return int(node(e)->ports.size()); };
    t.elGetPt = [](id_element e, int i) -> id_point { return &node(e)->ports.at(i); };
    t.elGetPtName = [](id_element e, const char *s) -> id_point { return named(node(e)->ports, s); };
    t.ptGetLinkPoint = [](id_point p) -> id_point { return port(p)->link; };
    t.ptGetName = [](id_point p) { return port(p)->name.data(); };
    t.ptGetParent = [](id_point p) -> id_element { return port(p)->owner; };
    t.ptGetType = [](id_point p) { return port(p)->kind; };
    t.ptGetDataType = [](id_point p) { return port(p)->type; };
    t.ptGetIndex = [](id_point p) {
        int index = 0;
        for (auto &other : port(p)->owner->ports) {
            if (&other == p) break;
            if (other.kind == port(p)->kind) ++index;
        }
        return index;
    };
    t.pt_dpeGetName = [](id_point p) { return port(p)->dynamicName.data(); };
    t.ptGetRLinkPoint = [](id_point p) -> id_point {
        Port *next = port(p)->link;
        QSet<Port *> visited;
        while (next && (next->owner->name == "HubEx" || next->owner->name == "GetDataEx")) {
            if (visited.contains(next)) return nullptr;
            visited.insert(next);
            auto *across = named(next->owner->ports, next->owner->name == "HubEx" ? "onEvent" : "Data");
            next = across && across != next ? across->link : nullptr;
        }
        return next;
    };
    t.elLinkIs = [](id_element) { return false; };
    t.elLinkMain = [](id_element) -> id_element { return nullptr; };
    t.GetParam = [](short index, void *out) {
        if (index == PARAM_BUILD_CONTER) return 1;
        if (index != PARAM_CODE_PATH && index != PARAM_PROJECT_NAME) return 1;
        qstrncpy(static_cast<char *>(out), index == PARAM_CODE_PATH ? active->codePath.constData() :
                 active->project.constData(), 256);
        return 0;
    };
    t._Debug = [](const char *msg, int color) {
        if (color == 0xff0000) active->errors.append(QString::fromUtf8(msg));
        return 0;
    };
    return t;
}
}

static GeneratedScheme generateCoreScheme(const SchemeDiagram &diagram, const QString &packs) {
    using Endpoint = QPair<int, QString>;
    QHash<Endpoint, Endpoint> wires;
    QHash<int, int> mirrors;
    QHash<int, int> layerParent, layerContent;
    QSet<int> allIds;
    QSet<Endpoint> destinations;
    SchemeLayer layer;
    QHash<int, int> parents;
    for (int i = 0; i < diagram.layers.size(); ++i) {
        const auto &current = diagram.layers.at(i);
        QHash<Endpoint, int> portKinds;
        for (const auto &e : current.elements)
            for (const auto &port : schemeElementPorts(packs, "_base", diagram, e))
                portKinds.insert({e.id,QString::fromUtf8(port.name)},port.kind);
        for (const auto &connection : current.connections) {
            const Endpoint source{current.elements.at(connection.source).id, connection.sourcePoint};
            const Endpoint target{connection.targetId, connection.targetPoint};
            const int a = portKinds.value(source), b = portKinds.value(target);
            if (!((a == pt_Event && b == pt_Work) || (a == pt_Data && b == pt_Var)))
                fail(QStringLiteral("Invalid Core link: %1 -> %2").arg(source.second, target.second));
            if (wires.contains(source) || destinations.contains(target)) fail(QStringLiteral("Duplicate Core connection"));
            destinations.insert(target);
            wires.insert(source, target);
        }
        const auto append = [&](const SchemeElement &element, int parent) {
            layer.elements.append(element);
            parents.insert(element.id, parent);
        };
        const int content = layerContent.value(i, -1);
        // Create the nested container before its child widgets, regardless of file order.
        for (const auto &e : current.elements)
            if (e.id == content) append(e, layerParent.value(i, -1));
        for (const auto &e : current.elements) {
            if (allIds.contains(e.id)) fail(QStringLiteral("Duplicate element ID: %1").arg(e.id));
            allIds.insert(e.id);
            if (e.id == content || mirrors.contains(e.id)) continue;
            const int parent = content >= 0 ? content : layerParent.value(i, -1);
            if (e.child < 0) { append(e, parent); continue; }
            const auto config = elementConfig(packs, "_base", e.name);
            if (config.className != "HCMultiElement") fail(QStringLiteral("Unsupported Core container: %1").arg(e.name));
            int frame = -1, wrapper = -1;
            for (const auto &child : diagram.layers.at(e.child).elements) {
                if (child.name == "hcEditMulti") frame = child.id;
                if (!config.sub.isEmpty() && child.name == QString::fromUtf8(config.sub)) wrapper = child.id;
            }
            if (frame < 0 || (!config.sub.isEmpty() && wrapper < 0))
                fail(QStringLiteral("Incomplete Core container: %1").arg(e.name));
            mirrors.insert(e.id, frame);
            mirrors.insert(frame, e.id);
            layerParent.insert(e.child, parent);
            layerContent.insert(e.child, wrapper);
        }
    }
    for (int i = 0; i < layer.elements.size(); ++i) {
        const auto &e = layer.elements.at(i);
        // Only actual source points produce runtime connections. Cross frame
        // boundaries in both directions for events and data requests.
        for (const auto &port : elementConfig(packs, "_base", e.name).ports) {
            if (port.kind != pt_Event && port.kind != pt_Data) continue;
            const QString name = QString::fromUtf8(port.name);
            Endpoint point{e.id, name};
            QSet<Endpoint> visited;
            while (wires.contains(point)) {
                if (visited.contains(point)) fail(QStringLiteral("Cyclic Core frame connection"));
                visited.insert(point);
                const auto target = wires.value(point);
                if (!mirrors.contains(target.first)) {
                    layer.connections.append({i, target.first, name, target.second});
                    break;
                }
                point = {mirrors.value(target.first), target.second};
            }
        }
    }
    QJsonArray nodes;
    QSet<int> ids;
    QHash<int, ElementConfig> configs;
    int root = -1;
    for (const auto &e : layer.elements) {
        if (ids.contains(e.id)) fail(QStringLiteral("Duplicate element ID: %1").arg(e.id));
        ids.insert(e.id);
        if (e.name == "InfoTip") continue;
        const auto config = elementConfig(packs, "_base", e.name);
        if (!config.error.isEmpty()) fail(config.error);
        const auto kind = config.className;
        if (!kind.startsWith("HCWE_") && kind != "HCMessage" && kind != "HCOpenDialog" &&
            kind != "HCSaveDialog" && kind != "HCPopupMenu" && kind != "HCDoData" && kind != "HCConsole" && kind != "HCMemory")
            fail(QStringLiteral("Unsupported Core element: %1").arg(e.name));
        if (kind == "HCWE_Form" || kind == "HCWE_MainForm" || kind == "HCWE_Dialog") {
            if (root != -1) fail(QStringLiteral("Multiple top-level Core forms"));
            root = e.id;
        }
        configs.insert(e.id, config);
        QJsonObject props;
        for (const auto &p : config.properties) {
            Property prop{p.name, p.type, p.initial, p.initial, p.choices};
            for (const auto &[name,value] : e.properties)
                if (name.toUtf8().compare(p.name, Qt::CaseInsensitive) == 0) prop.value = value.toUtf8();
            prop.decode();
            QString value = QString::fromUtf8(prop.text);
            if (p.type == data_comboEx) value = QString::number(prop.number);
            if (p.type == data_array) {
                QStringList items;
                if (value.startsWith('[')) {
                    static const QRegularExpression item(QStringLiteral(R"core('(?:[^']*)'\s*=\s*"([^"]*)")core"));
                    auto matches = item.globalMatch(value);
                    while (matches.hasNext()) items.append(matches.next().captured(1));
                }
                value = items.join(u'\n');
            }
            props.insert(QString::fromUtf8(p.name), value);
        }
        nodes.append(QJsonObject{{"id",e.id},{"parent",parents.value(e.id,-1)},{"kind",QString::fromUtf8(kind)},{"props",props}});
    }
    if (root == -1) fail(QStringLiteral("Core project requires a form or dialog"));
    QJsonArray links;
    for (const auto &link : layer.connections) {
        const int from = layer.elements.at(link.source).id;
        auto portKind = [&](int id, const QString &name) {
            for (const auto &p : configs.value(id).ports) if (p.name == name.toUtf8()) return p.kind;
            return 0;
        };
        const int a = portKind(from, link.sourcePoint), b = portKind(link.targetId, link.targetPoint);
        if (!((a == pt_Event && b == pt_Work) || (a == pt_Data && b == pt_Var)))
            fail(QStringLiteral("Invalid Core link: %1 -> %2").arg(link.sourcePoint,link.targetPoint));
        links.append(QJsonArray{from,link.sourcePoint,link.targetId,link.targetPoint});
    }
    const QJsonDocument model(QJsonObject{{"root",root},{"nodes",nodes},{"links",links}});
    GeneratedScheme result;
    result.type = ProjectType::Form;
    result.gui = result.core = true;
    result.source = "#include <QByteArray>\nint runCore(int, char **, const QByteArray &);\n"
        "int main(int argc, char **argv) { return runCore(argc, argv, QByteArray::fromBase64(\n";
    const auto encoded = model.toJson(QJsonDocument::Compact).toBase64();
    for (qsizetype i = 0; i < encoded.size(); i += 512)
        result.source += '"' + encoded.mid(i,512) + "\"\n";
    result.source += ")); }\n";
    return result;
}

GeneratedScheme generateCppScheme(const QString &source, const QString &packs,
                                  const QString &library, const QString &projectName) {
    static const QRegularExpression make(QStringLiteral(R"(^\s*Make\((cpp|_base|web|modules)\)\s*$)"),
                                         QRegularExpression::MultilineOption);
    const auto pack = make.match(source).captured(1);
    if (pack.isEmpty()) fail(QStringLiteral("Unknown scheme package"));
    const bool coreProject = pack == "_base";
    static const QRegularExpression linkedElement(QStringLiteral(R"(^\s*(?:elink|MakeExt)\()"),
        QRegularExpression::MultilineOption);
    if (linkedElement.match(source).hasMatch()) fail(QStringLiteral("External and linked elements are not supported yet."));
    const auto diagram = readSchemeDiagram(source);
    if (diagram.errorLine) fail(QStringLiteral("Invalid scheme at line %1").arg(diagram.errorLine));
    if (coreProject) return generateCoreScheme(diagram, packs);
    Model model;
    const QDir packDir(packs);
    model.codePath = (packDir.absoluteFilePath(pack + "/code") + '/').toUtf8();
    model.project = projectName.toUtf8();
    if (model.codePath.size() >= 256 || model.project.size() >= 256) fail(QStringLiteral("RTCG path exceeds its ABI limit."));
    QList<QHash<int, Node *>> layerIds(diagram.layers.size());
    QSet<int> reserved, assigned;
    for (const auto &layer : diagram.layers)
        for (const auto &e : layer.elements) reserved.insert(e.id);
    int spareId = 1;
    Node *entry = nullptr;
    std::deque<Sdk> sdks(diagram.layers.size());
    for (int li = 0; li < diagram.layers.size(); ++li) {
      auto &ids = layerIds[li];
      for (const auto &e : diagram.layers[li].elements) {
        if (ids.contains(e.id)) fail(QStringLiteral("Duplicate element ID: %1").arg(e.id));
        if (e.name == "InfoTip") { ids.insert(e.id, nullptr); continue; }
         const bool isEntry = projectType(e.name,pack) != ProjectType::Unknown &&
             !(li != 0 && e.name.toUtf8() == sdks[li].hostClass);
        int codeId = e.id;
        if (assigned.contains(codeId)) {
            while (reserved.contains(spareId)) ++spareId;
            codeId = spareId++;
        }
        assigned.insert(codeId);
        model.nodes.push_back({e.name.toUtf8(), codeId, isEntry});
        auto &n = model.nodes.back();
        ids.insert(e.id, &n);
        n.parent = &sdks[li];
        n.parent->elements.append(&n);
        if (!n.parent->host && n.parent->hostClass == n.name) n.parent->host = &n;
        const auto config = elementConfig(packs, pack, e.name);
        if (!config.error.isEmpty()) fail(config.error);
        const bool poly = n.name == "ChildFormPoly" || n.name == "ChildPanelPoly";
        if (n.name == "PageControl" || poly) {
            const QString host = poly ? QString::fromUtf8(config.sub) : QStringLiteral("PagePanel");
            QStringList classes;
            for (const auto &[key, value] : e.properties)
                if (key == "Childrens" && !value.isEmpty() && !decodeSchemeLines(value, classes))
                    fail(QStringLiteral("Invalid PageControl class list"));
            if (e.childLayers.size() != classes.size() + 1) fail(QStringLiteral("PageControl class and scheme counts differ"));
            QSet<QString> seen;
            for (qsizetype i = 0; i < e.childLayers.size(); ++i) {
                const auto layer = e.childLayers[i];
                const auto &elements = diagram.layers[layer].elements;
                if (elements.isEmpty() || elements.first().name != "EditPolyMulti" ||
                    (i && (elements.size() < 2 || elements[1].name != host)))
                    fail(QStringLiteral("Invalid PageControl child scheme"));
                auto *sdk = &sdks[layer];
                sdk->owner = &n;
                sdk->polyContainer = poly;
                n.childSdks.append(sdk);
                if (i) {
                    const auto &name = classes[i - 1];
                    if (name.isEmpty() || seen.contains(name.toCaseFolded())) fail(QStringLiteral("Invalid PageControl class name"));
                    seen.insert(name.toCaseFolded());
                    sdk->pageClass = name.toUtf8();
                    if (poly) sdk->hostClass = config.sub;
                }
            }
            n.child = n.childSdks.first();
            n.props.push_back({poly ? "qtPolyClassCount" : "qtPageClassCount", data_int, {}, QByteArray::number(classes.size()), {}});
        } else if (e.child >= 0) {
            if (config.className != "MultiElement" && config.className != "MultiElementEx") fail(QStringLiteral("Unsupported nested container: %1").arg(e.name));
            if (e.childLayers.size() != 1) fail(QStringLiteral("Unexpected multiple child schemes: %1").arg(e.name));
             if (config.className == "MultiElementEx")
                for (const auto &[key,value] : e.properties)
                    if (key == "Mode" && value.toInt() != 0) fail(QStringLiteral("Dynamic MultiElementEx is not supported"));
            n.child = &sdks[e.child];
            n.childSdks.append(n.child);
            n.child->owner = &n;
            n.child->hostClass = config.sub;
            n.classIndex = CI_MultiElement;
        } else if (config.className == "EditMulti" || config.className == "EditMultiEx") {
            if (!n.parent->owner) fail(QStringLiteral("Unexpected top-level EditMulti"));
            n.classIndex = CI_EditMulti;
        }
        if (config.className == "DPLElement") n.classIndex = CI_DPLElement;
        for (const auto &interfaceName : config.interfaces.split(','))
            if (interfaceName.trimmed() == "ControlManager") n.controlManager = true;
        if (isEntry) {
            if (entry || li != 0) fail(QStringLiteral("Multiple project entry elements"));
            entry = &n;
        }
        for (const auto &p : config.properties)
            n.props.push_back({p.name, ((n.name == "BitBtn" && p.name == "Bitmap") ||
                ((n.name == "Image" || n.name == "AlphaImage") && p.name == "Picture") ||
                (n.name == "ImgBtn" && p.type == data_bitmap) || ((n.name == "ButtonRush" || n.name == "PanelRush") && p.name == "Glyphs") ||
                (n.name == "TransparentManager" && p.name == "Picture") ||
                ((n.name == "TabControl" || n.name == "MenuEx" || n.name == "PopupMenuEx") && p.name == "Bitmaps") || (n.name == "ToolBar" && p.name == "Buttons") ||
                (n.name == "IndexManager" && p.name == "Index") || (n.name == "CommandCenter" && p.name == "Commands") ||
                ((n.name == "IconsManager" || n.name == "TreeView" || n.name == "TreeViewEx") && p.name == "Icons")) ? data_pixbuf : p.type,
                 p.initial, p.initial, p.choices});
        if (n.name == "PagePanel") n.props.push_back({"qtPageClassName", data_str, {}, n.parent->pageClass, {}});
        if (n.name == "MainForm")
            n.props.push_back({"qtPolyForm", data_int, {}, n.parent->polyContainer ? "1" : "0", {}});
        if (n.name == "EditPolyMulti" && n.parent->polyContainer)
            n.props.push_back({"qtPolyClassName", data_str, {}, n.parent->pageClass, {}});
        if (n.name == "TreeView" || n.name == "TreeViewEx")
            n.props.push_back({"qtExtendedTree", data_int, {}, n.name == "TreeViewEx" ? "1" : "0", {}});
        if (n.name == "StringTable" || n.name == "StringTableMT")
            n.props.push_back({"qtExtendedTable", data_int, {}, n.name == "StringTableMT" ? "1" : "0", {}});
        if (n.name == "MenuEx" || n.name == "PopupMenuEx")
            n.props.push_back({"qtExtendedMainMenu", data_int, {}, n.name == "MenuEx" ? "1" : "0", {}});
        for (const auto &p : elementPorts(config, e.properties)) {
            n.ports.push_back({p.name, &n, p.kind, p.type});
            if (n.name == "EditPolyMulti" && n.parent->polyContainer) {
                const int inverted = p.kind % 2 ? p.kind + 1 : p.kind - 1;
                if (n.parent->pageClass.isEmpty()) {
                    auto *owner = n.parent->owner;
                    const int kind = inverted;
                    owner->ports.push_back({p.name, owner, kind, p.type});
                    if (kind == pt_Work) owner->ports.back().dynamicName = "polyWork";
                    if (kind == pt_Var) owner->ports.back().dynamicName = "polyVar";
                } else {
                    auto &port = n.ports.back();
                    if (port.kind == pt_Work) port.dynamicName = "polyEmit";
                    if (port.kind == pt_Var) port.dynamicName = "polyData";
                }
            }
            if (n.name == "EditPolyMulti" && n.parent->owner && n.parent->owner->name == "PageControl" &&
                n.parent->pageClass.isEmpty()) {
                static const QHash<QByteArray, QByteArray> lifecycle{
                    {"##add", "pageAdd"}, {"##select", "pageSelect"}, {"##delete", "pageDelete"},
                    {"##clear", "pageClear"}, {"##nselect", "pageSelectName"}, {"##count", "pageCount"},
                    {"##index", "pageIndex"}, {"##name", "pageName"}};
                if (const auto method = lifecycle.value(p.name); !method.isEmpty() && !named(n.parent->owner->ports, p.name)) {
                    auto &mirrored = n.parent->owner->ports.emplace_back(Port{p.name, n.parent->owner,
                        p.name == "##count" || p.name == "##index" || p.name == "##name" ? pt_Var : pt_Work, p.type});
                    mirrored.dynamicName = method;
                }
            }
            if (config.className == "Hub") {
                const auto fields = config.sub.split(',').value(p.kind - 1).split('|');
                if (fields.size() == 2) n.ports.back().dynamicName = fields[1];
            }
            if (n.classIndex == CI_DPLElement)
                n.ports.back().dynamicName = config.sub.split(',').value(p.kind - 1).split('|').value(0);
            if (n.classIndex == CI_EditMulti) {
                const int mirror = p.kind % 2 ? p.kind + 1 : p.kind - 1;
                n.parent->owner->ports.push_back({p.name, n.parent->owner, mirror, p.type});
                if (config.className == "EditMultiEx") {
                    static const QByteArray methods[] = {"", "doWork", "onEvent", "getVar", "_Data"};
                    n.ports.back().dynamicName = methods[p.kind];
                    n.parent->owner->ports.back().dynamicName = methods[mirror];
                }
            }
        }
        for (const auto &[name, value] : e.properties) {
            if (auto *p = named(n.props, name.toUtf8())) p->value = value.toUtf8();
        }
        for (auto &p : n.props) p.decode();
      }
    }
    if (!entry) fail(QStringLiteral("Project entry element not found"));
    GeneratedScheme result;
    result.type = projectType(QString::fromUtf8(entry->name),pack);
    result.gui = result.type == ProjectType::Form;
    result.webWidgets = pack == "cpp" ? schemeWebWidgets(diagram) : SchemeWebWidgets{};
    if (result.webWidgets.needsWebEngine() && !result.gui)
        fail(QStringLiteral("WebBrowser and Flash require a C++ form project."));
    if (auto *mode = named(entry->props, "BuildMode")) result.release = mode->number == 1;
    const auto *target = named(entry->props, "TargetPlatform");
#ifdef Q_OS_WIN
    constexpr int nativeTarget = 1;
#else
    constexpr int nativeTarget = 2;
    if (result.type == ProjectType::WindowsLibrary || result.type == ProjectType::WindowsService ||
        result.type == ProjectType::ControlPanel || result.type == ProjectType::ComExtension)
        fail(QStringLiteral("This project requires Windows"));
#endif
    if (target && target->number && target->number != nativeTarget) fail(QStringLiteral("Select the Native target platform."));
    for (auto &sdk : sdks) if (sdk.polyContainer && sdk.host) {
        auto *frame = sdk.elements.first();
        frame->props.push_back({"qtPolyHostId", data_int, {}, QByteArray::number(sdk.host->id), {}});
        frame->props.back().decode();
    }
    const auto boxLayout = [](const Node &n) {
        return n.name == "HBoxLayout" || n.name == "VBoxLayout" || n.name == "GridLayout" || n.name == "ToolBarLayout";
    };
    const auto layoutName = [](Node &n) {
        const auto *reference = named(n.props, "Layout");
        auto value = reference ? reference->text : QByteArray();
        if (value.startsWith('"') && value.endsWith('"')) value = value.mid(1, value.size() - 2);
        return value == "(empty)" ? QByteArray() : value;
    };
    QHash<Node *, Node *> layoutParents;
    if (result.gui) {
        for (auto &member : model.nodes) {
            const auto name = layoutName(member);
            if (name.isEmpty()) continue;
            Node *layout = nullptr;
            for (auto *candidate : member.parent->elements) {
                const auto *property = named(candidate->props, "Name");
                if (!boxLayout(*candidate) || !property || property->text != name) continue;
                if (layout) fail(QStringLiteral("Ambiguous layout: %1").arg(QString::fromUtf8(name)));
                layout = candidate;
            }
            if (!layout) fail(QStringLiteral("Missing or unsupported layout: %1").arg(QString::fromUtf8(name)));
            layoutParents.insert(&member, layout);
        }
        for (auto &member : model.nodes) {
            if (!boxLayout(member)) continue;
            QSet<Node *> ancestors;
            for (auto *ancestor = &member; ancestor; ancestor = layoutParents.value(ancestor)) {
                if (ancestors.contains(ancestor)) fail(QStringLiteral("Cyclic layout reference"));
                ancestors.insert(ancestor);
            }
        }
    }
    for (auto &n : model.nodes) {
        if (result.gui && (n.name == "ChildPanel" || n.name == "ChildForm" || n.name == "ChildGroupBox" || n.name == "ChildScrollBox" ||
                          n.name == "ChildPanelEx" || n.name == "ChildFormEx" || n.name == "ChildPanelRush") && (!n.child || !n.child->host))
            fail(QStringLiteral("%1 is missing its inner control").arg(QString::fromUtf8(n.name)));
        static const QList<QByteArray> gui{"Message", "DoData", "Hub", "BoxDrawManager", "DS_StaticData", "DS_UserData", "MReadWrite", "MEnumerator", "MListBox", "CtrlPalette", "MenuEx", "PopupMenuEx", "MainForm", "Button", "BitBtn", "Edit", "Label", "CheckBox", "Panel", "ChildPanel", "GroupBox", "ChildForm", "WebBrowser", "Flash",
                                             "ChildGroupBox", "ChildPanelEx", "ChildFormEx", "RadioButton", "TrackBar", "ProgressBar", "ProgressBarRush", "ScrollBar", "UpDown", "ComboBox", "ListBox", "Memo", "HilightMemo", "LED", "FontBox", "ToolBar", "TreeView", "TreeViewEx", "StringTable", "StringTableMT",
                                             "LedNumber", "LedLadder", "StatusBar", "DatePicker", "Image", "AlphaImage", "ImgBtn", "GProgressBar", "Grapher", "Plotter", "PlotLines", "PlotPoints", "PlotHistogram", "VisualShape", "PaintBox", "DriveBox", "ColorBox", "Timer", "TabControl", "ChildScrollBox", "ScrollBox", "ScrollBarEx", "MultiElement", "MultiElementEx", "EditMulti", "EditMultiEx"};
        static const QList<QByteArray> console{"Entry", "DoData", "hcConsole", "MultiElement", "EditMulti", "MultiElementEx", "EditMultiEx", "Hub", "Data", "Write", "Print", "Math", "Memory", "If", "For", "Counter", "StrCat", "HubEx", "GetDataEx"};
        const bool controlClient = n.name == "RedrawManager" || n.name == "DropFileManager" ||
            n.name == "DropTextManager" || n.name == "FontManager" || n.name == "UseExtCursor" || n.name == "UseWinAnimation";
        const bool mstClient = n.name.startsWith("MST_") && QFileInfo::exists(QDir(packs).filePath(QStringLiteral("cpp/code/hi%1.hws").arg(QString::fromUtf8(n.name))));
        const bool managedWidget = mstClient || n.name == "ArrayEnum" || n.name == "ChildFormPoly" || n.name == "ChildPanelPoly" || n.name == "MTStrTbl" || n.name == "CommandCenter" || n.name == "CMD_State" || n.name == "CMD_PopupMenu" ||
            n.name == "CMD_ToolBar" || n.name == "TreeViewTrain" || n.name.startsWith("TVT_") || n.name == "PlotMarker" || n.name == "PlotStream" || n.name == "PlotSeriesMarker" || n.name == "PlotDiffSeries";
        if (!textArtifact(result.type) && !n.entry && !(result.gui ? gui : console).contains(n.name) && !(result.gui && (managedWidget || n.name == "PlotAxis" || n.name == "PlotMouseValues")) &&
            !(result.gui && (n.name == "ButtonRush" || n.name == "CheckBoxRush" || n.name == "RadioButtonRush" || n.name == "TrackBarRush" || n.name == "ChildPanelRush" || n.name == "PanelRush")) &&
            !(result.gui && (boxLayout(n) || n.name == "LayoutSpacer" || controlClient || n.name == "RichEdit" || n.name == "Splitter" || n.name == "Menu" || n.name == "PopupMenu" || n.name == "GLabel" || n.name == "StatusBarEx" || n.name == "TransparentManager" || n.name == "HintManager" || n.name == "UserHintManager" || n.name == "IndexManager" || n.name == "IconsManager" || n.name == "WorkIconsManager" || n.name == "PageControl" || n.name == "PagePanel" || n.name == "EditPolyMulti")))
            fail(QStringLiteral("Unsupported element for this project: %1").arg(QString::fromUtf8(n.name)));
        if (result.gui && managedWidget) {
            const auto resolve = [&](const char *key, const char *output, const QList<QByteArray> &types) {
                const auto *reference = named(n.props, key);
                auto name = reference ? reference->text : QByteArray();
                if (name.startsWith('"') && name.endsWith('"')) name = name.mid(1, name.size() - 2);
                QByteArray target;
                if (!name.isEmpty() && name != "(empty)") {
                    auto *scope = n.parent;
                    while (name.startsWith("parent.") && scope && scope->owner) {
                        scope = scope->owner->parent;
                        name.remove(0, 7);
                    }
                    for (auto *candidate : scope->elements) {
                        const auto *property = named(candidate->props, "Name");
                        if (!types.contains(candidate->name) || !property || property->text.compare(name, Qt::CaseInsensitive)) continue;
                        if (!target.isEmpty()) fail(QStringLiteral("Ambiguous %1: %2").arg(QString::fromLatin1(key), QString::fromUtf8(name)));
                        target = candidate->name + QByteArray::number(candidate->id);
                    }
                    if (target.isEmpty()) fail(QStringLiteral("Missing %1: %2").arg(QString::fromLatin1(key), QString::fromUtf8(name)));
                }
                n.props.push_back({output, data_str, {}, target, {}});
                n.props.back().decode();
            };
            if (n.name.startsWith("CMD_")) resolve("CommandCenter", "qtCommandCenter", {"CommandCenter"});
            if (n.name == "CMD_ToolBar") resolve("ToolBar", "qtCommandToolBar", {"ToolBar"});
            if (n.name.startsWith("TVT_")) resolve("TreeView", "qtTreeView", {"TreeViewTrain"});
            if (n.name == "TreeViewTrain") resolve("IconsManager", "qtIconsManager", {"IconsManager"});
            if (n.name == "TVT_DataSource") resolve("DataSource", "qtDataSource", {"DS_StaticData", "DS_UserData"});
            if (mstClient && n.name != "MST_DrawManager") resolve("MSTControl", "qtMstTarget", {"MTStrTbl"});
            if (n.name == "MTStrTbl") resolve("DrawManager", "qtMstDrawManager", {"MST_DrawManager"});
            if (n.name == "PlotMarker") {
                resolve("Grapher", "qtPlotterTarget", {"Plotter"});
                resolve("AxisX", "qtPlotAxisX", {"PlotAxis"});
                resolve("AxisY", "qtPlotAxisY", {"PlotAxis"});
            }
            if (n.name == "PlotStream" || n.name == "PlotSeriesMarker") resolve("Series", "qtPlotSeries", {"PlotLines", "PlotPoints", "PlotHistogram"});
            if (n.name == "PlotSeriesMarker") resolve("Grapher", "qtPlotterTarget", {"Plotter"});
            if (n.name == "PlotDiffSeries") {
                const QList<QByteArray> types{"PlotLines", "PlotPoints", "PlotHistogram"};
                resolve("Series1", "qtPlotFirst", types);
                resolve("Series2", "qtPlotSecond", types);
                resolve("ResultSeries", "qtPlotResult", types);
                const auto *mask = named(n.props, "Mask");
                const auto formula = mask ? QString::fromUtf8(mask->text) : QString();
                static const QRegularExpression token(QStringLiteral(
                    R"(\s*(%[12]|(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?|[()+*/-]))"));
                QByteArray expression;
                int depth = 0;
                bool operand = true;
                const auto source = formula.trimmed();
                for (qsizetype offset = 0; offset < source.size();) {
                    const auto match = token.match(source, offset, QRegularExpression::NormalMatch,
                        QRegularExpression::AnchorAtOffsetMatchOption);
                    if (!match.hasMatch()) fail(QStringLiteral("Unsupported PlotDiffSeries formula: %1").arg(formula));
                    offset = match.capturedEnd();
                    const auto part = match.captured(1);
                    if (operand && (part == "+" || part == "-" || part == "(")) {
                        if (part == "(") ++depth;
                        expression += part.toLatin1();
                    } else if (!operand && part == ")" && depth > 0) {
                        --depth;
                        expression += ')';
                    } else if (!operand && (part == "+" || part == "-" || part == "*" || part == "/")) {
                        expression += part.toLatin1();
                        operand = true;
                    } else if (operand && (part == "%1" || part == "%2")) {
                        expression += part == "%1" ? "first" : "second";
                        operand = false;
                    } else if (operand) {
                        bool valid = false;
                        const double value = part.toDouble(&valid);
                        if (!valid || !qIsFinite(value)) fail(QStringLiteral("Invalid PlotDiffSeries formula: %1").arg(formula));
                        expression += "double(" + QByteArray::number(value, 'g', 17) + ')';
                        operand = false;
                    } else fail(QStringLiteral("Invalid PlotDiffSeries formula: %1").arg(formula));
                    expression += ' ';
                }
                if (operand || depth) fail(QStringLiteral("Invalid PlotDiffSeries formula: %1").arg(formula));
                n.props.push_back({"qtPlotExpression", data_str, {}, expression, {}});
                n.props.back().decode();
            }
            if (n.name == "MTStrTbl") {
                const auto *style = named(n.props, "Style");
                if (!style || (style->number != 3 && style->number != 4))
                    fail(QStringLiteral("MTStrTbl supports the Report and NoHeader styles on Qt"));
            }
            if (n.name == "MST_RowAction") {
                const auto *action = named(n.props, "Action");
                if (action && action->number >= 4) fail(QStringLiteral("MST_RowAction: this action is not supported on Qt"));
            }
        }
        if (result.gui && boxLayout(n)) {
            QJsonArray children;
            for (auto *member : n.parent->elements) {
                if (layoutParents.value(member) != &n) continue;
                const auto number = [member](const char *key) {
                    const auto *property = named(member->props, key);
                    return property ? property->number : 0;
                };
                children.append(QJsonArray{QString::fromUtf8(member->name + QByteArray::number(member->id)),
                    boxLayout(*member), member->name == "LayoutSpacer", number("Left"), number("Top"),
                    number("Width"), number("Height"), number("WidthScale"), number("HeightScale")});
            }
            n.props.push_back({"qtLayoutItems", data_str, {}, QJsonDocument(children).toJson(QJsonDocument::Compact), {}});
            n.props.back().decode();
            n.props.push_back({"qtNestedLayout", data_int, {}, layoutParents.contains(&n) ? "1" : "0", {}});
            n.props.back().decode();
        }
        if (result.gui && (n.name == "PlotLines" || n.name == "PlotPoints" || n.name == "PlotHistogram" || n.name == "PlotAxis" || n.name == "PlotMouseValues")) {
            const auto *reference = named(n.props, "Grapher");
            auto name = reference ? reference->text : QByteArray();
            if (name.startsWith('"') && name.endsWith('"')) name = name.mid(1, name.size() - 2);
            QByteArray target;
            if (!name.isEmpty() && name != "(empty)") {
                for (auto *candidate : n.parent->elements) {
                    if (candidate->name != "Plotter") continue;
                    const auto *property = named(candidate->props, "Name");
                    if (!property || property->text != name) continue;
                    if (!target.isEmpty()) fail(QStringLiteral("Ambiguous Plotter: %1").arg(QString::fromUtf8(name)));
                    target = candidate->name + QByteArray::number(candidate->id);
                }
                if (target.isEmpty()) fail(QStringLiteral("Missing Plotter: %1").arg(QString::fromUtf8(name)));
            }
            n.props.push_back({"qtPlotterTarget", data_str, {}, target, {}});
            n.props.back().decode();
        }
        if (result.gui && (n.name == "PlotLines" || n.name == "PlotPoints" || n.name == "PlotHistogram")) {
            const auto *shape = named(n.props, "Shape");
            const int marker = n.name == "PlotPoints" ? qBound(0, shape ? shape->number : 0, 3) : -1;
            n.props.push_back({"qtPlotMarker", data_int, {}, QByteArray::number(marker), {}});
            n.props.back().decode();
            n.props.push_back({"qtPlotHistogram", data_int, {}, n.name == "PlotHistogram" ? "1" : "0", {}});
            n.props.back().decode();
            if (n.name == "PlotHistogram") {
                const auto *reference = named(n.props, "Axis");
                auto name = reference ? reference->text : QByteArray();
                if (name.startsWith('"') && name.endsWith('"')) name = name.mid(1, name.size() - 2);
                QByteArray target;
                if (!name.isEmpty() && name != "(empty)") {
                    for (auto *candidate : n.parent->elements) {
                        if (candidate->name != "PlotAxis") continue;
                        const auto *property = named(candidate->props, "Name");
                        if (!property || property->text != name) continue;
                        if (!target.isEmpty()) fail(QStringLiteral("Ambiguous PlotAxis: %1").arg(QString::fromUtf8(name)));
                        target = candidate->name + QByteArray::number(candidate->id);
                    }
                    if (target.isEmpty()) fail(QStringLiteral("Missing PlotAxis: %1").arg(QString::fromUtf8(name)));
                }
                n.props.push_back({"qtPlotBaseline", data_str, {}, target, {}});
                n.props.back().decode();
            }
        }
        if (result.gui && (n.name == "MReadWrite" || n.name == "MEnumerator" || n.name == "MListBox")) {
            const auto *reference = named(n.props, "DataSource");
            auto name = reference ? reference->text : QByteArray();
            if (name.startsWith('"') && name.endsWith('"')) name = name.mid(1, name.size() - 2);
            QByteArray target;
            if (!name.isEmpty() && name != "(empty)") {
                for (auto *candidate : n.parent->elements) {
                    if (candidate->name != "DS_StaticData" && candidate->name != "DS_UserData") continue;
                    const auto *property = named(candidate->props, "Name");
                    if (!property || property->text != name) continue;
                    if (n.name == "MReadWrite" && candidate->name == "DS_UserData")
                        fail(QStringLiteral("MReadWrite requires a data source with indexed access: %1").arg(QString::fromUtf8(name)));
                    if (!target.isEmpty()) fail(QStringLiteral("Ambiguous DataSource: %1").arg(QString::fromUtf8(name)));
                    target = candidate->name + QByteArray::number(candidate->id);
                }
                if (target.isEmpty()) fail(QStringLiteral("Missing or unsupported DataSource: %1").arg(QString::fromUtf8(name)));
            }
            n.props.push_back({"qtDataSource", data_str, {}, target, {}});
            n.props.back().decode();
        }
        if (result.gui && controlClient) {
            QByteArray targetName;
            const auto *reference = named(n.props, "ControlManager");
            auto name = reference ? reference->text : QByteArray();
            if (name.startsWith('"') && name.endsWith('"')) name = name.mid(1, name.size() - 2);
            if (!name.isEmpty() && name != "(empty)") {
                for (auto *candidate : n.parent->elements) {
                    const auto *property = named(candidate->props, "Name");
                    if (!candidate->controlManager || !property || property->text != name) continue;
                    if (!targetName.isEmpty()) fail(QStringLiteral("Ambiguous ControlManager: %1").arg(QString::fromUtf8(name)));
                    targetName = candidate->name + QByteArray::number(candidate->id);
                }
                if (targetName.isEmpty()) fail(QStringLiteral("Missing ControlManager: %1").arg(QString::fromUtf8(name)));
            }
            auto &target = n.props.emplace_back();
            target.name = "qtControlTarget";
            target.type = data_str;
            target.text = targetName;
        }
        const bool listView = n.name == "ComboBox" || n.name == "ListBox" || n.name == "MListBox" || n.name == "CtrlPalette";
        if (result.gui && (listView || n.name == "WorkIconsManager" || n.name == "PageControl")) {
            for (const QByteArray &kind : {QByteArray("IconsManager"), QByteArray("IndexManager"), QByteArray("BoxDrawManager")}) {
                if (!listView && kind != "IconsManager") continue;
                const auto *reference = named(n.props, kind);
                auto name = reference ? reference->text : QByteArray();
                if (name.startsWith('"') && name.endsWith('"')) name = name.mid(1, name.size() - 2);
                QByteArray target;
                if (!name.isEmpty() && name != "(empty)") {
                    for (auto *candidate : n.parent->elements) {
                        if (candidate->name != kind) continue;
                        const auto *property = named(candidate->props, "Name");
                        if (!property || property->text != name) continue;
                        if (!target.isEmpty()) fail(QStringLiteral("Ambiguous %1: %2").arg(QString::fromLatin1(kind), QString::fromUtf8(name)));
                        target = candidate->name + QByteArray::number(candidate->id);
                    }
                    if (target.isEmpty()) fail(QStringLiteral("Missing %1: %2").arg(QString::fromLatin1(kind), QString::fromUtf8(name)));
                }
                auto &property = n.props.emplace_back();
                property.name = QByteArray("qt") + kind;
                property.type = data_str;
                property.text = target;
            }
        }
        if (result.gui && n.name == "TransparentManager") {
            if (const auto *aero = named(n.props, "AeroMode"); aero && aero->number != 0)
                fail(QStringLiteral("TransparentManager: AeroMode is not supported by the Qt backend"));
            QByteArray targetName;
            const auto *name = named(n.props, "Name");
            if (name && !name->text.isEmpty()) {
                for (auto *candidate : n.parent->elements) {
                    if (candidate->name != "MainForm") continue;
                    const auto *reference = named(candidate->props, "TransparentManager");
                    if (!reference) continue;
                    auto value = reference->text;
                    if (value.startsWith('"') && value.endsWith('"')) value = value.mid(1, value.size() - 2);
                    if (value != name->text) continue;
                    if (!targetName.isEmpty()) fail(QStringLiteral("TransparentManager is assigned to multiple forms"));
                    targetName = candidate->name + QByteArray::number(candidate->id);
                }
            }
            auto &target = n.props.emplace_back();
            target.name = "qtTransparentTarget";
            target.type = data_str;
            target.text = targetName;
        }
        if (result.gui && (n.name == "HintManager" || n.name == "UserHintManager")) {
            QByteArray targets;
            const auto *name = named(n.props, "Name");
            if (name && !name->text.isEmpty()) {
                for (auto *candidate : n.parent->elements) {
                    if (!candidate->controlManager) continue;
                    const auto *reference = named(candidate->props, "HintManager");
                    if (!reference) continue;
                    auto value = reference->text;
                    if (value.startsWith('"') && value.endsWith('"')) value = value.mid(1, value.size() - 2);
                    if (value != name->text) continue;
                    if (!targets.isEmpty()) targets += ',';
                    targets += candidate->name + QByteArray::number(candidate->id);
                }
            }
            auto &property = n.props.emplace_back();
            property.name = "qtHintTargets";
            property.type = data_str;
            property.text = targets;
        }
    }
    // Existing MainForm schemes keep their serialized Backend index. Generate
    // the Qt implementation for both old and new files without rewriting them.
    if (result.gui) {
        auto *backend = named(entry->props, "Backend");
        if (!backend) fail(QStringLiteral("MainForm has no Backend property"));
        backend->number = 1;
    }
    for (int li = 0; li < diagram.layers.size(); ++li) {
      const auto &layer = diagram.layers.at(li);
      const auto &ids = layerIds.at(li);
      for (const auto &wire : layer.connections) {
        auto *sourceNode = ids.value(layer.elements.at(wire.source).id);
        auto *targetNode = ids.value(wire.targetId);
        if (!sourceNode || !targetNode || sourceNode->parent != targetNode->parent) fail(QStringLiteral("Missing link endpoint"));
        auto *from = named(sourceNode->ports, wire.sourcePoint.toUtf8());
        auto *to = named(targetNode->ports, wire.targetPoint.toUtf8());
        if (!from || !to || (result.gui && (!supportedQtPort(*sourceNode, from->name) || !supportedQtPort(*targetNode, to->name))))
            fail(QStringLiteral("Unsupported link: %1 -> %2").arg(wire.sourcePoint, wire.targetPoint));
        if (from->link || to->link || !((from->kind == pt_Event && to->kind == pt_Work) ||
                                        (from->kind == pt_Data && to->kind == pt_Var)))
            fail(QStringLiteral("Invalid connection: %1 -> %2").arg(wire.sourcePoint, wire.targetPoint));
        from->link = to;
        to->link = from;
      }
    }
    QLibrary rtcg(library);
    using Build = int (*)(TBuildProcessRec *);
    const auto build = reinterpret_cast<Build>(rtcg.resolve("buildProcessProc"));
    if (!build) fail(rtcg.errorString());
    auto api = callbacks();
    TBuildProcessRec request{&api, &sdks.front(), nullptr};
    active = &model;
    const int status = build(&request);
    active = nullptr;
    if (status || !request.result || !model.errors.isEmpty())
        fail(QStringLiteral("RTCG (%1): %2").arg(status).arg(model.errors.join(u'\n')));
    result.source = static_cast<const char *>(request.result);
    return result;
}
