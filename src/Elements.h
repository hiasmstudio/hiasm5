/*
 * Elements.h
 *
 *  Created on: 07.05.2010
 *      Author: dilma
 */

#ifndef ELEMENTS_H_
#define ELEMENTS_H_

#include "share.h"
#include "Element.h"
#include "ElementGrips.h"

#define MAX_POINTS 100

#define CI_HubEx 20
#define CI_GetDataEx 21
#define CI_Transmitter 22

extern Element *createElement(const ustring &name, SDK *sdk, gdouble x, gdouble y);

class ElementWinElement : virtual public Element {
	public:
		ElementWinElement(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		void place(int x, int y);
};

class ElementDynPoints : virtual public Element {
	protected:
		void on_change_property(ElementProperty *prop);
	public:
		struct DynInfo {
			ElementProperty *prop;
			ustring name;
		} dynInfoList[4];

		ElementDynPoints(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void init();

		void connectToPoint(ElementPoint *point);
};

class ElementNamedDynPoints : virtual public Element {
	protected:
		void on_change_property(ElementProperty *prop);
	public:
		struct DynInfo {
			ElementProperty *prop;
		} dynInfoList[4];

		ElementNamedDynPoints(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void init();
};

class ElementMulti : virtual public Element {
	protected:
		TypePixbuf icon;

		void on_change_property(ElementProperty *prop);
	public:
		Element *child;

		ElementMulti(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		~ElementMulti();

		void drawIcon(DrawContext dc);

		void saveToText(ustring &text, const ustring offset, int flags);
		void createELink(Element *owner);
};

class ElementEditMultiBase : virtual public Element, public ElementGrip {
	private:
		int *fwidth;
		int *fheight;
		int *voff;
		int *hoff;
		int state;
		gdouble oldx;
		gdouble oldy;

		GripInfo *vgrip;
		GripInfo *hgrip;
	protected:
		virtual void changePoints(Element *multi, int i) = 0;
		virtual int isDynPointProp(ElementProperty *prop) = 0;
		void on_change_property(ElementProperty *prop);
		void reSize();

		int getStateAtPos(gdouble x, gdouble y);

		//--------------------- ElementGrip interface -------------------------
		void on_grip_change(GripInfo *grip);
		void on_grip_draw(DrawContext dc, GripInfo *grip);
		bool on_grip_can_change(GripInfo *grip, int &dx, int &dy);
	public:
		ElementEditMultiBase(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void drawIcon(DrawContext dc) {  }
		void drawBody(DrawContext dc, double zoom);

		bool checkCollise(gdouble x, gdouble y);
		bool checkColliseRect(gdouble x1, gdouble y1, gdouble x2, gdouble y2);

		void rePosPoints();

		void init();

		bool mouseDown(gdouble x, gdouble y, int button);
		void mouseMove(gdouble x, gdouble y);
		void mouseUp(gdouble x, gdouble y, int button);
		int mouseCursor(gdouble x, gdouble y);
};

class ElementEditMulti : public ElementDynPoints, public ElementEditMultiBase {
	protected:
		void changePoints(Element *multi, int i);
		int isDynPointProp(ElementProperty *prop);
		void on_change_property(ElementProperty *prop) {
			ElementDynPoints::on_change_property(prop);
			ElementEditMultiBase::on_change_property(prop);
		}
	public:
		ElementEditMulti(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void init() { ElementEditMultiBase::init(); }
};

class ElementEditMultiEx : public ElementNamedDynPoints, public ElementEditMultiBase {
	protected:
		void changePoints(Element *multi, int i);
		int isDynPointProp(ElementProperty *prop);
		void on_change_property(ElementProperty *prop) {
			ElementNamedDynPoints::on_change_property(prop);
			ElementEditMultiBase::on_change_property(prop);
		}
	public:
		ElementEditMultiEx(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void init() { ElementEditMultiBase::init(); }
};

class ElementDynGeometry : public Element, public ElementGrip {
	protected:
		gdouble dx;
		gdouble dy;
		Gdk::Rectangle r;
		int *fwidth;
		int *fheight;
		Glib::RefPtr<Pango::Layout> textLayout;

