/*
 * SdkSerializer.cpp
 *
 * Writes a scheme snapshot for src/hiasm4_cgbridge. Answers that CGT would
 * compute (dynamic point names, real links, string forms) are produced by the
 * IDE's own CGT functions so the bridge and the native path cannot diverge.
 * Flags are converted to the HiAsm 4 encoding expected by Delphi CodeGen.
 */

#include "SdkSerializer.h"
#include "CodeGen.h"
#include "ElementConfig.h"
#include "ElementPoint.h"
#include "ElementProperty.h"
#include "Pack.h"

#include <glib.h>
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

// CGT implementations from CodeGen.cpp.
ElementPoint *ptGetRLinkPoint(ElementPoint *p);
const char *pt_dpeGetName(ElementPoint *p);
int propToInteger(ElementProperty *prop);
const char *propToString(ElementProperty *p);

namespace {

// HiAsm 4 element flags (Elements/CGTShare.pas).
enum { V4_NO_DELETE = 0x2, V4_IS_EDIT = 0x8, V4_IS_MULTI = 0x40, V4_IS_SELECT = 0x100,
       V4_IS_PARENT = 0x200, V4_IS_HIDE = 0x400 };

struct Writer {
	std::string out;
	std::map<SDK*, int> sdkIds;
	std::map<Element*, int> elementIds;
	std::vector<SDK*> sdks;
	std::vector<Element*> elements;

