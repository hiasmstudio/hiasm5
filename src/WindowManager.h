/*
 * WindowManager.h
 *
 *  Created on: 11.05.2010
 *      Author: dilma
 */

#ifndef WINDOWMANAGER_H_
#define WINDOWMANAGER_H_

#include "share.h"
#include "SDK_Editor.h"
#include "CommandManager.h"
#include "ProjectManager.h"
#include "FormEditor.h"
#include "ProjectBuilder.h"

#ifndef G_OS_WIN32
	#define USE_GTK_SOURCE_VIEW
#endif

#ifdef USE_GTK_SOURCE_VIEW
#include <gtksourceviewmm.h>
using namespace Gsv;
#endif

typedef enum {
	WMT_SDK_EDITOR, 	/**< SDK_Editor tab */
	WMT_TEXT_EDITOR,	/**< Text_Editor tab */
	WMT_NEW_PROJECT,	/**< create new project tab */
} WM_TabTypes;

/*! \class WM_Tab
    \brief base class for tabs in WindowManager

    Provides an interface tabs for class WindowManager
*/

class WindowManager;
class WM_Tab {
	public:
		WM_TabTypes type; 		/**< type of current tab (see WM_TabTypes) */
		WindowManager *parent;	/**< parent window manager */
		ToolButton *closeBtn;	/**< close button in tab */
		Label *caption;			/**< caption of the tab */
		bool changed;			/**< true if user changed content of the tab */

		/**
		 * Create WM_Tab object with WindowManager as parent
		 * @param parent pointer to WindowManager object
		 */
		WM_Tab(WindowManager *parent) { this->parent = parent; changed = false; }
		virtual ~WM_Tab() { /* do nothing */ }
		/**
		 * Open file in this tab
		 * @param file_name name of the file you want to open
		 * @return pointer to widget who insert to tab
		 */
		virtual Widget *open(const ustring &file_name, bool asNew = false) = 0;
		/**
		 * Change current tab for this
		 */
		virtual void select() { /* do nothing */ }
		/**
		 * Close tab by user request
		 * @return if TRUE tab has been closed, else no
		 */
		virtual bool close();
		/**
		 * Provide access to tab widget
		 * @return primary widget for this tab
		 */
		virtual Widget *widget() = 0;
		/**
		 * Process receive command
		 * @param cmd command name
		 * @param arg command parameters
		 * @return true if command was processed else false
		 */
		virtual bool processCommand(const ustring &cmd, const ustring &arg) { return false; }
		/**
		 * Update commands state
		 */
		virtual void update() { /* do nothing */ }
		/**
		 * Name of opened file
		 */
		virtual ustring fileName() { return ustring(); }

		/**
		 * Set changed flag and update commands state
		 * @param value new state
		 */
		void setChanged(bool value);
		/**
		 * Provide icon for tab
		 * @return pointer to icon
		 */
		virtual TypePixbuf icon() { return cmdMan->findByName(ustring("new"))->icon; }
};

/*! \class SI_ImageMenuItem
    \brief contain menu items from icons

    Provide tools for select current SDK elements by class name
*/

class SI_ImageMenuItem : public ImageMenuItem {
	private:
		Element *element;
	public:
		SI_ImageMenuItem(Element *e, int count);
		virtual void on_activate();
};

/*! \class WM_SDK_Editor
    \brief contain SDK_Editor widget

    Allows you to open and edit schemes
*/

class WM_SDK_Editor : public WM_Tab, public CallBack {
	private:
		ProjectBuilder pbuilder;
		Glib::Thread *runThread;

		virtual bool on_sel_info_click(GdkEventButton *event);
		void fillSelectionMenu();
		void updateSelectionLabel();
	protected:
		FileChooserDialog *saveDlg;	/**< save file dialog */
		ScrolledWindow scroll;		/**< scroll widget for sde */
		FormEditor *formEdit;		/**< FormEditor widget */
		Label *sdkPath;				/**< SDK path panel */
		VBox *box;					/**< parent for all SDK_Editor widgets */
		Label *elementsList;		/**< info about count of selected elements */
		Menu selInfoMenu;			/**< list of selected elements */
		HScale zoomSlider;

		virtual void callback(void *owner, CallbackType type, const void *data);
		virtual void on_change_value();

		/**
		 * Save SDK to file
		 * @param file_name file in which you want to save data
		 */
		void saveToFile(const ustring &file_name);
		/**
		 * Save SDK with the choice of filename. Open save file dialog.
		 */
		void saveToFileAs();

