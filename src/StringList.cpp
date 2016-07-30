/*
 * StringList.cpp
 *
 *  Created on: Jul 1, 2010
 *      Author: dilma
 */

#include "StringList.h"

void StringList::loadFromFile(const ustring &fileName) {
	clear();
	if(file_test(fileName, FILE_TEST_EXISTS)) {
		gchar *cont;
		gsize len;
		g_file_get_contents(fileName.c_str(), &cont, &len, NULL);
		loadFromChar(cont);
		g_free(cont);
	}
}

void StringList::saveToFile(const ustring &fileName) {
	ustring data = text();
	g_file_set_contents(fileName.c_str(), data.c_str(), data.bytes(), NULL);
}

ustring StringList::text() {
	ustring result;
	for(iterator line = begin(); line != end(); line++)
		if(line == begin())
			result = *line;
		else
			result += LINE_END + *line;
	return result;
}

int StringList::indexOf(const ustring &text) {
	for(unsigned int i = 0; i < size(); i++)
		if(at(i) == text)
			return i;
	return -1;
}

void StringList::loadFromChar(char *buf) {
	clear();

	if(!*buf) return;

	char *tok = buf;
	while(*buf) {
		if(*buf == '\n') {
			*buf = '\0';
			int l = strlen(tok);
			if(l > 0 && tok[l-1] == '\r')
				tok[l-1] = '\0';
			push_back(tok);
			buf++;
			tok = buf;
		}
		else
			buf++;
	}
	push_back(tok);
}

void StringList::setText(const ustring &text) {
	char *buf = new char[text.bytes() + 1], *sbuf = buf;
	strcpy(buf, text.c_str());

	loadFromChar(buf);

	delete[] sbuf;
}

void StringList::split(const char *text, char delimiter) {
	const char *c = text;
	while(*text) {
		if(*text == delimiter) {
			add(ustring(c, text - c));
			c = text + 1;
		}
		text++;
	}
	if(c != text)
		add(c);
}
