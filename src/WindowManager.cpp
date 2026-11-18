/*
 * WindowManager.cpp
 *
 *  Created on: 11.05.2010
 *      Author: dilma
 */

#include <locale>
#include <cmath>
#include "WindowManager.h"
#include "LegacyPackCatalog.h"
#include "CodeGen.h"
#include "MainDataBase.h"
#include "commands.h"
#include "SHADialogs.h"
#include "SettingsManager.h"
#include "RecentFileManager.h"
#include "main_gui.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <memory>
#include <glib/gstdio.h>

namespace {
bool parse_pair(const ustring &text, gdouble &x, gdouble &y) {
	std::string value(text);
	std::replace(value.begin(), value.end(), ';', ' ');
	std::replace(value.begin(), value.end(), ',', ' ');
	std::stringstream stream(value);
	stream.imbue(std::locale::classic());
	if(!(stream >> x >> y)) return false;
	stream >> std::ws;
	return stream.eof() && std::isfinite(x) && std::isfinite(y) &&
		x >= G_MININT && x <= G_MAXINT && y >= G_MININT && y <= G_MAXINT &&
		std::trunc(x) == x && std::trunc(y) == y;
}

// A compiled package (cpp) edits its form with the built-in GTK4 designer
// when the scheme has a form element: no external formedit program needed.
bool hasInternalForm(MSDK *sdk) {
    if(!sdk || !sdk->pack || sdk->pack->name != "cpp") return false;
    for(Element *element : sdk->elements)
        if(element->isParent() && element->isCore() && element->tpl->conf->editClassName == "Form") return true;
    return false;
}

// Form editor mode "Separate window" (IDS_FORM_EDITOR = 2): the form is shown in its own window
// with its real frame, as HiAsm 4 showed it. Used for C++ schemes too, not only for legacy packs.
bool separateFormWindow(MSDK *sdk) {
    return settingsMan && settingsMan->readInt(IDS_FORM_EDITOR) == 2 && FE_Legacy::findForm(sdk) != nullptr;
}

bool hasFormEditor(MSDK *sdk) {
    if(!sdk || !sdk->pack) return false;
    // HiAsm 4 had the designer inside HiAsm.exe: edit schemes with a form here.
    if(sdk->pack->isLegacy()) return FE_Legacy::findForm(sdk) != nullptr;
    if(separateFormWindow(sdk)) return true;
    if(sdk->pack->name == "_base") return !sdk->isCoreDocument();
    if(hasInternalForm(sdk)) return true;
#ifdef G_OS_WIN32
    const ustring executable = sdk->pack->pathMake() + "formedit.exe";
#else
    const ustring executable = sdk->pack->pathMake() + "formedit";
#endif
    return g_file_test(executable.c_str(), G_FILE_TEST_IS_EXECUTABLE);
}

bool remove_directory_tree(const ustring &path) {
    if(path.empty() || !g_file_test(path.c_str(), G_FILE_TEST_EXISTS)) return true;
    GDir *dir = g_dir_open(path.c_str(), 0, nullptr);
    if(!dir) return g_remove(path.c_str()) == 0;
    while(const gchar *name = g_dir_read_name(dir)) {
        const ustring child = Glib::build_filename(path, name);
        if(g_file_test(child.c_str(), G_FILE_TEST_IS_DIR))
            remove_directory_tree(child);
        else
            g_remove(child.c_str());
    }
    g_dir_close(dir);
    return g_rmdir(path.c_str()) == 0;
}

}

SI_ImageMenuItem::SI_ImageMenuItem(Element *e, int count):
	ImageMenuItem(e->tpl->name + " (" + int_to_str(count) + ")")
{
	element = e;
	// TODO remove?
	Image *img = new Image(e->tpl->image);
	set_image(*img);
	show();
}

void SI_ImageMenuItem::on_activate() {
	SelectManager *sm = element->parent->selMan;
	std::list<Element *> list;
	for(ElementsList::iterator e = sm->elements.begin(); e != sm->elements.end(); e++)
		if((*e)->tpl == element->tpl)
			list.push_back(*e);
	sm->clear();

	for(std::list<Element *>::iterator e = list.begin(); e != list.end(); e++)
		sm->add(*e);
}

//------------------------------------------------------------------------------

void WM_Tab::setChanged(bool value) {
	if(changed != value) {
		changed = value;
		parent->changeTab(this);
	}
}

bool WM_Tab::close() {
	if(!changed)
		return true;

	MessageDialog msg(ustring("File ") + fileName() + " has not been saved! Save project before close?", false, MESSAGE_QUESTION, BUTTONS_YES_NO);
	msg.set_title(ui_tr("Warning"));
	msg.add_button(Stock::CANCEL, 0);

	int i = dialog_run(msg);
	if(i == -8) {
		processCommand(CMD_SAVE, "");
		return !changed;
	}

	// Dialog dismissal (DELETE_EVENT), Escape and unknown responses cancel.
	return i == GTK_RESPONSE_NO;
}

//------------------------------------------------------------------------------

class MEvent : public EventBox {
	private:
		gdouble sx, sy;
		gdouble cx, cy;
		guint timerID;
		guint button;

		static gboolean lo_timer_proc(void *data) {
			((MEvent*)data)->move();
			return false;
		}
		int get_hscroll(ScrolledWindow *sc) {
			return (int)scrolled_get_hscroll(*sc);
		}
		void set_hscroll(ScrolledWindow *sc, int value) {
			scrolled_set_hscroll(*sc, value);
		}
		int get_vscroll(ScrolledWindow *sc) {
			return (int)scrolled_get_vscroll(*sc);
		}
		void set_vscroll(ScrolledWindow *sc, int value) {
			scrolled_set_vscroll(*sc, value);
		}
	public:
		double dx, dy;

		MEvent(): sx(0), sy(0), cx(0), cy(0), timerID(0), button(0), dx(0), dy(0) {}
		~MEvent() override {
			if(timerID) {
				g_source_remove(timerID);
				timerID = 0;
			}
		}

		virtual bool on_button_press_event(GdkEventButton* event) {
			//DEBUG_MSG("BUTTON - DOWN")
			button = event->button;
			if(button == BTN_MIDDLE) {
				ScrolledWindow *sc = (ScrolledWindow *)widget_get_child(*this);
				if(!sc) {
					button = 0;
					return false;
				}
				dx = get_hscroll(sc);
				dy = get_vscroll(sc);
				sx = event->x - dx;
				sy = event->y - dy;
			}
			return true;
		}
		virtual bool on_button_release_event(GdkEventButton* event) {
			//DEBUG_MSG("BUTTON - UP")
			if(event->button == button)
				button = 0;
			return true;
		}
		virtual bool on_motion_notify_event(GdkEventMotion* event) {
			//DEBUG_MSG("BUTTON - MOVE")
			if(button == BTN_MIDDLE) {
				ScrolledWindow *sc = (ScrolledWindow *)widget_get_child(*this);
				if(!sc)
					return false;
				cx = event->x - get_hscroll(sc);
				cy = event->y - get_vscroll(sc);
				//printf("cx = %f\n", event->x);
				if(timerID) return true;

				timerID = g_timeout_add(2, lo_timer_proc, this);
				//ScrolledWindow *sc = (ScrolledWindow *)widget_get_child(*this);
				//sc->get_vscrollbar()->set_value(dy + cy - sc->get_vscrollbar()->get_value() - sy);
				//sc->get_hscrollbar()->set_value(dx + cx - sc->get_hscrollbar()->get_value() - sx);
			}
			return true;
		}
		void move() {
			//printf("%f : %f\n",dx, cx - sx);
			ScrolledWindow *sc = (ScrolledWindow *)widget_get_child(*this);
			if(button == BTN_MIDDLE && sc) {
				set_hscroll(sc, dx + sx - cx);
				set_vscroll(sc, dy + sy - cy);
			}
			g_source_remove(timerID);
			timerID = 0;
		}
};

