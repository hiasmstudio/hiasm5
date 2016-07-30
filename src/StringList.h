/*
 * StringList.h
 *
 *  Created on: Jul 1, 2010
 *      Author: dilma
 */

#ifndef STRINGLIST_H_
#define STRINGLIST_H_

#include "share.h"

/*! \class StringList
    \brief string list

    Provides methods for working with a list of strings
*/

class StringList : public std::vector<ustring> {
	private:
		/**
		 * Load string list from memory buffer
		 * @param buf memory buffer
		 */
		void loadFromChar(char *buf);
	public:
		/**
		 * Default constructor
		 */
		StringList() {}
		/**
		 * Create string list and load it from file
		 * @param fileName name of file to load
		 */
		explicit StringList(const ustring &fileName) { loadFromFile(fileName); }
		/**
		 * Load string list from file
		 * @param fileName name of file to load
		 */
		void loadFromFile(const ustring &fileName);
		/**
		 * Save string list to file
		 * @param fileName name of file to save
		 */
		void saveToFile(const ustring &fileName);
		/**
		 * Return all lines as text with END_LINE delimiter
		 * @return one long string line with END_LINE delimiter
		 */
		ustring text();
		/**
		 * Parse lines from one long string line
		 * @param text long string line with \n or \r\n delimiter
		 */
		void setText(const ustring &text);
		/**
		 * Add new line to end of list
		 * @param text new line to add
		 */
		void add(const ustring &text) { push_back(text); }
		/**
		 * Get index of string
		 * @param text search string
		 * @return index of text or -1 if not found
		 */
		int indexOf(const ustring &text);

		/**
		 * Split text by delimiter and create list from all parts
		 * @param text source string
		 * @param delimiter split text by this char
		 */
		void split(const char *text, char delimiter);

		/**
		 * Add new line to end of list
		 * @param text new line to add
		 */
		StringList &operator << (const ustring &text) {
			add(text);
			return *this;
		}
		/**
		 * Parse lines from one long string line
		 * @param text long string line with \n or \r\n delimiter
		 */
		StringList &operator = (const ustring &text) {
			setText(text);
			return *this;
		}
		/**
		 * Return all lines as text with END_LINE delimiter
		 * @return one long string line with END_LINE delimiter
		 */
		inline operator ustring() { return text(); }
};

#endif /* STRINGLIST_H_ */
