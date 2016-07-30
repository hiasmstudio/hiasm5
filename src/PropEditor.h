/*
 * PropEditor.h
 *
 *  Created on: 01.05.2010
 *      Author: dilma
 */

#ifndef PROPEDITOR_H_
#define PROPEDITOR_H_

#include "share.h"
#include "Data.h"
#include "ElementProperty.h"

#define PROP_FLG_EDIT	0x01
#define PROP_FLG_BTN	0x02
#define PROP_FLG_NDEF	0x04
#define PROP_FLG_MAIN	0x08
#define PROP_FLG_CHECK	0x10

class PropEditor;
class PropertyItem;

typedef std::list<PropertyItem*> Properties;

class PropertyItem {
	protected:
		/**
		 * Draw select frame
		 */
		virtual void drawSelect(DrawContext dc, Glib::RefPtr<StyleContext> style, int x, int y);
		/**
		 * Draw property value
		 */
		virtual void drawValue(DrawContext dc, Glib::RefPtr<StyleContext> style, int x, int y);
	public:
		ustring name;		/**< internal unique name of property */
		ustring caption;	/**< text display in widget grid */
		ustring info;		/**< property description */
		int flag;			/**< some property flags see PROP_FLG_XXX */
		DataType type;		/**< type of property (i.e data_xxx) */
		void *value;		/**< property value */
		PropEditor *parent;	/**< pointer to parent PropEditor */
		void *data;			/**< pointer to user data */
		bool checked;		/**< check visible state(draw only PROP_FLG_CHECK is set) */

		bool opened;		/**< contain true if items show in editor */
		Properties items;	/**< child items */

		PropertyItem *group;	/**< pointer to parent group */

		PropertyItem(const ustring &name);
		virtual ~PropertyItem();
		/**
		 * Return text who display in text editor
		 */
		virtual ustring editText() { return ustring(); }
		/**
		 * Parse value from edit who user typed
		 */
		virtual void setValue(const ustring &value) {}
		/**
		 * Draw property
		 */
		virtual void draw(DrawContext dc, Glib::RefPtr<StyleContext> style, int x, int y);

		/**
		 * Add children property
		 * @return adding property
		 */
		PropertyItem *add(PropertyItem *item);
		/**
		 * Open property if is group
		 */
		void open();

		/**
		 * Set parent property editor for this item and all children
		 * @param editor parent property editor
		 */
		void setParent(PropEditor *editor);
		/**
		 * Level of property
		 * @return level
		 */
		int getLevel();
};

class PropertyGroup : public PropertyItem {
	public:
		PropertyGroup(const ustring &name);

		void draw(DrawContext dc, Glib::RefPtr<StyleContext> style, int x, int y);
};

class PropertyTitle : public PropertyItem {
	private:
		TypePixbuf icon;
	public:
		PropertyTitle(const ustring &caption, const ustring &info);

		void draw(DrawContext dc, Glib::RefPtr<StyleContext> style, int x, int y);
		inline void setIcon(TypePixbuf icon) { this->icon = icon; }
};

class PropertyInteger : public PropertyItem {
	public:
		PropertyInteger(const ustring &name, void *value);
		ustring editText() {
			char buf[16];
			sprintf(buf, "%d", *(int*)value);
			return ustring(buf);
		}
		void setValue(const ustring &value) { *(int*)this->value = atoi(value.c_str()); }
};

class PropertyString : public PropertyItem {
	public:
		PropertyString(const ustring &name, void *value);
		ustring editText() { return *(ustring*)value; }
		void setValue(const ustring &value) { *(ustring*)this->value = value; }
};

class PropertyDouble : public PropertyItem {
	public:
		PropertyDouble(const ustring &name, void *value);
		ustring editText() { return double_to_str(*(double*)value); }
		void setValue(const ustring &value) { *(double*)this->value = atof(value.c_str()); }
};

class PropertyEnum : public PropertyItem {
	public:
		std::vector<ustring> list;

		PropertyEnum(const ustring &name, void *value, const ustring &list);
		ustring editText() { return list[*(int*)value]; }
		void setValue(const ustring &value);
		virtual int getSelectIndex() { return *(int*)value; }
};

class PropertyEnumText : public PropertyEnum {
	public:
		PropertyEnumText(const ustring &name, void *value, const ustring &list);
		ustring editText() { return *(ustring*)value; }
		void setValue(const ustring &value);
		int getSelectIndex();
};

class PropertyTData : public PropertyItem {
	public:
		PropertyTData(const ustring &name, void *value);
		ustring editText() {
			if(((TData*)value)->type == data_str)
				return ustring("#") + ((TData*)value)->toStr();
			return ((TData*)value)->toStr();
		}
		void setValue(const ustring &value);
};