Widget *WM_SDK_Editor::open(const ustring &file_name, bool asNew) {
	TRACE_PROC
	sde = new SDK_Editor();
    ustring source_file = file_name;
	if(asNew) {
        // GLib's Windows implementation expects a basename template here;
        // it places the directory in the user/system temporary directory.
        gchar *tmp_dir = g_dir_make_tmp("hiasm-new-XXXXXX", nullptr);
        if(tmp_dir) {
            temporaryProjectDir = tmp_dir;
            g_free(tmp_dir);
            const ustring target = Glib::build_filename(temporaryProjectDir, "project.sha");
            GFile *src = g_file_new_for_path(file_name.c_str());
            GFile *dst = g_file_new_for_path(target.c_str());
            GError *error = nullptr;
            if(g_file_copy(src, dst, G_FILE_COPY_NONE, nullptr, nullptr, nullptr, &error))
                source_file = target;
            if(error) g_error_free(error);
            g_object_unref(src); g_object_unref(dst);
        }
        temporaryProject = !temporaryProjectDir.empty() && source_file != file_name;
    }
    if(!sde->load(source_file)) {
        cleanupTemporaryProject();
        source_file = file_name;
        if(!sde->load(source_file)) {
            // A scheme that does not load is not opened: the caller drops the tab.
            cgt_on_debug.run(("!Cannot open scheme: " + file_name).c_str());
            return nullptr;
        }
    }
	if(asNew) {
		ustring fn;
		int i = 1;
		ustring mask = SettingsManager::readStr(IDS_MASK_OF_NAME), m;
		int u = mask.find("%name%");
		if(u != -1) {
			mask.replace(u, 6, g_get_user_name());
		}
		int p = mask.find("%num%");
		if(p != -1)
			while(file_test(fn = (m = mask).replace(p, 5, int_to_str(i)), FILE_TEST_EXISTS) || parent->checkProjectName(Glib::filename_display_basename(fn)))
				i++;
        // Keep the real temporary path so builds resolve relative files there.
        if(!temporaryProject) sde->sdk->fileName = fn;
        else sde->sdk->fileName = source_file;
        changed = true;
	}

	sde->csdk->selMan->on_selection_change += this;

	sde->on_popup_menu += this;
	sde->on_key_down += this;
	sde->sdk->on_run_state += this;
	sde->on_change_sdk += this;
	sde->on_change_scheme += this;
	sde->on_enter_request += this;
    sde->on_end_operation += this;
    sde->on_begin_operation += this;

	sdkPath = new HBox();
	updateSdkPath(sde->csdk);

	box = new VBox();
	box->add(*sde);

	scroll.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scrolled_set_child(scroll, *box);
	MEvent *ebs = new MEvent();
	ebs->add(scroll);

	box = new VBox();
	box->add(*ebs);

	elementsList = new Label(ui_tr("(none)"));
	elementsList->set_size_request(150, 0);
	EventBox *eb = new EventBox();
	eb->add(*elementsList);
	eb->signal_button_release_event().connect(sigc::mem_fun(*this, &WM_SDK_Editor::on_sel_info_click));

	zoomSlider.set_draw_value(true);
	zoomSlider.set_value_pos(POS_LEFT);
	zoomSlider.set_size_request(130, 16);
	zoomSlider.set_value(100);
	zoomSlider.signal_value_changed().connect(sigc::mem_fun(*this, &WM_SDK_Editor::on_change_value));

	HBox *hb = new HBox();
	hb->pack_start(*sdkPath, true, true);
	hb->pack_end(zoomSlider, false, false);
	hb->pack_end(*eb, false, false);

	box->pack_end(*hb, false, false);

	saveDlg = NULL;
	formEdit = NULL;
    recordSchemeState();
    // A scheme that did not load has no pack, so nothing was recorded.
    if(!history.empty()) {
        savedScheme = history.back().text;
        savedFrozen = history.back().frozen;
    }

	return box;
}

WM_SDK_Editor::~WM_SDK_Editor() {
    historyIdle.disconnect();
	pathIdle.disconnect();
	pbuilder.on_run_state -= this;
	if(saveDlg) delete saveDlg;
	closeFormEditor();

	for(Label *item : sdkPathItems) {
		sdkPath->remove(*item);
		delete item;
	}
	delete sdkPath;
	delete sde;
    cleanupTemporaryProject();

	// TODO scroll and 2 x box
}

void WM_SDK_Editor::cleanupTemporaryProject() {
    if(temporaryProject && !temporaryProjectDir.empty())
        remove_directory_tree(temporaryProjectDir);
    temporaryProjectDir.clear();
    temporaryProject = false;
}

bool WM_SDK_Editor::processCommand(const ustring &cmd, const ustring &arg) {
    if((cmd == CMD_UNDO || cmd == CMD_REDO) && sde->operationActive()) sde->endOperation();
    recordSchemeState();
    if(cmd == CMD_UNDO || cmd == CMD_REDO) {
        if(!isRun()) {
            if(cmd == CMD_UNDO && historyPosition) restoreSchemeState(historyPosition - 1);
            if(cmd == CMD_REDO && historyPosition + 1 < history.size()) restoreSchemeState(historyPosition + 1);
        }
        return true;
    }
    const bool handled = processSchemeCommand(cmd, arg);
    const SchemeState previous = history[historyPosition];
    recordSchemeState();
    if(previous.text != history[historyPosition].text || previous.frozen != history[historyPosition].frozen)
        setChanged(history[historyPosition].text != savedScheme || history[historyPosition].frozen != savedFrozen);
    return handled;
}

void WM_SDK_Editor::recordSchemeState() {
    if(restoringScheme || !sde || !sde->sdk->pack) return;
    SchemeState state;
    state.text = "Make(" + sde->sdk->pack->name + ")" + LINE_END;
    sde->sdk->saveToText(state.text);
    std::function<void(SDK*)> collectFrozen = [&](SDK *sdk) {
        for(Element *element : sdk->elements) {
            if(element->flag & ELEMENT_FLG_IS_FREEZE) state.frozen.push_back(element->id);
            if(element->sdk && !element->isELink()) collectFrozen(element->sdk);
        }
    };
    collectFrozen(sde->sdk);
    for(SDK *sdk = sde->csdk; sdk->parent; sdk = sdk->parent->parent) state.path.push_back(sdk->parent->id);
    std::reverse(state.path.begin(), state.path.end());
    for(Element *element : sde->csdk->selMan->elements) state.selected.push_back(element->id);
    if(!history.empty() && history[historyPosition].text == state.text && history[historyPosition].frozen == state.frozen) {
        history[historyPosition] = std::move(state);
        return;
    }
    if(!history.empty()) history.erase(history.begin() + historyPosition + 1, history.end());
    history.push_back(std::move(state));
    size_t bytes = 0;
    for(const SchemeState &item : history) bytes += item.text.bytes();
    while(history.size() > 1 && (history.size() > 100 || bytes > 32*1024*1024)) {
        bytes -= history.front().text.bytes(); history.erase(history.begin());
    }
    historyPosition = history.size() - 1;
}

bool WM_SDK_Editor::restoreSchemeState(size_t position) {
    if(position >= history.size() || position == historyPosition) return false;
    // Validate in a separate model before replacing objects visible to the UI.
    MSDK *replacement = new MSDK();
    gchar *text = g_strdup(history[position].text.c_str()), *cursor = text;
    int line = 1;
    const SDKParseError error = replacement->loadFromText(cursor, line, ELEMENT_LOAD_FILE);
    g_free(text);
    if(error || !replacement->pack) { delete replacement; return false; }
    replacement->fileName = sde->sdk->fileName;
    replacement->info = sde->sdk->info;
    for(int id : history[position].frozen)
        if(Element *element = replacement->getGlobalElementById(id)) element->flag |= ELEMENT_FLG_IS_FREEZE;
    for(Element *element : replacement->elements) {
        if((replacement->packProject = replacement->pack->getProjectByEntry(element->tpl->name))) {
            element->flag |= ELEMENT_FLG_IS_PARENT; break;
        }
    }
    const bool editForm = formEdit != nullptr;
    if(editForm) closeFormEditor();
    restoringScheme = true; historyIdle.disconnect();
    sde->sdk->on_run_state -= this;
    sde->replaceScheme(replacement, history[position].path, history[position].selected);
    sde->sdk->on_run_state += this;
    historyPosition = position; restoringScheme = false;
    if(editForm) openFormEditor();
    setChanged(history[position].text != savedScheme || history[position].frozen != savedFrozen);
    parent->update();
    return true;
}

