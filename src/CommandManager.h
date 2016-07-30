/*
 * CommandManager.h
 *
 *  Created on: 07.05.2010
 *      Author: dilma
 */

#ifndef COMMANDMANAGER_H_
#define COMMANDMANAGER_H_

#include "share.h"

#define MENUNAME_ELEMENT	"el_menu"
#define MENUNAME_SDK		"sdk_menu"
#define MENUNAME_LINK		"line_menu"
#define MENUNAME_MAIN		"menu"
#define MENUNAME_TOOLBAR	"main"
#define MENUNAME_PAL_TAB	"pal_tab_menu"

#define CMD_TYPE const gchar *

typedef std::list<Widget*> CommandWidgets;

/*! \struct CommandItem
    \brief command descriptor for CommandManager

    The structure containing the description of the team environment
*/

typedef struct {
	ustring name;  				/**< command name (described in hiasm.db) */
	ustring info;				/**< command info (described in hiasm.db) */
	TypePixbuf icon;			/**< command icon (contain in %hiasm%/int/icons folder) */
	bool enabled;				/**< command is now available? */
	bool checked;				/**< command is now checked? */
	CommandWidgets widgets;		/**< list of widgets who call this command */
} CommandItem;

typedef std::list<CommandItem*> Commands;

/*! \class CommandManager
    \brief interface command manager

    Provides methods for the management teams of environment and interface elements that cause them
*/

class CommandManager {
	private:
		void loadCommandList();
		MenuShell *fillMenu(MenuShell *menu, const ustring &name);
		CommandItem *getComandByName(CMD_TYPE codeName);
	public:
		Commands cmds;			/**< array of environment commands */
		CommandWidgets widgets;	/**< array of all created widgets who contain interface commands */
		Event on_command;		/**< call when user run command */
		Event on_cmd_enabled;	/**< call when commands need to refresh her status (index CBT_CMD_ENABLED) */

		CommandManager();

		/**
		 * Load con file from int dir
		 * @param name file icon name wo extension
		 */
		static TypePixbuf loadIcon(const ustring &name);

		/**
		 * Create Toolbar widget by name
		 * @param name menu name from hiasm.db
		 */
		Toolbar *createToolbar(const ustring &name);
		/**
		 * Create MenuBar widget by name
		 * @param name menu name from hiasm.db
		 */
		inline MenuBar *createMenuBar(const ustring &name) { return (MenuBar*)fillMenu((MenuShell*)(new MenuBar()), name); }
		/**
		 * Create Menu widget by name
		 * @param name menu name from hiasm.db
		 */
		inline Menu *createMenu(const ustring &name) { return (Menu*)fillMenu((MenuShell*)(new Menu()), name); }
		/**
		 * Search description of the command on its name
		 * @param name command
		 * @return pointer to command description
		 */
		CommandItem *findByName(const ustring &name);

		/**
		 * Call custom command. Iterate event with ::CBT_RUN_COMMAND index from #CallbackType.
		 * @param name command name from hiasm.db
		 */
		void command(const ustring &name);

		/**
		 * Initiates the update status of commands. After this call used enable() and check()
		 */
		void beginUpdate();
		/**
		 * Determines the availability of command to perform
		 * @param cmd command descriptor
		 * @return true if command found
		 */
		bool enable(CMD_TYPE cmd);
		/**
		 * Turn on command
		 * @param cmd command descriptor
		 * @return true if command found
		 */
		bool check(CMD_TYPE cmd);
		/**
		 * Completes the update status of the commands and change the state of all widgets in the bars and menu control
		 */
		void endUpdate();

		/**
		 * Attach popup menu to command
		 * @param name command code name
		 * @param menu attached popup menu
		 */
		void attachMenu(CMD_TYPE name, Menu &menu);

		/**
		 * Check command enabled
		 * @param cmd command descriptor
		 * @return true if command enable, else false
		 */
		bool isEnabled(CMD_TYPE cmd);
};

/*! \class CM_ToggleToolButton
    \brief special button for CommandManager

    A special button to call the commands
*/

class CM_ToggleToolButton : public ToggleToolButton {
	private:
		ustring cmd;
		bool _change;
	public:
		CommandManager *parent;

		CM_ToggleToolButton(CommandManager *parent, const ustring &name, const ustring &info);
		virtual void on_clicked();
		void check(bool value);
};

/*! \class CM_ToolButton
    \brief special button for CommandManager

    A special button to call the commands
*/

class CM_ToolButton : public ToolButton {
	private:
		ustring cmd;
	public:
		CommandManager *parent;

		CM_ToolButton(CommandManager *parent, const ustring &name, const ustring &info);
		virtual void on_clicked();
};

/*! \class CM_MenuToolButton
    \brief special button for CommandManager

    A special button to call the commands
*/

class CM_MenuToolButton : public MenuToolButton {
	private:
		ustring cmd;
	public:
		CommandManager *parent;

		CM_MenuToolButton(CommandManager *parent, const ustring &name, const ustring &info);
		virtual void on_clicked();
};

/*! \class CM_ImageMenuItem
    \brief special MenuItem for CommandManager

    A special MenuItem to call the commands
*/

class CM_ImageMenuItem : public ImageMenuItem {
	private:
		ustring cmd;
	public:
		CommandManager *parent;

		CM_ImageMenuItem(CommandManager *parent, const ustring &name, const ustring &info);
		virtual void on_activate();
};

extern CommandManager *cmdMan;

#endif /* COMMANDMANAGER_H_ */
