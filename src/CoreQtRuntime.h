#pragma once

class Element;
class ElementPoint;
struct _TData;

namespace CoreQtRuntime {
bool start(const char *runtimeDirectory);
void stop();
void pump();
bool closeRequested();
bool active();
bool owns(Element *element);
bool isWidget(Element *element);
bool supports(Element *element);
bool prepare(Element *element, Element *parent);
bool show(Element *element);
bool work(ElementPoint *point, _TData *data);
bool read(ElementPoint *point, _TData *data);
}