bool WM_SDK_Editor::processSchemeCommand(const ustring &cmd, const ustring &arg) {
    if((cmd == CMD_BUILD || cmd == CMD_RUN) && sde->sdk->isCoreDocument()) return true;
	if(cmd == CMD_RUN) {
		if(pbuilder.build(sde->sdk))
			run();
	}
	else if(cmd == CMD_STOP) {
		pbuilder.stop();
	}
	else if(cmd == CMD_ZOOMIN) {
		sde->setZoom(sde->getZoom() + 0.2);
	}
	else if(cmd == CMD_ZOOMOUT) {
		sde->setZoom(sde->getZoom() - 0.2);
	}
	else if(cmd == CMD_DELETE) {
		sde->csdk->selMan->erase();
	}
	else if(cmd == CMD_SHA_INI) {
		// TODO optimize
		for(ElementsList::iterator e = sde->csdk->selMan->elements.begin(); e != sde->csdk->selMan->elements.end(); e++) {
			parent->open(sde->sdk->pack->pathElementConf((*e)->tpl->name));
		}
	}
	else if(cmd == CMD_SHA_PAS) {
		for(ElementsList::iterator e = sde->csdk->selMan->elements.begin(); e != sde->csdk->selMan->elements.end(); e++) {
			ustring f = sde->sdk->pack->pathElementCode((*e)->tpl->name);
			if(file_test(f, FILE_TEST_EXISTS))
				parent->open(f);
		}
	}
	else if(cmd == CMD_SAVE) {
        if(temporaryProject || sde->sdk->isInstalledCoreScheme())
            processCommand(CMD_SAVEAS, arg);
		else if(Glib::file_test(sde->sdk->fileName, Glib::FILE_TEST_EXISTS)) {
			if(saveToFile(sde->sdk->fileName))
				setChanged(false);
		}
		else
			processCommand(CMD_SAVEAS, arg);
	}
	else if(cmd == CMD_SAVEAS)
		saveToFileAs();
	else if(cmd == CMD_FREEZ) {
		if(sde->csdk->selMan->checkFlag(ELEMENT_FLG_IS_FREEZE))
			sde->csdk->selMan->unsetFlag(ELEMENT_FLG_IS_FREEZE);
		else
			sde->csdk->selMan->setFlag(ELEMENT_FLG_IS_FREEZE);
		sde->reDraw();
	}
	else if(cmd == CMD_ERASE)
		sde->csdk->selMan->clearLinks();
	else if(cmd == CMD_SENDTOBACK)
		sde->csdk->moveToDown(sde->csdk->selMan->getFirst());
	else if(cmd == CMD_BRINGTOFRONT)
		sde->csdk->moveToTop(sde->csdk->selMan->getFirst());
	else if(cmd == CMD_SELECTALL)
		sde->csdk->selMan->selectAll();
	else if(cmd == CMD_PASTE_DEBUG)
		sde->pasteElementOnLine("Debug");
	else if(cmd == CMD_COPY)
		sde->csdk->selMan->copy();
	else if(cmd == CMD_CUT) {
		sde->csdk->selMan->copy();
		sde->csdk->selMan->erase();
	}
	else if(cmd == CMD_PASTE)
		sde->csdk->selMan->paste();
	else if(cmd == CMD_FORMEDIT) {
		if(formEdit)
			closeFormEditor();
		else
			openFormEditor();
	}
	else if(cmd == CMD_COPY_LINK)
		sde->csdk->selMan->copyAsLink();
	else if(cmd == CMD_FORWARD)
		enterScheme(sde->csdk->selMan->getFirst());
	else if(cmd == CMD_BACK)
		leaveSchemeTo(sde->csdk->parent->parent);
	else if(cmd == CMD_BUILD)
		pbuilder.build(sde->sdk);
	else if(cmd == CMD_EDITOR) {
		TData dt;
		if(sde->csdk->selMan->getFirst())
			dt = sde->csdk->selMan->getFirst()->sysProps.getByName("Hint")->readStr();
		ElementCoreTransmitter *t = shaDlg.run("text_editor");
		if(!t)
			return true;
		ustring s = t->run(dt).toStr();
		SelectManager *sm = sde->csdk->selMan;
		for(ElementsList::iterator e = sm->elements.begin(); e != sm->elements.end(); e++)
			(*e)->sysProps.getByName("Hint")->setValue(&s);
	}
	else if(cmd == CMD_LINECOLOR) {
		ColorSelectionDialog cd("Change element color");
		cd.get_color_selection()->set_has_palette(true);
		TypeColor  c;
		if(sde->csdk->selMan->getFirst()) {
			c = *(TypeColor*)sde->csdk->selMan->getFirst()->sysProps.getByName("Color")->value;
			cd.get_color_selection()->set_previous_rgba(c);
			cd.get_color_selection()->set_current_rgba(c);
		}
		if(dialog_run(cd)) {
			SelectManager *sm = sde->csdk->selMan;
			c = cd.get_color_selection()->get_current_rgba();
			for(ElementsList::iterator e = sm->elements.begin(); e != sm->elements.end(); e++)
				(*e)->sysProps.getByName("Color")->setValue(&c);
		}
	}
	else if(cmd == CMD_MAKEHINT)
		sde->beginOperation(MS_ADD_LINK_HINT);
	else if(cmd == CMD_REMOVE_LH)
		sde->beginOperation(MS_REMOVE_LINK_HINT);
	else if(cmd == CMD_SHOW_LH) {
		sde->endOperation();
		sde->setLinkHintsVisible(!sde->sdk->showLinkHints);
	}
	else if(cmd == CMD_BIND_RECT || cmd == CMD_BIND_CENTER || cmd == CMD_BIND_PADDING) {
		if(!WidgetEditorEx::getActive(sde)) return true;
		const auto mode = cmd == CMD_BIND_RECT ? SettingsManager::BindEdges :
			cmd == CMD_BIND_CENTER ? SettingsManager::BindCenter : SettingsManager::BindPadding;
		SettingsManager::setFormBinding(mode, !SettingsManager::formBinding(mode));
		parent->update();
	}
	else if(cmd == CMD_SLIDEDOWN)
		sde->beginOperation(MS_SELECT_BY_EDGE,(void*)(int)(SelectManager::SBE_EDGE_VERTICAL));
	else if(cmd == CMD_SLIDERIGHT)
		sde->beginOperation(MS_SELECT_BY_EDGE,(void*)(int)(SelectManager::SBE_EDGE_HORIZONTAL));
	else if(cmd == CMD_SLIDEDIAG)
		sde->beginOperation(MS_SELECT_BY_EDGE,(void*)(int)(SelectManager::SBE_EDGE_BOTH));
	else if(cmd == CMD_GROUP || cmd == CMD_UNGROUP) {
		SelectManager *selection = sde->csdk->selMan;
		if(cmd == CMD_GROUP ? selection->group() : selection->ungroup()) sde->changeScheme();
	}
	else if(cmd == CMD_REPLACETO) {
		Element *element = sde->csdk->selMan->getFirst();
		if(!element || element->isNoDelete() || element->sdk || element->isELink() || isRun()) return true;
		ustring target = arg;
		if(target.empty()) {
			Dialog dialog; dialog.set_title(ui_tr("Replace element"));
			Label label(ui_tr("Replacement template name (properties and connected points must be compatible):"));
			Entry name; name.set_text(element->tpl->name);
			box_pack_start(*dialog.get_content_area(), label, false, false, 8);
			box_pack_start(*dialog.get_content_area(), name, false, false, 8);
			dialog.add_button(Stock::CANCEL, 0); dialog.add_button(Stock::OK, 1);
			compat_show_all(dialog);
			if(dialog_run(dialog) != 1) return true;
			target = name.get_text();
		}
		// Validate in an isolated model. Swapping also clears pointer-backed hover
		// layers and property panels before their old element is destroyed.
		const int id = element->id;
		const auto state = history[historyPosition];
		std::unique_ptr<MSDK> replacement(new MSDK());
		gchar *text = g_strdup(state.text.c_str()), *cursor = text;
		int line = 1;
		const auto parsed = replacement->loadFromText(cursor, line, ELEMENT_LOAD_FILE);
		g_free(text);
		if(parsed || !replacement->pack) return true;
		SDK *scope = replacement.get();
		for(int parentId : state.path) {
			Element *container = scope->getElementById(parentId);
			if(!container || !container->sdk) return true;
			scope = container->sdk;
		}
		ustring error;
		if(!scope->replaceElement(id, target, error)) {
			if(!error.empty()) cgt_on_debug.run(("!" + error).c_str());
			return true;
		}
		replacement->fileName = sde->sdk->fileName;
		replacement->info = sde->sdk->info;
		for(int frozen : state.frozen)
			if(Element *item = replacement->getGlobalElementById(frozen)) item->flag |= ELEMENT_FLG_IS_FREEZE;
		for(Element *item : replacement->elements)
			if((replacement->packProject = replacement->pack->getProjectByEntry(item->tpl->name))) { item->flag |= ELEMENT_FLG_IS_PARENT; break; }
		const bool editForm = formEdit != nullptr;
		if(editForm) closeFormEditor();
		restoringScheme = true; historyIdle.disconnect();
		sde->sdk->on_run_state -= this;
		sde->replaceScheme(replacement.release(), state.path, state.selected);
		sde->sdk->on_run_state += this;
		restoringScheme = false;
		if(editForm) openFormEditor();
		sde->changeScheme();
	}
	else if(cmd == CMD_MOVETO) {
		// Move the current selection so its top-left corner reaches x,y.
		gdouble x = 0, y = 0;
		if(!parse_pair(arg, x, y) || sde->csdk->selMan->empty())
			return true;
		if(sde->csdk->selMan->moveTo(static_cast<int>(x), static_cast<int>(y))) {
			setChanged(true);
			parent->update();
		}
	}
	else if(cmd == CMD_ALIGN) {
		SelectManager *sm = sde->csdk->selMan;
		if(sm->elements.empty())
			return true;
		std::string mode(arg);
		std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		if(sm->align(mode)) {
			setChanged(true);
			parent->update();
		}
	}
	else return false;

	return true;
}

