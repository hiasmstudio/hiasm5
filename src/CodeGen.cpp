/*
 * CodeGen.cpp
 *
 *  Created on: Nov 9, 2010
 *      Author: dilma
 */

#include "CodeGen.h"
#include "ProjectBuilder.h"
#include "Elements.h"

#define CGT_TRACE //TRACE_PROC

Resources resources;

Event cgt_on_debug(NULL, CBT_DEBUG_INFO);

ustring _int_buf_;

//~~~~~~~~~~~~~~~~~~~~~~~~ SDK ~~~~~~~~~~~~~~~~~~~~~~~~~~

int sdkGetCount(SDK *sdk) {
	CGT_TRACE
	return sdk->elements.size();
}

Element *sdkGetElement(SDK *sdk, unsigned int index) {
	CGT_TRACE
	unsigned int n = 0;
	if(index >= 0 && index < sdk->elements.size()) {
		for(ElementsList::iterator i = sdk->elements.begin(); i != sdk->elements.end(); i++)
			if(index == n)
				return *i;
			else
				n++;
	}
	return NULL;
}

Element *sdkGetElementName(SDK *sdk, const char *name) {
	CGT_TRACE
	return sdk->getElementByName(name);
}

Element *sdkGetParent(SDK *sdk) {
	CGT_TRACE
	return sdk->parent;
}

//~~~~~~~~~~~~~~~~~~~~~~~~ Element ~~~~~~~~~~~~~~~~~~~~~~~~~~

int elGetFlag(Element *e) {
	CGT_TRACE
	return e->flag;
}

int elGetPropCount(Element *e) {
	CGT_TRACE
	return e->props.size();
}

ElementProperty *elGetProperty(Element *e, unsigned int index) {
	CGT_TRACE
	if(index >= 0 && index < e->props.size())
		return e->props[index];
	return NULL;
}

bool elIsDefProp(Element *e, int index) {
	CGT_TRACE
	return e->props[index]->isDefault();
}

Element *elSetCodeName(Element *e, const char *name) {
	CGT_TRACE
	e->codeName = name;
	return e;
}

const char *elGetCodeName(Element *e) {
	CGT_TRACE
	return e->codeName.c_str();
}

const char *elGetClassName(Element *e) {
	CGT_TRACE
	return e->tpl->name.c_str();
}

const char *elGetInfSub(Element *e) {
	CGT_TRACE
	return e->tpl->conf->sub.c_str();
}

int elGetPtCount(Element *e) {
	CGT_TRACE
	return e->points.size();
}

ElementPoint *elGetPt(Element *e, int index) {
	CGT_TRACE
	return e->points[index];
}

ElementPoint *elGetPtName(Element *e, const char *name) {
	CGT_TRACE
	return e->points.getByName(name);
}

unsigned char elGetClassIndex(Element *e) {
	CGT_TRACE
	return e->cIndex;
}

void *elGetSDK(Element *e) {
	CGT_TRACE
	return e->sdk ? (void*)e->sdk : (void*)e->parent->parent;
}

SDK *elGetSDKByIndex(Element *e, int index) {
	CGT_TRACE
	//TPoliMultiElement(e).getSDK(index);
	return NULL;
}

int elGetSDKCount(Element *e) {
	CGT_TRACE
	switch(e->cIndex) {
		//CI_PolyMulti: Result := TPoliMultiElement(e).getSDKCount;
		case CI_MultiElement:	return 1;
		default:				return 0;
	}
}

const char *elGetSDKBName(Element *e, int index) {
	CGT_TRACE
    //_res := PChar(TPoliMultiElement(e).getSDKName(index) + #0);
    return NULL;
}

bool elLinkIs(Element *e) {
	CGT_TRACE
	return e->isELink();
}

Element *elLinkMain(Element *e) {
	CGT_TRACE
	return e->isELink() ? e->getMainELink() : NULL;
}

int elGetEID(Element *e) {
	CGT_TRACE
	return e->id;
}

int elGetGroup(Element *e) {
	CGT_TRACE
	//Result := integer(TElement(e).Group);
	return 0;
}

void elGetPos(Element *e, int *x, int *y) {
	CGT_TRACE
	*x = e->x;
	*y = e->y;
}

void elGetSize(Element *e, int *w, int *h) {
	CGT_TRACE
	*w = e->width;
	*h = e->height;
}

