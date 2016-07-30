/*
 * Elements.cpp
 *
 *  Created on: 07.05.2010
 *      Author: dilma
 */

#include "Elements.h"

//--------------------------------------------------------------------------------------------------

Element *createElement(const ustring &name, SDK *sdk, gdouble x, gdouble y) {
	PackElement *pe = sdk->getMSDK()->pack->getElementByName(name);
	if(pe) {
		if(pe->conf->cClass == "HCWE_Form")
			return new ElementTplForm(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_Button")
			return new ElementTplButton(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_Edit")
			return new ElementTplEdit(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_Label")
			return new ElementTplLabel(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_CheckBox")
			return new ElementTplCheckBox(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_ScrollBar")
			return new ElementTplScrollBar(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_ListBox")
			return new ElementTplListBox(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_Memo")
			return new ElementTplMemo(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_Image")
			return new ElementTplImage(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_GroupBox")
			return new ElementTplGroupBox(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_FileChooserButton")
			return new ElementTplFileChooserButton(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_ColorButton")
			return new ElementTplColorButton(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_Dialog")
			return new ElementTplDialog(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_ProgressBar")
			return new ElementTplProgressBar(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_ComboBox")
			return new ElementTplComboBox(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_Fixed")
			return new ElementTplFixed(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_VBox" || pe->conf->cClass == "HCWE_HBox")
			return new ElementTplVBox(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_VButtonBox" || pe->conf->cClass == "HCWE_HButtonBox")
			return new ElementTplButtonBox(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_VPaned" || pe->conf->cClass == "HCWE_HPaned")
			return new ElementTplPaned(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_Expander")
			return new ElementTplExpander(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCWE_HandleBox")
			return new ElementTplHandleBox(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCMessage")
			return new ElementCoreMessage(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCHub")
			return new ElementCoreHub(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCDoData")
			return new ElementCoreDoData(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCStrCat")
			return new ElementCoreStrCat(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCStrPos")
			return new ElementCoreStrPos(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCFormatStr")
			return new ElementCoreFormatStr(pe, sdk, x ,y);
		else if(pe->conf->cClass == "HCCopy")
			return new ElementCoreCopy(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCStrPart")
			return new ElementCoreStrPart(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCReplace")
			return new ElementCoreReplace(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCBlockFind")
			return new ElementCoreBlockFind(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCFor")
			return new ElementCoreFor(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCIfElse")
			return new ElementCoreIfElse(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCRandom")
			return new ElementCoreRandom(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCTimer")
			return new ElementCoreTimer(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCMath")
			return new ElementCoreMath(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCMemory")
			return new ElementCoreMemory(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCIndexToChanel")
			return new ElementCoreIndexToChanel(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCChanelToIndex")
			return new ElementCoreChanelToIndex(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCExecute")
			return new ElementCoreExecute(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCConsole")
			return new ElementCoreConsole(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCClipboard")
			return new ElementCoreClipboard(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCFileSearch")
			return new ElementCoreFileSearch(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCTimeCounter")
			return new ElementCoreTimeCounter(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCMultiElement")
			return new ElementCoreMulti(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCEditMulti")
			return new ElementCoreEditMulti(pe, sdk, x, y);
		else if(pe->conf->cClass == "WinElement")
			return new ElementWinElement(pe, sdk, x, y);
		else if(pe->conf->cClass == "EditMulti")
			return new ElementEditMulti(pe, sdk, x, y);
		else if(pe->conf->cClass == "EditMultiEx")
			return new ElementEditMultiEx(pe, sdk, x, y);
		else if(pe->conf->cClass == "DPElement")
			return new ElementDynPoints(pe, sdk, x, y);
		else if(pe->conf->cClass == "DPLElement")
			return new ElementNamedDynPoints(pe, sdk, x, y);
		else if(pe->conf->cClass == "MultiElement" || pe->conf->cClass == "MultiElementEx")
			return new ElementMulti(pe, sdk, x, y);
		else if(pe->conf->cClass == "Hub")
			return new ElementCoreHub(pe, sdk, x, y);
		else if(pe->conf->cClass == "HubEx")
			return new ElementHubEx(pe, sdk, x, y);
		else if(pe->conf->cClass == "GetDataEx")
			return new ElementGetDataEx(pe, sdk, x, y);
		else if(pe->conf->cClass == "ITElement")
			return new ElementInfoTip(pe, sdk, x, y);
		else if(pe->conf->cClass == "VTElement")
			return new ElementVisualText(pe, sdk, x, y);
		else if(pe->conf->cClass == "CapElement")
			return new ElementCaption(pe, sdk, x, y);
		else if(pe->conf->cClass == "Debug")
			return new ElementDebug(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCEventFromData")
			return new ElementCoreEventFromData(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCSaveDialog")
			return new ElementCoreSaveDialog(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCOpenDialog")
			return new ElementCoreOpenDialog(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCHiTransmitter")
			return new ElementCoreTransmitter(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCStrList")
			return new ElementCoreStrList(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCArrayEnum")
			return new ElementCoreArrayEnum(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCHTTPText")
			return new ElementCoreHTTPText(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCDS_SQLite")
			return new ElementCoreDS_SQLite(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCDSC_Exec")
			return new ElementCoreDSC_Exec(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCDSC_QueryScalar")
			return new ElementCoreDSC_QueryScalar(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCDSC_Query")
			return new ElementCoreDSC_Query(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCConvertor")
			return new ElementCoreConvertor(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCPopupMenu")
			return new ElementCorePopup(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCDelete")
			return new ElementCoreDelete(pe, sdk, x, y);
		else if(pe->conf->cClass == "HCInsert")
			return new ElementCoreInsert(pe, sdk, x, y);

		return new Element(pe, sdk, x, y);
	}
	else {
		std::cout << "Element not found: " << name.c_str() << std::endl;
		// TODO pointelement create
		return new Element(sdk->getMSDK()->pack->getElementByName("PointElement"), sdk, x, y);
	}
}

//--------------------------------------------------------------------------------------------------
ElementWinElement::ElementWinElement(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y)
{
	cIndex = CI_WinElement;
}

void ElementWinElement::place(int x, int y) {
	Element::place(x, y);

	*(int*)props[0]->value = GRID_FRM_X(x);
	*(int*)props[1]->value = GRID_FRM_Y(y);
}

ElementDynPoints::ElementDynPoints(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y)
{
	int l = tpl->conf->sub.length();
	char *sub = new char[l+1];
	tpl->conf->sub.copy(sub, l);
	sub[l] = '\0';
	for(int i = 0; i < 4; i++) {
		char *pt = getTok(&sub, ',');
		ElementProperty *p = props.getByName(getTok(&pt, '|'));
		dynInfoList[i].prop = p;
		if(p) {
			dynInfoList[i].name = pt;
			on_change_property(p);
		}
		else
			dynInfoList[i].name = ustring();
	}

	cIndex = CI_DPElement;
}

void ElementDynPoints::init() {
	Element::init();

	for(int i = 0; i < 4; i++)
		if(dynInfoList[i].prop)
			on_change_property(dynInfoList[i].prop);
}

void ElementDynPoints::connectToPoint(ElementPoint *point) {
	int i = point->getPair() - 1;
	if(dynInfoList[i].prop) {
		if(!getFirstFreePoint(i + 1)) {
			int c = dynInfoList[i].prop->readInt() + 1;
			dynInfoList[i].prop->setValue(&c);
			on_change_property(dynInfoList[i].prop);
		}
	}

	Element::connectToPoint(point);
}

void ElementDynPoints::on_change_property(ElementProperty *prop) {
	Gdk::Rectangle r = drawRect();
	for(int i = 0; i < 4; i++)
		if(dynInfoList[i].prop == prop) {
			int count = getPointsCount(i+1);
			int val = prop->readInt();

			if(val < 0 || val > MAX_POINTS)
				prop->setValue(&count);
			else {
				if(count < val) {	// add
					for(int j = count; j < val; j++) {
						ustring n = dynInfoList[i].name + int_to_str(j+1);
						DEBUG_MSG("Add point: " << n.c_str())
						addPoint(n, "", i+1);
					}
				}
				else {				// remove
					ElementPoint *p;
					while((p = getLastFreePoint(i+1)) && count > val) {
						removePoint(p);
						count--;
					}
					if(count != val)
						prop->setValue(&count);
				}
				rePosPoints();
				r.join(drawRect());
				parent->on_redraw_rect.run(&r);
			}
		}

	Element::on_change_property(prop);
}

//--------------------------------------------------------------------------------------------------

ElementNamedDynPoints::ElementNamedDynPoints(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y)
{
	int l = tpl->conf->sub.length();
	char *sub = new char[l+1];
	tpl->conf->sub.copy(sub, l);
	sub[l] = '\0';
	for(int i = 0; i < 4; i++) {
		char *pt = getTok(&sub, ',');
		ElementProperty *p = props.getByName(getTok(&pt, '|'));
		dynInfoList[i].prop = p;
		if(p) {
			//dynInfoList[i].name = pt;
			on_change_property(p);
		}
	}

	cIndex = CI_DPLElement;
}

void ElementNamedDynPoints::init() {
	Element::init();

	for(int i = 0; i < 4; i++)
		if(dynInfoList[i].prop)
			on_change_property(dynInfoList[i].prop);
}

bool pointInList(const StringList &list, const ustring &name) {
	ustring n, h;
	for(unsigned int i = 0; i < list.size(); i++) {
		parseHintName(list[i], n, h);
		if(n == name)
			return true;
	}
	return false;
}

void ElementNamedDynPoints::on_change_property(ElementProperty *prop) {
	Gdk::Rectangle r = drawRect();
	for(int i = 0; i < 4; i++)
		if(dynInfoList[i].prop == prop) {
			StringList list;
			list = prop->readStr();

			for(unsigned int p = 0; p < points.size(); p++)
				if(points[p]->type == (i + 1) && !pointInList(list, points[p]->name) && !tpl->conf->def.getByName(points[p]->name)) {
					removePoint(points[p]);
					p--;
				}

			ustring n, h;
			for(StringList::iterator line = list.begin(); line != list.end(); line++) {
				parseHintName(*line, n, h);
				if(n.length() && !points.getByName(n)) {
					addPoint(n, h, i + 1);
				}
			}
			rePosPoints();
			r.join(drawRect());
			parent->on_redraw_rect.run(&r);
		}

	Element::on_change_property(prop);
}

//--------------------------------------------------------------------------------------------------

ElementMulti::ElementMulti(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y)
{
	this->sdk = new SDK();
	this->sdk->parent = this;
	ustring cl = tpl->conf->editClassName;
	if(cl.empty()) {
		cl = (pe->name == "MultiElement") ? "EditMulti" : "EditMultiEx";
	}
	child = this->sdk->add(cl, 14, 14);

	if(!tpl->conf->sub.empty()) {
		Element *e = this->sdk->add(tpl->conf->sub, POINT_SPACE*8, POINT_SPACE*8);
		e->flag |= ELEMENT_FLG_IS_PARENT;
	}

	sysProps.push_back(new ElementProperty(this, "Icon", "Icon on element", data_stock));
	//icon = parent->render_icon(StockID(*(ustring*)value), ICON_SIZE_MENU, ustring());

	cIndex = CI_MultiElement;
}

ElementMulti::~ElementMulti() {
	if(isELink()) {
		if(isMainELink()) {
			Element *e = *(++elinkList->begin());
			sdk->parent = e;
			MSDK *msdk = sdk->getMSDK();
			msdk->sdkLib.remove(this);
			if(elinkList->size() > 2)
				msdk->sdkLib.push_back(e);
		}
	}
	else
		delete sdk;
}

void ElementMulti::saveToText(ustring &text, const ustring offset, int flags) {
	Element::saveToText(text, offset, flags);
	if(!isELink() || isMainELink()) {
		text += offset + "BEGIN_SDK"LINE_END;
		sdk->saveToText(text, offset + ustring(" "));
		text += offset + "END_SDK"LINE_END;
	}
}

void ElementMulti::createELink(Element *owner) {
	Element::createELink(owner);

	ElementMulti *mowner = dynamic_cast<ElementMulti *>(getMainELink());
	if(mowner) {
		delete sdk;
		sdk = mowner->sdk;
		child = mowner->child;
		child->props.change();
	}
}

void ElementMulti::on_change_property(ElementProperty *prop) {
	Element::on_change_property(prop);

	if(prop == sysProps[2]) {
		if(stockRender)
			icon = stockRender(prop->readStr());
		invalidate();
	}
}

void ElementMulti::drawIcon(DrawContext dc) {
	Element::drawIcon(dc);
	if(icon) {
		dc->save();
		dc->translate(x + width - 14, y + height - 14);
		Gdk::Cairo::set_source_pixbuf(dc, icon, 0.0, 0.0);
		dc->rectangle (0, 0, 16, 16);
		dc->clip();
		dc->paint();
		dc->restore();
	}
}

//--------------------------------------------------------------------------------------------------

ElementEditMulti::ElementEditMulti(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementDynPoints(pe, sdk, x, y), ElementEditMultiBase(pe, sdk, x, y)
{
	static const char *names[] = {"onEvent", "doWork", "Data", "Var"};
	for(int i = 0; i < 4; i++) {
		dynInfoList[i].name = names[i];
		dynInfoList[i].prop = props[i];
	}
	cIndex = CI_EditMulti;
}

int ElementEditMulti::isDynPointProp(ElementProperty *prop) {
	for(int i = 0; i < 4; i++)
		if(prop == dynInfoList[i].prop)
			return i;
	return -1;
}

void ElementEditMulti::changePoints(Element *multi, int i) {
	static const unsigned char mt_types[] = {pt_event, pt_work, pt_data, pt_var};

	int c = multi->getPointsCount(mt_types[i]);
	int val = getPointsCount(i+1);

	if(c < val) {	// add
		for(int j = c; j < val; j++)
			multi->addPoint(dynInfoList[i].name + int_to_str(j+1), "", mt_types[i]);
	}
	else if(c > val) {				// remove
		ElementPoint *p;
		while((p = multi->getLastPoint(mt_types[i])) && c > val) {
			multi->removePoint(p);
			c--;
		}
	}
	multi->rePosPoints();
}

//--------------------------------------------------------------------------------------------------

ElementEditMultiEx::ElementEditMultiEx(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementNamedDynPoints(pe, sdk, x, y), ElementEditMultiBase(pe, sdk, x, y)
{
	dynInfoList[0].prop = props[1];
	dynInfoList[1].prop = props[0];
	dynInfoList[2].prop = props[3];
	dynInfoList[3].prop = props[2];
	cIndex = CI_EditMultiEx;
}

int ElementEditMultiEx::isDynPointProp(ElementProperty *prop) {
	for(int i = 0; i < 4; i++)
		if(prop == dynInfoList[i].prop)
			return i;
	return -1;
}

void ElementEditMultiEx::changePoints(Element *multi, int i) {
	static const unsigned char mt_types[] = {pt_event, pt_work, pt_data, pt_var};

	StringList list;
	list = dynInfoList[i].prop->readStr();

	for(unsigned int p = 0; p < multi->points.size(); p++)
		if(multi->points[p]->type == mt_types[i] && list.indexOf(multi->points[p]->name) == -1) {
			multi->removePoint(multi->points[p]);
			p--;
		}

	ustring n, h;
	for(StringList::iterator line = list.begin(); line != list.end(); line++)
		if(!multi->points.getByName(*line)) {
			parseHintName(*line, n, h);
			if(n.length())
				multi->addPoint(n, h, mt_types[i]);
		}
	multi->rePosPoints();
}

//--------------------------------------------------------------------------------------------------

static const struct {
	const gchar *name;
	const gchar *info;
} sprops[] = {
	{"Width", "Width of element in editor"},
	{"Height", "Height of element in editor"},
	{"VOffset", "Points offset at top edge"},
	{"HOffset", "Points offset at left edge"}
};

ElementEditMultiBase::ElementEditMultiBase(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementGrip()
{
	for(int i = 0; i < 4; i++)
		sysProps.push_back(new ElementProperty(this, sprops[i].name, sprops[i].info, data_int));

	width = 300;
	height = 200;
	ElementProperty *prop = sysProps.getByName(sprops[0].name);
	fwidth = (int*)prop->value;
	*fwidth = width;
	prop->setAsDefault();
	prop = sysProps.getByName(sprops[1].name);
	fheight = (int*)prop->value;
	*fheight = height;
	prop->setAsDefault();

	voff = (int*)sysProps.getByName(sprops[2].name)->value;
	hoff = (int*)sysProps.getByName(sprops[3].name)->value;

	state = -1;

	flag |= ELEMENT_FLG_IS_NODELETE;

	// --- grips
	vgrip = addGrip(width - 20, 0, 20, 3, 0, POINT_SPACE, 3);
	vgrip->addValue(voff, 0, 1);
	hgrip = addGrip(0, height - 20, 3, 20, POINT_SPACE, 0, 2);
	hgrip->addValue(hoff, 1, 0);
}

void ElementEditMultiBase::init() {
	props.change();

	vgrip->y = *voff;
	hgrip->x = *hoff;
}

void ElementEditMultiBase::on_change_property(ElementProperty *prop) {
	Element::on_change_property(prop);

	if(prop->value == fwidth) {
		width = *fwidth;
		rePosPoints();
		parent->on_redraw_rect.run(NULL);
	}
	else if(prop->value == fheight) {
		height = *fheight;
		rePosPoints();
		parent->on_redraw_rect.run(NULL);
	}
	else if(prop->value == voff) {
		*voff = (*voff/POINT_SPACE) * POINT_SPACE;
		vgrip->move(0, *voff);
		rePosPoints();
		invalidate();
	}
	else if(prop->value == hoff) {
		*hoff = (*hoff/POINT_SPACE) * POINT_SPACE;
		hgrip->move(*hoff, 0);
		rePosPoints();
		invalidate();
	}

	int i = isDynPointProp(prop);
	if(i != -1) {
		if(parent->parent->isELink())
			for(ElementsList::iterator e = parent->parent->elinkList->begin(); e != parent->parent->elinkList->end(); e++)
				changePoints(*e, i);
		else
			changePoints(parent->parent, i);
	}
}

void ElementEditMultiBase::drawBody(DrawContext dc, double zoom) {
	dc->set_source_rgba(0.5, 0.5, 0.5, isSelect() ? 0.9 : 0.5);
	dc->set_line_width(2);
	dc->move_to(x, y);
	dc->rel_line_to(width, 0);
	dc->rel_line_to(0, height);
	dc->rel_line_to(-width, 0);
	dc->rel_line_to(0, -height);
	dc->stroke();

	if(isSelect())
		drawGrips(dc);

	dc->set_line_width(1);
}

bool ElementEditMultiBase::checkCollise(gdouble x, gdouble y) {
	if(isSelect() && getGripAtPos(x - this->x, y - this->y))
		return true;
	if(Element::checkCollise(x, y))
		return getStateAtPos(x, y) != -1;
	return false;
}

bool ElementEditMultiBase::checkColliseRect(gdouble x1, gdouble y1, gdouble x2, gdouble y2) {
	if(Element::checkColliseRect(x1, y1, x2, y2))
		return !((x1 > x + 5)&&(x2 < x + width - 5)&&(y1 > y + 5)&&(y2 < y + height - 5));
	return false;
}

void ElementEditMultiBase::reSize() {
	Element::reSize();
	width += *hoff;
	height += *voff;

	if(*fwidth > width)
		width = *fwidth;
	else
		*fwidth = width;
	if(*fheight > height)
		height = *fheight;
	else
		*fheight = height;

	vgrip->x = width - vgrip->width;
	hgrip->y = height - hgrip->height;
}

void ElementEditMultiBase::rePosPoints() {
	Element::rePosPoints();
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		switch((*p)->type) {
			case pt_event:
				(*p)->pos->x -= width;
				(*p)->pos->y += *voff;
				break;
			case pt_work:
				(*p)->pos->x += width;
				(*p)->pos->y += *voff;
				break;
			case pt_data:
				(*p)->pos->y += height;
				(*p)->pos->x += *hoff;
				break;
			case pt_var:
				(*p)->pos->y -= height;
				(*p)->pos->x += *hoff;
				break;
		}
}

int ElementEditMultiBase::getStateAtPos(gdouble x, gdouble y) {
	gdouble dx = x - this->x;
	gdouble dy = y - this->y;
	if(dy < 5 || dx < 5)
		return 0;
	else if(dx > width - 5 && dy > height - 5)
		return 3;
	else if(dx > width - 5)
		return 1;
	else if(dy > height - 5)
		return 2;

	return -1;
}

bool ElementEditMultiBase::mouseDown(gdouble x, gdouble y, int button) {
	GripInfo *g = gripDown(x - this->x, y - this->y);
	if(isSelect() && g)
		return true;

	state = getStateAtPos(x, y);
	oldx = x;
	oldy = y;
	return state;
}

void ElementEditMultiBase::mouseMove(gdouble x, gdouble y) {
	if(isActiveGrip()) {
		gripMove(x - this->x, y - this->y);
		return;
	}

	Gdk::Rectangle r = drawRect();
	switch(state) {
		case 1:
		case 3:
			(*fwidth) += x - oldx;
			if(state == 1)
				break;
		case 2:
			(*fheight) += y - oldy;
	}
	int oldw = width, oldh = height;
	reSize();
	rePosPoints();
	oldx += width - oldw;
	oldy += height - oldh;

	r.join(drawRect());
	parent->on_redraw_rect.run(&r);
}

void ElementEditMultiBase::mouseUp(gdouble x, gdouble y, int button) {
	if(isActiveGrip())
		gripUp(x - this->x, y - this->y);
	else
		state = -1;
}

int ElementEditMultiBase::mouseCursor(gdouble x, gdouble y) {
	if(isActiveGrip())
		return isActiveGrip()->cursor;

	GripInfo *g = getGripAtPos(x - this->x, y - this->y);
	if(isSelect() && g)
		return g->cursor;

	if(state > 0)
		return state + 1;

	return getStateAtPos(x, y) + 1;
}

//--- ElementGrip

void ElementEditMultiBase::on_grip_change(GripInfo *grip) {
	rePosPoints();
	invalidate();
}

void ElementEditMultiBase::on_grip_draw(DrawContext dc, GripInfo *grip) {
	dc->set_source_rgb(0.0, 0.0, 1.0);
	if(grip->cursor == 3) {
		dc->move_to(x + width, y + *voff);
		dc->rel_line_to(-20, 0);
	}
	else {
		dc->move_to(x + *hoff, y + height);
		dc->rel_line_to(0, -20);
	}
	dc->stroke();
}

bool ElementEditMultiBase::on_grip_can_change(GripInfo *grip, int &dx, int &dy) {
	if(grip->cursor == 3) {
		int pe = getPointsCount(pt_event);
		int pw = getPointsCount(pt_work);
		return (*voff + dy >= 0) && (*voff + dy + max(pe, pw)*POINT_SPACE < height);
	}
	else {
		int pv = getPointsCount(pt_var);
		int pd = getPointsCount(pt_data);
		return (*hoff + dx >= 0) && (*hoff + dx + max(pv, pd)*POINT_SPACE < width);
	}
}

//--------------------------------------------------------------------------------------------------

ElementDynGeometry::ElementDynGeometry(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementGrip()
{
	static const unsigned char cur[8] = {1, 3, 1, 2, 1, 3, 1, 2};
	for(int i = 0; i < 8; i++) {
		GripInfo *g = addGrip(0, 0, 5, 5, 1, 1, cur[i]);
		if(i == 0 || i == 6 || i == 7) {
			g->addValue(&this->x, 1, 0);
			g->addValue(&width, -1, 0);
		}
		if(i < 3) {
			g->addValue(&this->y, 0, 1);
			g->addValue(&height, 0, -1);
		}
		if(i >= 2 && i <= 4)
			g->addValue(&width, 1, 0);
		if(i >= 4 && i <= 6)
			g->addValue(&height, 0, 1);
	}
	rePosGrids();

	for(int i = 0; i < 2; i++) {
		ElementProperty *ep = new ElementProperty(this, sprops[i].name, sprops[i].info, data_int);
		sysProps.push_back(ep);
		if(i)
			fheight = (int*)ep->value;
		else
			fwidth = (int*)ep->value;
	}
	*fheight = height;
	*fwidth = width;
}

void ElementDynGeometry::init() {
	height = *fheight;
	width = *fwidth;
	rePosGrids();
}

void ElementDynGeometry::rePosGrids() {
	static const double kx[8] = {0, 0.5, 1, 1, 1, 0.5, 0, 0};
	static const double ky[8] = {0, 0, 0, 0.5, 1, 1, 1, 0.5};
	for(int i = 0; i < 8; i++) {
		grips[i]->x = width*kx[i] - 2;
		grips[i]->y = height*ky[i] - 2;
	}
}

void ElementDynGeometry::drawBody(DrawContext dc, double zoom) {
	if(isSelect())
		drawGrips(dc);
}

bool ElementDynGeometry::checkCollise(gdouble x, gdouble y) {
	if(isSelect() && getGripAtPos(x - this->x, y - this->y))
		return true;

	return false;
}

void ElementDynGeometry::on_grip_change(GripInfo *grip) {
	rePosGrids();
	Gdk::Rectangle nr = drawRect();
	r.join(nr);
	parent->on_redraw_rect.run(&r);
	r = nr;

	height = max(1, height);
	width = max(1, width);
	minHeight = height;
	minWidth = width;

	*fheight = height;
	*fwidth = width;
}

void ElementDynGeometry::on_grip_draw(DrawContext dc, GripInfo *grip) {
	int _x = grip->x + this->x;
	int _y = grip->y + this->y;
	dc->set_source_rgb(1, 1, 1);
	dc->rectangle(_x, _y, grip->width, grip->height);
	dc->fill();

	dc->set_source_rgb(0, 0, 0);
	dc->rectangle(_x, _y, grip->width, grip->height);
	dc->stroke();
}

bool ElementDynGeometry::on_grip_can_change(GripInfo *grip, int &dx, int &dy) {
	return true;
}

bool ElementDynGeometry::mouseDown(gdouble x, gdouble y, int button) {
	r = drawRect();
	dx = this->x;
	dy = this->y;
	if(isSelect() && gripDown(x - dx, y - dy))
		return true;
	return false;
}

void ElementDynGeometry::mouseMove(gdouble x, gdouble y) {
	gripMove(x - dx, y - dy);
}

void ElementDynGeometry::mouseUp(gdouble x, gdouble y, int button) {
	gripUp(x - dx, y - dy);
}

int ElementDynGeometry::mouseCursor(gdouble x, gdouble y) {
	if(isActiveGrip())
		return isActiveGrip()->cursor;
	if(isSelect())
		if(GripInfo *g = getGripAtPos(x - this->x, y - this->y))
			return g->cursor;
	return 0;
}

//--------------------------------------------------------------------------------------------------

ElementCaption::ElementCaption(PackElement *pe, SDK *sdk, gdouble x, gdouble y): Element(pe, sdk, x, y) {
	minHeight = 14;
	height = minHeight;
	minWidth = minHeight;
	width = 32;
	rePosPoints();
}

void ElementCaption::on_change_property(ElementProperty *prop) {
	if(prop == viewProp) {
		initView();
		int nw = 14 + viewText.length()*5;
		if(nw < width)
			invalidate();
		minWidth = nw;
		rePosPoints();
		if(nw >= width)
			invalidate();
	}
	else
		Element::on_change_property(prop);
}

//--------------------------------------------------------------------------------------------------

ElementInfoTip::ElementInfoTip(PackElement *pe, SDK *sdk, gdouble x, gdouble y): ElementDynGeometry(pe, sdk, x, y) {
	flag |= ELEMENT_FLG_IS_SYSTEM;
}

void ElementInfoTip::storePropPointers() {
	Element::storePropPointers();

	text = (ustring*)props[0]->value;
	if(tpl->conf->cClass == "ITElement") {
		formatText = (int*)props[1]->value;
		frame = (int*)props[2]->value;
		trans = (int*)props[3]->value;
		halign = (int*)props[4]->value;
		margin = (int*)props[5]->value;
	}
}

void ElementInfoTip::on_change_property(ElementProperty *prop) {
	Element::on_change_property(prop);

	if(prop->value == text || prop->value == formatText) {
		if(textLayout) {
			if(*formatText == 1)
				textLayout->set_text(*text);
			else
				textLayout->set_markup(*text);
		}
	}
	else if(prop->value == halign) {
		if(textLayout)
			textLayout->set_alignment((Pango::Alignment)(*halign + Pango::ALIGN_LEFT));
	}
	invalidate();
}

bool ElementInfoTip::checkCollise(gdouble x, gdouble y) {
	if(ElementDynGeometry::checkCollise(x, y))
		return true;
	if(Element::checkCollise(x, y)) {
		if(*trans || !*frame)
			return true;
		else
			return (x - this->x <= 5) || (y - this->y <= 5) || (this->x + width - x <= 5) || (this->y + height - y <= 5);
	}
	return false;
}

bool ElementInfoTip::checkColliseRect(gdouble x1, gdouble y1, gdouble x2, gdouble y2) {
	if(Element::checkColliseRect(x1, y1, x2, y2))
		return !((x1 > x + 5)&&(x2 < x + width - 5)&&(y1 > y + 5)&&(y2 < y + height - 5));
	return false;
}

void ElementInfoTip::drawBody(DrawContext dc, double zoom) {
	dc->set_line_width(1);

	if(*trans) {
		Gdk::Cairo::set_source_rgba(dc, *color);
		dc->rectangle(x, y, width, height);
		dc->fill();
	}

	if(*frame) {
		dc->set_source_rgb(0.8, 0.8, 0.8);
		dc->rectangle(x, y, width, height);
		dc->stroke();
	}

	if(!textLayout) {
		textLayout = Pango::Layout::create(dc);
		if(props[1]->isDefault())
			textLayout->set_text(*text);
		else
			textLayout->set_markup(*text);
		textLayout->set_wrap(Pango::WRAP_WORD);
		textLayout->set_ellipsize(Pango::ELLIPSIZE_END);
		Pango::FontDescription f("arial 10");
		textLayout->set_font_description(f);
		textLayout->set_indent(10);
		textLayout->set_alignment((Pango::Alignment)(*halign + Pango::ALIGN_LEFT));
	}
	int w = width - *margin*2;
	int h = height - *margin*2;
	textLayout->set_width(w*Pango::SCALE);
	textLayout->set_height(h*Pango::SCALE);
	//textLayout->context_changed();

	dc->rectangle(x + *margin, y + *margin, w, h);
	dc->clip();

	dc->set_source_rgb(0, 0, 0);
	dc->move_to(x + *margin, y + *margin);
	textLayout->show_in_cairo_context(dc);

	dc->reset_clip();

	ElementDynGeometry::drawBody(dc, zoom);
}

int ElementInfoTip::mouseCursor(gdouble x, gdouble y) {
	if(int m = ElementDynGeometry::mouseCursor(x, y)) return m;

	return 1;
}

//--------------------------------------------------------------------------------------------------

ElementVisualText::ElementVisualText(PackElement *pe, SDK *sdk, gdouble x, gdouble y):ElementInfoTip(pe, sdk, x, y) {
	text = (ustring*)props.getByName("Lines")->value;
	frame = new int;
	trans = new int;
	margin = new int;
	halign = new int;
	formatText = new int;
	*frame = 1;
	*margin = 1;
	*trans = 1;
	*halign = 0;
	*formatText = 1;
}

ElementVisualText::~ElementVisualText() {
	delete frame;
	delete margin;
	delete trans;
	delete halign;
	delete formatText;
}

void ElementVisualText::on_grip_change(GripInfo *grip) {
	ElementDynGeometry::on_grip_change(grip);
	rePosPointsWOsize();
}

//--------------------------------------------------------------------------------------------------

ElementDebug::ElementDebug(PackElement *pe, SDK *sdk, gdouble x, gdouble y):Element(pe, sdk, x, y) {
	points[1]->pos->x = points[0]->pos->x + POINT_SPACE + POINT_OFF + 2;
	points[2]->pos->y = points[3]->pos->y + POINT_SPACE + POINT_OFF + 2;
	width = points[1]->pos->x - points[0]->pos->x;
	height = width;
}

void ElementDebug::on_change_property(ElementProperty *prop) {
	Element::on_change_property(prop);
}

bool ElementDebug::getObjectAtPos(gdouble x, gdouble y, ObjectType *obj) {
	bool res = Element::getObjectAtPos(x, y, obj);

	if(!isSelect() && obj->type == SDK_OBJ_POINT) {
		*obj = ObjectType(this);
		return true;
	}
	return res;
}

void ElementDebug::draw(DrawContext dc, double zoom) {
	dc->set_source_rgb(1.0, 0.0, 0.0);
	int r = width/2;
	dc->arc(x + r, y + r, r, 0, 3.1415*2);
	dc->fill();
	dc->set_source_rgb(colorDark.get_red(), colorDark.get_green(), colorDark.get_blue());
	dc->arc(x + r, y + r, r, 0, 3.1415*2);
	dc->stroke();

	if(isSelect())
		drawPoints(dc);
}

void ElementDebug::remove() {
	removeFromLine(true, true);
}


//--------------------------------------------------------------------------------------------------

ElementCore::ElementCore(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y)
{
	flag |= ELEMENT_FLG_IS_CORE;
}

ustring ElementCore::readString(TData *data, ElementPoint *point, ElementProperty *prop) {
	if(point && point->point) {
		TData dt;
		point->get_data(&dt);
		return dt.toStr();
	}
	else if(prop && !prop->readStr().empty()) {
		return prop->readStr();
	}
	else if(data) {
		ustring s = data->pop().toStr();
		return s;
	}

	return ustring();
}

int ElementCore::readInteger(TData *data, ElementPoint *point, ElementProperty *prop) {
	if(point && point->point) {
		TData dt;
		point->get_data(&dt);
		return dt.toInt();
	}
	else if(prop && prop->readInt()) {
		return prop->readInt();
	}
	else if(data) {
		int i = data->pop().toInt();
		return i;
	}

	return 0;
}

double ElementCore::readReal(TData *data, ElementPoint *point, ElementProperty *prop) {
	if(point && point->point) {
		TData dt;
		point->get_data(&dt);
		return dt.toReal();
	}
	else if(prop && prop->readReal()) {
		return prop->readReal();
	}
	else if(data) {
		double d = data->pop().toReal();
		return d;
	}

	return 0.0;
}

TData ElementCore::readData(TData *data, ElementPoint *point, ElementProperty *prop) {
	if(point && point->point) {
		TData dt;
		point->get_data(&dt);
		return dt;
	}
	else if(prop && prop->readData() && !prop->readData()->empty()) {
		return *prop->readData();
	}
	else if(data) {
		return data->pop();
	}

	return TData();
}

//--------------------------------------------------------------------------------------------------

ElementCoreMessage::ElementCoreMessage(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreMessage::prepareForRun(Element **parent) {
	Container *p = (Container*)dynamic_cast<ElementTplWinControl*>(*parent)->ctl;
	while(p->get_parent())
		p = p->get_parent();
	msg = new MessageDialog(*(Window*)p, "", false, MESSAGE_INFO, (ButtonsType)props[2]->readInt());
	// TODO memory
}

void ElementCoreMessage::do_work(ElementPoint *point, TData *data) {
	msg->set_message(readString(data, points[2], props[0]), true);
	msg->set_title(readString(data, points[3], props[1]));
	if(!props[3]->isDefault()) {
		// TODO memory
		BuiltinStockID sid = {props[3]->readStr().c_str()};
		Image *img = new Image(sid, (BuiltinIconSize)(ICON_SIZE_DIALOG));
		img->show();
		msg->set_image(*img);
	}
	points[1]->on_event(msg->run());
	msg->hide();
}

//--------------------------------------------------------------------------------------------------

ElementCoreDoData::ElementCoreDoData(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreDoData::do_work(ElementPoint *point, TData *data) {
	TData dt = readData(NULL, points[2], props[0]);
	points[1]->on_event(&dt);
}

//--------------------------------------------------------------------------------------------------

ElementCoreHub::ElementCoreHub(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementDynPoints(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	minHeight = POINT_SPACE*2 - 2;
	minWidth = POINT_SPACE*2;
	rePosPoints();
}

void ElementCoreHub::do_work(ElementPoint *point, TData *data) {
	TData tmp;
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		if((*p)->type == pt_event) {
			if(data) {
				tmp = *data;
				(*p)->on_event(&tmp);
			}
			else
				(*p)->on_event();
		}
}

void ElementCoreHub::drawIcon(DrawContext dc) {
	dc->set_source_rgb(0.0, 0.0, 1.0);
	gdouble _x = x + width/2;

	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++) {
		dc->move_to(_x, (*p)->pos->y);
		if((*p)->type == pt_work)
			dc->rel_line_to(-(POINT_OFF + 1), 0);
		else
			dc->rel_line_to(POINT_OFF + 1, 0);
		dc->stroke();
	}

	dc->move_to(_x, y + POINT_OFF + 3);
	dc->rel_line_to(0, height - 2*(POINT_OFF + 2));
	dc->stroke();
}

//--------------------------------------------------------------------------------------------------

ElementCoreMulti::ElementCoreMulti(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementMulti(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreMulti::do_work(ElementPoint *point, TData *data) {
	int i = points.indexOf(point);
	if(ElementPoint *p = child->points[i]->point)
		dynamic_cast<ElementCore*>(p->parent)->do_work(p, data);
}

void ElementCoreMulti::read_var(ElementPoint *point, TData *data) {
	int i = points.indexOf(point);
	if(ElementPoint *p = child->points[i]->point)
		dynamic_cast<ElementCore*>(p->parent)->read_var(p, data);
}

//--------------------------------------------------------------------------------------------------

ElementCoreEditMulti::ElementCoreEditMulti(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementEditMulti(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreEditMulti::do_work(ElementPoint *point, TData *data) {
	int i = points.indexOf(point);
	if(ElementPoint *p = parent->parent->points[i]->point)
		dynamic_cast<ElementCore*>(p->parent)->do_work(p, data);
}

void ElementCoreEditMulti::read_var(ElementPoint *point, TData *data) {
	int i = points.indexOf(point);
	if(ElementPoint *p = parent->parent->points[i]->point)
		dynamic_cast<ElementCore*>(p->parent)->read_var(p, data);
}


//--------------------------------------------------------------------------------------------------

ElementCoreStrCat::ElementCoreStrCat(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{

}

void ElementCoreStrCat::do_work(ElementPoint *point, TData *data) {
	value = readString(data, points[3], props[0]) + readString(data, points[4], props[1]);
	points[1]->on_event(value);
}

void ElementCoreStrCat::read_var(ElementPoint *point, TData *data) {
	*data = value;
}

//--------------------------------------------------------------------------------------------------

ElementCoreStrPos::ElementCoreStrPos(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{

}

void ElementCoreStrPos::do_work(ElementPoint *point, TData *data){
	ustring str = readString(data, points[3], NULL);
	int _pos = str.find(readString(data, points[4], props[0]), readInteger(data, points[5], props[1]));
	if(_pos+1)
		points[1]->on_event(_pos);
	else
		points[2]->on_event();
}

//--------------------------------------------------------------------------------------------------

ElementCoreFormatStr::ElementCoreFormatStr(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementDynPoints(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{

}

void ElementCoreFormatStr::do_work(ElementPoint *point, TData *data){
	_res = props[1]->readStr();
	ustring trg, dst;
	for (int i = 0; i < props[0]->readInt(); i++){
		trg = "%" + int_to_str(i + 1);
		dst = readString(data, points[i + 3], NULL);
		int pos = 0;
		while ((pos = _res.find(trg, pos)) + 1) {
			_res.replace(pos, trg.size(), dst);
			pos += dst.size();
		}
	}
	points[1]->on_event(_res);
}

void ElementCoreFormatStr::read_var(ElementPoint *point, TData *data){
	*data = _res;
}

//--------------------------------------------------------------------------------------------------

ElementCoreCopy::ElementCoreCopy(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{

}

void ElementCoreCopy::do_work(ElementPoint *point, TData *data){
	ustring src = readString(data,points[2],NULL);
	unsigned int pos = readInteger(data,points[3],props[0]);
	unsigned int con = readInteger(data,points[4],props[1]);
	if (props[2]->readInt()){
		pos = src.size() - con - pos;
		if (pos > src.size()) pos = 0;
	}
	_res = ustring(src, pos, con);
	points[1]->on_event(_res);
}

void ElementCoreCopy::read_var(ElementPoint *point, TData *data){
	*data = _res;
}

//--------------------------------------------------------------------------------------------------

ElementCoreStrPart::ElementCoreStrPart(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{

}

void ElementCoreStrPart::do_work(ElementPoint *point, TData *data){
	ustring str = readString(data, points[3], NULL);
	ustring trg = props[0]->readStr();
	int pos = str.find(trg);
	if(pos + 1) {
		_left = ustring(str, 0, pos);
		_right = ustring(str, pos + trg.size());
		points[1]->on_event(_right);
		points[2]->on_event(_left);
	}
	else {
		_left = str;
		_right.clear();
		if (ElementPoint *p = points.getByName("onNotFound")) p->on_event();
	}
}

void ElementCoreStrPart::read_var(ElementPoint *point, TData *data){
	if (point->name == "Left") *data = _left; else *data = _right;
}

//--------------------------------------------------------------------------------------------------

ElementCoreReplace::ElementCoreReplace(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{

}

void ElementCoreReplace::do_work(ElementPoint *point, TData *data){
	_res = readString(data, points[3], NULL);
	ustring	src = readString(data, points[4], props[0]),
			dst = readString(data, points[5], props[1]);
	int pos = 0;
	while ((pos = _res.find(src,pos)) + 1){
		_res.replace(pos, src.size(), dst);
		pos += dst.size();
	}
	points[1]->on_event(_res);
}

void ElementCoreReplace::read_var(ElementPoint *point, TData *data){
	*data = _res;
}

//--------------------------------------------------------------------------------------------------

ElementCoreBlockFind::ElementCoreBlockFind(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{

}

void ElementCoreBlockFind::prepareForRun(Widget **parent){
	_rs = props[2]->readStr();
	_sb = props[4]->readStr();
	_eb = props[5]->readStr();
}

void ElementCoreBlockFind::do_work(ElementPoint *point, TData *data){
	if (point->name == "doSearch"){
		ustring text = readString(data, points[3], NULL);
		int i = text.find(_sb);
		_stop = false;
		int j = 0;
		while (i >= 0) {
			j = text.find(_eb,i + _sb.size());
			if (j == -1) break;
			if (!props[0]->readInt()) points[1]->on_event(ustring(text, i, j - i + _eb.size()));
			else points[1]->on_event(ustring(text,i + _sb.size(),j - i - _sb.size()));
			if (_stop) break;
			if (props[1]->readInt()) j += _eb.size();
			else if ((props[3]->readInt()) || (readInteger(NULL, points[4], NULL) != 0)){
				if (_stop) break;
				if (!props[0]->readInt()) text.replace(i, j - i + _eb.size(), "");
				else {
					i += _sb.size();
					text.replace(i, j - i, "");
				}
				if (_rs != "") text.insert(i, _rs);
				j = i + _rs.size();
			}
			if (_stop) break;
			i = text.find(_sb,j);
		}

		if (j == -1) points.getByName("onNotFind")->on_event();
		points[2]->on_event(text);
	} else if (point->name == "doStop"){
		_stop = true;
	} else if (point->name == "doReplaceStr"){
		_rs = data ? data->toStr() : "";
	} else if (point->name == "doStartBlock"){
		_sb = data ? data->toStr() : "";
	} else if (point->name == "doEndBlock"){
		_eb = data ? data->toStr() : "";
	}
}

//--------------------------------------------------------------------------------------------------
ElementCoreFor::ElementCoreFor(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{

}

void ElementCoreFor::do_work(ElementPoint *point, TData *data) {
	if(point == points[0]) {
		if(props[4]->readInt())
			data = NULL;
		i = readInteger(data, points[5], props[0]);
		e = readInteger(data, points[6], props[1]);
		int s = props[2]->readInt();
		if(!props[3]->readInt())
			e += s;
		for(; i < e; i += s)
			points[2]->on_event(i);
		points[3]->on_event();
	}
	else
		i = e;
}

void ElementCoreFor::read_var(ElementPoint *point, TData *data) {
	*data = i;
}

//--------------------------------------------------------------------------------------------------

ElementCoreIfElse::ElementCoreIfElse(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreIfElse::do_work(ElementPoint *point, TData *data) {
	TData saved = *data;
	TData op1 = readData(data, points[3], props[1]);
	TData op2 = readData(data, points[4], props[2]);
	bool ok = false;
	switch(props[0]->readInt()) {
		case 0: ok = op1.compare(op2); break;
		case 1: ok = op1.toReal() < op2.toReal(); break;
		case 2: ok = op1.toReal() > op2.toReal(); break;
		case 3: ok = op1.toReal() <= op2.toReal(); break;
		case 4: ok = op1.toReal() >= op2.toReal(); break;
		case 5: ok = op1.toReal() != op2.toReal(); break;
	}
	points[ok ? 1 : 2]->on_event(&saved);
}

//--------------------------------------------------------------------------------------------------

ElementCoreRandom::ElementCoreRandom(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreRandom::do_work(ElementPoint *point, TData *data) {
	if(point == points[0]) {
		double r = (double)rand() / RAND_MAX;
		int _min = readInteger(data, points[3], props[0]);
		int _max = readInteger(data, points[4], props[1]);
		value = (int)(r * (_max - _min + 1) + _min);
		points[1]->on_event(value);
	}
	else {
		srand(time(NULL));
	}
}

void ElementCoreRandom::read_var(ElementPoint *point, TData *data) {
	*data = value;
}

//--------------------------------------------------------------------------------------------------

ElementCoreMath::ElementCoreMath(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreMath::prepareForRun(Element **parent) {
	type = props[0]->readInt();
	value = props[4]->readReal();
}

void ElementCoreMath::do_work(ElementPoint *point, TData *data) {
	if(point == points[0]) {
		double op1 = readReal(data, points[3], props[1]);
		double op2 = readReal(data, points[4], props[2]);

		switch(type) {
			case 0:	value = op1 + op2; break;
			case 1:	value = op1 - op2; break;
			case 2:	value = op1 * op2; break;
			case 3:	value = op1 / op2; break;
			case 4:	value = (int)op1 & (int)op2; break;
			case 5:	value = (int)op1 | (int)op2; break;
			case 6:	value = (int)op1 ^ (int)op2; break;
			case 7:	value = (int)op1 / (int)op2; break;
			case 8:	value = (int)op1 % (int)op2; break;
			case 9:	value = (int)op1 | (int)op2; break;
			case 10:	value = (int)op1 << (int)op2; break;
			case 11:	value = (int)op1 >> (int)op2; break;
			case 12:	/*x^y*/ break;
			case 13:	value = cos(op1); break;
			case 14:	value = sin(op1); break;
			//tg,ctg,acos,asin,atan,ch,sh,th,cth,ach,ash,ath,acth,log,lg,ln,exp,sqr,sqrt,abs,sign,roun,frac,trun,min,max
			// TODO
		}
	}
	else {
		value = props[4]->readReal();
	}
	points[2]->on_event(value);
}

void ElementCoreMath::read_var(ElementPoint *point, TData *data) {
	*data = value;
}

//--------------------------------------------------------------------------------------------------

ElementCoreMemory::ElementCoreMemory(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreMemory::prepareForRun(Element **parent) {
	value = *props[0]->readData();
}

void ElementCoreMemory::do_work(ElementPoint *point, TData *data) {
	if(point == points[0]) {
		value = readData(data, points[4], NULL);
		TData *d = &value;
		points[2]->on_event(d);
	}
	else {
		value = *props[0]->readData();
	}
}

void ElementCoreMemory::read_var(ElementPoint *point, TData *data) {
	*data = value;
}

//--------------------------------------------------------------------------------------------------

ElementCoreTimer::ElementCoreTimer(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreTimer::prepareForRun(Element **parent) {
	if(props[1]->readInt() == 0)
		start();
}

void ElementCoreTimer::stopRun() {
	stop();
}

gboolean _timer_proc(void *data) {
	return ((ElementCoreTimer*)data)->on_timer();
}

bool ElementCoreTimer::on_timer() {
	if(autoStop) {
		autoStop--;
		points[2]->on_event();
		return autoStop;
	}
	points[2]->on_event();
	// TODO onStop
	return true;
}

void ElementCoreTimer::start() {
	autoStop = props[2]->readInt();
	timerID = gdk_threads_add_timeout(props[0]->readInt(), _timer_proc, this);
}

void ElementCoreTimer::stop() {
	if(timerID) {
		g_source_remove(timerID);
		timerID = 0;
	}
}

void ElementCoreTimer::do_work(ElementPoint *point, TData *data) {
	if(point == points[0]) {
		if(!timerID) start();
	}
	else {
		if(timerID) stop();
	}
}

//--------------------------------------------------------------------------------------------------

ElementCoreIndexToChanel::ElementCoreIndexToChanel(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementDynPoints(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreIndexToChanel::do_work(ElementPoint *point, TData *data) {
	int i = readInteger(data, points.getByName("Index"), NULL);
	TData dt = readData(data, points.getByName("Data"), NULL);
	if((i < (props[0]->readInt())) && (i >= 0))
		if (ElementPoint *p = points.getByName("onEvent" + int_to_str(i + 1))) p->on_event(&dt);
}

//--------------------------------------------------------------------------------------------------

ElementCoreChanelToIndex::ElementCoreChanelToIndex(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementDynPoints(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreChanelToIndex::do_work(ElementPoint *point, TData *data) {
	remdt = data;
	int i = points.getByName("Data") ? 1 : 0;
	for (ElementPoints::iterator p = points.begin(); p != points.end(); p++){
		if ((*p) == point) {
			points[0]->on_event(i - 2);
			break;
		}
		i++;
	}
}

void ElementCoreChanelToIndex::read_var(ElementPoint *point, TData *data) {
	(*data) = remdt;
}

//--------------------------------------------------------------------------------------------------

ElementCoreExecute::ElementCoreExecute(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreExecute::do_work(ElementPoint *point, TData *data) {
	ustring app = readString(data, points[2], props[0]);
	app += ustring(" ") + readString(data, points[3], props[1]);
	system(app.c_str());
	points[1]->on_event();
}

//--------------------------------------------------------------------------------------------------

ElementCoreConsole::ElementCoreConsole(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementDynPoints(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreConsole::do_work(ElementPoint *point, TData *data) {
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		if((*p)->type == pt_data)
			std::cout << readString(data, *p, NULL).c_str();
	std::cout << std::endl;
	points[1]->on_event();
}

//--------------------------------------------------------------------------------------------------

ElementCoreClipboard::ElementCoreClipboard(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreClipboard::do_work(ElementPoint *point, TData *data) {
	Clipboard::get()->set_text(readString(data, points[1], props[0]));
	points[1]->on_event();
}

void ElementCoreClipboard::read_var(ElementPoint *point, TData *data) {
	*data = Clipboard::get()->wait_for_text();
}

//--------------------------------------------------------------------------------------------------

ElementCoreFileSearch::ElementCoreFileSearch(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreFileSearch::enumDir(const ustring &path, bool fn, int i) {
	bool outFile;
	Dir dir(path);
	bool subDir = props[2]->readInt() == 0;
	bool isDir = false;
	for(Dir::iterator d = dir.begin(); d != dir.end() && !stop; d++) {
		std::string fullName = path + "/" + *d;
		if(i != 2 || subDir)
			isDir = file_test(fullName, FILE_TEST_IS_DIR);

		if(i == 2)
			outFile = true;
		else {
			outFile = isDir ? i == 1 : i == 0;
		}

		if(outFile)
			points[2]->on_event(fn ? fullName : *d);

		if(subDir && isDir)
			enumDir(fullName, fn, i);
	}
}

void ElementCoreFileSearch::do_work(ElementPoint *point, TData *data) {
	if(points[0] == point) {
		ustring ext = readString(data, points[5], props[0]);
		ustring path = readString(data, points[6], props[1]);

		stop = false;
		enumDir(path, props[3]->readInt() == 0, props[4]->readInt());

		points[3]->on_event();
	}
	else {
		stop = true;
	}
}

void ElementCoreFileSearch::read_var(ElementPoint *point, TData *data) {
	*data = count;
}

//--------------------------------------------------------------------------------------------------

ElementCoreTimeCounter::ElementCoreTimeCounter(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreTimeCounter::do_work(ElementPoint *point, TData *data) {
	if(point == points[0]) {
		timer = g_timer_new();
		points[2]->on_event();
	}
	else {
		gulong t;
		g_timer_elapsed(timer, &t);
		g_timer_destroy(timer);
		points[3]->on_event((int)t);
	}
}

//--------------------------------------------------------------------------------------------------

ElementHubEx::ElementHubEx(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	minWidth = 9;
	minHeight = minWidth + 2;

	angle = (int*)props[0]->value;

	for(int i = 1; i < 4; i++)
		addPoint(ustring("doWork") + int_to_str(i), "Provide thread", pt_work);
	addPoint("onEvent", "Provide thread", pt_event);
	rePosPoints();

	cIndex = CI_HubEx;

	flag |= ELEMENT_FLG_IS_SYSTEM;
}

void ElementHubEx::do_work(ElementPoint *point, TData *data) {
	points[3]->on_event(data);
}

void ElementHubEx::on_change_property(ElementProperty *prop) {
	rePosPoints();
	invalidate();
	for(int i = 0; i < 4; i++) {
		ElementPoint *p = points[i]->point;
		if(p) {
			p->clear();
			p->connect(points[i])->createPath();
		}
	}
	Element::on_change_property(prop);
}

void ElementHubEx::rePosPoints() {
	Element::rePosPoints();
	height = 11;

	points[1]->pos->y = points[0]->pos->y;
	points[1]->pos->x++;
	points[0]->pos->x += POINT_OFF + 1;
	points[0]->pos->y -= 3;
	points[2]->pos->x += POINT_OFF + 1;
	points[2]->pos->y -= 11;
	points[3]->pos->x -= 3;

	int ind = 3 - *angle;
	int t = points[ind]->pos->x;
	points[ind]->pos->x = points[3]->pos->x;
	points[3]->pos->x = t;
	t = points[ind]->pos->y;
	points[ind]->pos->y = points[3]->pos->y;
	points[3]->pos->y = t;
}

bool ElementHubEx::getObjectAtPos(gdouble x, gdouble y, ObjectType *obj) {
	bool res = Element::getObjectAtPos(x, y, obj);

	if(!isSelect() && obj->type == SDK_OBJ_POINT) {
		*obj = ObjectType(this);
		return true;
	}
	return res;
}

int ElementHubEx::calcSide(ElementPoint *p) {
	int i = points.indexOf(p);
	static const unsigned char vals[4][4] = {{3, 2, 1, 0}, {3, 2, 0, 1}, {3, 0, 1, 2}, {0, 2, 1, 3}};
	return vals[*angle][i];
}

int ang(int i, int a) {
	switch(a) {
		case 1: if(i == 2) return 3; else if(i == 3) return 2; break;
		case 2: if(i == 1) return 3; else if(i == 3) return 1; break;
		case 3: if(i == 0) return 3; else if(i == 3) return 0; break;
	}
	return i;
}

void ElementHubEx::draw(DrawContext dc, double zoom) {
	static const int kx[] = {0, -1, 0, 0};
	static const int ky[] = {-1, 0, 1, 0};

//	if(isSelect())
//		dc->set_line_width(3);
//	else
		dc->set_line_width(1);
	dc->set_source_rgb(0.0, 0.0, 0.5);

	int _x, _y;
	if(*angle == 2) {
		_x = points[3]->pos->x + POINT_OFF;
		_y = points[3]->pos->y;
	}
	else {
		_x = points[1]->pos->x + POINT_OFF;
		_y = points[1]->pos->y;
	}
	dc->rectangle(_x-1, _y-1, 3, 3);
	dc->fill();

	int pp = 0;
	for(int i = 0; i < 4; i++)
		if(points[i]->point) {
			dc->move_to(_x, _y);
			int a = ang(i, *angle);
			dc->line_to(points[i]->pos->x + kx[a], points[i]->pos->y + ky[a]);
			dc->stroke();
			pp++;
		}

	if(pp > 2)
		switch(*angle) {
			case 0:
				_x--;
				dc->move_to(_x, _y-3);
				dc->rel_line_to(0, 7);
				dc->rel_line_to(2, -4);
				dc->rel_line_to(-2, -3);
				dc->stroke();
				break;
			case 1:
				_y--;
				dc->move_to(_x-3, _y);
				dc->rel_line_to(7, 0);
				dc->rel_line_to(-3, 2);
				dc->rel_line_to(0, -2);
				dc->stroke();
				break;
			case 2:
				_x++;
				dc->move_to(_x, _y-3);
				dc->rel_line_to(0, 5);
				dc->rel_line_to(-2, -2);
				dc->rel_line_to(2, -2);
				dc->stroke();
				break;
			case 3:
				_y++;
				dc->move_to(_x-3, _y);
				dc->rel_line_to(6, 0);
				dc->rel_line_to(-2, -2);
				dc->rel_line_to(-2, 2);
				dc->stroke();
				break;
		}

	if(isSelect()) {
		dc->set_source_rgb(0.7, 0.7, 0.7);
		dc->rectangle(x, y, width, height);
		dc->stroke();
	}
}

int hubDirection(PointPos *pp1, PointPos *pp2, int t) {
	if(pp1->x == pp2->x)
		if(pp1->y > pp2->y)
			return (3 + t) % 4;
		else
			return 1 + t;
	else
		if(pp1->x > pp2->x)
			return 2 + t;
		else
			return t;
}

static const int hx_dirs[4][4] = {
       {1, 0, 0, 2},
       {1, 0, 2, 0},
       {0, 0, 1, 2},
       {1, 0, 0, 2} };

void ElementHubEx::insertInLine(ElementPoint *point, PointPos *pos) {
	int i = hubDirection(pos, pos->next, 0);
	int k;

	//if(EMouse.SelectState == selPointLine)
	//k = hubDirection(pos->prev, pos, 0);
	//else
	k = i;

/*    else
      begin
        e := SDK.AddUndo('GetDataEx',EMouse.X,EMouse.Y);
        i := HubDirection(P1, P2, 1);
        if EMouse.SelectState = selPointLine then
          k := HubDirection(P1.Prev, P1, 1)
        else k := i;
      end;
*/
    props[0]->setValue(&i);
    PointPos *p2 = pos->next;
    injectElementAtLine(point, points[hx_dirs[i][k]], points[3], pos);
	if(pos->x == p2->x)
		move(pos->x - points[3]->pos->x, 0);
	else if(pos->y == p2->y)
		move(0, pos->y - points[3]->pos->y);

/*    e.FPoints[3].UColor := LineInsert.UColor;
    if EMouse.SelectState = selPointLine then
      begin
        SDK.Undo.AddElementPointRemove(LineInsert, P1);
        e.DeletePoint(P1);
      end;
*/
}

void ElementHubEx::connectToPoint(ElementPoint *point) {
	if(!point->isPrimary()) {
		Element::connectToPoint(point);
		return;
	}

	int res[4];
	int m,l;
	for(int i = 0; i < 3; i++)
		if(!points[i]->point) {
			point->connect(points[i])->createPath();
			m = 0;
			l = 0;
			PointPos *pp = point->pos;
			while(pp) {
				m++;
				if(pp->next)
					l += max(3, abs(pp->x - pp->next->x) + abs(pp->y - pp->next->y));
				pp = pp->next;
			}
			res[i] = (m << 16) + l;
			point->clear();
		}
		else
			res[i] = 0;	m = 0xFFFFFF;
	int j = 0;
	for(int i = 0; i < 3; i++)
		if(res[i] < m && res[i] > 0) {
			m = res[i];
			j = i;
		}
	point->connect(points[j])->createPath();
}

void ElementHubEx::remove() {
	if(!points[3]->point) return;
	int _x,_y;
	ElementPoint *p = NULL;
	for(int i = 0; i < 3; i++) {
		PointPos *prev = points[i]->pos->prev;
		if(points[i]->point && prev)
			if( prev->x != points[3]->pos->next->x && prev->y != points[3]->pos->next->y) {
				_x = prev->x;
				_y = prev->y;
				p = points[i]->point;
				break;
			}
	}
	removeFromLine(points[0]->type == pt_work, points[0]->type == pt_var);
	if(p) {
		PointPos *pp = p->pos;
		while(pp && (pp->x != _x || pp->y != _y))
			pp = pp->next;
		if(pp == NULL) return;
		if(pp->next->y == points[3]->pos->y)
			ElementPoint::addLinePoint(pp, pp->x, pp->next->y);
		else
			ElementPoint::addLinePoint(pp, pp->next->x, pp->y);
	}
}

//--------------------------------------------------------------------------------------------------

ElementGetDataEx::ElementGetDataEx(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementHubEx(pe, sdk, x, y)
{
	minHeight = 9;
	minWidth = minHeight + 2;

	for(int i = 0; i < 3; i++) {
		points[i]->name = "Var" + int_to_str(i+1);
		points[i]->type = pt_var;
		points[i]->info = "Provide data";
	}
	points[3]->name = "Data";
	points[3]->type = pt_data;
	points[3]->info = "Provide data";
	rePosPoints();

	cIndex = CI_GetDataEx;

	flag |= ELEMENT_FLG_IS_SYSTEM;
}

void ElementGetDataEx::rePosPoints() {
	Element::rePosPoints();
	width = 11;

	points[1]->pos->x = points[0]->pos->x;
	points[1]->pos->y--;
	points[0]->pos->y -= POINT_OFF + 1;
	points[0]->pos->x -= 3;
	points[2]->pos->y -= POINT_OFF + 1;
	points[2]->pos->x -= 11;
	points[3]->pos->y += 3;

	int ind = 3 - *angle;
	int t = points[ind]->pos->y;
	points[ind]->pos->y = points[3]->pos->y;
	points[3]->pos->y = t;
	t = points[ind]->pos->x;
	points[ind]->pos->x = points[3]->pos->x;
	points[3]->pos->x = t;
}

void ElementGetDataEx::insertInLine(ElementPoint *point, PointPos *pos) {
	int i = hubDirection(pos, pos->next, 1);
	int k;

	//if(EMouse.SelectState == selPointLine)
	//k = hubDirection(pos->prev, pos, 1);
	//else
	k = i;

    props[0]->setValue(&i);
    PointPos *p2 = pos->next;
    injectElementAtLine(point, points[hx_dirs[i][k]], points[3], pos);
	if(pos->x == p2->x)
		move(pos->x - points[3]->pos->x, 0);
	else if(pos->y == p2->y)
		move(0, pos->y - points[3]->pos->y);

/*    e.FPoints[3].UColor := LineInsert.UColor;
    if EMouse.SelectState = selPointLine then
      begin
        SDK.Undo.AddElementPointRemove(LineInsert, P1);
        e.DeletePoint(P1);
      end;
*/
}

int ElementGetDataEx::calcSide(ElementPoint *p) {
	int i = points.indexOf(p);
	static const unsigned char vals[4][4] = {{2, 1, 0, 3}, {2, 1, 3, 0}, {2, 3, 0, 1}, {3, 1, 0, 2}};
	return vals[*angle][i];
}

void ElementGetDataEx::draw(DrawContext dc, double zoom) {
	static const int kx[] = {-1, 0, 1, 0};
	static const int ky[] = {0, 1, 0, -1};

	dc->set_line_width(1);
	dc->set_source_rgb(1.0, 0.0, 0.0);

	int _x, _y, i = (*angle == 3) ? 3 : 0;
	_x = points[i]->pos->x + POINT_OFF;
	_y = points[i]->pos->y;
	dc->rectangle(_x-1, _y-1, 3, 3);
	dc->fill();

	int pp = 0;
	for(int i = 0; i < 4; i++)
		if(points[i]->point) {
			dc->move_to(_x, _y);
			int a = ang(i, *angle);
			dc->line_to(points[i]->pos->x + kx[a], points[i]->pos->y + ky[a]);
			dc->stroke();
			pp++;
		}

	if(pp > 2)
		switch(*angle) {
			case 1:
				_x--;
				dc->move_to(_x, _y-3);
				dc->rel_line_to(0, 7);
				dc->rel_line_to(2, -4);
				dc->rel_line_to(-2, -3);
				dc->stroke();
				break;
			case 2:
				_y--;
				dc->move_to(_x-3, _y);
				dc->rel_line_to(7, 0);
				dc->rel_line_to(-3, 2);
				dc->rel_line_to(0, -2);
				dc->stroke();
				break;
			case 3:
				_x++;
				dc->move_to(_x, _y-3);
				dc->rel_line_to(0, 5);
				dc->rel_line_to(-2, -2);
				dc->rel_line_to(2, -2);
				dc->stroke();
				break;
			case 0:
				_y++;
				dc->move_to(_x-3, _y);
				dc->rel_line_to(6, 0);
				dc->rel_line_to(-2, -2);
				dc->rel_line_to(-2, 2);
				dc->stroke();
				break;
		}

	if(isSelect()) {
		dc->set_source_rgb(0.7, 0.7, 0.7);
		dc->rectangle(x, y, width, height);
		dc->stroke();
	}
}

void ElementGetDataEx::read_var(ElementPoint *point, TData *data) {
	points[3]->get_data(data);
}


//--------------------------------------------------------------------------------------------------

ElementCoreEventFromData::ElementCoreEventFromData(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{

}

void ElementCoreEventFromData::do_work(ElementPoint *point, TData *data){
	_dt = readData(data,points[3],NULL);
}

void ElementCoreEventFromData::read_var(ElementPoint *point, TData *data){
	points[1]->on_event(data);
	*data = _dt;
}

//--------------------------------------------------------------------------------------------------

ElementCoreOpenDialog::ElementCoreOpenDialog(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{

}

void ElementCoreOpenDialog::prepareForRun(Element **parent) {
	dlg = new FileChooserDialog(props[1]->readStr());
	dlg->add_button(Gtk::Stock::CANCEL, 0);
	dlg->add_button(Gtk::Stock::OK, 1);
}

void ElementCoreOpenDialog::stopRun() {
	delete dlg;
}

void ElementCoreOpenDialog::do_work(ElementPoint *point, TData *data){
	ustring fname = readString(data, points[3], props[3]);
	if(!props[2]->isDefault())
		dlg->set_current_folder(props[2]->readStr());
	//dlg.set_filename(fname);
	if(dlg->run())
		points[1]->on_event(dlg->get_filename());
	else
		points[2]->on_event();
	dlg->hide();
}

//--------------------------------------------------------------------------------------------------

ElementCoreSaveDialog::ElementCoreSaveDialog(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{

}

void ElementCoreSaveDialog::do_work(ElementPoint *point, TData *data){
	FileChooserDialog dlg(props[1]->readStr(), FILE_CHOOSER_ACTION_SAVE);
	ustring fname = readString(data, points[3], props[3]);
	dlg.set_current_folder(props[2]->readStr());
	dlg.add_button(Gtk::Stock::CANCEL, 0);
	dlg.add_button(Gtk::Stock::OK, 1);
	//dlg.set_filename(fname);
	if(dlg.run())
		points[1]->on_event(dlg.get_filename());
	else
		points[2]->on_event();
}

//--------------------------------------------------------------------------------------------------

ElementCoreTransmitter::ElementCoreTransmitter(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementDynPoints(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	cIndex = CI_Transmitter;
}

void ElementCoreTransmitter::do_work(ElementPoint *point, TData *data){
	for(int i = 0; i < props[0]->readInt(); i++)
		ret << readData(data, points[2 + i], NULL);
}

TData &ElementCoreTransmitter::run(const TData &value) {
	TData dt = value;
	ret.clear();
	points[1]->on_event(&dt);
	return ret;
}

//--------------------------------------------------------------------------------------------------

ElementCoreStrList::ElementCoreStrList(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreStrList::prepareForRun(Element **parent) {
	list = props[1]->readStr();
}

void ElementCoreStrList::do_work(ElementPoint *point, TData *data) {
	if(point == points[0])
		list << readString(data, points.getByName("Str"), NULL);
	else if(point == points[1])
		list.setText(data->toStr());
	else if(point == points[2])
		list.loadFromFile(readString(data, points.getByName("FileName"), props[0]));
	else if(point == points[3])
		list.saveToFile(readString(data, points.getByName("FileName"), props[0]));
}

void ElementCoreStrList::read_var(ElementPoint *point, TData *data) {
	if(point->name == "Text")
		*data = list;
	else if(point->name == "Count")
		*data = (int)list.size();
	else if(point->name == "Array")
		*data = this;
}

//--------------------------------------------------------------------------------------------------

ElementCoreArrayEnum::ElementCoreArrayEnum(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreArrayEnum::do_work(ElementPoint *point, TData *data) {
	if(point == points[0]) {
		TData d = readData(data, points[6], NULL);
		DataArrayInterface *arr = d.toArray();
		if(arr) {
			for(i = 0; i < arr->_arr_count(); i++) {
				d = i;
				d = arr->_arr_get(d);
				item = d;
				points[2]->on_event(&d);
			}
			points[3]->on_event();
		}
	}
	else
		i = 0xffffffff;		// <-- ;)
}

void ElementCoreArrayEnum::read_var(ElementPoint *point, TData *data) {
	if(point == points[4])
		*data = item;
	else
		*data = i;
}

//--------------------------------------------------------------------------------------------------

ElementCoreHTTPText::ElementCoreHTTPText(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreHTTPText::do_work(ElementPoint *point, TData *data) {
	ustring url = readString(data, points[2], props[0]);

	//http://ya.ru/
	int h_pos = url.find('/', 7);
	ustring host, location;
	if(h_pos > 0) {
		host = url.substr(7, h_pos - 7);
		location = url.substr(h_pos);
	}
	else {
		host = url.substr(7);
		location = "/";
	}
//	DEBUG_MSG("Try to connect " << host << " at address " << location)

	GSocketClient *client = g_socket_client_new();
	GSocketConnection *con = g_socket_client_connect_to_host(client, host.c_str(), 80, NULL, NULL);
	GOutputStream *out = g_io_stream_get_output_stream(G_IO_STREAM(con));
	ustring buf = 	"GET " + location + " HTTP/1.1\r\n"
					"Host: " + host + "\r\n"
					"Connection: close\r\n\r\n";
	g_output_stream_write_all(out, buf.c_str(), buf.bytes(), NULL, NULL, NULL);
	GInputStream *in = g_io_stream_get_input_stream(G_IO_STREAM(con));
	char in_data[1025];
	gsize len;
	char *str = NULL;
	int out_len = 0;
	do {
		len = g_input_stream_read(in, in_data, 1024, NULL, NULL);
		if(len) {
			//in_data[len] = '\0';
			str = (char*)realloc(str, out_len + len + 1);
			strncpy(str + out_len, in_data, len);
			out_len += len;
			str[out_len] = '\0';
		}
	} while(len);
	g_object_unref(con);
	g_object_unref(client);

//	DEBUG_MSG("read len: " << out_len)
	char *f = strstr(str, "\r\n\r\n");
	points[1]->on_event(ustring(f + 4));
	free(str);
}

//--------------------------------------------------------------------------------------------------

ElementCoreDS_SQLite::ElementCoreDS_SQLite(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
	db = NULL;
}

void ElementCoreDS_SQLite::stopRun() {
	close();
}

void ElementCoreDS_SQLite::close() {
	if(db) {
		sqlite3_close(db);
		db = NULL;
	}
}

void ElementCoreDS_SQLite::do_work(ElementPoint *point, TData *data) {
	if(point == points[0]) {
		close();
		sqlite3_open(readString(data, points[3], props[1]).c_str(), &db);
		points[2]->on_event();
	}
	else if(point == points[1])
		close();
}

void ElementCoreDS_SQLite::exec(const ustring &sql) {
	sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
}

sqlite3_stmt *ElementCoreDS_SQLite::query(const ustring &sql) {
	sqlite3_stmt *result;
	sqlite3_prepare(db, sql.c_str(), -1, &result, NULL);
	if(!result)
		std::cout << "Sql query " << sql.c_str() << " return empty result!" << std::endl;
	return result;
}

//--------------------------------------------------------------------------------------------------

ElementCoreDSC_Exec::ElementCoreDSC_Exec(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreDSC_Exec::prepareForRun(Element **parent) {
	sqlite = dynamic_cast<ElementCoreDS_SQLite*>(this->parent->getManagerByName(props[1]->readStr()));
}

void ElementCoreDSC_Exec::do_work(ElementPoint *point, TData *data) {
	if(sqlite) {
		sqlite->exec(readString(data, points[4], props[0]));
		last_row = sqlite->lastId();
		points[1]->on_event((int)last_row);
	}
}

void ElementCoreDSC_Exec::read_var(ElementPoint *point, TData *data) {
	*data = (int)last_row;
}

//--------------------------------------------------------------------------------------------------

ElementCoreDSC_QueryScalar::ElementCoreDSC_QueryScalar(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreDSC_QueryScalar::prepareForRun(Element **parent) {
	sqlite = dynamic_cast<ElementCoreDS_SQLite*>(this->parent->getManagerByName(props[1]->readStr()));
}

void ElementCoreDSC_QueryScalar::do_work(ElementPoint *point, TData *data) {
	if(sqlite) {
		sqlite3_stmt *r = sqlite->query(readString(data, points[4], props[0]));
		if(!r) {
			points[2]->on_event(ustring(sqlite->error()));
			return;
		}
		sqlite3_step(r);
		if(sqlite3_data_count(r)) {
			switch(sqlite3_column_type(r, 0)) {
				case SQLITE_INTEGER: result = sqlite3_column_int(r, 0); break;
				case SQLITE_FLOAT: result = sqlite3_column_double(r, 0); break;
				default:
					result = ustring((const char*)sqlite3_column_text(r, 0));
			}
		}
		else
			result.clear();
		TData dt = result;
		points[1]->on_event(&dt);
	}
}

void ElementCoreDSC_QueryScalar::read_var(ElementPoint *point, TData *data) {
	*data = result;
}

//--------------------------------------------------------------------------------------------------

ElementCoreDSC_Query::ElementCoreDSC_Query(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreDSC_Query::prepareForRun(Element **parent) {
	sqlite = dynamic_cast<ElementCoreDS_SQLite*>(this->parent->getManagerByName(props[1]->readStr()));
}

void ElementCoreDSC_Query::do_work(ElementPoint *point, TData *data) {
	if(sqlite) {
		sqlite3_stmt *r = sqlite->query(readString(data, points[4], props[0]));
		if(!r) {
			points[3]->on_event(ustring(sqlite->error()));
			return;
		}
		TData dt;

		int c = sqlite3_column_count(r);
		for(int i = 0; i < c; i++)
			dt << sqlite3_column_name(r, i);
		dt.dump();
		points[2]->on_event(&dt);

		while(sqlite3_step(r) != SQLITE_DONE) {
			dt.clear();
			for(int i = 0; i < sqlite3_data_count(r); i++) {
				switch(sqlite3_column_type(r, i)) {
					case SQLITE_INTEGER: dt << sqlite3_column_int(r, i); break;
					case SQLITE_FLOAT: dt << sqlite3_column_double(r, i); break;
					default:
						dt << ustring((const char*)sqlite3_column_text(r, i));
				}
			}
			dt.dump();
			points[1]->on_event(&dt);
		}
	}
}

//--------------------------------------------------------------------------------------------------

ElementCoreConvertor::ElementCoreConvertor(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCoreConvertor::do_work(ElementPoint *point, TData *data) {
	TData dt = readData(data, points[3], NULL);
	switch(props[0]->readInt()) {
		case 0: result = dt.toStr(); break;
		case 2:
		case 1: result = dt.toInt(); break;
		case 3: {
			ustring s = dt.toStr();
			result = s.empty() ? 0 : s[0];
			break;
		}
		case 4: result = ustring(1, (char)dt.toInt()); break;
		case 5: break;	//hextoint
		case 6: {
			char buf[64];
			int i = dt.toInt();
			sprintf(buf, "%X", i);
			result = ustring(buf);
			break;
		}
		default: result.clear();
	}
	dt = result;
	points[1]->on_event(&dt);
}

void ElementCoreConvertor::read_var(ElementPoint *point, TData *data) {
	*data = result;
}

//--------------------------------------------------------------------------------------------------

ElementCorePopup::ElementCorePopup(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	// none
}

void ElementCorePopup::do_work(ElementPoint *point, TData *data) {
	popup->popup(3, 11113044);
}

void ElementCorePopup::prepareForRun(Element **parent) {
	popup = new Menu();
	MenuItem *mi = new MenuItem("test");
	mi->show();
	popup->append(*mi);
}

void ElementCorePopup::stopRun() {
	delete popup;
}

//--------------------------------------------------------------------------------------------------

ElementCoreDelete::ElementCoreDelete(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{

}

void ElementCoreDelete::do_work(ElementPoint *point, TData *data){
	ustring str = readString(data, points[2], NULL);
	if (!str.empty()){
		size_t Pos = readInteger(data, points[3], props[0]);
		size_t Count = readInteger(data, points[4], props[1]);
		switch (props[2]->readInt()){
			case 1:
				Pos = str.size() - Count - Pos;
				if (Pos > str.size()) Pos = 0;
				break;
		}
		str.replace(Pos, Count, "");
		points[1]->on_event(str);
	}
}

//--------------------------------------------------------------------------------------------------

ElementCoreInsert::ElementCoreInsert(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{

}

void ElementCoreInsert::do_work(ElementPoint *point, TData *data){
	ustring str = readString(data, points[2], props[0]);
	if (!str.empty()){
		ustring substr = readString(data, points[3], props[1]);
		size_t Pos = readInteger(data, points[4], props[2]);
		switch (props[3]->readInt()){
			case 1:
				Pos = str.size() - Pos;
				if (Pos > str.size()) Pos = 0;
				break;
		}
		str.insert(Pos, substr);
		points[1]->on_event(str);
	}
}

//--------------------------------------------------------------------------------------------------

ElementTplWinControl::ElementTplWinControl(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementCore(pe, sdk, x, y)
{
	*(int*)props[0]->value = GRID_FRM_X(x);
	*(int*)props[1]->value = GRID_FRM_Y(y);
}

void ElementTplWinControl::prepareForRun(Element **parent) {
	ElementTplParent *op = dynamic_cast<ElementTplParent*>(*parent);
	ctl = createWidget(parent);
	if(op) {
		//((Fixed*)op)->put(*ctl, props[0]->readInt(), props[1]->readInt());
		op->insertElement(this);
		ctl->set_size_request(props[2]->readInt(), props[3]->readInt());
		if(props[4]->readInt() == 0)
			ctl->show();
		if(props[5]->readInt())
			ctl->set_sensitive(false);
		if(!props[6]->isDefault())
			ctl->set_tooltip_text(props[6]->readStr());
	}
}

void ElementTplWinControl::stopRun() {
	delete ctl;
}

void ElementTplWinControl::do_work(ElementPoint *point, TData *data) {
	if(point->name == "doVisible")
		ctl->set_visible(data ? data->toInt() : false);
	else if(point->name == "doEnabled")
		ctl->set_sensitive(data ? data->toInt() : false);
}
//--------------------------------------------------------------------------------------------------

ElementTplParent::ElementTplParent(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplWinControl(pe, sdk, x, y)
{
	parent->on_add_element += this;
	flag |= ELEMENT_FLG_IS_PARENT;
}

ElementTplParent::~ElementTplParent() {
	parent->on_add_element -= this;
}

void ElementTplParent::callback(void *owner, CallbackType type, const void *data) {
	addElement((Element *)data);
}

void ElementTplParent::addElement(Element *ie) {
	if(flag & ELEMENT_FLG_ONE_WIDGET) {
		ElementTplWinControl *e;
		if(ie->sdk) {
			Element *pe = ie->sdk->parent->getParentElement();
			e = dynamic_cast<ElementTplWinControl*>(pe);
		}
		else
			e = dynamic_cast<ElementTplWinControl*>(ie);
		if(e)
			for(ElementsList::iterator el = parent->elements.begin(); el != parent->elements.end(); el++)
				if(*el != ie && *el != this) {
					ElementTplWinControl *e2;
					if((*el)->sdk) {
						Element *pe = (*el)->sdk->parent->getParentElement();
						e2 = dynamic_cast<ElementTplWinControl*>(pe);
					}
					else
						e2 = dynamic_cast<ElementTplWinControl*>(*el);
					if(e2) {
						MessageDialog msg("Can not insert more then one interface widget!", false, MESSAGE_ERROR);
						msg.run();
						break;
					}
				}
	}
}

//--------------------------------------------------------------------------------------------------

ElementTplForm::ElementTplForm(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplParent(pe, sdk, x, y)
{
	flag |= ELEMENT_FLG_ONE_WIDGET;
}

Widget *ElementTplForm::createWidget(Element **parent) {
	form = new Window();
	form->set_size_request(props[2]->readInt(), props[3]->readInt());
	form->set_title(props[7]->readStr());
	if(!props[8]->isDefault())
		form->set_modal(true);
	if(!props[9]->isDefault())
		form->set_resizable(false);
	form->signal_delete_event().connect(sigc::mem_fun(*this, &ElementTplForm::on_delete_event));
/*
	form->set_name("test");
	GtkRcStyle *s = gtk_rc_style_new();
	s->bg[0].red = 0xffff;
	gtk_rc_add_widget_name_style(s, "test");
*/
	*parent = this;
	return form;
}

bool ElementTplForm::on_button_press(GdkEventButton* event) {
	DEBUG_MSG("event!!!")
	return true;
}

bool ElementTplForm::on_delete_event(GdkEventAny *event) {
	MSDK *m = parent->getMSDK();
	if(m->isRun())
		m->stop();
	else
		form->hide();
	return true;
}

void ElementTplForm::stopRun() {
	ElementTplWinControl::stopRun();
	//delete form;
}

void ElementTplForm::run() {
	points.getByName("onCreate")->on_event();
	if(props[4]->isDefault()) {
		form->show();
	}
}

void ElementTplForm::insertElement(ElementTplWinControl *e) {
	((Container*)ctl)->add(*e->ctl);
}

void ElementTplForm::do_work(ElementPoint *point, TData *data) {
	if(point->name == "doShow")
		form->show();
	else if(point->name == "doClose")
		form->hide();
}

//--------------------------------------------------------------------------------------------------

ElementTplDialog::ElementTplDialog(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplParent(pe, sdk, x, y)
{
	form = NULL;
}

Widget *ElementTplDialog::createWidget(Element **parent) {
	form = new Dialog();
	form->set_size_request(props[2]->readInt(), props[3]->readInt());
	form->set_title(props[7]->readStr());

	ArrayValue *arr = (ArrayValue *)props[8]->value;
	int i = 0;
	for(ArrayValue::iterator s = arr->begin(); s != arr->end(); s++)
		form->add_button(StockID(*(ustring *)s->data), i++);

	*parent = this;
	return form;
}

void ElementTplDialog::stopRun() {
	ElementTplWinControl::stopRun();
	form = NULL;
}

void ElementTplDialog::run() {
	points[1]->on_event();
	if(props[4]->isDefault()) {
		points[2]->on_event(form->run());
		form->hide();
		MSDK *m = parent->getMSDK();
		if(m->isRun())
			m->stop();
	}
}

void ElementTplDialog::addElement(Element *ie) {
	ElementTplWinControl *e;
	if(ie->sdk) {
		Element *pe = ie->sdk->parent->getParentElement();
		e = dynamic_cast<ElementTplWinControl*>(pe);
	}
	else
		e = dynamic_cast<ElementTplWinControl*>(ie);
	if(e && e != this) {
		ElementProperty *p = new ElementProperty(this, "Expand", "", data_combo, "True,False");
		p->group = tpl->conf->editClassName + "=" + tpl->info;
		e->props.push_back(p);
		p = new ElementProperty(this, "Fill", "", data_combo, "True,False");
		p->group = tpl->conf->editClassName + "=" + tpl->info;
		e->props.push_back(p);
		p = new ElementProperty(this, "Padding", "", data_int);
		p->group = tpl->conf->editClassName + "=" + tpl->info;
		e->props.push_back(p);
	}
}

void ElementTplDialog::insertElement(ElementTplWinControl *e) {
	Box *vb;
	if(form)
		vb = form->get_content_area();
	else	// TODO: form edit fix
		vb = (Box*)ctl;
	vb->pack_start(*e->ctl, e->props.getByName("Expand")->isDefault(),
							e->props.getByName("Fill")->isDefault(),
							e->props.getByName("Padding")->readInt());
}

void ElementTplDialog::do_work(ElementPoint *point, TData *data) {
	if(point->name == "doRun") {
		points[2]->on_event(form->run());
	}
	else
		ElementTplWinControl::do_work(point, data);
}

//--------------------------------------------------------------------------------------------------

ElementTplGroupBox::ElementTplGroupBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplParent(pe, sdk, x, y)
{
	flag |= ELEMENT_FLG_ONE_WIDGET;
}

Widget *ElementTplGroupBox::createWidget(Element **parent) {
	frame = new Frame(props[7]->readStr());

	*parent = this;

	return frame;
}

void ElementTplGroupBox::do_work(ElementPoint *point, TData *data) {
	if(point->name == "doCaption")
		((Frame*)ctl)->set_label(data ? data->toStr() : "");
	else
		ElementTplWinControl::do_work(point, data);
}

//--------------------------------------------------------------------------------------------------

ElementTplFixed::ElementTplFixed(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplParent(pe, sdk, x, y)
{
	// none
}

Widget *ElementTplFixed::createWidget(Element **parent) {
	*parent = this;

	return new Fixed();
}

void ElementTplFixed::insertElement(ElementTplWinControl *e) {
	((Fixed*)ctl)->put(*e->ctl, e->props[0]->readInt(), e->props[1]->readInt());
}

//--------------------------------------------------------------------------------------------------

ElementTplVBox::ElementTplVBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplParent(pe, sdk, x, y)
{
	// do nothing
}

Widget *ElementTplVBox::createWidget(Element **parent) {
	*parent = this;

	if(tpl->conf->cClass == "HCWE_VBox")
		return new VBox(false, props[7]->readInt());
	return new HBox(false, props[7]->readInt());
}

void ElementTplVBox::insertElement(ElementTplWinControl *e) {
	((VBox*)ctl)->pack_start(*e->ctl, e->props.getByName("Expand")->isDefault(),
									e->props.getByName("Fill")->isDefault(),
									e->props.getByName("Padding")->readInt());
}

void ElementTplVBox::addElement(Element *ie) {
	ElementTplWinControl *e;
	if(ie->sdk) {
		Element *pe = ie->sdk->parent->getParentElement();
		e = dynamic_cast<ElementTplWinControl*>(pe);
	}
	else
		e = dynamic_cast<ElementTplWinControl*>(ie);
	if(e && e != this) {
		ElementProperty *p = new ElementProperty(this, "Expand", "", data_combo, "True,False");
		p->group = tpl->conf->editClassName + "=" + tpl->info;
		e->props.push_back(p);
		p = new ElementProperty(this, "Fill", "", data_combo, "True,False");
		p->group = tpl->conf->editClassName + "=" + tpl->info;
		e->props.push_back(p);
		p = new ElementProperty(this, "Padding", "", data_int);
		p->group = tpl->conf->editClassName + "=" + tpl->info;
		e->props.push_back(p);
	}
}

//--------------------------------------------------------------------------------------------------

ElementTplExpander::ElementTplExpander(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplParent(pe, sdk, x, y)
{
	flag |= ELEMENT_FLG_ONE_WIDGET;
}

Widget *ElementTplExpander::createWidget(Element **parent) {
	*parent = this;
	Expander *exp = new Expander(props[7]->readStr());
	exp->set_spacing(props[8]->readInt());
	if(!props[9]->isDefault())
		exp->set_expanded(true);
	return exp;
}

void ElementTplExpander::insertElement(ElementTplWinControl *e) {
	((Expander*)ctl)->add(*e->ctl);
}

//--------------------------------------------------------------------------------------------------

ElementTplButtonBox::ElementTplButtonBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplParent(pe, sdk, x, y)
{
	// do nothing
}

Widget *ElementTplButtonBox::createWidget(Element **parent) {
	*parent = this;

	if(tpl->conf->cClass == "HCWE_VButtonBox")
		return new VButtonBox((ButtonBoxStyle)(props[8]->readInt() + BUTTONBOX_SPREAD), props[7]->readInt());
	return new HButtonBox((ButtonBoxStyle)(props[8]->readInt() + BUTTONBOX_SPREAD), props[7]->readInt());
}

void ElementTplButtonBox::insertElement(ElementTplWinControl *e) {
	((ButtonBox*)ctl)->pack_start(*e->ctl);
	if(!e->props.getByName("Secondary")->isDefault())
		((ButtonBox*)ctl)->set_child_secondary(*e->ctl, true);
}

void ElementTplButtonBox::addElement(Element *ie) {
	ElementTplWinControl *e;
	if(ie->sdk) {
		Element *pe = ie->sdk->parent->getParentElement();
		e = dynamic_cast<ElementTplWinControl*>(pe);
	}
	else
		e = dynamic_cast<ElementTplWinControl*>(ie);
	if(e && e != this) {
		ElementProperty *p = new ElementProperty(this, "Secondary", "", data_combo, "True,False");
		p->initFromText("1", true);
		p->group = tpl->conf->editClassName + "=" + tpl->info;
		e->props.push_back(p);
	}
}

//--------------------------------------------------------------------------------------------------

ElementTplHandleBox::ElementTplHandleBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplParent(pe, sdk, x, y)
{
	flag |= ELEMENT_FLG_ONE_WIDGET;
}

Widget *ElementTplHandleBox::createWidget(Element **parent) {
	*parent = this;
	HandleBox *h = new HandleBox();

	return h;
}

void ElementTplHandleBox::insertElement(ElementTplWinControl *e) {
	((HandleBox*)ctl)->add(*e->ctl);
}

//--------------------------------------------------------------------------------------------------

ElementTplPaned::ElementTplPaned(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplParent(pe, sdk, x, y)
{
	// do nothing
}

Widget *ElementTplPaned::createWidget(Element **parent) {
	*parent = this;

	if(tpl->conf->cClass == "HCWE_VPaned")
		return new VPaned();
	return new HPaned();
}

void ElementTplPaned::insertElement(ElementTplWinControl *e) {
	if(((Paned*)ctl)->get_child1())
		((Paned*)ctl)->pack2(*e->ctl, e->props.getByName("Resize")->isDefault(),
									e->props.getByName("Shrink")->isDefault());
	else
		((Paned*)ctl)->pack1(*e->ctl, e->props.getByName("Resize")->isDefault(),
									e->props.getByName("Shrink")->isDefault());
}

void ElementTplPaned::addElement(Element *ie) {
	ElementTplWinControl *e;
	if(ie->sdk) {
		Element *pe = ie->sdk->parent->getParentElement();
		e = dynamic_cast<ElementTplWinControl*>(pe);
	}
	else
		e = dynamic_cast<ElementTplWinControl*>(ie);
	if(e && e != this) {
		ElementProperty *p = new ElementProperty(this, "Resize", "", data_combo, "True,False");
		p->group = tpl->conf->editClassName + "=" + tpl->info;
		e->props.push_back(p);
		p = new ElementProperty(this, "Shrink", "", data_combo, "True,False");
		p->group = tpl->conf->editClassName + "=" + tpl->info;
		e->props.push_back(p);
	}
}

//--------------------------------------------------------------------------------------------------

ElementTplButton::ElementTplButton(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplWinControl(pe, sdk, x, y)
{
	// none
}

void ElementTplButton::on_button_clicked() {
	TData dt = *(TData*)props[8]->value;
	points[0]->on_event(&dt);
}

Widget *ElementTplButton::createWidget(Element **parent) {
	Button *btn = new Button(props[7]->readStr());
	btn->signal_clicked().connect(sigc::mem_fun(*this, &ElementTplButton::on_button_clicked));

	if(!props[12]->isDefault()) {
		Image *img = new Image( *(TypePixbuf*)props[12]->value );
		btn->set_image(*img);
		img->show();
	}
	else if(!props[9]->isDefault()) {
		if(!props[11]->isDefault())
			btn->set_image_position((PositionType)(props[11]->readInt() + POS_LEFT));

		Image *img = new Image(StockID(props[9]->readStr().c_str()), (BuiltinIconSize)(props[10]->readInt() + ICON_SIZE_MENU));
		btn->set_image(*img);
		img->show();
	}
	return btn;
}

//--------------------------------------------------------------------------------------------------

ElementTplEdit::ElementTplEdit(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplWinControl(pe, sdk, x, y)
{
	// none
}

Widget *ElementTplEdit::createWidget(Element **parent) {
	Entry *e = new Entry();
	e->set_text(props[7]->readStr());
	e->signal_changed().connect(sigc::mem_fun(*this, &ElementTplEdit::on_edit_changed));
	e->signal_key_press_event().connect(sigc::mem_fun(*this, &ElementTplEdit::on_button_press));
	e->add_events(Gdk::KEY_PRESS_MASK);
	return e;
}

void ElementTplEdit::on_edit_changed() {
	points[2]->on_event(((Entry*)ctl)->get_text());
}

bool ElementTplEdit::on_button_press(GdkEventKey *key) {
	DEBUG_MSG("Key: " << key->keyval)
	if(key->keyval == 65293) {
		ustring text = ((Entry*)ctl)->get_text();
		((Entry*)ctl)->set_text("");
		points[3]->on_event(text);
	}
	return true;
}

void ElementTplEdit::read_var(ElementPoint *point, TData *data) {
	*data = ((Entry*)ctl)->get_text();
}

void ElementTplEdit::do_work(ElementPoint *point, TData *data) {
	if(point == points[0])
		((Entry*)ctl)->set_text(readString(data, points[5], NULL));
}

//--------------------------------------------------------------------------------------------------

ElementTplMemo::ElementTplMemo(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplWinControl(pe, sdk, x, y)
{
	// none
}

Widget *ElementTplMemo::createWidget(Element **parent) {
	t = new TextView();
	t->get_buffer()->signal_changed().connect(sigc::mem_fun(*this, &ElementTplMemo::on_edit_changed));

	if(!props[8]->isDefault())
		t->get_buffer()->set_text(props[8]->readStr());
	if(!props[9]->isDefault())
		t->set_editable(false);

	ScrolledWindow *sc = new ScrolledWindow();
	sc->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	sc->add(*t);
	t->show();

	return sc;
}

void ElementTplMemo::stopRun() {
	delete t;
	ElementTplWinControl::stopRun();
}

void ElementTplMemo::on_edit_changed() {
	points[4]->on_event(t->get_buffer()->get_text());
}

void ElementTplMemo::read_var(ElementPoint *point, TData *data) {
	if(point == getPointByName("Text"))
		*data = t->get_buffer()->get_text(false);
	else if(point == getPointByName("Count"))
		*data = t->get_buffer()->size();
}

void ElementTplMemo::do_work(ElementPoint *point, TData *data) {
	if(point == points[0]) {
		ustring s = readString(data, getPointByName("Str"), NULL) + LINE_END;
		t->get_buffer()->insert(t->get_buffer()->end(), s);
	}
	else if(point == points[1]) {
		t->get_buffer()->set_text(data ? data->toStr() : ustring());
	}
	else if(point == points[2]) {
		ustring f = readString(data, getPointByName("FileName"), props[7]);
		gchar *cont;
		gsize len;
		if(Glib::file_test(f, FILE_TEST_EXISTS)) {
			g_file_get_contents(f.c_str(), &cont, &len, NULL);
			t->get_buffer()->set_text(cont);
			g_free(cont);
		}
	}
	else if(point == points[3]) {
		ustring f = readString(data, getPointByName("FileName"), props[7]);

		g_file_set_contents(f.c_str(), t->get_buffer()->get_text(false).c_str(), -1, NULL);
	}
}

//--------------------------------------------------------------------------------------------------

ElementTplLabel::ElementTplLabel(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplWinControl(pe, sdk, x, y)
{
	// none
}

Widget *ElementTplLabel::createWidget(Element **parent) {
	Label *l = new Label(props[7]->readStr());
	l->set_justify((Justification)(props[8]->readInt() + JUSTIFY_LEFT));

	return l;
}

void ElementTplLabel::do_work(ElementPoint *point, TData *data) {
	if(point->name == "doCaption")
		((Label *)ctl)->set_text(data ? data->toStr() : ustring());
}

//--------------------------------------------------------------------------------------------------

ElementTplCheckBox::ElementTplCheckBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplWinControl(pe, sdk, x, y)
{
	// none
}

Widget *ElementTplCheckBox::createWidget(Element **parent) {
	CheckButton *c = new CheckButton(props[8]->readStr());
	c->set_active(props[7]->readInt() == 0);

	return c;
}

//--------------------------------------------------------------------------------------------------

ElementTplScrollBar::ElementTplScrollBar(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplWinControl(pe, sdk, x, y)
{
	// none
}

Widget *ElementTplScrollBar::createWidget(Element **parent) {
	adj = Adjustment::create(props[9]->readReal(), props[7]->readReal(), props[8]->readReal());
	Scrollbar *sb;
	if(props[10]->readInt() == 0)
		sb = new HScrollbar(adj);
	else
		sb = new VScrollbar(adj);

	sb->signal_value_changed().connect(sigc::mem_fun(*this, &ElementTplScrollBar::on_change_value));
	return sb;
}

void ElementTplScrollBar::stopRun() {
	//delete adj;
	ElementTplWinControl::stopRun();
}

void ElementTplScrollBar::on_change_value() {
	points[1]->on_event((int)((Scrollbar*)ctl)->get_value());
}

void ElementTplScrollBar::read_var(ElementPoint *point, TData *data) {
	*data = ((Scrollbar*)ctl)->get_value();
}

void ElementTplScrollBar::do_work(ElementPoint *point, TData *data) {
	if(point == points[0])
		((Scrollbar*)ctl)->set_value(readReal(data, points[3], NULL));
}

//--------------------------------------------------------------------------------------------------

ElementTplListBox::ElementTplListBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplWinControl(pe, sdk, x, y)
{
	// none
}

Widget *ElementTplListBox::createWidget(Element **parent) {
	list = Gtk::ListStore::create(listColumns);

	t = new TreeView(list);
	t->append_column("text", listColumns.text);
	t->set_headers_visible(false);

	setText(props[8]->readStr());

	t->signal_cursor_changed().connect(sigc::mem_fun(*this, &ElementTplListBox::on_show_ep_row_activated));

	ScrolledWindow *sc = new ScrolledWindow();
	sc->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	sc->add(*t);
	t->show();

	return sc;
}

void ElementTplListBox::do_work(ElementPoint *point, TData *data) {
	if(point->name == "doAdd") {
		(*list->append())[listColumns.text] = readString(data, points[4], NULL);
	}
	else if(point->name == "doClear") {
		list->clear();
	}
	else if(point->name == "doText") {
		if(data)
			setText(data->toStr());
		else
			list->clear();
	}
}

void ElementTplListBox::read_var(ElementPoint *point, TData *data) {
	if(point->name == "Text") {
		StringList lst;
		for(TreeNodeChildren::iterator n = list->children().begin(); n != list->children().end(); n++)
			lst << (*n)[listColumns.text];
		*data = lst.text();
	}
	else if(point->name == "Count")
		*data = list->children().size();
	else if(point->name == "Array")
		*data = this;
}

void ElementTplListBox::on_show_ep_row_activated() {
	TreeModel::Path p;
	TreeViewColumn *c;
	t->get_cursor(p, c);

	TreeModel::Row r = *list->get_iter(p);

	points.getByName("onClick")->on_event(r[listColumns.text]);
}

void ElementTplListBox::setText(const ustring &text) {
	StringList lst;
	lst = text;
	list->clear();
	for(StringList::iterator l = lst.begin(); l != lst.end(); l++)
		(*list->append())[listColumns.text] = *l;
}

//--------------------------------------------------------------------------------------------------

ElementTplImage::ElementTplImage(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplWinControl(pe, sdk, x, y)
{
	// none
}

Widget *ElementTplImage::createWidget(Element **parent) {
	if(!props[10]->isDefault()) {
		image = new Image(StockID(props[10]->readStr().c_str()), (BuiltinIconSize)(props[11]->readInt() + ICON_SIZE_MENU));
	}
	else if(!props[12]->isDefault())
		image = new Image( *(TypePixbuf*)props[12]->value );
	else
		image = new Image();

	if(props[7]->readInt() != 2 || props[8]->readInt() != 2) {
		ScrolledWindow *sc = new ScrolledWindow();
		sc->set_policy( (PolicyType)(props[7]->readInt() + POLICY_ALWAYS),
						(PolicyType)(props[8]->readInt() + POLICY_ALWAYS));
		sc->add(*image);
		image->show();
		return sc;
	}

	return image;
}

void ElementTplImage::do_work(ElementPoint *point, TData *data) {
	if(point == points[0]) {
		ustring fn = readString(data, points[2], props[7]);
		if(file_test(fn, FILE_TEST_EXISTS))
			image->set(fn);
		else
			image->clear();
	}
	else {
		if(data && data->toPixbuf())
			image->set(*data->toPixbuf());
		else
			image->clear();
	}
}

void ElementTplImage::read_var(ElementPoint *point, TData *data) {
	img = image->get_pixbuf();
	*data = &img;
}

void ElementTplImage::stopRun() {
	if(ctl != image)
		delete image;
	ElementTplWinControl::stopRun();
}

//--------------------------------------------------------------------------------------------------

ElementTplFileChooserButton::ElementTplFileChooserButton(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplWinControl(pe, sdk, x, y)
{
	// none
}

Widget *ElementTplFileChooserButton::createWidget(Element **parent) {
	FileChooserButton *fb = new FileChooserButton(props[7]->readStr(), (FileChooserAction)(props[8]->readInt() + FILE_CHOOSER_ACTION_OPEN));
	fb->signal_file_set().connect(sigc::mem_fun(*this, &ElementTplFileChooserButton::on_file_set));

	return fb;
}

void ElementTplFileChooserButton::do_work(ElementPoint *point, TData *data) {

}

void ElementTplFileChooserButton::on_file_set() {
	points[0]->on_event(((FileChooserButton*)ctl)->get_filename());
}

//--------------------------------------------------------------------------------------------------

ElementTplColorButton::ElementTplColorButton(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplWinControl(pe, sdk, x, y)
{
	// none
}

Widget *ElementTplColorButton::createWidget(Element **parent) {
	ColorButton *cb = new ColorButton();
	cb->set_rgba(*(TypeColor*)props[7]->value);
	cb->set_title(props[8]->readStr());
	cb->signal_color_set().connect(sigc::mem_fun(*this, &ElementTplColorButton::on_color_set));

	return cb;
}

void ElementTplColorButton::on_color_set() {
	points[0]->on_event(((ColorButton*)ctl)->get_color().to_string());
}

void ElementTplColorButton::do_work(ElementPoint *point, TData *data) {
	if(point->name == "doColor") {
		((ColorButton*)ctl)->set_rgba(TypeColor(data ? data->toStr() : "black"));
	}
	else
		ElementTplWinControl::do_work(point, data);
}

//--------------------------------------------------------------------------------------------------

ElementTplProgressBar::ElementTplProgressBar(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplWinControl(pe, sdk, x, y)
{
	// none
}

Widget *ElementTplProgressBar::createWidget(Element **parent) {
	ProgressBar *pb = new ProgressBar();
	pb->set_text(props[7]->readStr());
	int val = props[8]->readInt();
	pb->set_orientation((Orientation)(val/2 + ORIENTATION_HORIZONTAL));
	pb->set_inverted(val%2 == 1);
	return pb;
}

void ElementTplProgressBar::do_work(ElementPoint *point, TData *data) {
	if(point == points[0]) {
		double v = readReal(data, points[2], NULL);
		((ProgressBar *)ctl)->set_fraction(v);
		points[1]->on_event(v);
	}
	else if(point->name == "doText") {
		((ProgressBar *)ctl)->set_text(data ? data->toStr() : "");
	}
	else
		ElementTplWinControl::do_work(point, data);
}

//--------------------------------------------------------------------------------------------------

ElementTplComboBox::ElementTplComboBox(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
	Element(pe, sdk, x, y), ElementTplWinControl(pe, sdk, x, y)
{
	// none
}

Widget *ElementTplComboBox::createWidget(Element **parent) {
	ComboBoxText *cb = new ComboBoxText();
	if(!props[8]->isDefault()) {
		StringList list;
		list = props[8]->readStr();
		for(StringList::iterator l = list.begin(); l != list.end(); l++)
			cb->append(*l);
	}
	cb->signal_changed().connect(sigc::mem_fun(*this, &ElementTplComboBox::on_changed));
	return cb;
}

void ElementTplComboBox::on_changed() {
	if(props[9]->isDefault())
		points[1]->on_event(((ComboBoxText*)ctl)->get_active_row_number());
	else
		points[1]->on_event(((ComboBoxText*)ctl)->get_active_text());
}

void ElementTplComboBox::do_work(ElementPoint *point, TData *data) {
	if(point == points[0]) {
		((ComboBoxText*)ctl)->append(data ? data->toStr() : "");
	}
	else if(point->name == "doClear")
		((ComboBoxText*)ctl)->remove_all();
	else if(point->name == "doSelectByIndex")
		((ComboBoxText*)ctl)->set_active(data ? data->toInt() : -1);
	else
		ElementTplWinControl::do_work(point, data);
}

void ElementTplComboBox::read_var(ElementPoint *point, TData *data) {
	if(point->name == "EditText")
		*data = ((ComboBoxText*)ctl)->get_active_text();
	else if(point->name == "Index")
		*data = ((ComboBoxText*)ctl)->get_active_row_number();
}