void WM_SDK_Editor::update() {
    if(WidgetEditorEx::getActive(sde)) {
        const char *bindings[] = {CMD_BIND_RECT, CMD_BIND_CENTER, CMD_BIND_PADDING};
        for(int i = 0; i < 3; ++i) {
            cmdMan->enable(bindings[i]);
            if(SettingsManager::formBinding(static_cast<SettingsManager::FormBinding>(i))) cmdMan->check(bindings[i]);
        }
    }
    if(sde->sdk->showLinkHints) cmdMan->check(CMD_SHOW_LH);
    if(!isRun()) {
        if(historyPosition) cmdMan->enable(CMD_UNDO);
        if(historyPosition + 1 < history.size()) cmdMan->enable(CMD_REDO);
    }
	static const gchar *staticCommands[] = {
		CMD_SDK_EDITOR, CMD_MN_RUN, CMD_SHA_PAS, CMD_SHA_INI,
		CMD_SAVEAS, CMD_SELECTALL, CMD_PASTE,
		CMD_SLIDEDOWN, CMD_SLIDERIGHT, CMD_SLIDEDIAG, CMD_MAKEHINT, CMD_REMOVE_LH, CMD_SHOW_LH,
		NULL
	};
	for(const gchar **c = staticCommands; *c; c++)
		cmdMan->enable(*c);
	if(hasFormEditor(sde->sdk)) cmdMan->enable(CMD_FORMEDIT);

	changed && cmdMan->enable(CMD_SAVE);

	bool sel = !sde->csdk->selMan->elements.empty();
	if(sde->sdk->pack->name != "_base" && !sde->sdk->pack->isLegacy()) cmdMan->enable(CMD_ECREATOR);

	(sde->getZoom() < 4.0) && cmdMan->enable(CMD_ZOOMIN);
	(sde->getZoom() > 0.4) && cmdMan->enable(CMD_ZOOMOUT);

	sde->csdk->parent && cmdMan->enable(CMD_BACK);

	(sde->csdk->selMan->canDelete()) && cmdMan->enable(CMD_DELETE) && cmdMan->enable(CMD_CUT);

	formEdit && cmdMan->check(CMD_FORMEDIT);

	if(isRun())
		cmdMan->enable(CMD_STOP);
	else if(!sde->sdk->isCoreDocument()) {
		if(!sde->sdk->packProject || sde->sdk->packProject->entry != "Library") cmdMan->enable(CMD_RUN);
		cmdMan->enable(CMD_BUILD);
	}

	// if selection not empty
	if(sel) {
		cmdMan->enable(CMD_MOVETO);
		if(sde->csdk->selMan->elements.size() > 1) {
			cmdMan->enable(CMD_ALIGN);
			cmdMan->enable(CMD_GROUP);
		}
		for(Element *element : sde->csdk->selMan->elements)
			if(element->groupId() > 0) { cmdMan->enable(CMD_UNGROUP); break; }
		cmdMan->enable(CMD_ERASE);
		cmdMan->enable(CMD_FREEZ);
		cmdMan->enable(CMD_COPY);
		cmdMan->enable(CMD_EDITOR);
		cmdMan->enable(CMD_LINECOLOR);

		// ... and select only single element
		if(Element *e = sde->csdk->selMan->getFirst()) {
			if(!isRun() && !e->isNoDelete() && !e->sdk && !e->isELink()) cmdMan->enable(CMD_REPLACETO);
			(!e->isNoDelete()) && cmdMan->enable(CMD_BRINGTOFRONT)
							   && cmdMan->enable(CMD_SENDTOBACK)
							   && cmdMan->enable(CMD_COPY_LINK);

			e->sdk && cmdMan->enable(CMD_FORWARD);
		}
	}

	cmdMan->enable(CMD_PASTE_DEBUG);

	sel && sde->csdk->selMan->checkFlag(ELEMENT_FLG_IS_FREEZE) && cmdMan->check(CMD_FREEZ);
	formEdit && cmdMan->check(CMD_FORMEDIT);
}

void WM_SDK_Editor::callback(void *owner, CallbackType type, const void *data) {
	switch(type) {
		case CBT_ENTER_SCHEME_REQUEST:
			enterScheme((Element*)data);
			break;
		case CBT_CHANGE_SDK: {
			ChangeSDKInfo *sdi = (ChangeSDKInfo*)data;
			if(sdi->change) {
				sdi->sdk->selMan->on_selection_change += this;
				updateSdkPath(sdi->sdk);
				scroll.get_hadjustment()->set_value(sdi->sdk->scrollX);
				scroll.get_vadjustment()->set_value(sdi->sdk->scrollY);
			}
			else {
				sdi->sdk->scrollX = scroll.get_hadjustment()->get_value();
				sdi->sdk->scrollY = scroll.get_vadjustment()->get_value();
				sdi->sdk->selMan->on_selection_change -= this;
			}
			updateSelectionLabel();
		}
		case CBT_SDK_RUN_CHANGE:
			parent->update();
			break;
		case CBT_SDK_CHANGE_SELECT:
			updateSelectionLabel();
			parent->update();
			break;
		case CBT_POPUP_MENU:
			parent->showMenu((SDKObjectType)((int*)data)[0], ((int*)data)[1]);
			break;
		case CBT_KEY_DOWN:
			parent->translateKey((GdkEventKey*)data);
			break;
		case CBT_CHANGE_SCHEME:
            if(!restoringScheme && !sde->operationActive() && !historyIdle.connected())
                historyIdle = Glib::signal_idle().connect([this]() {
                    recordSchemeState();
                    setChanged(history[historyPosition].text != savedScheme || history[historyPosition].frozen != savedFrozen);
                    parent->update(); return false;
                });
			setChanged(true);
			break;
        case CBT_END_OPERATION:
            recordSchemeState();
            setChanged(history[historyPosition].text != savedScheme || history[historyPosition].frozen != savedFrozen);
            parent->update();
            break;
        case CBT_BEGIN_OPERATION:
            recordSchemeState();
            break;
		default:;
	}
}

bool WM_SDK_Editor::saveToFile(const ustring &file_name) {
	if(!sde->saveToFile(file_name)) {
		MessageDialog error("Unable to save scheme: " + file_name, false, MESSAGE_ERROR);
		dialog_run(error);
		return false;
	}
    recordSchemeState();
    savedScheme = history[historyPosition].text;
    savedFrozen = history[historyPosition].frozen;
    sde->sdk->fileName = file_name;
	parent->update();
	return true;
}

void WM_SDK_Editor::saveToFileAs() {
	if(!saveDlg) {
		saveDlg = new FileChooserDialog("Save file", FILE_CHOOSER_ACTION_SAVE);
		saveDlg->add_button(Stock::CANCEL, 0);
		saveDlg->add_button(Stock::OK, 1);
		//saveDlg->set_extra_widget(*(new Button("test")));
		Glib::RefPtr<FileFilter> f = FileFilter::create();
		f->add_pattern("*.sha");
		f->set_name("Source HiAsm files");
		saveDlg->set_filter(f);
	}
	if(!temporaryProject && !sde->sdk->isInstalledCoreScheme() && file_test(sde->sdk->fileName, FILE_TEST_EXISTS))
		file_chooser_set_filename(*saveDlg, sde->sdk->fileName);
	else {
        const auto name = temporaryProject ? ustring("project.sha") : Glib::filename_display_basename(sde->sdk->fileName);
		saveDlg->set_current_name(name);
		file_chooser_set_current_folder(*saveDlg, SettingsManager::readProjectsDir());
	}

	if(dialog_run(*saveDlg) == 1) {
        const ustring destination = file_chooser_get_filename(*saveDlg);
		if(saveToFile(destination)) {
            const ustring oldTemporaryDir = temporaryProjectDir;
            if(temporaryProject && !oldTemporaryDir.empty())
                remove_directory_tree(oldTemporaryDir);
            temporaryProject = false;
            temporaryProjectDir.clear();
            if(caption) caption->set_text(Glib::filename_display_basename(destination));
            rfMan.add(destination);
			setChanged(false);
        }
	}
	saveDlg->hide();
}