void *elGetData(Element *e) {
	CGT_TRACE
	return e->userData;
}

void elSetData(Element *e, void *data) {
	CGT_TRACE
	e->userData = data;
}

bool isDebug(Element *e) {
	CGT_TRACE
	//Result := TSDK_Edit(TSDK(e).Sde).RunState = RUN_STARTED_DEBUG;
	return false;
}

SDK *elGetParent(Element *e) {
	CGT_TRACE
	return e->parent;
}

int elGetPropertyListCount(Element *e) {
	CGT_TRACE
	/*
	var p:TPropList;
	p := TPropList.Create;
	TElement(e).GetPropertyList(p);
	Result := p.Count;
	p.Destroy;
	*/
	return 0;
}

void *elGetPropertyListItem(Element *e, int index) {
	CGT_TRACE
	/*
	var p:TPropList;
	p := TPropList.Create;
	TElement(e).GetPropertyList(p);
	Result := p.Items[i];
	p.Destroy;
	*/
	return NULL;
}

const char *elGetInterface(Element *e) {
	CGT_TRACE
	return e->tpl->conf->interfaces.c_str();
}

const char *elGetInherit(Element *e) {
	CGT_TRACE
	return e->tpl->conf->inherit.c_str();
}

//~~~~~~~~~~~~~~~~~~~~~~~~ PropertyList ~~~~~~~~~~~~~~~~~~~~~~~~~~

const char *plGetName(ElementProperty *p) {
	CGT_TRACE
	return p->name.c_str();
}

const char *plGetInfo(ElementProperty *p) {
	CGT_TRACE
	return p->info.c_str();
}

const char *plGetGroup(ElementProperty *p) {
	CGT_TRACE
	return p->group.c_str();
}

void *plGetProperty(ElementProperty *p) {
	CGT_TRACE
	return p->value;
}

Element *plGetOwner(ElementProperty *p) {
	CGT_TRACE
	return p->parent;
}

//~~~~~~~~~~~~~~~~~~~~~~~~ Point ~~~~~~~~~~~~~~~~~~~~~~~~~~

ElementPoint *ptGetLinkPoint(ElementPoint *p) {
	CGT_TRACE
	return p->point;
}

ElementPoint *ptGetRLinkPoint(ElementPoint *p) {
	CGT_TRACE
	return p->point ? p->point->getRealPointWithPath() : NULL;
	/*
   with TElementPoint(p) do
    if Point = nil then
     Result := nil
    else Result := TElement(Parent.Element).AbsPoint(point)
    */
}

int ptGetType(ElementPoint *p) {
	CGT_TRACE
	return p->type;
}

const char *ptGetName(ElementPoint *p) {
	CGT_TRACE
	if(!p)
		return NULL;

	// TODO real point name
	return p->name.c_str();
}

Element *ptGetParent(ElementPoint *p) {
	CGT_TRACE
	return p->parent;
}

int ptGetIndex(ElementPoint *p) {
	CGT_TRACE
	return p->parent->points.indexOfType(p);
}

const char *pt_dpeGetName(ElementPoint *p) {
	CGT_TRACE

	int pt = p->type;
	Element *e = p->parent;

	static const char *MENames[] = {"doWork","OnEvent","getVar","_Data"};

	switch(e->cIndex) {
		case CI_DPElement:
		case CI_EditMulti:
			return dynamic_cast<ElementDynPoints*>(e)->dynInfoList[pt - 1].name.c_str();
		case CI_DPLElement: {
			ElementProperty *ep = dynamic_cast<ElementNamedDynPoints*>(e)->dynInfoList[pt - 1].prop;
			return ep ? ep->name.c_str() : "";
		}
		case CI_MultiElement: {
			Element *child = dynamic_cast<ElementMulti*>(e)->child;
			if( child ) {
				if(child->cIndex == CI_EditMulti)
					return dynamic_cast<ElementDynPoints*>(child)->dynInfoList[pt % 2 ? pt : (pt - 2)].name.c_str();
				return MENames[pt-1];
			}
			break;
		}
		case CI_EditMultiEx:
			return MENames[pt-1];
	}
	return "";
	/*
    pt := TElementPoint(p).PType;
    e := TElement(TElementPoint(p).Parent.Element);
    case e.CIndex of
     CI_DPElement:  Result := PChar(TDPointElement(e).Names[pt]);
     CI_DPLElement:  Result := PChar(TDPointLElement(e).List.Strings[pt-1]);
     CI_MultiElement:
       if TMultiElement(e).EM is TDPointElement then
         Result := PChar(TDPointElement(TMultiElement(e).EM).Names[pt+1])
       else Result := PChar(MENames[pt]);
     CI_EditMulti: Result := PChar(TDPointElement(e).Names[pt]);
     CI_EditMultiEx: Result := PChar(MENames[pt]);
     else result := '';
    end;
    */
}

