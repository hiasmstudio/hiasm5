/*
 * Palette.h
 *
 *  Created on: 01.05.2010
 *      Author: dilma
 */

#ifndef PALETTE_H_
#define PALETTE_H_

#include "share.h"
#include "Pack.h"
#include "StringList.h"
#include "SvnClient.h"

class PaletteItem;
class Palette;

typedef std::list<PaletteItem*> PaletteItems;

class PaletteItem {
	protected:
		PaletteItem *selected;
		bool select;
		bool down;
		int minHeight;
		bool fullWidth;
	public:
		int x;
		int y;
		int width;
		int height;
		bool opened;
		bool hint;
		Palette *parent;
		PaletteItem *parentItem;
		ustring name;
		ustring caption;
		ustring title;

		PaletteItem(Palette *parent, const ustring &name, const ustring &title);
		virtual ~PaletteItem() { clear(); }
		PaletteItems items;

		Gdk::Rectangle area() { return Gdk::Rectangle(x, y, width, height); }

		virtual void mouseEnter();
		virtual void mouseDown(int x, int y, int button);
		virtual void mouseMove(int x, int y, int button);
		virtual void mouseUp(int x, int y, int button);
		virtual void mouseLeave();

		virtual void draw(DrawContext dc);

		virtual PaletteItem *add(PaletteItem *item);

		virtual void reSize();

		virtual void open();
		virtual void close();

		void clear();

		PaletteItem *getItemAtPos(int x, int y);

		void invalidate();

		virtual void save(HStringList &list);
		virtual void load(HStringList &list);
};

class PaletteItemTab : public PaletteItem {
	private:
		guint timerID;

		void start();
		void stop();
	public:
		PaletteItemTab(Palette *parent, const ustring &name, const ustring &title);
		~PaletteItemTab();

		bool on_timer();

		void mouseUp(int x, int y, int button);

		void draw(DrawContext dc);

		void save(HStringList &list) { if(opened) list << name; PaletteItem::save(list); }
		void load(HStringList &list) { if(list.indexOf(name) != -1) open(); PaletteItem::load(list); }
};

class PaletteItemGroup : public PaletteItem {
	private:

	public:
		PaletteItemGroup(Palette *parent, const ustring &name);
		~PaletteItemGroup();

		void mouseUp(int x, int y, int button);

		void draw(DrawContext dc);

		void save(HStringList &list) { if(opened) list << name; PaletteItem::save(list); }
		void load(HStringList &list) { if(list.indexOf(name) != -1) open(); PaletteItem::load(list);  }
};

class PaletteItemButton : public PaletteItem {
	private:
		Glib::RefPtr<Gdk::Pixbuf> icon;
	public:
		static const int BTN_SIZE = 30;

		PaletteItemButton(Palette *parent, const ustring &name, const ustring &title, Glib::RefPtr<Gdk::Pixbuf> icon);

		void draw(DrawContext dc);

		void mouseUp(int x, int y, int button);

		void open();
		void close();
};

class PaletteItemRoot : public PaletteItem {
	public:
		PaletteItemRoot(Palette *parent, const ustring &name);
		void reSize();
};

/*! \class PaletteHint
    \brief display info about element

    Displays some information about selected element of Palette
*/

class PaletteHint : public Window {
	private:
		VBox vb;
		Label caption;
		Label info;
	public:
		PaletteHint();

		void setText(const ustring &name, const ustring &info);
};

/*! \class Palette
    \brief displays elements of the package

    Provides tools for adding elements to the package currently open schema
*/

#ifdef HIASM_GTK4
class Palette : public HDrawingArea {
#else
class Palette : public DrawingArea {
#endif
	private:
		PaletteItemRoot *items;
		int downButton;
		Menu *palTabMenu;
		Menu *palElementMenu;
		ustring menuElement;
		SvnClient svn;
		bool commandAvailable(const ustring &command) const;
		int layout_width;
	protected:
		guint32 eventTime;			// need for popup menu...
		PaletteItemButton *selected;

		friend class PaletteItemButton;
		friend class PaletteItemTab;

		void select(PaletteItemButton *item);
		void closeAll();

		void on_tab_menu_popup(PaletteItemTab *tab);
		void on_element_menu_popup(PaletteItemButton *item);

		virtual bool on_draw (const ::Cairo::RefPtr< ::Cairo::Context >& cr);
		virtual bool on_expose_event(GdkEventExpose* event);
		virtual bool on_button_press_event(GdkEventButton* event);
		virtual bool on_button_release_event(GdkEventButton* event);
		virtual bool on_motion_notify_event(GdkEventMotion* event);
		virtual bool on_leave_notify_event(GdkEventCrossing* event);

#ifdef HIASM_GTK4
		void on_map() override;
		void size_allocate_vfunc(int width, int height, int baseline) override;
#else
		void on_size_allocate(Allocation &alloc);
#endif
	public:
		PaletteHint hint;
		Event on_element_select;	/**< do then user select element from palette */
		std::function<void(const ustring&)> on_open_file;
		Pack *pack;					/**< package of which shows the elements */
		PaletteItemTab *currentTab;	/**< contains pointer to active tab */
		int layoutWidth() const { return layout_width > 0 ? layout_width : get_width(); }

		Palette();
		virtual ~Palette();

		/**
		 * Load elements from pack
		 * @param pack pointer to pack, elements which want to display
		 */
		void loadPack(Pack *pack);
		/**
		 * Removes the selection from the selected item
		 */
		void unSelect();
		/**
		 * Remove all elements and groups from palette
		 */
		void clear();
		/**
		 * Redraw palette area
		 */
		void invalidate();

		/**
		 * Save information about opened tabs
		 */
		void save();
		/**
		 * Load information about opened tabs
		 */
		void load();

		/**
		 * Add new group to selected Tab
		 * @param name group name
		 */
		void addGroup(const ustring &name);
		bool processCommand(const ustring &command);
		void updateCommands();
};

#endif /* PALETTE_H_ */