void WM_SDK_Editor::_on_end_edit(WM_SDK_Editor *obj) {
	obj->closeFormEditor();
}

void WM_SDK_Editor::enterScheme(Element *container) {
	sde->endOperation();
	bool fe = formEdit;
	if(fe) closeFormEditor();
	sde->enterSDK(container);
	if(fe) openFormEditor();
}

void WM_SDK_Editor::leaveSchemeTo(SDK *target) {
	sde->endOperation();
	bool fe = formEdit;
	if(fe) closeFormEditor();
	sde->leaveSDKTo(target);
	if(fe) openFormEditor();
}

void WM_SDK_Editor::updateSdkPath(SDK *shown) {
	for(Label *item : sdkPathItems) {
		sdkPath->remove(*item);
		delete item;
	}
	sdkPathItems.clear();
	// The schemes from ROOT to the shown one. The name of a scheme is the element that holds it, as in SDK::getPath.
	std::vector<SDK*> chain;
	for(SDK *s = shown; s; s = s->parent ? s->parent->parent : nullptr)
		chain.insert(chain.begin(), s);
	for(size_t i = 0; i < chain.size(); i++) {
		if(i) addSdkPathItem("\\", nullptr);
		addSdkPathItem(i ? chain[i]->parent->getName() : ustring("ROOT"), i + 1 < chain.size() ? chain[i] : nullptr);
	}
	addSdkPathItem("\\", nullptr);
}

void WM_SDK_Editor::addSdkPathItem(const ustring &text, SDK *target) {
	Label *label = new Label(text);
	sdkPath->pack_start(*label, false, false);
	sdkPathItems.push_back(label);
	if(!target) return;
	// A name above the shown scheme is a link: it is underlined while the pointer is over it.
	auto underlined = std::make_shared<bool>(false);
	const ustring markup = "<u>" + Glib::Markup::escape_text(text) + "</u>";
	connect_motion_notify(*label, [label, underlined, markup](GdkEventMotion *) {
		if(*underlined) return;
		*underlined = true;
		label->set_markup(markup);
	});
	connect_leave_notify(*label, [label, underlined, text](GdkEventCrossing *) {
		*underlined = false;
		label->set_text(text);
	});
	connect_button_release(*label, [this, target](GdkEventButton *event) {
		if(event->button != BTN_LEFT) return false;
		// The path line is rebuilt when the scheme changes, so the change waits until this click is handled.
		pathIdle.disconnect();
		pathIdle = Glib::signal_idle().connect([this, target]() {
			leaveSchemeTo(target);
			return false;
		});
		return true;
	});
}

ustring WM_SDK_Editor::sdkPathText() const {
	ustring text;
	for(Label *item : sdkPathItems) text += item->get_text();
	return text;
}

void WM_SDK_Editor::openFormEditor() {
	if(formEdit) return;
	if(!hasFormEditor(sde->sdk)) {
		cgt_on_debug.run("!No form editor is installed for this package");
		return;
	}
	if(sde->sdk->pack->isLegacy() || separateFormWindow(sde->sdk))
		formEdit = new FE_Legacy();
	else if(sde->sdk->pack->name == "_base" || hasInternalForm(sde->sdk))
		formEdit = new FE_Internal();
	else
		formEdit = new FE_ExternalTcp();
	formEdit->start(sde);
	if(!formEdit->isActive()) {
		delete formEdit; formEdit = nullptr;
		parent->update(); return;
	}
	parent->update();
	formEdit->on_end_edit = EVENT(_on_end_edit);
}

void WM_SDK_Editor::closeFormEditor() {
	if(!formEdit) return;

	formEdit->stop();
	delete formEdit;
	formEdit = NULL;
	parent->update();
}

void WM_SDK_Editor::run() {
    pbuilder.run(sde->sdk);
    parent->update();
}

typedef struct _elementInfo {
	int count;
	Element *e;
	_elementInfo(Element *e) { count = 1; this->e = e; }
} elementInfo;

void WM_SDK_Editor::updateSelectionLabel() {
	elementsList->set_text(sde->csdk->selMan->empty() ? "(none)" : (ustring("Selected: ") + int_to_str(sde->csdk->selMan->elements.size())));
}

void WM_SDK_Editor::fillSelectionMenu() {
	std::vector<Widget*> wlist = selInfoMenu.get_children();
	for(std::vector<Widget*>::iterator item = wlist.begin(); item != wlist.end(); item++)
		selInfoMenu.remove(**item.base());

	std::list<elementInfo> list;
	int c;
	for(ElementsList::iterator e = sde->csdk->selMan->elements.begin(); e != sde->csdk->selMan->elements.end(); e++) {
		c = 0;
		for(std::list<elementInfo>::iterator s = list.begin(); s != list.end(); s++)
			if((*s).e->tpl == (*e)->tpl) {
				(*s).count++;
				c = 1;
				break;
			}
		if(c == 0)
			list.push_back(elementInfo(*e));
	}

	for(std::list<elementInfo>::iterator s = list.begin(); s != list.end(); s++) {
		SI_ImageMenuItem *item = new SI_ImageMenuItem((*s).e, (*s).count);
		selInfoMenu.append(*item);
	}
}

bool WM_SDK_Editor::on_sel_info_click(GdkEventButton *event) {
	if(sde->csdk->selMan->elements.size()) {
		fillSelectionMenu();
		selInfoMenu.popup(event->button, event->time);
	}
	return false;
}

void WM_SDK_Editor::on_change_value() {
	sde->setZoom(zoomSlider.get_value() / 100);
}

//---------------------------------------------------------------

