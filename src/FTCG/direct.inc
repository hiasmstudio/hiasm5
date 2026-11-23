{$define NULL_TO_STR}

procedure tostr_php_java(var s:string);
begin
  replace(s, '\', '\\');
  replace(s, '"', '\"');
end;

procedure tostr_html(var s:string);
begin

end;

function _toCode(data:PScData; toType:byte):PScData;
begin
   Result := data;
{   case toType of
    data_str:
      case data.getSubType of
       data_int:
        begin
          Result := MakeData(data);
          Result.AddValue(MakeData('.toString()',true), true);
        end;
      end
   end;}
end;

type
  lng_indexer = (
      lng_linked,
      lng_begin_func,
      lng_end_func,
      lng_last);

function ReadPointByTypeIndex(parser:TParser; ind:integer; ptype:byte):id_point; forward;

const
  Multi:array[1..4]of byte = (pt_Event,pt_Work,pt_Data,pt_Var);

procedure _lng_begin_func(parser:TParser; const name:string);
var oldb, tmp, old_loc, loc_dt, fname:PScData;
    s:string;
begin
  oldb := parser.VarList.find('old_block');
  tmp := parser.VarList.find('tmp_block');
  old_loc := parser.VarList.find('loc_block');
  fname := parser.VarList.find('_fname_');

  if oldb = nil then
    oldb := parser.VarList.addArg('old_block');
  if tmp = nil then
    tmp := parser.VarList.addArg('tmp_block');
  if old_loc = nil then
    old_loc := parser.VarList.addArg('loc_block');
  if fname = nil then
    fname := parser.VarList.addArg('_fname_');

  fname.setValue(name);

  loc_dt := GVarList.find('blk_loc');
  old_loc.setValue(loc_dt.toStr());
  loc_dt.SetValue(parser.codeb.reggen(), data_code);

  s := parser.codeb.reggen();
  tmp.SetValue(s);

  oldb.setValue(parser.codeb.select(s));
end;

procedure _lng_end_func(parser:TParser; const scope:string);
var
    loc_dt:PScData;
    name,b:string;
begin
  loc_dt := GVarList.find('blk_loc');

  name := parser.VarList.find('_fname_').toStr();
  parser.codeb.select(scope);
  parser.Print('void ' + name + ';');
  parser.PrintLine();

  parser.codeb.select(GVarList.find('blk_mtd').toStr());
  parser.Print('void ' + GVarList.find('cur_class').toStr() + '::' + name + ' {');
  parser.PrintLine();
  parser.codeb.level := parser.codeb.level + 1;
    parser.codeb.CopyTo(loc_dt.toStr(), parser.codeb.CurBlockName);
    parser.codeb.delete(loc_dt.toStr());
    loc_dt.setValue(parser.VarList.find('loc_block').toStr(), data_code);
    b := parser.VarList.find('tmp_block').toStr();
    parser.codeb.CopyTo(b, parser.codeb.CurBlockName);
    parser.codeb.delete(b);
  parser.codeb.level := parser.codeb.level - 1;
  parser.Print('}');
  parser.PrintLine();
  parser.codeb.select(parser.VarList.find('old_block').toStr());
end;

function lng_proc(parser:TParser; obj:pointer; index:integer; args:TArgs):TScData;
var lp:id_point;
    e,oe:id_element;
begin
  Result.SetValue(0);
  case lng_indexer(index) of
   lng_linked:
    begin
      lp := parser.cgt.ptGetRLinkPoint(pointer(args.Values[0].toInt()));
      if lp <> nil then
       begin
        e := parser.cgt.ptGetParent(lp);
        with parser.cgt^ do
        while elGetClassIndex(e) in [CI_EditMulti, CI_MultiElement] do
         begin
          if elGetClassIndex(e) = CI_EditMulti then
           begin
            e := elGetSDK(e);
            if byte(propGetValue(elGetProperty(e, 0))^) = 0 then
              break;
            oe := parser.el;
            parser.el := e;
            lp := ptGetRLinkPoint(ReadPointByTypeIndex(parser, ptGetIndex(lp), Multi[ptGetType(lp)]));
            parser.el := oe;
           end
          else if elGetClassIndex(e) = CI_MultiElement then
           begin
            if byte(propGetValue(elGetProperty(e, 0))^) = 0 then
              break;
            oe := parser.el;
            parser.el := sdkGetElement(elGetSDK(e), 0);
            lp := ptGetRLinkPoint(ReadPointByTypeIndex(parser, ptGetIndex(lp), Multi[ptGetType(lp)]));
            parser.el := oe;
           end;
          if lp = nil then break;
          e := parser.cgt.ptGetParent(lp);
         end;
          
        Result.SetValue(DATA_INTEGER(lp));
       end;   
    end;
   lng_begin_func: _lng_begin_func(parser, args.Values[0].toStr());
   lng_end_func: _lng_end_func(parser, args.Values[0].toStr());
  end;
end;

procedure fill_lng_object(var methods:TAObjMethod; var count:integer);
begin
  count := integer(lng_last);
  setlength(methods, count);
  methods[integer(lng_linked)] := MakeMethod('linked',     1, 'name');
  methods[integer(lng_begin_func)] := MakeMethod('begin_func',    1, 'name');
  methods[integer(lng_end_func)] := MakeMethod('end_func',    1, 'scope');
end;

const
  lng_count = 4;
  lngs:array[0..lng_count-1] of TLangRec = (
     (entry:'EntryPoint';     name:'php';  str_del_o: '"'; str_del_c: '"'; op_del: '.';   var_mask:'$%n%i'; tostr_proc:tostr_php_java),
     (entry:'JavaEntryPoint'; name:'java'; str_del_o: '"'; str_del_c: '"'; op_del: ' + '; var_mask:'%n%i';  tostr_proc:tostr_php_java),
     (entry:'HtmlEntryPoint'; name:'html'; str_del_o: ''; str_del_c: '';  op_del: '';    var_mask:'';   tostr_proc:tostr_html),
     (entry:'CSSEntryPoint';  name:'html'; str_del_o: ''; str_del_c: '';  op_del: '';    var_mask:'';   tostr_proc:tostr_html)
  );
