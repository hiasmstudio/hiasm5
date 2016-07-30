/*
 * ArrayEditor.h
 *
 *  Created on: Jul 4, 2010
 *      Author: dilma
 */

#ifndef ARRAYEDITOR_H_
#define ARRAYEDITOR_H_

#include "share.h"
#include "ElementProperty.h"
#include "PropEditor.h"

class ArrayEditor : public Dialog, public CallBack {
	private:
		ArrayValue *array;

		Label state;
		HBox hbox;
		EventBox pFixed;
		PropEditor pEditor;
		ScrolledWindow pScroll;
		VButtonBox bbox;

		Button btnAdd;
		Button btnClear;

		virtual void on_add_click();
		virtual void on_clear_click();
		virtual void props_on_size_allocate(Allocation &alloc);

		void addValueToEditor(const ustring &name, void *value);
		void updateState();
	protected:
		virtual void callback(void *owner, CallbackType type, const void *data);
	public:
		ArrayEditor();

		void edit(void *value);
};

#endif /* ARRAYEDITOR_H_ */