Widget *WM_Text_Editor::open(const ustring &file_name, bool asNew) {
	TRACE_PROC

#ifdef USE_GTK_SOURCE_VIEW
#ifdef HIASM_GTK4
	memo = Gsv::create_view();
	Gsv::configure_view(*memo, file_name);
#else
	memo = new Gsv::View();
	memo->set_show_line_numbers(true);
	Glib::RefPtr<Gsv::LanguageManager> sm = Gsv::LanguageManager::create();
	ustring basename = Glib::filename_display_basename(file_name);
	std::string ext;
	size_t dot = basename.rfind('.');
	if(dot != std::string::npos && dot + 1 < basename.size())
		ext = basename.substr(dot + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	if(ext == "pas" || ext == "dpr" || ext == "pp")
		ext = "pascal";
	else if(ext ==  "hws" || ext == "sha")
		ext = "hiasm";
	Glib::RefPtr<Gsv::Language> lng = sm->get_language(ext);
	if(lng) {
		Glib::RefPtr<Gsv::Buffer> sb = Gsv::Buffer::create(lng);
		sb->set_highlight_syntax(true);
		memo->set_source_buffer(sb);
	}
#endif
#else
	memo = new TextView();
#endif

	const bool legacyText = packSet && packSet->isLegacyFile(file_name);
	if(file_test(file_name, FILE_TEST_EXISTS)) {
        gchar *cont = nullptr;
        gsize len = 0;
        GError *error = nullptr;
        if(g_file_get_contents(file_name.c_str(), &cont, &len, &error) && (legacyText || g_utf8_validate(cont, len, nullptr))) {
            const auto text = legacyText ? LegacyPackCatalog::decodeText(cont, len) : std::string(cont, len);
            memo->get_buffer()->set_text(text);
        }
        else {
            MessageDialog message("Unable to open UTF-8 text: " + file_name, false, MESSAGE_ERROR);
            dialog_run(message);
        }
        g_clear_error(&error);
        g_free(cont);
	}
	if(legacyText) memo->set_editable(false);
#ifdef HIASM_GTK4
    memo->get_buffer()->set_enable_undo(true);
    memo->get_buffer()->set_modified(false);
#endif
	memo->get_buffer()->signal_changed().connect(sigc::mem_fun(*this, &WM_Text_Editor::on_edit_changed));
	fName = file_name;

	scroll = new ScrolledWindow();
	scroll->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scrolled_set_child(*scroll, *memo);

	changed = false;

	return scroll;
}

void WM_Text_Editor::on_edit_changed() {
	setChanged(true);
    parent->update();
}

bool WM_Text_Editor::saveToFile(const ustring &file_name) {
    if(packSet && packSet->isLegacyFile(file_name)) return false;
    const auto text = memo->get_buffer()->get_text(false);
    GError *error = nullptr;
    if(!g_file_set_contents(file_name.c_str(), text.c_str(), text.bytes(), &error)) {
        std::cerr << "Unable to save " << file_name << ": " << (error ? error->message : "unknown error") << std::endl;
        g_clear_error(&error);
        return false;
    }
    fName = file_name;
    memo->get_buffer()->set_modified(false);
    setChanged(false);
    return true;
}

bool WM_Text_Editor::findNext(const ustring &text) {
    if(text.empty()) return false;
    searchText = text;
    auto buffer = memo->get_buffer();
    GtkTextIter start, end, cursor, foundStart, foundEnd;
    gtk_text_buffer_get_bounds(buffer->gobj(), &start, &end);
    if(!gtk_text_buffer_get_selection_bounds(buffer->gobj(), &foundStart, &cursor))
        gtk_text_buffer_get_iter_at_mark(buffer->gobj(), &cursor, gtk_text_buffer_get_insert(buffer->gobj()));
    bool found = gtk_text_iter_forward_search(&cursor, text.c_str(), GTK_TEXT_SEARCH_TEXT_ONLY, &foundStart, &foundEnd, &end);
    if(!found)
        found = gtk_text_iter_forward_search(&start, text.c_str(), GTK_TEXT_SEARCH_TEXT_ONLY, &foundStart, &foundEnd, &cursor);
    if(!found) return false;
    gtk_text_buffer_select_range(buffer->gobj(), &foundStart, &foundEnd);
    gtk_text_view_scroll_to_iter(memo->gobj(), &foundStart, 0.1, FALSE, 0, 0);
    memo->grab_focus();
    return true;
}

bool WM_Text_Editor::processCommand(const ustring &cmd, const ustring &arg) {
	if(cmd == CMD_SAVE) {
        if(!saveToFile(fName)) {
            MessageDialog error("Unable to save text: " + fName, false, MESSAGE_ERROR);
            dialog_run(error);
        }
	}
	else if(cmd == CMD_SAVEAS) {
        FileChooserDialog chooser("Save text as", FILE_CHOOSER_ACTION_SAVE);
        chooser.add_button(Stock::CANCEL, 0);
        chooser.add_button(Stock::SAVE, 1);
        file_chooser_set_filename(chooser, fName);
        if(dialog_run(chooser) == 1 && !saveToFile(file_chooser_get_filename(chooser))) {
            MessageDialog error("Unable to save text", false, MESSAGE_ERROR);
            dialog_run(error);
        }
	}
    else if(cmd == CMD_SEARCH) {
        ustring text = arg;
        if(text.empty()) {
            Dialog dialog;
			dialog.set_title(ui_tr("Find text"));
            Entry entry;
            entry.set_text(searchText);
            entry.set_activates_default(true);
            box_pack_start(*dialog.get_content_area(), entry, false, false, 6);
            dialog.add_button(Stock::CANCEL, 0);
            dialog.add_button(Stock::FIND, 1);
            dialog.set_default_response(1);
            if(dialog_run(dialog) != 1) return true;
            text = entry.get_text();
        }
        findNext(text);
    }
    else if(cmd == CMD_SEARCHNEXT) findNext(searchText);
    else if(cmd == CMD_SELECTALL) {
        auto buffer = memo->get_buffer();
        buffer->select_range(buffer->begin(), buffer->end());
    }
    else if(cmd == CMD_COPY || cmd == CMD_CUT) {
        auto buffer = memo->get_buffer();
        auto start = buffer->begin(), end = buffer->end();
        if(buffer->get_selection_bounds(start, end)) {
            clipboard_set_text(buffer->get_text(start, end));
            if(cmd == CMD_CUT) buffer->erase_selection();
        }
    }
    else if(cmd == CMD_PASTE) {
        auto buffer = memo->get_buffer();
        auto text = clipboard_wait_for_text();
        if(!text.empty()) { buffer->erase_selection(); buffer->insert_at_cursor(text); }
    }
#ifdef HIASM_GTK4
    else if(cmd == CMD_UNDO) memo->get_buffer()->undo();
    else if(cmd == CMD_REDO) memo->get_buffer()->redo();
#elif defined(USE_GTK_SOURCE_VIEW)
    else if(cmd == CMD_UNDO) memo->get_source_buffer()->undo();
    else if(cmd == CMD_REDO) memo->get_source_buffer()->redo();
#endif
	else return false;

	return true;
}

void WM_Text_Editor::update() {
	changed && cmdMan->enable(CMD_SAVE);
	cmdMan->enable(CMD_SAVEAS);
    cmdMan->enable(CMD_SELECTALL);
    cmdMan->enable(CMD_PASTE);
    cmdMan->enable(CMD_SEARCH);
    if(!searchText.empty()) cmdMan->enable(CMD_SEARCHNEXT);
    if(memo->get_buffer()->get_has_selection()) {
        cmdMan->enable(CMD_COPY); cmdMan->enable(CMD_CUT);
    }
#ifdef HIASM_GTK4
    if(memo->get_buffer()->get_can_undo()) cmdMan->enable(CMD_UNDO);
    if(memo->get_buffer()->get_can_redo()) cmdMan->enable(CMD_REDO);
#elif defined(USE_GTK_SOURCE_VIEW)
    if(memo->get_source_buffer()->can_undo()) cmdMan->enable(CMD_UNDO);
    if(memo->get_source_buffer()->can_redo()) cmdMan->enable(CMD_REDO);
#endif
}

//---------------------------------------------------------------

WM_New_Project::~WM_New_Project() {
	rfMan.on_change -= this;
}

bool WM_New_Project::close() {
	return parent->tabs.size() > 1;
}

Widget *WM_New_Project::open(const ustring &file_name, bool asNew) {
	rfMan.on_change += this;

	files.set_size_request(100, 400);
	fileStore = Gtk::ListStore::create(fileColumns);
	files.set_model(fileStore);
	files.append_column("*", fileColumns.icon);
	files.append_column("Path", fileColumns.name);
	files.append_column("Size", fileColumns.size);
	files.append_column("Date", fileColumns.date);

	files.signal_row_activated().connect(sigc::mem_fun(*this, &WM_New_Project::on_row_activate));

	fillFileList();

	scrolled_set_child(filesScroll, files);
	filesScroll.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);

	paned.pack1(filesScroll, false, false);
	paned.pack2(memo, true, true);
	paned.show();
	filesScroll.show();
	files.show();
	memo.show();
	return &paned;
}

void WM_New_Project::on_row_activate(const TreeModel::Path &path, TreeViewColumn *column) {
	TreeModel::Row r = *fileStore->get_iter(path);
	parent->open(r[fileColumns.name]);
}