int ptGetDataType(ElementPoint *p) {
	CGT_TRACE
	return p->dataType;
}

const char *ptGetInfo(ElementPoint *p) {
	CGT_TRACE
	// TODO point help
	//e := p.parent.element;
	//s := e.PointHelp(p.Name);
	return p->info.c_str();
}

//~~~~~~~~~~~~~~~~~~~~~~~~ Property ~~~~~~~~~~~~~~~~~~~~~~~~~~

int propGetType(ElementProperty *prop) {
	CGT_TRACE
	return prop->type;
}

int _Debug(const char *text, int color);

const char *propGetName(ElementProperty *prop) {
	CGT_TRACE
	return prop->name.c_str();
}

void *propGetValue(ElementProperty *prop) {
	CGT_TRACE
	return prop->value;
}

unsigned char propToByte(ElementProperty *prop) {
	CGT_TRACE
	return *(int*)prop->value;
}

int propToInteger(ElementProperty *prop) {
	CGT_TRACE
	if(prop->type == data_color) {
		TypeColor &c = *(TypeColor*)prop->value;

		return (c.get_red_u() >> 8) | ((c.get_green_u() >> 8) << 8) | ((c.get_blue_u() >> 8) << 16);
	}
	return *(int*)prop->value;
}

double propToReal(ElementProperty *prop) {
	CGT_TRACE
	return *(double*)prop->value;
}

char prop_buf[128];

const char *propToString(ElementProperty *p) {
	CGT_TRACE
	switch(p->type) {
		case data_str:
		case data_list:
		case data_menu:
		case data_code:
			return (*(ustring *)p->value).c_str();
		//case data_script: _res := TScript(p.value).Text + #0;
		case data_comboEx: {
			const char *buf = p->list.c_str();
			const char *c = buf;
			int i = 0;
			prop_buf[0] = '\0';
			while(*buf) {
				if(*buf == ',') {
					if(i == *(int*)p->value) {
						strncpy(prop_buf, c, buf - c);
						prop_buf[buf - c] = '\0';
						return prop_buf;
					}
					c = buf + 1;
					i++;
				}
				buf++;
			}
			return c; //p->list[*(int*)p->value].c_str();
		}
		default: ;
	}
	return NULL;
}

Element *propGetLinkedElement(Element *e, const char *propName) {
	CGT_TRACE

	ElementProperty *prop = e->props.getByName(propName);
	if(prop) {
		ustring s = prop->readStr().lowercase();
		SDK *sdk = e->parent;
		while(s.substr(0, 7) == "parent.") {
			if(sdk->parent)
				sdk = sdk->parent->parent;
			else
				return NULL;
			s = s.substr(7);
		}

		for(ElementsList::iterator e = sdk->elements.begin(); e != sdk->elements.end(); e++)
			if((int)(*e)->tpl->conf->interfaces.find(prop->list) != -1)
				if((*e)->props.getByName("name")->readStr().lowercase() == s)
					return *e;
	}
/*
var prop:PParamRec;
    i:integer;
    sdk:TSDK;
    s:string;
begin
   Result := nil;
   prop := e.Params.FindItem(propName);

   s := LowerCase(string(prop.Value^));
   sdk := TSDK(e.sdkParent);
   while copy(s, 1, 7) = 'parent.' do
     begin
       if sdk.Parent <> nil then
          sdk := TSDK(sdk.Parent.sdkParent)
       else exit;
       delete(s,1,7);
     end;

   if prop <> nil then
        for i := 0 to sdk.FCount - 1 do
          if pos(string(prop.buf^), sdk.FElements[i].Inf.Interfaces) > 0 then
            if StrIComp( PChar(string(sdk.FElements[i].Params.Find('Name')^)),  PChar(s)) = 0 then
              begin
                 Result := sdk.FElements[i];
                 exit;
              end;
*/
	return NULL;
}

