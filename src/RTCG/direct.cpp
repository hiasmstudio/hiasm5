#include <string>

#include "dll.h"
#include "direct.h"
#include "share.h"
#include <unistd.h>

std::string __level;
std::string __call_level;

void _log_text_(std::string text) {
	FILE* f = NULL;
	printf("%s", text.c_str());
	f = fopen("rtlg_log.txt", "a+");
	fputs(text.c_str(), f);
	fclose(f);
}

void _log_clear_() {
	unlink("rtlg_log.txt");
	__call_level = "";
}

void inclevel() {
	__level.append(" ");
}

void declevel() {
	if(__level.size() > 0)
		__level.resize(__level.size() - 1);
}