void WM_New_Project::fillFileList() {
	fileStore->clear();
	for(RecentFileManager::iterator f = rfMan.begin(); f != rfMan.end(); f++) {
		Gtk::TreeRow row = *(fileStore->append());
		row[fileColumns.name] = f->name;
		row[fileColumns.icon] = f->icon;

		GFile *file = g_file_new_for_path(f->name.c_str());
		GFileInfo *inf = g_file_query_info(file, "*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
		row[fileColumns.size] = int_to_str(g_file_info_get_size(inf)) + "b";

		// Modification time in UTC as ISO 8601 (GTimeVal is deprecated since GLib 2.62).
		if(GDateTime *modified = g_file_info_get_modification_date_time(inf)) {
			GDateTime *utc = g_date_time_to_utc(modified);
			gchar *text = g_date_time_format_iso8601(utc);
			row[fileColumns.date] = text ? text : "";
			g_free(text);
			g_date_time_unref(utc);
			g_date_time_unref(modified);
		}

		g_object_unref(file);
		g_object_unref(inf);

		//row[fileColumns.ctl];
	}
}

void WM_New_Project::callback(void *owner, CallbackType type, const void *data) {
	fillFileList();
}

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------

WindowManager::WindowManager():Notebook(),
	on_tab_open(this, CBT_TAB_OPEN),
	on_tab_change(this, CBT_TAB_CHANGE),
	on_tab_close(this, CBT_TAB_CLOSE)
{
	cmdMan->on_command += this;
	set_scrollable(true);
#ifdef HIASM_GTK4
	switchPageConnection = signal_switch_page().connect(sigc::mem_fun(*this, &WindowManager::handleSwitchPage), false);
#else
	switchPageConnection = signal_switch_page().connect(sigc::mem_fun(*this, &WindowManager::handleSwitchPage));
#endif
	openDlg = NULL;
	aboutDlg = NULL;

	oldTab = NULL;

	for(int i = 0; i < SDK_OBJ_COUNT; i++) popupMenus[i] = NULL;

#ifdef USE_GTK_SOURCE_VIEW
#ifdef HIASM_GTK4
    Gsv::init(dataDir);
#else
	Gsv::init();
#endif
#endif
}

WindowManager::~WindowManager() {
	// GtkNotebook emits switch-page while its base destructor removes pages.
	// Disconnect before the derived members (including tabs) are destroyed.
	switchPageConnection.disconnect();
	cmdMan->on_command -= this;
}

WM_Tab *WindowManager::open(const ustring &file_name, bool asNew) {
	TRACE_PROC
	WM_Tab *tab;

	DEBUG_MSG("Open file: " << file_name.c_str());

	Glib::ustring ext = file_name.length() < 3 ? "" : file_name.substr(file_name.length()-3, 3).lowercase();
	if(file_name.empty())
		tab = new WM_New_Project(this);
	else if(ext == "sha")
		tab = new WM_SDK_Editor(this);
//	else if(ext == "ini") {
//		if(oldTab != NULL && oldTab->type == WMT_SDK_EDITOR) {
//			//((WM_SDK_Editor*)oldTab)->sde->sdk->pack->addElement("Helpers", )
//		}
//	}
	else
		tab = new WM_Text_Editor(this);

	Widget *child = tab->open(file_name, asNew);
	if(!child) {
		// The tab of a scheme that does not load is not shown. It is kept: a partly loaded scheme cannot be
		// destroyed safely yet (its elements of the broken block are not complete).
		return nullptr;
	}
	child->show();
    const ustring captionText = (asNew && tab->type == WMT_SDK_EDITOR)
        ? ui_tr("New project")
        : Glib::filename_display_basename(tab->fileName());
	tab->caption = new Label(captionText);

	HBox *box = new HBox();
	Image *img = new Image(tab->icon());
	// TODO memory?
	box->add(*img);
	box->add(*tab->caption);
	Image *close = new Image(Gdk::Pixbuf::create_from_file(dataDir + INT_PATH"close.png"));
	close->set_tooltip_text("Close");

	tab->closeBtn = new ToolButton();
	tab->closeBtn->set_size_request(16, 16);
	tab->closeBtn->set_icon_widget(*close);
	//tab->close->set_name("btn_close");
	tab->closeBtn->signal_clicked().connect(sigc::mem_fun(*this, &WindowManager::on_close_click));

	box->add(*tab->closeBtn);
	box->set_spacing(1);
	box->show();

	tabs.push_back(tab);
	on_tab_open.run(tab);
	append_page(*child, *box);
	setCurrent(tab);

	update();

	if(!asNew) rfMan.add(file_name);

	return tab;
}

void WindowManager::on_close_click() {
    closeCurrent();
}

bool WindowManager::removeTab(WM_Tab *tab, bool prompt, bool createStartup) {
    auto position = std::find(tabs.begin(), tabs.end(), tab);
    if(position == tabs.end() || (prompt && !tab->close())) return false;
    // Detach SDK listeners while the closing editor still exists.
    if(oldTab == tab) {
        on_tab_change.run(nullptr);
        oldTab = nullptr;
    }
    on_tab_close.run(tab);
    tabs.erase(position);
    remove_page(*tab->widget());
    delete tab;
    if(tabs.empty()) {
        if(createStartup) open("", true);
    } else if(auto current = getCurrent()) switchToTab(current);
    update();
    return true;
}

bool WindowManager::closeCurrent() {
    if(confirmingClose) return false;
    auto current = getCurrent();
    return current && removeTab(current, true, true);
}

bool WindowManager::canCloseAll() {
    if(confirmingClose) return false;
    confirmingClose = true;
    struct Reset { bool &flag; ~Reset() { flag = false; } } reset{confirmingClose};
    const auto opened = tabs;
    for(auto tab : opened) {
        if(tab->changed) {
            setCurrent(tab);
            if(!tab->close()) return false;
        }
    }
    return true;
}

bool WindowManager::closeAll() {
    if(!canCloseAll()) return false;
    const auto opened = tabs;
    for(auto tab : opened) removeTab(tab, false, false);
    open("", true);
    return true;
}

void WindowManager::setCurrent(WM_Tab *tab) {
	int i = page_num(*tab->widget());
	if(i != -1) {
		set_current_page(i);
		tab->select();
	}
}

void WindowManager::handleSwitchPage(Gtk::Widget *page, guint index) {
	

	DEBUG_MSG("switch page " << index)
	if(index < tabs.size()) switchToTab(tabs[index]);
}

void WindowManager::switchToTab(WM_Tab *tab) {
	if(tab == oldTab) return;

	on_tab_change.run(tab);
	if(oldTab)
		oldTab->closeBtn->hide();
	oldTab = tab;
	oldTab->closeBtn->show();
	update();
}

void WindowManager::callback(void *owner, CallbackType type, const void *data) {
	switch(type) {
		case CBT_RUN_COMMAND:
			processCommand(*(ustring*)data, ustring());
			break;
		default:;
	}
}

WM_Tab *WindowManager::getCurrent() {
	int i = get_current_page();
	return (i < 0 || static_cast<size_t>(i) >= tabs.size()) ? NULL : tabs[i];
}

void WindowManager::processCommand(const ustring &cmd, const ustring &args) {
	std::cout << "command: " << cmd.c_str() << std::endl;

	WM_Tab *tab = getCurrent();
	if(tab && tab->processCommand(cmd, args))
		return;

	if(cmd == CMD_OPEN) {
		if(!openDlg) {
			openDlg = new FileChooserDialog("Open file");
			openDlg->add_button(Stock::CANCEL, 0);
			openDlg->add_button(Stock::OK, 1);
			//openDlg->set_extra_widget(*(new Button("test")));
			//openDlg->set_filter(FileFilter());
			for(PackCollection::iterator p = packSet->begin(); p != packSet->end(); p++) {
//				DEBUG_MSG((dataDir + ELEMENTS_PATH + (*p)->name + "/" ELEMENTS_EXAMPLE_PATH).c_str())
				file_chooser_add_shortcut_folder(*openDlg, dataDir + ELEMENTS_PATH + (*p)->name + "/" ELEMENTS_EXAMPLE_PATH);
			}
		}
		if(dialog_run(*openDlg) == 1)
			open(file_chooser_get_filename(*openDlg));
		openDlg->hide();
	}
	else if(cmd == CMD_NEW) {
		if(prjMan.open() == 1 && prjMan.selectProject)
			open(prjMan.selectProject->getTemplateFile(), true);
		prjMan.hide();
	}
	else if(cmd == CMD_CLOSE) closeCurrent();
	else if(cmd == CMD_CLOSEALL) closeAll();
	else if(cmd == CMD_EXIT) {
        if(form) gtk_window_close(form->gobj());
    }
	else if(cmd == CMD_FORUM)
		compat_show_uri("http://forum.hiasm.com");
	else if(cmd == CMD_MAIL)
		compat_show_uri("mailto:support@hiasm.com");
//	else if(cmd == CMD_EXAMPLES)
//		gtk_show_uri(form->get_screen()->gobj(), "file:///", GDK_CURRENT_TIME, NULL);
	else if(cmd == CMD_ABOUT) {
		if(!aboutDlg) {
			aboutDlg = new AboutDialog();
			aboutDlg->set_version(HIASM_VERSION());
			aboutDlg->set_program_name("HiAsm");
			aboutDlg->set_comments("a graphic programming studio");
			aboutDlg->set_copyright("Copyright (C) 2004-2026 HiAsm Studio Contributors");
			#ifdef HIASM_GTK4
			// icon names are only found when the icon theme ships them; load the logo file instead
			aboutDlg->set_logo(Gdk::Texture::create_from_filename((dataDir + INT_PATH"hiasm.png").c_str()));
#else
			aboutDlg->set_logo_icon_name("hiasm");
#endif
			aboutDlg->set_website("http://hiasm.com");
			const gchar *authors[] = {"Project owner:", "   Dilma <support@hiasm.com>", "", "Co-author:", "   ili4yov_ika <ilichyov.kolyan@yandex.ru>", "", "Developers and testers:", "   Assasin <iassasin@yandex.ru>", "   Nic <nic@savelovo.net>", "   UtoECat <https://github.com/UtoECat>", NULL};
			gtk_about_dialog_set_authors(aboutDlg->gobj(), authors);
			const gchar *artists[] = {"Nic <nic@savelovo.net>", NULL};
			gtk_about_dialog_set_artists(aboutDlg->gobj(), artists);
			gchar *cont;
			gsize len;
			g_file_get_contents((dataDir + INT_PATH"license").c_str(), &cont, &len, NULL);
			aboutDlg->set_license(ustring(cont, len));
			g_free(cont);
		}

		// the dialog used to be shown and hidden at once, so it never appeared
		compat_standard_frame(*aboutDlg);
		aboutDlg->set_transient_for(*form);
		aboutDlg->present();
	}
	else if(cmd == CMD_ECREATOR) {
		if(oldTab != NULL && oldTab->type == WMT_SDK_EDITOR) {
			Pack *pack = static_cast<WM_SDK_Editor*>(oldTab)->sde->sdk->pack;
			if(pack->name == "_base") return;
			pack->ensureLoaded();
			Dialog dialog("Create element");
			dialog.set_default_size(420, 240);
			if(form) dialog.set_transient_for(*form);
			Entry name, description;
			ComboBoxText tab;
			for(PackGroup *group : pack->groups) if(!group->name.empty()) tab.append(group->name);
			tab.set_active(0);
			name.set_text("UserElement"); description.set_text(ui_tr("Custom event relay"));
			Label nameLabel(ui_tr("Element name")), infoLabel(ui_tr("Description")), tabLabel(ui_tr("Palette tab"));
			Label help(ui_tr("Creates doWork → onEvent. Configuration and generator source open for editing.")), errorLabel;
#ifdef HIASM_GTK4
			help.set_wrap(true); errorLabel.set_wrap(true);
#else
			help.set_line_wrap(true); errorLabel.set_line_wrap(true);
#endif
			auto body = dialog.get_content_area();
			for(Widget *widget : std::vector<Widget*>{&nameLabel, &name, &infoLabel, &description, &tabLabel, &tab, &help, &errorLabel})
				box_pack_start(*body, *widget, false, false, 3);
			dialog.add_button(Stock::CANCEL, 0); dialog.add_button(Stock::ADD, 1);
			while(dialog_run(dialog) == 1) {
				ustring error;
				if(!pack->createElement(name.get_text(), tab.get_active_text(), description.get_text(), error)) {
					errorLabel.set_text(error); continue;
				}
				on_tab_change.run(NULL); on_tab_change.run(oldTab);
				open(pack->pathElementConf(name.get_text()));
				open(pack->pathElementCode(name.get_text()));
				break;
			}
		}
	}
	else if(cmd == CMD_TOOLS)
		SettingsManager::show();

	update();
}

void WindowManager::update() {
	static const gchar *staticCommands[] = {
		CMD_OPEN, CMD_ABOUT, CMD_NEW, CMD_EXIT, CMD_TOOLS, CMD_FULLSCREEN,
		CMD_MN_FILE, CMD_MN_EDIT, CMD_MN_VIEW, CMD_MN_HELP, CMD_MN_SERVICE,
		CMD_FORUM, CMD_MAIL,
		CMD_ELEMENTS, CMD_PROPERTY, CMD_PANELS, CMD_QPAN_SHOWALL, CMD_QPAN_HIDEALL,
		NULL
	};

	cmdMan->beginUpdate();

	if(WM_Tab *tab = getCurrent()) tab->update();
    if(getCurrent() && (getCurrent()->type != WMT_NEW_PROJECT || tabs.size() > 1)) cmdMan->enable(CMD_CLOSE);
    if(!tabs.empty() && (tabs.size() > 1 || tabs.front()->type != WMT_NEW_PROJECT)) cmdMan->enable(CMD_CLOSEALL);

	for(const gchar **c = staticCommands; *c; c++)
		cmdMan->enable(*c);

	cmdMan->endUpdate();
}

void WindowManager::showMenu(SDKObjectType type, guint32 time) {
	Menu **m = &popupMenus[type];
	if(!*m) {
		const gchar *name = NULL;

		switch(type) {
			case SDK_OBJ_ELEMENT: name = MENUNAME_ELEMENT; break;
			case SDK_OBJ_NONE: name = MENUNAME_SDK;	break;
			case SDK_OBJ_LINE: name = MENUNAME_LINK; break;
			default: return;
		}
		*m = cmdMan->createMenu(name);
		update();
	}

	(*m)->popup(BTN_RIGHT, time);
}

bool WindowManager::translateKey(GdkEventKey *key, bool globalOnly) {
	// Alt/Super combinations belong to menus/window navigation, not the
	// database's Ctrl/Shift-only shortcut format.
	if(key->state & (guint(1 << 3) | GDK_SUPER_MASK)) return false;
	bool handled = false;
	auto runKey = [&](guint keyval) -> bool {
		bool found = false;
		void *id = mdb.begin_read_command_by_key(keyval, key->state&0x01, key->state&0x04);
		CommandInfo cmd;
		while(mdb.read_command_by_key(id, cmd)) {
			if(globalOnly && cmd.name != CMD_NEW && cmd.name != CMD_OPEN &&
			   cmd.name != CMD_SAVE && cmd.name != CMD_BUILD && cmd.name != CMD_RUN &&
			   cmd.name != CMD_FULLSCREEN && cmd.name != CMD_ABOUT) continue;
			found = true;
			if(cmdMan->isEnabled(cmd.name.c_str())) {
				handled = true;
				cmdMan->command(cmd.name);
			}
		}
		mdb.end_read(id);
		return found;
	};

	if(!runKey(key->keyval)) {
		bool found = false;
		guint alt = gdk_keyval_to_upper(key->keyval);
		if(alt == key->keyval)
			alt = gdk_keyval_to_lower(key->keyval);
		if(alt != key->keyval)
			found = runKey(alt);
#ifdef G_OS_WIN32
		// hotkeys are stored in hiasm.db as latin keyvals, but non-latin
		// keyboard layouts produce localized keyvals; map back to the latin
		// key through the Windows virtual-key code so Ctrl+C/Ctrl+V & co
		// work in any layout
		guint vk = key->hardware_keycode;
		if(!found && key->keyval > 0x7F && vk >= 'A' && vk <= 'Z') {
			if(!runKey(vk | 0x20))
				runKey(vk);
		}
#endif
	}
	return handled;
}

void WindowManager::changeTab(WM_Tab *tab) {
    const bool temporary = tab->type == WMT_SDK_EDITOR &&
        static_cast<WM_SDK_Editor *>(tab)->isTemporaryProject();
    const ustring name = temporary ? ui_tr("New project") :
        Glib::filename_display_basename(tab->fileName());
	tab->caption->set_text((tab->changed ? ustring("*") : ustring()) + name);
	update();
}

bool WindowManager::checkProjectName(const ustring &name) {
	for(WM_Tabs::iterator t = tabs.begin(); t != tabs.end(); t++)
		if((*t)->caption->get_text() == name)
			return true;
	return false;
}

void WindowManager::init() {
    // This marker belongs to the user profile, never the installation.
    const ustring marker = homeDir + "welcome-core-v1.seen";
    const bool firstWelcome = !file_test(marker, FILE_TEST_IS_REGULAR);
	if(settingsMan->readInt(IDS_NEW_PROJECT)) {
		loadTabs();
	}
    if(firstWelcome && openWelcome())
        g_file_set_contents(marker.c_str(), "1\n", -1, nullptr);
    if(tabs.empty()) {
        auto cpp = packSet->getPackByName("cpp");
        // A new project starts as a form: the built-in designer works for it.
        // Gui cannot host the standard widgets, so it is only a fallback.
        auto project = cpp ? cpp->getProjectByEntry("MainForm") : nullptr;
        if(!project && cpp) project = cpp->getProjectByEntry("Gui");
        if(!settingsMan->readInt(IDS_NEW_PROJECT) && project) open(project->getTemplateFile(), true);
        else open("", true);
    }
	update();
}

bool WindowManager::openWelcome() {
    const ustring root = dataDir + "packs/_base/";
    const ustring base = root + "example/tutorial/intro.sha";
    ustring file = root + "lang/" + ui_language() + "/example/tutorial/intro.sha";
    auto valid = [](const ustring &path) {
        MSDK checked;
        return checked.loadFromFile(path) && checked.isCoreDocument() && !checked.elements.empty();
    };
    if(!valid(file)) {
        if(!valid(base)) return false;
        file = base;
    }
    for(auto tab : tabs) {
        if(tab->fileName() == file) { setCurrent(tab); return true; }
    }
    auto tab = open(file, false);
    if(tab && tab->caption) tab->caption->set_text(ui_tr("Welcome"));
    return tab != nullptr;
}

void WindowManager::saveTabs() {
	HStringList desktop;
	for(WM_Tabs::iterator t = tabs.begin(); t != tabs.end(); t++)
		desktop << (*t)->fileName();
	desktop.saveToFile(homeDir + "desktop.lst");
}

void WindowManager::loadTabs() {
	HStringList desktop(homeDir + "desktop.lst");
	for(HStringList::iterator line = desktop.begin(); line != desktop.end(); line++) {
		if(*line == "startup")
			open("", true);
		else if(file_test(*line, FILE_TEST_EXISTS))
			open(*line, false);
	}
}
