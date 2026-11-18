#include "share.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>

FILE *openUtf8(const char *path, const char *mode) {
#ifdef _WIN32
	const auto widePath = std::filesystem::u8path(path).wstring();
	const auto wideMode = std::filesystem::u8path(mode).wstring();
	return _wfopen(widePath.c_str(), wideMode.c_str());
#else
	return fopen(path, mode);
#endif
}

void lowerCase(char *to, const char *from) {
	while(*from) {
		if(*from >= 'A' && *from <= 'Z')
			*to = (*from - 'A') + 'a';
		else
			*to = *from;
		from++;
		to++;
	}
	*to = '\0';
}

void upperCase(char *to, const char *from) {
	while(*from) {
		if(*from >= 'a' && *from <= 'z')
			*to = (*from - 'a') + 'A';
		else
			*to = *from;
		from++;
		to++;
	}
	*to = '\0';
}

void int_to_str(intptr_t i, char *buf) {
	sprintf(buf,  "%d",  (int)i);
}

int min(int x, int y) {
	return x < y ? x : y;
}
