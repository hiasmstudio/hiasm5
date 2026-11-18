/*
 * DebugPanel.cpp
 *
 *  Created on: 01.05.2010
 *      Author: dilma
 */

#include "DebugPanel.h"

DebugPanel::DebugPanel():TextView() {
	error_tag = get_buffer()->create_tag("error");
	error_tag->property_foreground() = "red";
	info_tag = get_buffer()->create_tag("info");
	info_tag->property_foreground() = "gray";
	trace_tag = get_buffer()->create_tag("trace");
	trace_tag->property_foreground() = "green";
}

void DebugPanel::text(const ustring &text) {
	get_buffer()->insert(get_buffer()->end(), text + '\n');
}

void DebugPanel::error(const ustring &text) {
	get_buffer()->insert_with_tag(get_buffer()->end(), text + '\n', error_tag);
}

void DebugPanel::info(const ustring &text) {
	get_buffer()->insert_with_tag(get_buffer()->end(), text + '\n', info_tag);
}

void DebugPanel::trace(const ustring &text) {
	get_buffer()->insert_with_tag(get_buffer()->end(), text + '\n', trace_tag);
}