		/**
		 * Create and form editor for current SDK
		 */
		void openFormEditor();
		/**
		 * Close and destroy opened form editor
		 */
		void closeFormEditor();

		/**
		 * Check running state of current project
		 * @return running state
		 */
		inline bool isRun() { return runThread != NULL; }
		/**
		 * Run current project
		 */
		void run();
		/**
		 * Call then project need to run in separate thread
		 */
		void on_run_thread();

		/**
		 * Call then user close form editor
		 * @param obj pointer to self
		 */
		static void _on_end_edit(WM_SDK_Editor *obj);
	public:
		SDK_Editor *sde;	/**< pointer to SDK_Editor widget */

		/**
		 * Constructor for WM_SDK_Editor
		 * @param parent pointer to WindowManager object
		 */
		WM_SDK_Editor(WindowManager *parent):WM_Tab(parent), zoomSlider(40, 1000, 10) { type = WMT_SDK_EDITOR; runThread = NULL; }
		~WM_SDK_Editor();
		/**
		 * Open *.sha file in this tab
		 * @param file_name name of the file you want to open
		 * @return pointer to widget who insert to tab
		 */
		Widget *open(const ustring &file_name, bool asNew = false);
		/**
		 * Provide access to tab widget
		 * @return pointer to SDK_Editor
		 */
		Widget *widget() { return box; }
		/**
		 * Process receive command
		 * @param cmd command name
		 * @param arg command parameters
		 * @return true if command was processed else false
		 */
		bool processCommand(const ustring &cmd, const ustring &arg);
		/**
		 * Update commands state
		 */
		void update();
		/**
		 * Name of opened sheme file
		 */
		ustring fileName() { return sde->sdk->fileName; }
		/**
		 * Provide icon for tab
		 * @return pointer to icon
		 */
		TypePixbuf icon() { return sde->sdk->pack->getSmallIcon(); }
};

/*! \class WM_Text_Editor
    \brief contain TextView widget

    Allows you to open and edit text files
*/

class WM_Text_Editor : public WM_Tab {
	protected:
		ScrolledWindow *scroll;	/**< scroll widget for text editor */
		ustring fName;			/**< name of opened file */

		virtual void on_edit_changed();
	public:
#ifdef USE_GTK_SOURCE_VIEW
		Gsv::View *memo;	/**< pointer to the widget being edited text */
#else
		TextView *memo;	/**< pointer to the widget being edited text */
#endif

		/**
		 * Constructor for WM_Text_Editor
		 * @param parent pointer to WindowManager object
		 */
		WM_Text_Editor(WindowManager *parent):WM_Tab(parent) { type = WMT_TEXT_EDITOR; }
		/**
		 * Open *.* file in this tab
		 * @param file_name name of the file you want to open
		 * @return pointer to widget who insert to tab
		 */
		Widget *open(const ustring &file_name, bool asNew = false);
		/**
		 * Provide access to tab widget
		 * @return pointer to ScrolledWindow
		 */
		Widget *widget() { return scroll; }
		/**
		 * Process receive command
		 * @param cmd command name
		 * @param arg command parameters
		 * @return true if command was processed else false
		 */
		bool processCommand(const ustring &cmd, const ustring &arg);
		/**
		 * Update commands state
		 */
		void update();
		/**
		 * Name of opened text file
		 */
		ustring fileName() { return fName; }
		/**
		 * Provide icon for tab
		 * @return pointer to icon
		 */
		TypePixbuf icon() { return CommandManager::loadIcon("text"); }
};

/*! \class WM_Text_Editor
    \brief contain TextView widget

    Allows you to open and edit text files
*/

class WM_New_Project : public WM_Tab, public CallBack {
	private:
		Glib::RefPtr<ListStore> fileStore;
		const struct MenuModelColumns : public TreeModelColumnRecord {
			Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > icon;
			Gtk::TreeModelColumn<ustring> name;
			Gtk::TreeModelColumn<ustring> size;
			Gtk::TreeModelColumn<ustring> date;

			MenuModelColumns() { add(icon); add(name); add(size); add(date); }
		} fileColumns;

		void fillFileList();
	protected:
		ScrolledWindow *scroll;	/**< scroll widget for text editor */
		TextView memo;
		ustring fName;			/**< name of opened file */
		VPaned paned;
		TreeView files;
		ScrolledWindow filesScroll;

		void callback(void *owner, CallbackType type, const void *data);
		virtual void on_row_activate(const TreeModel::Path &path, TreeViewColumn *column);
	public:

		/**
		 * Constructor for WM_Text_Editor
		 * @param parent pointer to WindowManager object
		 */
		WM_New_Project(WindowManager *parent):WM_Tab(parent) { type = WMT_NEW_PROJECT; }
		/**
		 * Default destructor
		 */
		virtual ~WM_New_Project();
		/**
		 * Open *.* file in this tab
		 * @param file_name name of the file you want to open
		 * @return pointer to widget who insert to tab
		 */
		Widget *open(const ustring &file_name, bool asNew = false);
		/**
		 * Close tab by user request
		 * @return if TRUE tab has been closed, else no
		 */
		bool close();
		/**
		 * Provide access to tab widget
		 * @return pointer to ScrolledWindow
		 */
		Widget *widget() { return &memo; }
		/**
		 * Process receive command
		 * @param cmd command name
		 * @param arg command parameters
		 * @return true if command was processed else false
		 */
		bool processCommand(const ustring &cmd, const ustring &arg) { return false; }
		/**
		 * Update commands state
		 */
		void update() {}
		/**
		 * Name of opened text file
		 */
		ustring fileName() { return "startup"; }
		/**
		 * Provide icon for tab
		 * @return pointer to icon
		 */
		TypePixbuf icon() { return CommandManager::loadIcon("help"); }
};

typedef std::vector<WM_Tab*> WM_Tabs;

/*! \class WindowManager
    \brief contain all open files

    Provides methods for editing and management of opened files in a hiasm
*/

class WindowManager : public Notebook, public CallBack {
	private:
		FileChooserDialog *openDlg;	/**< open file dialog */
		AboutDialog *aboutDlg;		/**< hiasm about dialog */

		Menu *popupMenus[SDK_OBJ_COUNT];		/**< popup menu for SDK objects */

		WM_Tab *oldTab;				/**< pointer to the previous open WM_Tab */

		ProjectManager prjMan;		/**< new projects manager */

		void switchToTab(WM_Tab *tab);
	protected:
		/**
		 * Call when user switch tab in Notebook
		 * @param page current selected page
		 * @param index page index
		 */
		void on_switch_page(Gtk::Widget *page, guint index);
		/**
		 * callback for receive commands
		 * @param owner pointer to CommandManager
		 * @param type CBT_RUN_COMMAND
		 * @param data command name
		 */
		virtual void callback(void *owner, CallbackType type, const void *data);
		/**
		 * Call when user click to close button
		 */
		virtual void on_close_click();
	public:
		WM_Tabs tabs;			/**< list of opened files */
		Event on_tab_open;		/**< call when successfully opened a new file (index CBT_TAB_OPEN) */
		Event on_tab_change;	/**< call when selected a new current tab (index CBT_TAB_CHANGE) */
		Event on_tab_close;		/**< call when tab has been closed (index CBT_TAB_CLOSE) */

		WindowManager();
		/**
		 * Open new file in hiasm
		 * @param file_name name of the file you want to open
		 * @return pointer to new WM_Tab
		 */
		WM_Tab *open(const ustring &file_name, bool asNew = false);
		/**
		 * Set current tab
		 * @param tab pointer to WM_Tab want to make the current
		 */
		void setCurrent(WM_Tab *tab);
		/**
		 * Get current tag
		 * @return pointer to current WM_Tab or NULL for empty manager
		 */
		WM_Tab *getCurrent();

		/**
		 * Update all command state
		 */
		void update();

		/**
		 * Notify the WindowManager about tab change
		 */
		void changeTab(WM_Tab *tab);

		/**
		 * Verify the existence of the project with the given name
		 * @param name project name
		 * @return true if project with this name is exists
		 */
		bool checkProjectName(const ustring &name);

		/**
		 * Show popup menu for SDK object in current mouse position. Create menu if NULL.
		 * @param type menu index
		 * @param time menu show delay
		 */
		void showMenu(SDKObjectType type, guint32 time);

		/**
		 * Run command by key shortcut
		 * @param key information about key pressed
		 */
		void translateKey(GdkEventKey *key);

		/**
		 * Process command
		 * @param cmd command name
		 * @param args command arguments
		 */
		void processCommand(const ustring &cmd, const ustring &args);

		/**
		 * Init window manager after load all classes
		 */
		void init();

		/**
		 * Save all opened tabs
		 */
		void saveTabs();
		/**
		 * Load saved tabs and open them
		 */
		void loadTabs();
};

#endif /* WINDOWMANAGER_H_ */