Element *propGetLinkedElementInfo(Element *e, ElementProperty *prop, const char *_int) {
	CGT_TRACE
//   if e._getInterfaceInfo(prop, Result, int) then
//     StrCopy(_int, PChar(int))
//   else Result := nil;
	return NULL;
}

int propIsTranslate(Element *e, ElementProperty *p) {
	CGT_TRACE
//   if e.Params.FindTranslate(p.Name) then Result := 1 else Result := 0;
	return 0;
}

int propSaveToFile(ElementProperty *p, const char *fileName) {
	CGT_TRACE
	/*
   result := 1;
   case p.DataType of
     data_stream,data_bitmap,data_wave,data_jpeg:
       begin
          TMemoryStream(p.Value).SaveToFile(fileName);
          exit;
       end;
     data_icon:
       begin
          TIcon(p.Value).SaveToFile(fileName);
          exit;
       end;
   end;
   */
   return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~ Res ~~~~~~~~~~~~~~~~~~~~~~~~~~


int resAddFile(const char *name) {
	CGT_TRACE
	resources.push_back(ustring(name));
	return 0;
}

const char *resAddIcon(ElementProperty *p) {
	CGT_TRACE
	_int_buf_ = ustring::compose("icon%1.png", resources.nextIcon());
	ustring fname = p->parent->parent->getMSDK()->pack->pathCode() + _int_buf_;
	(*((TypePixbuf*)p->value))->save(fname, "png");
	resources.push_back(ustring(fname));
	return _int_buf_.c_str();
}

const char *resAddStr(const char *p) {
	CGT_TRACE
	/*
   st:PStream;
   s:string;
begin
   Result := nil;
   if Length(P) = 0 then exit;
   SetLength(s,Length(P));
   StrCopy(PChar(@s[1]),p);
   s := s + #0;
   _res := Resource.Pref + 'Str' + Int2Str(FRes.Str+1) + #0;
   Result := PChar(@_res[1]);
   st := NewWriteFileStream( GetStartDir + 'compiler\' + Result + '.txt');
   st.Write(s[1],Length(s));
   st.Free;
   Resource.AddString(Result);
   inc(FRes.Str);
   */
	return NULL;
}

const char *resAddStream(ElementProperty *p) {
	CGT_TRACE
	/*
   st:TMemoryStream;
begin
   Result := nil;
   St := TMemoryStream(p.Value);
   if St.Size = 0 then exit;
   _res := Resource.Pref + 'STREAM' + Int2Str(FRes.Stream) + #0;
   Result := PChar(@_res[1]);
   St.SaveToFile(GetStartDir + 'compiler\' + Result + '.dat');
   Resource.AddStream(Result);
   inc(FRes.Stream);
   */
	return NULL;
}

const char *resAddWave(ElementProperty *p) {
	CGT_TRACE
/*
var
    st:TMemoryStream;
begin
   Result := nil;
   St := TMemoryStream(p.Value);
   if St.Size = 0 then exit;
   _res := Resource.Pref + 'WAVE' + Int2Str(FRes.Wave) + #0;
   Result := PChar(@_res[1]);
   St.SaveToFile(GetStartDir + 'compiler\' + Result + '.wav');
   Resource.AddWave(Result);
   inc(FRes.Wave);
   */
	return NULL;
}

const char *resAddBitmap(ElementProperty *p) {
	CGT_TRACE
	/*
    st:TMemoryStream;
begin
   Result := nil;
   St := TMemoryStream(p.Value);
   if St.Size = 0 then exit;
   _res := Resource.Pref + 'BITMAP' + Int2Str(FRes.Bitmap) + #0;
   Result := PChar(@_res[1]);
   Bmp := TBitmap.Create;
   St.Position := 0;
   Bmp.LoadFromStream(st);
   Bmp.SaveToFile(GetStartDir + 'compiler\' + Result +'.bmp');
   Bmp.Destroy;
   Resource.AddBitmap(Result);
   inc(FRes.Bitmap);
   */
	return NULL;
}

const char *resAddMenu(ElementProperty *p) {
	CGT_TRACE
	return NULL;
	/*
   Result := nil;
   if Length(string(P.Value^)) = 0 then exit;
   _res := Resource.Pref + Int2Str(FRes.Str+1) + #0;
   Result := PChar(@_res[1]);
   Resource.AddMenu(Result,string(P.Value^));
   inc(FRes.Str);
   */
}

int resEmpty() {
	CGT_TRACE
	/*
   if Resource.Empty then
     Result := 1
   else
     Result := 0;
     */
	return 0;
}

int resSetPref(const char *pref) {
	CGT_TRACE
//   Resource.Pref := pref;
//   Result := 0;
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~ Other ~~~~~~~~~~~~~~~~~~~~~~~~~~

int _Debug(const char *text, int color) {
	CGT_TRACE

	if(color) {
		char *buf = new char[strlen(text) + 1];
		if(color == 0x0000FF)
			buf[0] = '#';
		else if(color == 0xFF0000)
			buf[0] = '!';
		else
			buf[0] = '@';
		strcpy(buf + 1, text);
		cgt_on_debug.run(buf);
		delete buf;
	}
	else {
		cgt_on_debug.run(text);
	}
	return 0;
}

int GetParam(short index, void *value) 	{
	CGT_TRACE
	/*
var e:TElement;
    sdk:TSDKMain;
    s:string;
    cmp:array[0..24] of char;
    procedure ReturnParam(index:integer);
    begin
      s := glbParams.readStr(index);
      StrLCopy(PChar(value), @s[1], length(s));
    end;
*/

	MSDK *sdk;
	ustring s;
	switch(index) {
		case PARAM_CODE_PATH :
			sdk = (*(Element **)value)->parent->getMSDK();
			strcpy((char*)value, sdk->pack->pathCode().c_str());
			return 0;
		case PARAM_DEBUG_MODE: *(int*)value = 1; return 0;
		case PARAM_DEBUG_SERVER_PORT: *(int*)value = 100; return 0; //glbParams.readInt(GP_dbg_ServerPort);
		case PARAM_DEBUG_CLIENT_PORT: *(int*)value = 101; return 0; //glbParams.readInt(GP_dbg_ClientPort);
		case PARAM_PROJECT_PATH:
			sdk = (*(Element **)value)->parent->getMSDK();
			strcpy((char*)value, sdk->projectDir().c_str());
			return 0;
		case PARAM_HIASM_VERSION:
			s = (char *)value;
			s = s.replace(s.find("%mj"), 3, int_to_str(HIASM_VERSION_MAJOR));
			s = s.replace(s.find("%mn"), 3, int_to_str(HIASM_VERSION_MINOR));
			s = s.replace(s.find("%bl"), 3, int_to_str(HIASM_VERSION_BUILD));
			strcpy((char*)value, s.c_str());
			return 0;
		case PARAM_USER_NAME: return 0; //ReturnParam(GP_usr_Name);
		case PARAM_USER_MAIL: return 0; //ReturnParam(GP_usr_Mail);
		case PARAM_PROJECT_NAME:
			sdk = (*(Element **)value)->parent->getMSDK();
			strcpy((char*)value, sdk->fileName.c_str());
			return 0;
		case PARAM_SDE_WIDTH:
			//e := TElement(value^);
			//cardinal(value^) := TSDK_Edit(TSDK(e.sdkParent).Sde).FMaxW;
			return 0;
		case PARAM_SDE_HEIGHT:
			//e := TElement(value^);
			//cardinal(value^) := TSDK_Edit(TSDK(e.sdkParent).Sde).FMaxH;
			return 0;
		case PARAM_COMPILER:
			sdk = (*(Element **)value)->parent->getMSDK();
			//GetCompiler(PChar(sdk.Pack.PackName),cmp);
			//StrCopy(PChar(value), cmp);
			return 0;
	}
	return 1;
}

int _Error(int line, Element *e, const char *text) {
	CGT_TRACE
	char *buf = new char[strlen(text) + 1];
	buf[0] = '!';
	strcpy(buf + 1, text);
	cgt_on_debug.run(buf);
	delete buf;
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~ Arrays ~~~~~~~~~~~~~~~~~~~~~~~~~~
int arrCount(ArrayValue *a) {
	CGT_TRACE
	return a->size();
}

int arrType(ArrayValue *a) {
	CGT_TRACE
	return a->type;
}

const char *arrItemName(ArrayValue *a, int index) {
	CGT_TRACE
	return a->at(index).name.c_str();
}

void *arrItemData(ArrayValue *a, int index) {
	CGT_TRACE
	return a->at(index).data;
}

ElementProperty *prop = NULL;

ElementProperty *arrGetItem(ArrayValue *a, int index) {
	if(!prop) {
		prop = new ElementProperty(NULL, ustring(), ustring(), data_int);
	}
	prop->name = a->at(index).name;
	prop->value = a->at(index).data;
	prop->type = a->type;
	return prop;
}

//~~~~~~~~~~~~~~~~~~~~~~~~ Data ~~~~~~~~~~~~~~~~~~~~~~~~~~
unsigned char dtType(TData *d) {
	CGT_TRACE
	return d->type;
}

const char *dtStr(TData *d) {
	CGT_TRACE
	return d->sdata.c_str();
}

int dtInt(TData *d) {
	CGT_TRACE
	return d->idata;
}

double dtReal(TData *d) {
	CGT_TRACE
	return d->rdata;
}

//~~~~~~~~~~~~~~~~~~~~~~~~ Font ~~~~~~~~~~~~~~~~~~~~~~~~~~
const char *fntName(void *f) {
	CGT_TRACE
	return ((FontRecord*)f)->name.c_str();
}

int fntSize(void *f) {
	CGT_TRACE
	return ((FontRecord*)f)->size;
}

unsigned char fntStyle(void *f) {
	CGT_TRACE
	return ((FontRecord*)f)->style;
}

int fntColor(void *f) {
	CGT_TRACE
	return ((FontRecord*)f)->color;
}

unsigned char fntCharSet(void *f) {
	CGT_TRACE
	return ((FontRecord*)f)->charset;
}

void *CGT[CGT_SIZE] = {
   (void*)&sdkGetCount,
   (void*)&sdkGetElement,
   (void*)&sdkGetElementName,

   (void*)&elGetFlag,
   (void*)&elGetPropCount,
   (void*)&elGetProperty,
   (void*)&elIsDefProp,
   (void*)&elSetCodeName,
   (void*)&elGetCodeName,
   (void*)&elGetClassName,
   (void*)&elGetInfSub,
   (void*)&elGetPtCount,
   (void*)&elGetPt,
   (void*)&elGetPtName,
   (void*)&elGetClassIndex,
   (void*)&elGetSDK,
   (void*)&elLinkIs,
   (void*)&elLinkMain,
   (void*)&elGetPos,
   (void*)&elGetSize,
   (void*)&elGetEID,

   (void*)&ptGetLinkPoint,
   (void*)&ptGetRLinkPoint,
   (void*)&ptGetType,
   (void*)&ptGetName,
   (void*)&ptGetParent,
   (void*)&ptGetIndex,
   (void*)&pt_dpeGetName,

   (void*)&propGetType,
   (void*)&propGetName,
   (void*)&propGetValue,
   (void*)&propToByte,
   (void*)&propToInteger,
   (void*)&propToReal,
   (void*)&propToString,

   (void*)&resAddFile,
   (void*)&resAddIcon,
   (void*)&resAddStr,
   (void*)&resAddStream,
   (void*)&resAddWave,
   (void*)&resAddBitmap,
   (void*)&resAddMenu,

   (void*)&_Debug,
   (void*)&GetParam,

   (void*)&arrCount,
   (void*)&arrType,
   (void*)&arrItemName,
   (void*)&arrItemData,
   (void*)&arrGetItem,

   (void*)&isDebug,

   (void*)&dtType,
   (void*)&dtStr,
   (void*)&dtInt,
   (void*)&dtReal,

   (void*)&fntName,
   (void*)&fntSize,
   (void*)&fntStyle,
   (void*)&fntColor,
   (void*)&fntCharSet,

   (void*)&elGetData,
   (void*)&elSetData,

   (void*)&ptGetDataType,
   (void*)&elGetParent,

   (void*)&elGetPropertyListCount,
   (void*)&elGetPropertyListItem,
   (void*)&plGetName,
   (void*)&plGetInfo,
   (void*)&plGetGroup,
   (void*)&plGetProperty,
   (void*)&plGetOwner,

   (void*)&ptGetInfo,

   (void*)&propGetLinkedElement,
   (void*)&propIsTranslate,
   (void*)&propGetLinkedElementInfo,

   (void*)&elGetSDKByIndex,
   (void*)&elGetSDKCount,
   (void*)&elGetSDKBName,

   (void*)&sdkGetParent,

   (void*)&elGetInterface,
   (void*)&elGetInherit,

   (void*)&resEmpty,
   (void*)&resSetPref,

   (void*)&_Error,

   (void*)&elGetGroup,

   (void*)&propSaveToFile
};