extern const gchar *emptyName;

class PropertyStock : public PropertyItem {
	public:
		PropertyStock(const ustring &name, void *value);
		ustring editText() { return *(ustring*)value == "" ? emptyName : *(ustring*)value; }
		void setValue(const ustring &value) { *(ustring*)this->value = value == emptyName ? "" : value; }
		void drawValue(DrawContext dc, Glib::RefPtr<StyleContext> style, int x, int y);
};

class PropertyPixbuf : public PropertyItem {
	public:
		PropertyPixbuf(const ustring &name, void *value);
		ustring editText() { return "[IMAGE]"; }
};

class PropertyColor : public PropertyItem {
	public:
		PropertyColor(const ustring &name, void *value);
		ustring editText() { return ((TypeColor*)value)->to_string(); }
		void setValue(const ustring &value) { ((TypeColor*)this->value)->set(value); }
		void drawValue(DrawContext dc, Glib::RefPtr<StyleContext> style, int x, int y);
};

class PropertyArray : public PropertyItem {
	public:
		PropertyArray(const ustring &name, void *value);
		ustring editText() { return ((ArrayValue*)value)->empty() ? emptyName : int_to_str(((ArrayValue*)value)->size()); }
};

class PropertyFont : public PropertyGroup {
	public:
		PropertyFont(const ustring &name, void *value);
		ustring editText() { return "[FONT]"; }
};

class PropertyFlagsItem;

class PropertyFlags : public PropertyGroup {
	private:
		friend class PropertyFlagsItem;

		void _change(PropertyFlagsItem *prop);
	public:
		PropertyFlags(const ustring &name, void *value, const ustring &list);
		ustring editText();
};

class PropertyFlagsItem : public PropertyEnum {
	private:
		PropertyFlags *parent;
		int flag_value;
	public:
		int shift;

		PropertyFlagsItem(const ustring &name, PropertyFlags *parent, int shift):PropertyEnum(name, &flag_value, "False,True") {
			this->parent = parent;
			this->shift = shift;
		}
		void setValue(const ustring &value) { PropertyEnum::setValue(value); parent->_change(this); }
		inline int bit() { return flag_value; }
};

class PropEditor : public Fixed {
	private:
		friend class PropertyItem;

		Entry *edit;
		int select_y;
		int downButton;
		TreeView *tree;
		EventBox *comboPanel;
		TreeView *stock;
		EventBox *stockPanel;
		Button *objBtn;

		Glib::RefPtr<ListStore> comboList;
		const struct ModelColumns : public TreeModelColumnRecord {
			Gtk::TreeModelColumn<ustring> param;

		    ModelColumns() { add(param); }
		} comboColumns;

		Glib::RefPtr<ListStore> stockList;
		const struct StockModelColumns : public TreeModelColumnRecord {
			Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > icon;
			Gtk::TreeModelColumn<ustring> param;

			StockModelColumns() { add(icon); add(param); }
		} stockColumns;

		void rePosChild();
		void getPropertyAtPos(double y);

		void fillStockPanel();

		void drawProperties(DrawContext dc, Properties &items, int x, int &y);
		int itemsHeight(Properties &items);
	protected:
		virtual void on_btn_click();
		virtual void on_edit_changed();
		virtual void on_icon_press(EntryIconPosition icon, const GdkEventButton *button);
		virtual void on_row_activated();
		virtual void on_stock_row_activated();
	public:
		Properties props;			/**< list of properties */
		PropertyItem *select;		/**< currently selected property */
		Event on_change_property;	/**< call when user change property value */
		Event on_select_property;	/**< call when user select property in editor */
		Event on_check_property;	/**< call when user click to property checkbox */
		Event on_edit_property;		/**< call when user click to property change button */

		PropEditor();
		~PropEditor();

		virtual bool on_draw (const ::Cairo::RefPtr< ::Cairo::Context >& cr);
		virtual bool on_button_press_event(GdkEventButton* event);
		virtual bool on_button_release_event(GdkEventButton* event);
		virtual bool on_motion_notify_event(GdkEventMotion* event);

		/**
		 * Add existing property to editor
		 * @param p - property to add
		 */
		void addProperty(PropertyItem *p);
		/**
		 * Invalidate editor
		 */
		void reDraw();
		/**
		 * Remove all propertys and clear editor
		 */
		void clear();
		/**
		 * Call when parent size changed
		 */
		void changeSize(int width);
		/**
		 * Get the full path to the property
		 * @return path of current property
		 */
		ustring getPath();
		/**
		 * Select property by full path
		 * @param path full path for the property
		 */
		void selectPropertyByPath(const ustring &path);

		/**
		 * Update editor and current selected property
		 */
		void update();
};

#endif /* PROPEDITOR_H_ */