		void rePosGrids();
		//--------------------- ElementGrip interface -------------------------
		void on_grip_change(GripInfo *grip);
		void on_grip_draw(DrawContext dc, GripInfo *grip);
		bool on_grip_can_change(GripInfo *grip, int &dx, int &dy);
	public:
		ElementDynGeometry(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void drawBody(DrawContext dc, double zoom);

		bool checkCollise(gdouble x, gdouble y);

		void init();

		bool mouseDown(gdouble x, gdouble y, int button);
		void mouseMove(gdouble x, gdouble y);
		void mouseUp(gdouble x, gdouble y, int button);
		int mouseCursor(gdouble x, gdouble y);
};

class ElementCaption : public Element {
	protected:
		void on_change_property(ElementProperty *prop);
	public:
		ElementCaption(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
};

class ElementInfoTip : public ElementDynGeometry {
	protected:
		ustring *text;
		int *frame;
		int *trans;
		int *margin;
		int *formatText;
		int *halign;

		void on_change_property(ElementProperty *prop);
	public:
		ElementInfoTip(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void storePropPointers();

		void drawIcon(DrawContext dc) {  }
		void drawBody(DrawContext dc, double zoom);

		bool checkCollise(gdouble x, gdouble y);
		bool checkColliseRect(gdouble x1, gdouble y1, gdouble x2, gdouble y2);

		int mouseCursor(gdouble x, gdouble y);
};

class ElementVisualText : public ElementInfoTip {
	protected:
		//--------------------- ElementGrip interface -------------------------
		void on_grip_change(GripInfo *grip);
	public:
		ElementVisualText(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		~ElementVisualText();
};

class ElementDebug : public Element {
	protected:
		void on_change_property(ElementProperty *prop);
	public:
		ElementDebug(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void draw(DrawContext dc, double zoom);

		bool getObjectAtPos(gdouble x, gdouble y, ObjectType *obj);

		void remove();
};


//--------------------------------------------------------------------------------------------------

class ElementCore : virtual public Element {
	protected:
		ustring readString(TData *data, ElementPoint *point, ElementProperty *prop);
		int readInteger(TData *data, ElementPoint *point, ElementProperty *prop);
		double readReal(TData *data, ElementPoint *point, ElementProperty *prop);
		TData readData(TData *data, ElementPoint *point, ElementProperty *prop);
	public:
		ElementCore(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		virtual void prepareForRun(Element **parent) {}
		virtual void stopRun() {}

		virtual void do_work(ElementPoint *point, TData *data) {}
		virtual void read_var(ElementPoint *point, TData *data) {}
};

class ElementCoreMessage : public ElementCore {
	protected:
		MessageDialog *msg;
	public:
		ElementCoreMessage(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		void prepareForRun(Element **parent);

		void do_work(ElementPoint *point, TData *data);
};

class ElementCoreDoData : public ElementCore {
	public:
		ElementCoreDoData(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
};

class ElementCoreHub : public ElementDynPoints, public ElementCore {
	public:
		ElementCoreHub(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
		void drawIcon(DrawContext dc);

		void remove() { removeFromLine(true, false); }
};

class ElementCoreMulti : public ElementMulti, public ElementCore {
	public:
		ElementCoreMulti(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Element **parent) { Element *e = *parent; sdk->prepareForRun(&e); }
		void stopRun() { sdk->stopRun(); }

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreEditMulti : public ElementEditMulti, public ElementCore {
	public:
		ElementCoreEditMulti(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreStrCat : public ElementCore {
	private:
		ustring value;
	public:
		ElementCoreStrCat(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Widget **parent) { value.clear(); }

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreStrPos : public ElementCore {
	public:
		ElementCoreStrPos(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
};

class ElementCoreFormatStr : public ElementDynPoints, public ElementCore {
	private:
		ustring _res;
	public:
		ElementCoreFormatStr(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Widget **parent) {_res.clear();}

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreReplace : public ElementCore {
	private:
		ustring _res;
	public:
		ElementCoreReplace(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Widget **parent) {_res.clear();}

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreCopy : public ElementCore {
	private:
		ustring _res;
	public:
		ElementCoreCopy(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Widget **parent) {_res.clear();}

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreStrPart : public ElementCore {
	private:
		ustring	_left,_right;
	public:
		ElementCoreStrPart(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		inline void prepareForRun(Widget **parent){_left.clear(); _right.clear();}

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreBlockFind : public ElementCore {
	private:
		bool _stop;
		ustring _rs,_sb,_eb;
	public:
		ElementCoreBlockFind(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		inline void prepareForRun(Widget **parent);

		void do_work(ElementPoint *point, TData *data);
};

class ElementCoreFor : public ElementCore {
	private:
		int i;
		int e;
	public:
		ElementCoreFor(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Widget **parent) { i = 0; }

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreIfElse : public ElementCore {
	public:
		ElementCoreIfElse(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
};

class ElementCoreRandom : public ElementCore {
	private:
		int value;
	public:
		ElementCoreRandom(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreMath : public ElementCore {
	private:
		double value;
		int type;
	public:
		ElementCoreMath(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Element **parent);
		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreMemory : public ElementCore {
	private:
		TData value;
	public:
		ElementCoreMemory(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Element **parent);
		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreTimer : public ElementCore {
	private:
		int autoStop;
		guint timerID;

		void start();
		void stop();
	public:
		ElementCoreTimer(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Element **parent);
		void stopRun();

		void do_work(ElementPoint *point, TData *data);

		bool on_timer();
};

class ElementCoreIndexToChanel : public ElementDynPoints, public ElementCore {
	private:

	public:
		ElementCoreIndexToChanel(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
};

class ElementCoreChanelToIndex : public ElementDynPoints, public ElementCore {
	private:
		TData remdt;
	public:
		ElementCoreChanelToIndex(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreExecute : public ElementCore {
	public:
		ElementCoreExecute(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
};

class ElementCoreConsole : public ElementDynPoints, public ElementCore {
	public:
		ElementCoreConsole(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
};

class ElementCoreClipboard : public ElementCore {
	public:
		ElementCoreClipboard(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreFileSearch : public ElementCore {
	private:
		int count;
		bool stop;

		void enumDir(const ustring &path, bool fn, int i);
	public:
		ElementCoreFileSearch(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreTimeCounter : public ElementCore {
	private:
		GTimer *timer;
	public:
		ElementCoreTimeCounter(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
};

class ElementHubEx : public ElementCore {
	protected:
		int *angle;
	public:
		ElementHubEx(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);

		virtual int calcSide(ElementPoint *p);

		void rePosPoints();
		void on_change_property(ElementProperty *prop);
		void init() { rePosPoints(); }
		ElementPoint *getRealPoint(ElementPoint *point) { return points[3]->point; }

		bool getObjectAtPos(gdouble x, gdouble y, ObjectType *obj);

		void draw(DrawContext dc, double zoom);

		void insertInLine(ElementPoint *point, PointPos *pos);
		void connectToPoint(ElementPoint *point);

		void remove();
};

class ElementGetDataEx : public ElementHubEx {
	public:
		ElementGetDataEx(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		void rePosPoints();
		ElementPoint *getRealPoint(ElementPoint *point) { return points[3]->point; }
		void insertInLine(ElementPoint *point, PointPos *pos);
		int calcSide(ElementPoint *p);
		void draw(DrawContext dc, double zoom);
		void read_var(ElementPoint *point, TData *data);
};


class ElementCoreEventFromData : public ElementCore {
	private:
		TData _dt;
	public:
		ElementCoreEventFromData(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreOpenDialog : public ElementCore {
	private:
		FileChooserDialog *dlg;
	public:
		ElementCoreOpenDialog(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		void do_work(ElementPoint *point, TData *data);

		void prepareForRun(Element **parent);
		void stopRun();
};

class ElementCoreSaveDialog : public ElementCore {
	private:
	public:
		ElementCoreSaveDialog(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		void do_work(ElementPoint *point, TData *data);
};

class ElementCoreTransmitter : public ElementDynPoints, public ElementCore {
	private:
		TData ret;
	public:
		ElementCoreTransmitter(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		void do_work(ElementPoint *point, TData *data);

		TData &run(const TData &value);
};

#include "StringList.h"

class ElementCoreStrList : public ElementCore, public DataArrayInterface {
	private:
		StringList list;
	public:
		ElementCoreStrList(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Element **parent);
		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);

		int _arr_count() { return list.size(); }
		void _arr_set(TData &index, TData &data) { list[index.toInt()] = data.toStr(); }
		TData _arr_get(TData &index) { return list[index.toInt()]; }
		void _arr_add(TData &data) { list << data; }
};

class ElementCoreArrayEnum : public ElementCore {
	private:
		int i;
		TData item;
	public:
		ElementCoreArrayEnum(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreHTTPText : public ElementCore {
	private:
		GTimer *timer;
	public:
		ElementCoreHTTPText(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
};

class ElementCoreDS_SQLite : public ElementCore {
	private:
		sqlite3 *db;
		void close();
	public:
		ElementCoreDS_SQLite(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		void stopRun();

		void do_work(ElementPoint *point, TData *data);
		void exec(const ustring &sql);
		sqlite3_stmt *query(const ustring &sql);
		const char *error() { return sqlite3_errmsg(db); }
		long lastId() { return sqlite3_last_insert_rowid(db); }
};

class ElementCoreDSC_Exec : public ElementCore {
	private:
		ElementCoreDS_SQLite *sqlite;
		long last_row;
	public:
		ElementCoreDSC_Exec(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Element **parent);
		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreDSC_QueryScalar : public ElementCore {
	private:
		ElementCoreDS_SQLite *sqlite;
		TData result;
	public:
		ElementCoreDSC_QueryScalar(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Element **parent);
		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCoreDSC_Query : public ElementCore {
	private:
		ElementCoreDS_SQLite *sqlite;
	public:
		ElementCoreDSC_Query(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Element **parent);
		void do_work(ElementPoint *point, TData *data);
};

class ElementCoreConvertor : public ElementCore {
	private:
		TData result;
	public:
		ElementCoreConvertor(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementCorePopup : public ElementCore {
	private:
		Menu *popup;
	public:
		ElementCorePopup(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void prepareForRun(Element **parent);
		void stopRun();

		void do_work(ElementPoint *point, TData *data);
};

class ElementCoreDelete : public ElementCore {
	private:

	public:
		ElementCoreDelete(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
};

class ElementCoreInsert : public ElementCore {
	public:
		ElementCoreInsert(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void do_work(ElementPoint *point, TData *data);
};

//--------------------------------------------------------------------------------------------------
class ElementTplParent;
class ElementTplWinControl : public ElementCore {
	public:
		Widget *ctl;

		ElementTplWinControl(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		void prepareForRun(Element **parent);
		void stopRun();
		void do_work(ElementPoint *point, TData *data);

		virtual Widget *createWidget(Element **parent) = 0;
};

class ElementTplParent : public ElementTplWinControl, public CallBack {
	protected:
		virtual void addElement(Element *ie);
		virtual void callback(void *owner, CallbackType type, const void *data);
	public:
		ElementTplParent(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		~ElementTplParent();
		virtual void run() {}
		virtual void insertElement(ElementTplWinControl *e) = 0;
};

class ElementTplForm : public ElementTplParent {
	private:
		Window *form;
		EventBox *eb;
	protected:
		virtual bool on_delete_event(GdkEventAny *event);
		virtual bool on_button_press(GdkEventButton* event);
	public:
		ElementTplForm(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void stopRun();

		void run();
		void do_work(ElementPoint *point, TData *data);
		void insertElement(ElementTplWinControl *e);

		inline Window *getForm() { return form; }

		Widget *createWidget(Element **parent);
};

class ElementTplDialog : public ElementTplParent {
	private:
		Dialog *form;

		void addElement(Element *ie);
	public:
		ElementTplDialog(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void stopRun();

		void run();
		void do_work(ElementPoint *point, TData *data);
		void insertElement(ElementTplWinControl *e);

		inline Dialog *getForm() { return form; }

		Widget *createWidget(Element **parent);
};

class ElementTplGroupBox : public ElementTplParent {
	private:
		Frame *frame;
	public:
		ElementTplGroupBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void insertElement(ElementTplWinControl *e) { frame->add(*e->ctl); }

		void do_work(ElementPoint *point, TData *data);

		Widget *createWidget(Element **parent);
};

class ElementTplFixed : public ElementTplParent {
	private:

	public:
		ElementTplFixed(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void insertElement(ElementTplWinControl *e);

		Widget *createWidget(Element **parent);
};

class ElementTplVBox : public ElementTplParent {
	private:
		void addElement(Element *ie);
	public:
		ElementTplVBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void insertElement(ElementTplWinControl *e);

		Widget *createWidget(Element **parent);
};

class ElementTplExpander : public ElementTplParent {
	private:
	public:
		ElementTplExpander(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void insertElement(ElementTplWinControl *e);

		Widget *createWidget(Element **parent);
};

class ElementTplButtonBox : public ElementTplParent {
	private:
		void addElement(Element *ie);
	public:
		ElementTplButtonBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void insertElement(ElementTplWinControl *e);

		Widget *createWidget(Element **parent);
};

class ElementTplHandleBox : public ElementTplParent {
	private:
	public:
		ElementTplHandleBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void insertElement(ElementTplWinControl *e);

		Widget *createWidget(Element **parent);
};

class ElementTplPaned : public ElementTplParent {
	private:
		void addElement(Element *ie);
	public:
		ElementTplPaned(PackElement *pe, SDK *sdk, gdouble x, gdouble y);

		void insertElement(ElementTplWinControl *e);

		Widget *createWidget(Element **parent);
};

class ElementTplButton : public ElementTplWinControl {
	private:
		virtual void on_button_clicked();
	public:
		ElementTplButton(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		Widget *createWidget(Element **parent);
};

class ElementTplEdit : public ElementTplWinControl {
	protected:
		virtual void on_edit_changed();
		virtual bool on_button_press(GdkEventKey *key);
	public:
		ElementTplEdit(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		Widget *createWidget(Element **parent);
		void read_var(ElementPoint *point, TData *data);
		void do_work(ElementPoint *point, TData *data);
};

class ElementTplMemo : public ElementTplWinControl {
	private:
		TextView *t;
	protected:
		virtual void on_edit_changed();
	public:
		ElementTplMemo(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		Widget *createWidget(Element **parent);
		void stopRun();
		void read_var(ElementPoint *point, TData *data);
		void do_work(ElementPoint *point, TData *data);
};

class ElementTplLabel : public ElementTplWinControl {
	public:
		ElementTplLabel(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		Widget *createWidget(Element **parent);
		void do_work(ElementPoint *point, TData *data);
};

class ElementTplCheckBox : public ElementTplWinControl {
	public:
		ElementTplCheckBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		Widget *createWidget(Element **parent);
};

class ElementTplScrollBar : public ElementTplWinControl {
	private:
		Glib::RefPtr<Adjustment> adj;
	protected:
		virtual void on_change_value();
	public:
		ElementTplScrollBar(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		Widget *createWidget(Element **parent);
		void stopRun();
		void read_var(ElementPoint *point, TData *data);
		void do_work(ElementPoint *point, TData *data);
};

class ElementTplListBox : public ElementTplWinControl, public DataArrayInterface {
	private:
		const struct ModelColumns : public TreeModelColumnRecord {
			Gtk::TreeModelColumn<ustring> text;

			ModelColumns() { add(text); }
		} listColumns;

		Glib::RefPtr<ListStore> list;

		TreeView *t;

		virtual void on_show_ep_row_activated();

		void setText(const ustring &text);
	public:
		ElementTplListBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		Widget *createWidget(Element **parent);
		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);

		int _arr_count() { return list->children().size(); }
		void _arr_set(TData &index, TData &data) { list->children()[index.toInt()][listColumns.text] = data.toStr(); }
		TData _arr_get(TData &index) { return ustring(list->children()[index.toInt()][listColumns.text]); }
		void _arr_add(TData &data) { (*list->append())[listColumns.text] = data; }
};

class ElementTplImage : public ElementTplWinControl {
	private:
		TypePixbuf img;
		Image *image;
	public:
		ElementTplImage(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		Widget *createWidget(Element **parent);
		void stopRun();
		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

class ElementTplFileChooserButton : public ElementTplWinControl {
	private:
		virtual void on_file_set();
	public:
		ElementTplFileChooserButton(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		Widget *createWidget(Element **parent);
		void do_work(ElementPoint *point, TData *data);
};

class ElementTplColorButton : public ElementTplWinControl {
	private:
		virtual void on_color_set();
	public:
		ElementTplColorButton(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		Widget *createWidget(Element **parent);
		void do_work(ElementPoint *point, TData *data);
};

class ElementTplProgressBar : public ElementTplWinControl {
	private:
	public:
		ElementTplProgressBar(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		Widget *createWidget(Element **parent);
		void do_work(ElementPoint *point, TData *data);
};

class ElementTplComboBox : public ElementTplWinControl {
	private:
		virtual void on_changed();
	public:
		ElementTplComboBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		Widget *createWidget(Element **parent);
		void do_work(ElementPoint *point, TData *data);
		void read_var(ElementPoint *point, TData *data);
};

#endif /* ELEMENTS_H_ */