	void str(const char *text) {
		gchar *valid = g_utf8_make_valid(text ? text : "", -1);
		out += '"';
		for(const char *c = valid; *c; ++c) {
			switch(*c) {
			case '"': out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			default:
				if((unsigned char)*c < 0x20) {
					char buf[8]; g_snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)*c); out += buf;
				} else out += *c;
			}
		}
		out += '"';
		g_free(valid);
	}
	void str(const ustring &text) { str(text.c_str()); }
	void num(int value) { out += std::to_string(value); }
	void real(double value) {
		char buf[G_ASCII_DTOSTR_BUF_SIZE];
		out += std::isfinite(value) ? g_ascii_dtostr(buf, sizeof(buf), value) : "0";
	}
	// Separator before the next object member or array item.
	void sep() { if(out.back() != '{' && out.back() != '[') out += ','; }
	void key(const char *name) { sep(); str(name); out += ':'; }

	void collect(SDK *sdk) {
		if(!sdk || sdkIds.count(sdk)) return;
		sdkIds[sdk] = (int)sdks.size(); sdks.push_back(sdk);
		for(Element *e : sdk->elements)
			if(!elementIds.count(e)) { elementIds[e] = (int)elements.size(); elements.push_back(e); }
		for(Element *e : sdk->elements) collect(e->sdk);
	}
	int id(Element *e) const { auto i = elementIds.find(e); return i == elementIds.end() ? -1 : i->second; }
	int id(SDK *s) const { auto i = sdkIds.find(s); return i == sdkIds.end() ? -1 : i->second; }

	static int v4Flags(Element *e) {
		int flags = 0;
		if(e->flag & ELEMENT_FLG_IS_NODELETE) flags |= V4_NO_DELETE;
		// Form-editor elements (a [Edit] section in the INI): Delphi CodeGen passes them the parent control
		// and calls Init. EditMulti and EditMultiEx have no [Edit] section; their Create takes no parameters.
		ElementConfig *conf = e->tpl ? e->tpl->conf : nullptr;
		if(conf && !conf->editClassName.empty())
			flags |= V4_IS_EDIT;
		if(e->sdk) flags |= V4_IS_MULTI;
		if(e->flag & ELEMENT_FLG_IS_SELECT) flags |= V4_IS_SELECT;
		if(e->isParent()) flags |= V4_IS_PARENT;
		// HubEx, GetDataEx and InfoTip are editor-only; HiAsm 4 marks them "not build".
		if(e->flag & ELEMENT_FLG_IS_SYSTEM) flags |= V4_IS_HIDE;
		return flags;
	}

	void pointRef(const char *name, ElementPoint *p) {
		if(!p || !p->parent || id(p->parent) < 0) return;
		const int index = p->parent->points.indexOf(p);
		if(index < 0) return;
		key(name); out += '['; num(id(p->parent)); out += ','; num(index); out += ']';
	}
	// Typed payload of a property or array item, matching what CGT returns for it.
	void value(ElementProperty *p) {
		if(!p->value) return;
		switch(p->type) {
		case data_int: case data_combo: case data_comboEx: case data_flags: case data_color:
			key("int"); num(propToInteger(p)); break;
		case data_real:
			key("real"); real(*(double*)p->value); break;
		case data_data: {
			TData *d = (TData*)p->value;
			key("data"); out += '{';
			key("type"); num(d->type); key("str"); str(d->sdata);
			key("int"); num(d->idata); key("real"); real(d->rdata);
			out += '}';
			break;
		}
		case data_array: {
			ArrayValue *a = (ArrayValue*)p->value;
			key("array"); out += '{'; key("type"); num(a->type); key("items"); out += '[';
			for(size_t i = 0; i < a->size(); ++i) {
				// Same temporary-property construction as arrGetItem().
				std::unique_ptr<ElementProperty> item(new ElementProperty(NULL, a->at(i).name, ustring(), a->type));
				if(!item->value || !a->at(i).data) continue;
				DataValue::copyValue(a->type, item->value, a->at(i).data);
				sep(); out += '{'; key("name"); str(a->at(i).name); key("type"); num(a->type);
				value(item.get()); out += '}';
			}
			out += "]}";
			break;
		}
		case data_font: {
			FontRecord *f = (FontRecord*)p->value;
			key("font"); out += '{';
			key("name"); str(f->name); key("size"); num(f->size); key("style"); num(f->style);
			key("color"); num(f->color); key("charset"); num(f->charset);
			out += '}';
			break;
		}
		case data_icon: case data_stream: case data_bitmap: case data_wave: case data_jpeg: case data_menu:
			// Original HiAsm 4 text ("[ZIP...]" or "[hex]"); the bridge decodes it.
			if(!((ustring*)p->value)->empty()) { key("resource"); str(*(ustring*)p->value); }
			break;
		default: break;
		}
		if(p->type != data_pixbuf) if(const char *text = propToString(p)) { key("str"); str(text); }
	}

	void prop(ElementProperty *p) {
		sep(); out += '{';
		key("name"); str(p->name); key("type"); num(p->type);
		key("default"); out += p->isDefault() ? "true" : "false";
		value(p);
		out += '}';
	}
	// HiAsm 4 names a method declared as doStart%Precision% by the index of the property value
	// (doStart0 for Precision = Millisecond). The C++ model keeps the plain name; CodeGen needs the real one.
	static std::string legacyPointName(ElementPoint *p) {
		Element *e = p->parent;
		if(!e || !e->tpl || !e->tpl->conf) return p->name;
		for(ConfMethod *m : e->tpl->conf->mtd)
			if(m->name == p->name && !m->prop.empty())
				if(ElementProperty *prop = e->props.getByName(m->prop)) return p->name + std::to_string(prop->readInt());
		for(ConfMethod *m : e->tpl->conf->def)
			if(m->name == p->name && !m->prop.empty())
				if(ElementProperty *prop = e->props.getByName(m->prop)) return p->name + std::to_string(prop->readInt());
		return p->name;
	}
	void point(ElementPoint *p) {
		sep(); out += '{';
		key("name"); str(legacyPointName(p)); key("info"); str(p->info);
		key("type"); num(p->type); key("dataType"); num(p->dataType);
		key("index"); num(p->parent->points.indexOfType(p));
		key("dpe"); str(pt_dpeGetName(p));
		pointRef("link", p->point);
		pointRef("rlink", p->point ? ptGetRLinkPoint(p) : nullptr);
		out += '}';
	}

	void element(Element *e) {
		sep(); out += '{';
		ElementConfig *conf = e->tpl ? e->tpl->conf : nullptr;
		key("class"); str(e->tpl ? e->tpl->name.c_str() : "");
		key("sub"); str(conf ? conf->sub.c_str() : "");
		key("interfaces"); str(conf ? conf->interfaces.c_str() : "");
		key("inherit"); str(conf ? conf->inherit.c_str() : "");
		key("codename"); str(e->codeName);
		key("flags"); num(v4Flags(e)); key("classIndex"); num(e->cIndex);
		key("eid"); num(e->id); key("group"); num(e->groupId());
		key("x"); num(e->x); key("y"); num(e->y); key("w"); num(e->width); key("h"); num(e->height);
		key("parent"); num(id(e->parent)); key("inner"); num(id(e->sdk));
		if(e->isELink()) { key("linkIs"); out += "true"; key("linkMain"); num(id(e->getMainELink())); }
		key("props"); out += '[';
		for(ElementProperty *p : e->props) prop(p);
		out += ']';
		key("points"); out += '[';
		for(ElementPoint *p : e->points) point(p);
		out += "]}";
	}

	// HiAsm 4 names a project after its scheme file: the name without folder and extension.
	static std::string schemeName(const ustring &fileName) {
		gchar *base = g_path_get_basename(fileName.c_str());
		std::string name = base;
		g_free(base);
		const auto dot = name.rfind('.');
		if(dot != std::string::npos && dot > 0) name.erase(dot);
		return name;
	}

	void write(MSDK *root, const ustring &projectPath) {
		collect(root);
		out += '{';
		key("version"); num(1);
		key("params"); out += '{';
		key("projectPath"); str(projectPath);
		key("projectName"); str(schemeName(root->fileName));
		out += '}';
		key("sdks"); out += '[';
		for(SDK *s : sdks) {
			sep(); out += '{';
			key("parent"); num(id(s->parent));
			key("elements"); out += '[';
			for(Element *e : s->elements) { sep(); num(id(e)); }
			out += "]}";
		}
		out += ']';
		key("elements"); out += '[';
		for(Element *e : elements) element(e);
		out += "]}";
	}
};

}

bool sdkSerialize(MSDK *sdk, const ustring &jsonPath, const ustring &projectPath) {
	if(!sdk) return false;
	Writer writer;
	writer.write(sdk, projectPath);
	GError *error = nullptr;
	if(!g_file_set_contents(jsonPath.c_str(), writer.out.data(), (gssize)writer.out.size(), &error)) {
		cgt_on_debug.run((ustring("!Cannot write scheme snapshot: ") + (error ? error->message : jsonPath.c_str())).c_str());
		g_clear_error(&error);
		return false;
	}
	return true;
}
