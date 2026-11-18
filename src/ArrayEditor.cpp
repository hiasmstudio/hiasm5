/*
 * ArrayEditor.cpp
 *
 *  Created on: Jul 4, 2010
 *      Author: dilma
 */

#include <cstdint>

#include "ArrayEditor.h"

ArrayEditor::ArrayEditor():
#ifdef HIASM_GTK4
	btnAdd(Gtk::StockIconMap::label(Stock::ADD)),btnClear(Gtk::StockIconMap::label(Stock::CLEAR))
#else
	btnAdd(Stock::ADD),btnClear(Stock::CLEAR)
#endif
{
	set_default_size(300, 400);

	add_button(Stock::OK, 1);

	pFixed.add(pEditor);
	connect_size_allocate(pFixed, sigc::mem_fun(*this, &ArrayEditor::props_on_size_allocate));
	connect_button_press(pFixed, sigc::mem_fun(pEditor, &PropEditor::on_button_press_event));
	pFixed.signal_button_release_event().connect(sigc::mem_fun(pEditor, &PropEditor::on_button_release_event));
	connect_motion_notify(pFixed, sigc::mem_fun(pEditor, &PropEditor::on_motion_notify_event));
	pScroll.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scrolled_set_child(pScroll, pFixed);
	pScroll.set_size_request(200, 300);

	bbox.add(btnAdd);
	bbox.add(btnClear);
	bbox.set_layout(BUTTONBOX_START);

	hbox.pack_start(pScroll, true, true);
	hbox.pack_end(bbox, false, false);

	box_pack_start(*get_content_area(), state, false, false, 0);
	box_pack_end(*get_content_area(), hbox, true, true, 0);

	get_content_area()->show();

	btnAdd.signal_clicked().connect(sigc::mem_fun(*this, &ArrayEditor::on_add_click));
	btnClear.signal_clicked().connect(sigc::mem_fun(*this, &ArrayEditor::on_clear_click));

	pEditor.on_select_property += this;
}

void ArrayEditor::edit(void *value) {
	array = (ArrayValue*)value;

	set_title(ustring(dataNames[array->type]) + " array");

	pEditor.clear();
	for(ArrayValue::iterator av = array->begin(); av != array->end(); av++)
		addValueToEditor(av->name, av->data);

	updateState();

	dialog_run(*this);
	hide();
}

void ArrayEditor::props_on_size_allocate(Allocation &alloc) {
	pEditor.changeSize(alloc.get_width());
}

void ArrayEditor::on_add_click() {
	addValueToEditor(dataNames[array->type], array->add());
	updateState();
}

void ArrayEditor::on_clear_click() {
	array->clear();
	pEditor.clear();
	updateState();
}

void ArrayEditor::addValueToEditor(const ustring &name, void *value) {
	PropertyItem *prop;
	switch(array->type) {
		case data_int:
			prop = new PropertyInteger(name, value);
			break;
		case data_stock:
			prop = new PropertyStock(name, value);
			break;
		case data_str:
		case data_list:
			prop = new PropertyString(name, value);
			break;
		case data_real:
			prop = new PropertyDouble(name, value);
			break;
		case data_data:
			prop = new PropertyTData(name, value);
			break;
		case data_pixbuf:
			prop = new PropertyPixbuf(name, value);
			break;
		case data_color:
			prop = new PropertyColor(name, value);
			break;
		case data_array:
			prop = new PropertyArray(name, value);
			break;
		default:
			;
	}
	prop->data = (void*)pEditor.props.size();
	pEditor.addProperty(prop);
}

void ArrayEditor::updateState() {
	int i = pEditor.select ? (int)(intptr_t)pEditor.select->data : -1;
	state.set_text(ustring("Items: ") + int_to_str(array->size()) + ", select: " + int_to_str(i));
}

void ArrayEditor::callback(void *owner, CallbackType type, const void *data) {
	switch(type) {
		case CBT_SELECT_PROPERTY:
			updateState();
			break;
		default:;
	}
}
