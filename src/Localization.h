#ifndef HIASM_LOCALIZATION_H
#define HIASM_LOCALIZATION_H
#include <string>
// Stable UTF-8 labels. Identifiers, scheme properties and user text are never translated.
const char *ui_tr(const char *english);
const std::string &ui_language();
void ui_set_language(const std::string &language);
// First supported system preference; English is the fallback.
std::string ui_system_language();
#endif
