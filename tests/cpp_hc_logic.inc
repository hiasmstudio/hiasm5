// Cases for the Core logic elements. Included by cpp_hc_test.cpp.

// Console with one text slot; the thread data reaches it through doPrint.
static Element *printer(Scheme &s) {
    auto out = s.add("hcConsole");
    s.set(out, "Count", "1");
    return out;
}
// hcDoData that puts a fixed string into the thread.
static Element *constant(Scheme &s, const char *text) {
    auto data = s.add("hcDoData");
    s.set(data, "Data", ustring("String(") + text + ")");
    return data;
}

static void checkLogic() {
    check("if_else", [](Scheme &s) {
        auto cmp = s.add("hcIf_else"), yes = constant(s, "yes"), no = constant(s, "no");
        auto out1 = printer(s), out2 = printer(s);
        s.set(cmp, "Type", "1");
        s.set(cmp, "Op1", "Integer(3)");
        s.set(cmp, "Op2", "Integer(5)");
        s.start(cmp, "doCompare");
        s.link(cmp, "onTrue", yes, "doData");
        s.link(cmp, "onFalse", no, "doData");
        s.link(yes, "onEventData", out1, "doPrint");
        s.link(no, "onEventData", out2, "doPrint");
    }, "yes\n");
    check("for", [](Scheme &s) {
        auto loop = s.add("hcFor"), out = printer(s), done = constant(s, "done"), out2 = printer(s);
        s.set(loop, "End", "3");
        s.start(loop, "doFor");
        s.link(loop, "onEvent", out, "doPrint");
        s.link(loop, "onStop", done, "doData");
        s.link(done, "onEventData", out2, "doPrint");
    }, "0\n1\n2\n3\ndone\n");
    check("for_exclusive", [](Scheme &s) {
        auto loop = s.add("hcFor"), out = printer(s);
        s.set(loop, "Start", "1");
        s.set(loop, "End", "3");
        s.set(loop, "IncludeEnd", "1");
        s.start(loop, "doFor");
        s.link(loop, "onEvent", out, "doPrint");
    }, "1\n2\n");
    check("math", [](Scheme &s) {
        auto math = s.add("hcMath"), out = printer(s);
        s.set(math, "OpType", "2");
        s.set(math, "Op1", "2");
        s.set(math, "Op2", "3");
        s.start(math, "doOperation");
        s.link(math, "onResult", out, "doPrint");
    }, "6\n");
    check("convertor_hex", [](Scheme &s) {
        auto conv = s.add("hcConvertor"), num = s.add("hcDoData"), out = printer(s);
        s.set(num, "Data", "Integer(255)");
        s.set(conv, "Mode", "6");
        s.set(conv, "Digits", "4");
        s.start(num, "doData");
        s.link(num, "onEventData", conv, "doConvert");
        s.link(conv, "onResult", out, "doPrint");
    }, "00FF\n");
    check("random_fixed", [](Scheme &s) {
        auto rnd = s.add("hcRandom"), out = printer(s);
        s.set(rnd, "Min", "5");
        s.set(rnd, "Max", "5");
        s.start(rnd, "doRandom");
        s.link(rnd, "onRandom", out, "doPrint");
    }, "5\n");
    check("memory", [](Scheme &s) {
        auto mem = s.add("hcMemory"), src = constant(s, "Stored"), out = printer(s);
        s.start(src, "doData");
        s.link(src, "onEventData", mem, "doValue");
        s.link(mem, "onData", out, "doPrint");
    }, "Stored\n");
    check("index_to_chanel", [](Scheme &s) {
        auto pick = s.add("hcIndexToChanel"), num = s.add("hcDoData"), one = constant(s, "one"), two = constant(s, "two");
        auto out1 = printer(s), out2 = printer(s);
        s.set(num, "Data", "Integer(1)");
        s.set(pick, "Count", "2");
        s.start(num, "doData");
        s.link(num, "onEventData", pick, "doEvent");
        s.link(pick, "onEvent1", one, "doData");
        s.link(pick, "onEvent2", two, "doData");
        s.link(one, "onEventData", out1, "doPrint");
        s.link(two, "onEventData", out2, "doPrint");
    }, "two\n");
    check("chanel_to_index", [](Scheme &s) {
        auto chan = s.add("hcChanelToIndex"), out = printer(s);
        s.set(chan, "Count", "2");
        s.start(chan, "doWork2");
        s.link(chan, "onIndex", out, "doPrint");
    }, "1\n");
    check("event_from_data", [](Scheme &s) {
        auto efd = s.add("hcEventFromData"), num = s.add("hcDoData"), seen = constant(s, "fired");
        auto hub = s.add("hcHub"), out = printer(s), out2 = printer(s);
        s.set(num, "Data", "Integer(7)");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", num, "doData");
        s.link(num, "onEventData", efd, "doData");
        s.link(hub, "onEvent2", out, "doPrint");
        s.link(efd, "GetData", out, "Text1");
        s.link(efd, "onEvent", seen, "doData");
        s.link(seen, "onEventData", out2, "doPrint");
    }, "fired\n7\n");
    check("convertor_char", [](Scheme &s) {
        auto conv = s.add("hcConvertor"), num = constant(s, "A"), out = printer(s);
        s.set(conv, "Mode", "3");
        s.start(num, "doData");
        s.link(num, "onEventData", conv, "doConvert");
        s.link(conv, "onResult", out, "doPrint");
    }, "65\n");
    check("convertor_ichar", [](Scheme &s) {
        auto conv = s.add("hcConvertor"), num = s.add("hcDoData"), out = printer(s);
        s.set(num, "Data", "Integer(66)");
        s.set(conv, "Mode", "4");
        s.start(num, "doData");
        s.link(num, "onEventData", conv, "doConvert");
        s.link(conv, "onResult", out, "doPrint");
    }, "B\n");
    check("math_result", [](Scheme &s) {
        auto math = s.add("hcMath"), out = printer(s);
        s.set(math, "OpType", "2");
        s.set(math, "Op1", "2");
        s.set(math, "Op2", "3");
        s.start(math, "doOperation");
        s.link(math, "onResult", out, "doPrint");
        s.link(out, "Text1", math, "Result");
    }, "6\n");
    check("math_clear", [](Scheme &s) {
        auto math = s.add("hcMath"), out = printer(s);
        s.set(math, "Default", "5");
        s.start(math, "doClear");
        s.link(math, "onResult", out, "doPrint");
    }, "5\n");
    check("random_value", [](Scheme &s) {
        auto rnd = s.add("hcRandom"), out = printer(s);
        s.set(rnd, "Min", "7");
        s.set(rnd, "Max", "7");
        s.start(rnd, "doRandom");
        s.link(rnd, "onRandom", out, "doPrint");
        s.link(out, "Text1", rnd, "Random");
    }, "7\n");
    check("array_index", [](Scheme &s) {
        auto list = s.add("hcStrList"), en = s.add("hcArrayEnum"), out = printer(s);
        s.set(list, "Strings", "p\nq");
        s.start(en, "doEnum");
        s.link(en, "Array", list, "Array");
        s.link(en, "onItem", out, "doPrint");
        s.link(out, "Text1", en, "Index");
    }, "0\n1\n");
    check("rt_event_from_data", [](Scheme &s) {
        auto efd = s.add("RT_EventFromData"), num = s.add("hcDoData"), seen = constant(s, "fired");
        auto hub = s.add("hcHub"), out = printer(s), out2 = printer(s);
        s.set(num, "Data", "Integer(7)");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", num, "doData");
        s.link(num, "onEventData", efd, "doData");
        s.link(hub, "onEvent2", out, "doPrint");
        s.link(efd, "GetData", out, "Text1");
        s.link(efd, "onEvent", seen, "doData");
        s.link(seen, "onEventData", out2, "doPrint");
    }, "fired\n7\n");
    check("rt_delete_result", [](Scheme &s) {
        auto del = s.add("RT_Delete"), src = constant(s, "abcd"), out = printer(s);
        s.set(del, "Position", "2");
        s.set(del, "CharCount", "2");
        s.start(src, "doData");
        s.link(src, "onEventData", del, "doDelete");
        s.link(del, "onDelete", out, "doPrint");
        s.link(out, "Text1", del, "Rusult");
    }, "ab\n");
    check("rt_copy", [](Scheme &s) {
        auto copy = s.add("RT_Copy"), src = constant(s, "Hello"), out = printer(s);
        s.set(copy, "Position", "1");
        s.set(copy, "CharCount", "2");
        s.start(src, "doData");
        s.link(src, "onEventData", copy, "doCopy");
        s.link(copy, "onCopy", out, "doPrint");
        s.link(out, "Text1", copy, "Result");
    }, "el\n");
    check("rt_length", [](Scheme &s) {
        auto len = s.add("RT_Length"), src = constant(s, "abcd"), out = printer(s);
        s.start(src, "doData");
        s.link(src, "onEventData", len, "doLength");
        s.link(len, "onLength", out, "doPrint");
        s.link(out, "Text1", len, "Result");
    }, "4\n");
    check("rt_strcase_upper", [](Scheme &s) {
        auto sc = s.add("RT_StrCase"), src = constant(s, "abc"), out = printer(s);
        s.set(sc, "Type", "1");
        s.start(src, "doData");
        s.link(src, "onEventData", sc, "doModify");
        s.link(sc, "onModify", out, "doPrint");
        s.link(out, "Text1", sc, "Result");
    }, "ABC\n");
    check("rt_replace", [](Scheme &s) {
        auto rep = s.add("RT_Replace"), src = constant(s, "a-b-c"), out = printer(s);
        s.set(rep, "SubStr", "-");
        s.set(rep, "DestStr", "+");
        s.start(src, "doData");
        s.link(src, "onEventData", rep, "doReplace");
        s.link(rep, "onReplace", out, "doPrint");
        s.link(out, "Text1", rep, "Result");
    }, "a+b+c\n");
    check("strpad_right", [](Scheme &s) {
        auto pad = s.add("StrPad"), src = constant(s, "ab"), out = printer(s);
        s.set(pad, "PadString", "0");
        s.set(pad, "PadLength", "5");
        s.set(pad, "PadType", "1");
        s.start(src, "doData");
        s.link(src, "onEventData", pad, "doStrPad");
        s.link(pad, "onStrPad", out, "doPrint");
        s.link(out, "Text1", pad, "Result");
    }, "ab000\n");
    check("between_true", [](Scheme &s) {
        auto between = s.add("Between"), yes = constant(s, "yes"), no = constant(s, "no");
        auto out = printer(s);
        s.set(between, "Data", "Integer(5)");
        s.set(between, "Left", "Integer(1)");
        s.set(between, "Right", "Integer(10)");
        s.start(between, "doBetween");
        s.link(between, "onTrue", yes, "doData");
        s.link(between, "onFalse", no, "doData");
        s.link(yes, "onEventData", out, "doPrint");
        s.link(no, "onEventData", out, "doPrint");
    }, "yes\n");
    check("sleep_event", [](Scheme &s) {
        auto sleep = s.add("Sleep"), done = constant(s, "done"), out = printer(s);
        s.set(sleep, "Delay", "1");
        s.start(sleep, "doSleep");
        s.link(sleep, "onSleep", done, "doData");
        s.link(done, "onEventData", out, "doPrint");
    }, "done\n");
    check("strcase_upper", [](Scheme &s) {
        auto sc = s.add("StrCase"), src = constant(s, "abc"), out = printer(s);
        s.set(sc, "Type", "1");
        s.start(src, "doData");
        s.link(src, "onEventData", sc, "doModify");
        s.link(sc, "onModify", out, "doPrint");
    }, "ABC\n");
    check("strmask_true", [](Scheme &s) {
        auto mask = s.add("StrMask"), src = constant(s, "abc"), yes = constant(s, "yes"), no = constant(s, "no");
        auto out = printer(s);
        s.set(mask, "Mask", "a*");
        s.start(src, "doData");
        s.link(src, "onEventData", mask, "doCompare");
        s.link(mask, "onTrue", yes, "doData");
        s.link(mask, "onFalse", no, "doData");
        s.link(yes, "onEventData", out, "doPrint");
        s.link(no, "onEventData", out, "doPrint");
    }, "yes\n");
    check("counterex_wrap", [](Scheme &s) {
        auto counter = s.add("CounterEx"), wrap = constant(s, "wrap"), out = printer(s), out2 = printer(s);
        s.set(counter, "Min", "0");
        s.set(counter, "Max", "2");
        s.set(counter, "Step", "1");
        s.set(counter, "Default", "2");
        s.reveal(counter, "onThroughMax");
        s.start(counter, "doNext");
        s.link(counter, "onThroughMax", wrap, "doData");
        s.link(counter, "onNext", out, "doPrint");
        s.link(wrap, "onEventData", out2, "doPrint");
    }, "wrap\n0\n");
    check("timecounter_events", [](Scheme &s) {
        auto tc = s.add("TimeCounter"), hub = s.add("hcHub");
        auto started = constant(s, "started"), stopped = constant(s, "stopped");
        auto out1 = printer(s), out2 = printer(s);
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", tc, "doStart");
        s.link(hub, "onEvent2", tc, "doStop");
        s.link(tc, "onStart", started, "doData");
        s.link(started, "onEventData", out1, "doPrint");
        s.link(tc, "onStop", stopped, "doData");
        s.link(stopped, "onEventData", out2, "doPrint");
    }, "started\nstopped\n");
    check("stringbuilder_mask", [](Scheme &s) {
        auto sb = s.add("StringBuilder"), a = s.add("hcMemory"), b = s.add("hcMemory"), out = printer(s);
        s.set(sb, "Args", "A\nB");
        s.set(sb, "Mask", "%A%-%B%");
        s.set(a, "Default", "String(x)");
        s.set(b, "Default", "String(y)");
        s.link(a, "Value", sb, "A");
        s.link(b, "Value", sb, "B");
        s.start(sb, "doBuild");
        s.link(sb, "onBuild", out, "doPrint");
    }, "x-y\n");
    check("multireplace", [](Scheme &s) {
        auto mr = s.add("MultiReplace"), src = constant(s, "a c"), out = printer(s);
        s.set(mr, "ReplaceList", "a=b;\nc=d;");
        s.start(src, "doData");
        s.link(src, "onEventData", mr, "doMultiReplace");
        s.link(mr, "onResult", out, "doPrint");
    }, "b d\n");
    check("trim_both", [](Scheme &s) {
        auto trim = s.add("Trim"), src = constant(s, "---abc---"), out = printer(s);
        s.set(trim, "Char", "-");
        s.set(trim, "Mode", "0");
        s.start(src, "doData");
        s.link(src, "onEventData", trim, "doTrim");
        s.link(trim, "onTrim", out, "doPrint");
    }, "abc\n");
    check("envvars_set_expand", [](Scheme &s) {
        auto env = s.add("EnvVars"), hub = s.add("hcHub"), out = printer(s);
        s.set(env, "Name", "HIASM_ENV_TEST");
        s.set(env, "Value", "42");
        s.set(env, "String", "x=%HIASM_ENV_TEST%");
        s.reveal(env, "doSet");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", env, "doSet");
        s.link(hub, "onEvent2", env, "doExpand");
        s.link(env, "onExpand", out, "doPrint");
    }, "x=42\n");
    check("envvars_get", [](Scheme &s) {
        auto env = s.add("EnvVars"), hub = s.add("hcHub"), out = printer(s);
        s.set(env, "Name", "HIASM_ENV_GET_TEST");
        s.set(env, "Value", "77");
        s.reveal(env, "doSet");
        s.reveal(env, "doGet");
        s.reveal(env, "onGet");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", env, "doSet");
        s.link(hub, "onEvent2", env, "doGet");
        s.link(env, "onGet", out, "doPrint");
    }, "77\n");
    check("beep_event", [](Scheme &s) {
        auto beep = s.add("Beep"), done = constant(s, "beep"), out = printer(s);
        s.set(beep, "Freq", "100");
        s.set(beep, "Duration", "1");
        s.start(beep, "doBeep");
        s.link(beep, "onBeep", done, "doData");
        s.link(done, "onEventData", out, "doPrint");
    }, "beep\n");
    check("datediff_seconds", [](Scheme &s) {
        auto diff = s.add("DateDiff"), a = s.add("hcMemory"), b = s.add("hcMemory"), out = printer(s);
        s.set(a, "Default", "Real(100)");
        s.set(b, "Default", "Real(103)");
        s.link(a, "Value", diff, "DateTime1");
        s.link(b, "Value", diff, "DateTime2");
        s.start(diff, "doCalc");
        s.link(diff, "onCalc", out, "doPrint");
    }, "259200\n");
    check("time_format", [](Scheme &s) {
        auto time = s.add("Time"), hub = s.add("hcHub"), out = printer(s);
        s.set(time, "Format", "ok");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", time, "doTimeDate");
        s.link(hub, "onEvent2", out, "doPrint");
        s.link(out, "Text1", time, "FormatTime");
    }, "ok\n");
    check("ini_write_read", [](Scheme &s) {
        auto ini = s.add("INI"), hub = s.add("hcHub"), out = printer(s), value = s.add("hcMemory");
        s.set(ini, "FileName", "hiasm_ini_test.ini");
        s.set(ini, "Section", "S");
        s.set(ini, "Key", "K");
        s.set(ini, "Type", "1");
        s.set(value, "Default", "String(v)");
        s.link(value, "Value", ini, "Value");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", ini, "doWrite");
        s.link(hub, "onEvent2", ini, "doRead");
        s.link(ini, "onResult", out, "doPrint");
    }, "v\n");
    check("case_true", [](Scheme &s) {
        auto c = s.add("Case"), src = constant(s, "x"), out = printer(s);
        s.set(c, "Value", "String(x)");
        s.set(c, "DataOnTrue", "String(y)");
        s.start(src, "doData");
        s.link(src, "onEventData", c, "doCase");
        s.link(c, "onTrue", out, "doPrint");
    }, "y\n");
    check("rgb_value", [](Scheme &s) {
        auto rgb = s.add("RGB"), out = printer(s);
        s.set(rgb, "R", "1");
        s.set(rgb, "G", "2");
        s.set(rgb, "B", "3");
        s.start(rgb, "doRGB");
        s.link(rgb, "onRGB", out, "doPrint");
    }, "197121\n");
    check("torgb_components", [](Scheme &s) {
        auto rgb = s.add("ToRGB"), hub = s.add("hcHub"), out = printer(s);
        s.set(rgb, "Color", "#FF0000");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", rgb, "doGetRGB");
        s.link(hub, "onEvent2", out, "doPrint");
        s.link(out, "Text1", rgb, "R");
    }, "255\n");
    check("strcatdelim", [](Scheme &s) {
        auto cat = s.add("StrCatDelim"), out = printer(s);
        s.set(cat, "Delimiter", ";");
        s.set(cat, "Str1", "a");
        s.set(cat, "Str2", "b");
        s.start(cat, "doStrCatDlm");
        s.link(cat, "onStrCatDlm", out, "doPrint");
    }, "a;b\n");
    check("counter_inc", [](Scheme &s) {
        auto counter = s.add("Counter"), out = printer(s);
        s.set(counter, "Min", "0");
        s.set(counter, "Max", "10");
        s.set(counter, "Step", "1");
        s.set(counter, "Default", "0");
        s.start(counter, "doNext");
        s.link(counter, "onNext", out, "doPrint");
    }, "1\n");
    check("legacy_length", [](Scheme &s) {
        auto len = s.add("Length"), src = constant(s, "abcd"), out = printer(s);
        s.start(src, "doData");
        s.link(src, "onEventData", len, "doLength");
        s.link(len, "onLength", out, "doPrint");
    }, "4\n");
    check("legacy_copy", [](Scheme &s) {
        auto copy = s.add("Copy"), src = constant(s, "Hello"), out = printer(s);
        s.set(copy, "Position", "1");
        s.set(copy, "Count", "2");
        s.start(src, "doData");
        s.link(src, "onEventData", copy, "doCopy");
        s.link(copy, "onCopy", out, "doPrint");
    }, "el\n");
    check("legacy_delete", [](Scheme &s) {
        auto del = s.add("Delete"), src = constant(s, "abcd"), out = printer(s);
        s.set(del, "Position", "2");
        s.set(del, "Count", "2");
        s.start(src, "doData");
        s.link(src, "onEventData", del, "doDelete");
        s.link(del, "onDelete", out, "doPrint");
    }, "ab\n");
    check("legacy_insert", [](Scheme &s) {
        auto ins = s.add("Insert"), src = constant(s, "ac"), out = printer(s);
        s.set(ins, "SubStr", "b");
        s.set(ins, "Position", "2");
        s.start(src, "doData");
        s.link(src, "onEventData", ins, "doInsert");
        s.link(ins, "onInsert", out, "doPrint");
    }, "abc\n");
    check("legacy_replace", [](Scheme &s) {
        auto rep = s.add("Replace"), src = constant(s, "a-b"), out = printer(s);
        s.set(rep, "SubStr", "-");
        s.set(rep, "DestStr", "+");
        s.start(src, "doData");
        s.link(src, "onEventData", rep, "doReplace");
        s.link(rep, "onReplace", out, "doPrint");
    }, "a+b\n");
    check("strpart_split", [](Scheme &s) {
        auto part = s.add("StrPart"), src = constant(s, "x=y"), left = printer(s), right = printer(s);
        s.set(part, "Char", "=");
        s.set(part, "DirectEvent", "1");
        s.start(src, "doData");
        s.link(src, "onEventData", part, "doSplit");
        s.link(part, "onPart", left, "doPrint");
        s.link(part, "onSplit", right, "doPrint");
    }, "x\ny\n");
    check("filepart_name", [](Scheme &s) {
        auto part = s.add("FilePart"), file = s.add("hcMemory"), out = printer(s);
        s.set(part, "NameWOExt", "1");
        s.set(part, "ExtPoint", "0");
        s.set(file, "Default", "String(C:/dir/file.txt)");
        s.link(file, "Value", part, "FileName");
        s.start(part, "doPart");
        s.link(part, "onName", out, "doPrint");
    }, "file.txt\n");
    check("filepartelm_name", [](Scheme &s) {
        auto part = s.add("FilePartElm"), file = s.add("hcMemory"), out = printer(s);
        s.set(part, "Mode", "1");
        s.set(file, "Default", "String(C:/dir/file.txt)");
        s.link(file, "Value", part, "FileName");
        s.start(part, "doPart");
        s.link(part, "onPart", out, "doPrint");
    }, "file.txt\n");
    check("pointxy", [](Scheme &s) {
        auto point = s.add("PointXY"), hub = s.add("hcHub"), out = printer(s);
        s.set(point, "X", "2");
        s.set(point, "Y", "3");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", out, "doPrint");
        s.link(out, "Text1", point, "Point");
    }, "196610\n");
    check("pointinrect_true", [](Scheme &s) {
        auto rect = s.add("PointInRect"), hub = s.add("hcHub"), yes = constant(s, "yes");
        auto out = printer(s), p = s.add("hcMemory"), r1 = s.add("hcMemory"), r2 = s.add("hcMemory");
        s.set(p, "Default", "Integer(327685)");
        s.set(r1, "Default", "Integer(0)");
        s.set(r2, "Default", "Integer(655370)");
        s.link(p, "Value", rect, "Point");
        s.link(r1, "Value", rect, "RPoint1");
        s.link(r2, "Value", rect, "RPoint2");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", rect, "doCheck");
        s.link(rect, "onTrue", yes, "doData");
        s.link(yes, "onEventData", out, "doPrint");
    }, "yes\n");
    check("arraysum_strings", [](Scheme &s) {
        auto arr = s.add("StrArray"), sum = s.add("ArraySum"), out = printer(s);
        s.set(arr, "Strings", "a\nb");
        s.set(sum, "Delimiter", ";");
        s.link(arr, "Array", sum, "Array");
        s.start(sum, "doSum");
        s.link(sum, "onSum", out, "doPrint");
    }, "a;b\n");
    check("arraysort_count", [](Scheme &s) {
        auto arr = s.add("IntArray"), sort = s.add("ArraySort"), out = printer(s);
        s.set(arr, "Numbers", "['a'=3,'b'=1,'c'=2]");
        s.set(sort, "ArrayType", "1");
        s.link(arr, "Array", sort, "Array");
        s.start(sort, "doSort");
        s.link(sort, "onEndSort", out, "doPrint");
        s.link(out, "Text1", sort, "Count");
    }, "3\n");
    check("arrayfind_index", [](Scheme &s) {
        auto arr = s.add("StrArray"), find = s.add("ArrayFind"), out = printer(s);
        s.set(arr, "Strings", "a\nb\nc");
        s.set(find, "Value", "String(b)");
        s.link(arr, "Array", find, "Array");
        s.start(find, "doFind");
        s.link(find, "onFind", out, "doPrint");
    }, "1\n");
    check("bitstoint", [](Scheme &s) {
        auto bits = s.add("BitsToInt"), out = printer(s), b1 = s.add("hcMemory"), b2 = s.add("hcMemory");
        auto b3 = s.add("hcMemory"), b4 = s.add("hcMemory");
        s.set(bits, "Count", "4");
        s.set(b1, "Default", "Integer(1)");
        s.set(b2, "Default", "Integer(0)");
        s.set(b3, "Default", "Integer(1)");
        s.set(b4, "Default", "Integer(0)");
        s.link(b1, "Value", bits, "Bit1");
        s.link(b2, "Value", bits, "Bit2");
        s.link(b3, "Value", bits, "Bit3");
        s.link(b4, "Value", bits, "Bit4");
        s.start(bits, "doNumber");
        s.link(bits, "onNumber", out, "doPrint");
    }, "5\n");
    check("inttobits", [](Scheme &s) {
        auto bits = s.add("IntToBits"), value = s.add("hcMemory");
        auto one = constant(s, "one"), zero = constant(s, "zero");
        auto out1 = printer(s), out2 = printer(s);
        s.set(bits, "Count", "2");
        s.set(bits, "ZeroBits", "1");
        s.set(bits, "Data_1", "String(1)");
        s.set(bits, "Data_0", "String(0)");
        s.set(value, "Default", "Integer(3)");
        s.link(value, "Value", bits, "Value");
        s.start(bits, "doBits");
        s.link(bits, "onBit1", out1, "doPrint");
        s.link(bits, "onBit2", out2, "doPrint");
    }, "1\n1\n");
    check("dir_create", [](Scheme &s) {
        auto dir = s.add("Dir"), path = constant(s, "hiasm_dir_test"), done = constant(s, "ok"), out = printer(s);
        s.start(path, "doData");
        s.link(path, "onEventData", dir, "doDir");
        s.link(dir, "onEnd", done, "doData");
        s.link(done, "onEventData", out, "doPrint");
    }, "ok\n");
    check("tempfile_exists", [](Scheme &s) {
        auto temp = s.add("TempFileStream"), tools = s.add("FileTools"), hub = s.add("hcHub"), out = printer(s);
        s.set(temp, "Prefix", "hiasm_");
        s.set(temp, "Extention", "tmp");
        s.link(temp, "TempFName", tools, "FileName");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", temp, "doCreate");
        s.link(hub, "onEvent2", tools, "doFileExists");
        s.link(tools, "onEnd", out, "doPrint");
    }, "1\n");
    check("searchinfile", [](Scheme &s) {
        auto ini = s.add("INI"), search = s.add("SearchInFile"), hub = s.add("hcHub"), out = printer(s);
        auto value = s.add("hcMemory");
        s.set(ini, "FileName", "hiasm_search_test.ini");
        s.set(ini, "Section", "S");
        s.set(ini, "Key", "K");
        s.set(ini, "Type", "1");
        s.set(value, "Default", "String(v)");
        s.link(value, "Value", ini, "Value");
        s.set(search, "FileName", "hiasm_search_test.ini");
        s.set(search, "Text", "K");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", ini, "doWrite");
        s.link(hub, "onEvent2", search, "doSearch");
        s.link(search, "onSearch", out, "doPrint");
    }, "K=v\n");
    check("pointinrectparam", [](Scheme &s) {
        auto rect = s.add("PointInRectParam"), yes = constant(s, "yes"), out = printer(s);
        auto px = s.add("hcMemory"), py = s.add("hcMemory");
        s.set(rect, "Parameters", "0,0,10,10");
        s.set(px, "Default", "Integer(5)");
        s.set(py, "Default", "Integer(5)");
        s.link(px, "Value", rect, "X");
        s.link(py, "Value", rect, "Y");
        s.start(rect, "doCheck");
        s.link(rect, "onTrue", yes, "doData");
        s.link(yes, "onEventData", out, "doPrint");
    }, "yes\n");
    check("registry_missing_value", [](Scheme &s) {
        auto reg = s.add("Registry"), out = printer(s);
        s.set(reg, "Key", "Software");
        s.set(reg, "Value", "HiAsmTestMissingValue");
        s.set(reg, "DataType", "1");
        s.set(reg, "HKey", "1");
        s.start(reg, "doRead");
        s.link(reg, "onRead", out, "doPrint");
    }, "\n");
    check("keymask_false", [](Scheme &s) {
        auto mask = s.add("KeyMask"), out = printer(s);
        s.start(mask, "doCheckMask");
        s.link(mask, "onFalse", out, "doPrint");
    }, "0\n");
    check("integerarray_enum_sorted", [](Scheme &s) {
        auto arr = s.add("IntegerArray"), hub = s.add("hcHub"), en = s.add("ArrayEnum");
        auto out = printer(s), done = constant(s, "done"), out2 = printer(s);
        s.set(arr, "IntArray", "['a'=3,'b'=1,'c'=2]");
        s.reveal(arr, "doSort");
        s.link(arr, "Array", en, "Array");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", arr, "doSort");
        s.link(hub, "onEvent2", en, "doEnum");
        s.link(en, "onItem", out, "doPrint");
        s.link(en, "onEndEnum", done, "doData");
        s.link(done, "onEventData", out2, "doPrint");
    }, "1\n2\n3\ndone\n");
    check("arrayenum_dec", [](Scheme &s) {
        auto arr = s.add("IntArray"), en = s.add("ArrayEnum");
        auto out = printer(s), done = constant(s, "done"), out2 = printer(s);
        s.set(arr, "Numbers", "['a'=1,'b'=2,'c'=3]");
        s.set(en, "Type", "1");
        s.link(arr, "Array", en, "Array");
        s.start(en, "doEnum");
        s.link(en, "onItem", out, "doPrint");
        s.link(en, "onEndEnum", done, "doData");
        s.link(done, "onEventData", out2, "doPrint");
    }, "3\n2\n1\ndone\n");
    check("arrayfilter_repeats", [](Scheme &s) {
        auto arr = s.add("StrArray"), filter = s.add("ArrayFilterRepeats");
        auto out = printer(s), done = constant(s, "done"), out2 = printer(s);
        s.set(arr, "Strings", "a\nb\na");
        s.link(arr, "Array", filter, "Array");
        s.start(filter, "doFilter");
        s.link(filter, "onFilter", out, "doPrint");
        s.link(filter, "onEndFilter", done, "doData");
        s.link(done, "onEventData", out2, "doPrint");
    }, "a\nb\ndone\n");
    check("arraymathstat_mean", [](Scheme &s) {
        auto arr = s.add("IntArray"), stat = s.add("ArrayMathStat"), out = printer(s);
        s.set(arr, "Numbers", "['a'=1,'b'=2,'c'=3]");
        s.link(arr, "Array", stat, "Array");
        s.start(stat, "doFunction");
        s.link(stat, "onResult", out, "doPrint");
    }, "2\n");
    check("formatstr_legacy",[](Scheme &s) {
        auto fmt = s.add("FormatStr"), a = s.add("hcMemory"), b = s.add("hcMemory"), out = printer(s);
        s.set(fmt, "DataCount", "2");
        s.set(fmt, "Mask", "100%% %2-%1");
        s.set(a, "Default", "String(x)");
        s.set(b, "Default", "String(y)");
        s.link(a, "Value", fmt, "Str1");
        s.link(b, "Value", fmt, "Str2");
        s.start(fmt, "doString");
        s.link(fmt, "onFString", out, "doPrint");
    }, "100% y-x\n");
    check("chararray", [](Scheme &s) {
        auto arr = s.add("CharArray"), hub = s.add("hcHub"), src = constant(s, "abc"), out = printer(s);
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", src, "doData");
        s.link(src, "onEventData", arr, "doLoad");
        s.link(hub, "onEvent2", arr, "doGetStr");
        s.link(arr, "onGetStr", out, "doPrint");
    }, "abc\n");
    check("str_enum", [](Scheme &s) {
        auto en = s.add("Str_Enum"), src = constant(s, "a;b;c"), out = printer(s), done = constant(s, "done");
        auto out2 = printer(s);
        s.set(en, "Delimiter", ";");
        s.set(en, "From", "1");
        s.set(en, "To", "-1");
        s.start(src, "doData");
        s.link(src, "onEventData", en, "doEnum");
        s.link(en, "onEnum", out, "doPrint");
        s.link(en, "onEndEnum", done, "doData");
        s.link(done, "onEventData", out2, "doPrint");
    }, "a\nb\nc\ndone\n");
    check("convertor_int_to_hex", [](Scheme &s) {
        auto conv = s.add("Convertor"), src = constant(s, "255"), out = printer(s);
        s.set(conv, "Mode", "6");
        s.set(conv, "Digits", "4");
        s.start(src, "doData");
        s.link(src, "onEventData", conv, "doConvert");
        s.link(conv, "onResult", out, "doPrint");
    }, "00FF\n");
    check("convertor_int_to_str_fill", [](Scheme &s) {
        auto conv = s.add("Convertor"), src = constant(s, "7"), out = printer(s);
        s.set(conv, "Mode", "0");
        s.set(conv, "Digits", "3");
        s.set(conv, "SymbolFill", "0");
        s.start(src, "doData");
        s.link(src, "onEventData", conv, "doConvert");
        s.link(conv, "onResult", out, "doPrint");
    }, "007\n");
    check("convertor_rom_to_int", [](Scheme &s) {
        auto conv = s.add("Convertor"), src = constant(s, "XIV"), out = printer(s);
        s.set(conv, "Mode", "14");
        s.start(src, "doData");
        s.link(src, "onEventData", conv, "doConvert");
        s.link(conv, "onResult", out, "doPrint");
    }, "14\n");
    check("convertorex_int_to_rom", [](Scheme &s) {
        auto conv = s.add("ConvertorEx"), src = constant(s, "14"), out = printer(s);
        s.set(conv, "Mode", "11");
        s.start(src, "doData");
        s.link(src, "onEventData", conv, "doConvert");
        s.link(conv, "onResult", out, "doPrint");
    }, "XIV\n");
    check("convertorex_num_to_fstr", [](Scheme &s) {
        auto conv = s.add("ConvertorEx"), src = constant(s, "3.14159"), out = printer(s);
        s.set(conv, "Mode", "15");
        s.set(conv, "Width", "6");
        s.set(conv, "Decimals", "2");
        s.start(src, "doData");
        s.link(src, "onEventData", conv, "doConvert");
        s.link(conv, "onResult", out, "doPrint");
    }, "  3.14\n");
    check("cryptography_xor", [](Scheme &s) {
        auto crypt = s.add("Cryptography"), src = constant(s, "abc"), out = printer(s);
        s.set(crypt, "Key", " ");
        s.start(src, "doData");
        s.link(src, "onEventData", crypt, "doCrypt");
        s.link(crypt, "onCrypt", out, "doPrint");
    }, "ABC\n");
    check("stream2hex", [](Scheme &s) {
        auto conv = s.add("Stream2Hex"), src = constant(s, "AB"), out = printer(s);
        s.set(conv, "Mode", "2");
        s.start(src, "doData");
        s.link(src, "onEventData", conv, "doConvert");
        s.link(conv, "onResult", out, "doPrint");
    }, "4142\n");
    check("charset_base64", [](Scheme &s) {
        auto cs = s.add("Charset"), src = constant(s, "abc"), out = printer(s);
        s.set(cs, "Type", "4");
        s.start(src, "doData");
        s.link(src, "onEventData", cs, "doCharset");
        s.link(cs, "onCharset", out, "doPrint");
    }, "YWJj\n");
    check("charset_url_encode", [](Scheme &s) {
        auto cs = s.add("Charset"), src = constant(s, "a b"), out = printer(s);
        s.set(cs, "Type", "11");
        s.start(src, "doData");
        s.link(src, "onEventData", cs, "doCharset");
        s.link(cs, "onCharset", out, "doPrint");
    }, "a%20b\n");
    check("charset_url_decode", [](Scheme &s) {
        auto cs = s.add("Charset"), src = constant(s, "a%20b"), out = printer(s);
        s.set(cs, "Type", "12");
        s.start(src, "doData");
        s.link(src, "onEventData", cs, "doCharset");
        s.link(cs, "onCharset", out, "doPrint");
    }, "a b\n");
    check("switch_toggle", [](Scheme &s) {
        auto sw = s.add("Switch"), out = printer(s);
        s.set(sw, "Default", "0");
        s.set(sw, "DataOn", "String(ON)");
        s.set(sw, "DataOff", "String(Off)");
        s.reveal(sw, "onOff");
        s.start(sw, "doSwitch");
        s.link(sw, "onOff", out, "doPrint");
    }, "Off\n");
    check("changemon", [](Scheme &s) {
        auto mon = s.add("ChangeMon"), src = constant(s, "a"), out = printer(s);
        s.set(mon, "Data", "String(z)");
        s.start(src, "doData");
        s.link(src, "onEventData", mon, "doData");
        s.link(mon, "onData", out, "doPrint");
    }, "a\n");
    check("stack_push", [](Scheme &s) {
        auto st = s.add("Stack"), src = constant(s, "P"), out = printer(s);
        s.start(src, "doData");
        s.link(src, "onEventData", st, "doPush");
        s.link(st, "onPush", out, "doPrint");
    }, "P\n");
    check("crc16_arc", [](Scheme &s) {
        auto crc = s.add("CRC16_32"), src = constant(s, "123456789"), out = printer(s);
        s.set(crc, "Metod", "0");
        s.start(src, "doData");
        s.link(src, "onEventData", crc, "doCalcCRC");
        s.link(crc, "onResult", out, "doPrint");
    }, "47933\n");
    check("hashsum_md5", [](Scheme &s) {
        auto h = s.add("Hashsum"), src = constant(s, "abc"), out = printer(s);
        s.set(h, "HashType", "0");
        s.start(src, "doData");
        s.link(src, "onEventData", h, "doHashsum");
        s.link(h, "onResult", out, "doPrint");
    }, "900150983cd24fb0d6963f7d28e17f72\n");
    check("hashsum_sha1", [](Scheme &s) {
        auto h = s.add("Hashsum"), src = constant(s, "abc"), out = printer(s);
        s.set(h, "HashType", "1");
        s.start(src, "doData");
        s.link(src, "onEventData", h, "doHashsum");
        s.link(h, "onResult", out, "doPrint");
    }, "a9993e364706816aba3e25717850c26c9cd0d89d\n");
    check("streamconvertor_strtohex", [](Scheme &s) {
        auto st = s.add("StreamConvertor"), src = constant(s, "AB"), out = printer(s);
        s.set(st, "Mode", "2");
        s.start(src, "doData");
        s.link(src, "onEventData", st, "doConvert");
        s.link(st, "onResult", out, "doPrint");
    }, "4142\n");
    check("multistrdata_parts", [](Scheme &s) {
        auto msd = s.add("MultiStrData"), src = constant(s, "a=b"), out1 = printer(s), out2 = printer(s);
        s.set(msd, "Count", "2");
        s.set(msd, "Char", "=");
        s.start(src, "doData");
        s.link(src, "onEventData", msd, "doSplit");
        s.link(msd, "onPart1", out1, "doPrint");
        s.link(msd, "onPart2", out2, "doPrint");
    }, "a\nb\n");
    check("crypt_roundtrip", [](Scheme &s) {
        auto enc = s.add("EnCrypt"), dec = s.add("DeCrypt"), src = constant(s, "Hello"), out = printer(s);
        s.set(enc, "Mode", "1");
        s.set(enc, "HashMode", "0");
        s.set(enc, "Key", "k");
        s.set(dec, "Mode", "1");
        s.set(dec, "HashMode", "0");
        s.set(dec, "Key", "k");
        s.start(src, "doData");
        s.link(src, "onEventData", enc, "doEnCrypt");
        s.link(enc, "onEnCrypt", dec, "doDeCrypt");
        s.link(dec, "onDeCrypt", out, "doPrint");
    }, "Hello\n");
    check("index_to_chanel", [](Scheme &s) {
        auto i2c = s.add("IndexToChanel"), src = constant(s, "0"), out = printer(s);
        s.set(i2c, "Count", "3");
        s.set(i2c, "Data", "String(Q)");
        s.start(src, "doData");
        s.link(src, "onEventData", i2c, "doEvent");
        s.link(i2c, "onEvent1", out, "doPrint");
    }, "Q\n");
    check("chanel_to_index", [](Scheme &s) {
        auto c2i = s.add("ChanelToIndex"), src = constant(s, "0"), out = printer(s);
        s.set(c2i, "Count", "3");
        s.start(src, "doData");
        s.link(src, "onEventData", c2i, "doWork3");
        s.link(c2i, "onIndex", out, "doPrint");
    }, "2\n");
    check("dodata_alias", [](Scheme &s) {
        auto d = s.add("DoData"), out = printer(s);
        s.set(d, "Data", "String(alias)");
        s.start(d, "doData");
        s.link(d, "onEventData", out, "doPrint");
    }, "alias\n");
    check("blockfind_simple", [](Scheme &s) {
        auto bf = s.add("BlockFind"), src = constant(s, "xxAByy"), out = printer(s);
        s.set(bf, "StartBlock", "AA");
        s.set(bf, "EndBlock", "BB");
        s.start(src, "doData");
        s.link(src, "onEventData", bf, "doSearch");
        s.link(bf, "onSearch", out, "doPrint");
    }, "");
    check("memfifo_evict", [](Scheme &s) {
        auto mem = s.add("MemFifo"), src = constant(s, "X"), out = printer(s);
        s.set(mem, "Count", "2");
        s.set(mem, "Default", "String(D)");
        s.start(src, "doData");
        s.link(src, "onEventData", mem, "doValue");
        s.link(mem, "onData", out, "doPrint");
    }, "D\n");
    check("memfifo_value", [](Scheme &s) {
        auto mem = s.add("MemFifo"), src = constant(s, "X"), fmt = s.add("FormatStr"), out = printer(s);
        s.set(mem, "Count", "2");
        s.set(mem, "Default", "String(D)");
        s.set(fmt, "DataCount", "2");
        s.set(fmt, "Mask", "%1-%2");
        s.start(src, "doData");
        s.link(src, "onEventData", mem, "doValue");
        s.link(mem, "onData", fmt, "doString");
        s.link(fmt, "Str1", mem, "Value1");
        s.link(fmt, "Str2", mem, "Value2");
        s.link(fmt, "onFString", out, "doPrint");
    }, "X-D\n");
    check("repeat_counter", [](Scheme &s) {
        auto rep = s.add("Repeat"), cnt = s.add("Counter"), out = printer(s);
        s.set(rep, "Type", "1");
        s.set(rep, "Op2", "Integer(3)");
        s.set(rep, "Check", "0");
        s.set(cnt, "Default", "0");
        s.start(rep, "doRepeat");
        s.link(rep, "onRepeat", cnt, "doNext");
        s.link(cnt, "onNext", out, "doPrint");
        s.link(rep, "Op1", cnt, "Count");
    }, "1\n2\n3\n");
    check("getdata_shared", [](Scheme &s) {
        auto mem = s.add("hcMemory"), gd = s.add("GetData"), fmt = s.add("FormatStr"), out = printer(s);
        s.set(mem, "Default", "String(V)");
        s.set(gd, "Count", "2");
        s.set(fmt, "DataCount", "2");
        s.set(fmt, "Mask", "%1%2");
        s.link(gd, "Data", mem, "Value");
        s.link(fmt, "Str1", gd, "Data1");
        s.link(fmt, "Str2", gd, "Data2");
        s.start(fmt, "doString");
        s.link(fmt, "onFString", out, "doPrint");
    }, "VV\n");
    check("getindexdata_var", [](Scheme &s) {
        auto m1 = s.add("hcMemory"), m2 = s.add("hcMemory"), g = s.add("GetIndexData");
        auto idx = constant(s, "1"), fmt = s.add("FormatStr"), out = printer(s);
        s.set(m1, "Default", "String(one)");
        s.set(m2, "Default", "String(two)");
        s.set(g, "Count", "2");
        s.set(fmt, "DataCount", "1");
        s.set(fmt, "Mask", "%1");
        s.link(g, "Data1", m1, "Value");
        s.link(g, "Data2", m2, "Value");
        s.start(idx, "doData");
        s.link(idx, "onEventData", g, "doIndex");
        s.link(fmt, "Str1", g, "Var");
        s.link(g, "onIndex", fmt, "doString");
        s.link(fmt, "onFString", out, "doPrint");
    }, "two\n");
    check("multistrpart_parts", [](Scheme &s) {
        auto msp = s.add("MultiStrPart"), src = constant(s, "a=b"), fmt = s.add("FormatStr"), out = printer(s);
        s.set(msp, "Char", "=");
        s.set(msp, "Count", "2");
        s.set(fmt, "DataCount", "2");
        s.set(fmt, "Mask", "%1|%2");
        s.start(src, "doData");
        s.link(src, "onEventData", msp, "doSplit");
        s.link(msp, "onSplit", fmt, "doString");
        s.link(fmt, "Str1", msp, "Part1");
        s.link(fmt, "Str2", msp, "Part2");
        s.link(fmt, "onFString", out, "doPrint");
    }, "a|b\n");
    check("date_real_to_heapint", [](Scheme &s) {
        auto dc = s.add("DateConvertor"), src = constant(s, "719163"), fmt = s.add("FormatStr"), out = printer(s);
        s.set(dc, "Mode", "0");
        s.set(fmt, "DataCount", "5");
        s.set(fmt, "Mask", "%1 %2 %3 %4 %5");
        s.reveal(dc, "Year");
        s.reveal(dc, "Month");
        s.reveal(dc, "Day");
        s.reveal(dc, "DayOfWeek");
        s.reveal(dc, "LeapYear");
        s.start(src, "doData");
        s.link(src, "onEventData", dc, "doConvert");
        s.link(dc, "onResult", fmt, "doString");
        s.link(fmt, "Str1", dc, "Year");
        s.link(fmt, "Str2", dc, "Month");
        s.link(fmt, "Str3", dc, "Day");
        s.link(fmt, "Str4", dc, "DayOfWeek");
        s.link(fmt, "Str5", dc, "LeapYear");
        s.link(fmt, "onFString", out, "doPrint");
    }, "1970 1 1 4 0\n");
    check("date_unix_to_heapint", [](Scheme &s) {
        auto dc = s.add("DateConvertor"), src = constant(s, "0"), fmt = s.add("FormatStr"), out = printer(s);
        s.set(dc, "Mode", "12");
        s.set(fmt, "DataCount", "5");
        s.set(fmt, "Mask", "%1 %2 %3 %4 %5");
        s.reveal(dc, "Year");
        s.reveal(dc, "Month");
        s.reveal(dc, "Day");
        s.reveal(dc, "DayOfWeek");
        s.reveal(dc, "LeapYear");
        s.start(src, "doData");
        s.link(src, "onEventData", dc, "doConvert");
        s.link(dc, "onResult", fmt, "doString");
        s.link(fmt, "Str1", dc, "Year");
        s.link(fmt, "Str2", dc, "Month");
        s.link(fmt, "Str3", dc, "Day");
        s.link(fmt, "Str4", dc, "DayOfWeek");
        s.link(fmt, "Str5", dc, "LeapYear");
        s.link(fmt, "onFString", out, "doPrint");
    }, "1970 1 1 4 0\n");
    check("date_heapint_to_real", [](Scheme &s) {
        auto dc = s.add("DateConvertor"), m1 = s.add("hcMemory"), m2 = s.add("hcMemory"), m3 = s.add("hcMemory");
        auto fmt = s.add("FormatStr"), out = printer(s);
        s.set(dc, "Mode", "1");
        s.set(m1, "Default", "String(1970)");
        s.set(m2, "Default", "String(1)");
        s.set(m3, "Default", "String(1)");
        s.set(fmt, "DataCount", "1");
        s.set(fmt, "Mask", "%1");
        s.reveal(dc, "DYear");
        s.reveal(dc, "DMonth");
        s.reveal(dc, "DDay");
        s.reveal(dc, "Result");
        s.link(dc, "DYear", m1, "Value");
        s.link(dc, "DMonth", m2, "Value");
        s.link(dc, "DDay", m3, "Value");
        s.link(fmt, "Str1", dc, "Result");
        s.start(dc, "doConvert");
        s.link(dc, "onResult", fmt, "doString");
        s.link(fmt, "onFString", out, "doPrint");
    }, "719163\n");
    check("date_heapint_to_unix", [](Scheme &s) {
        auto dc = s.add("DateConvertor"), m1 = s.add("hcMemory"), m2 = s.add("hcMemory"), m3 = s.add("hcMemory");
        auto fmt = s.add("FormatStr"), out = printer(s);
        s.set(dc, "Mode", "13");
        s.set(m1, "Default", "String(1970)");
        s.set(m2, "Default", "String(1)");
        s.set(m3, "Default", "String(1)");
        s.set(fmt, "DataCount", "1");
        s.set(fmt, "Mask", "%1");
        s.reveal(dc, "DYear");
        s.reveal(dc, "DMonth");
        s.reveal(dc, "DDay");
        s.reveal(dc, "Result");
        s.link(dc, "DYear", m1, "Value");
        s.link(dc, "DMonth", m2, "Value");
        s.link(dc, "DDay", m3, "Value");
        s.link(fmt, "Str1", dc, "Result");
        s.start(dc, "doConvert");
        s.link(dc, "onResult", fmt, "doString");
        s.link(fmt, "onFString", out, "doPrint");
    }, "0\n");
    check("date_strfmt_to_heapint", [](Scheme &s) {
        auto dc = s.add("DateConvertor"), src = constant(s, "01/01/2026"), fmt = s.add("FormatStr"), out = printer(s);
        s.set(dc, "Mode", "4");
        s.set(dc, "Format", "dd/MM/yyyy");
        s.set(fmt, "DataCount", "3");
        s.set(fmt, "Mask", "%1-%2-%3");
        s.reveal(dc, "Year");
        s.reveal(dc, "Month");
        s.reveal(dc, "Day");
        s.start(src, "doData");
        s.link(src, "onEventData", dc, "doConvert");
        s.link(dc, "onResult", fmt, "doString");
        s.link(fmt, "Str1", dc, "Year");
        s.link(fmt, "Str2", dc, "Month");
        s.link(fmt, "Str3", dc, "Day");
        s.link(fmt, "onFString", out, "doPrint");
    }, "2026-1-1\n");
    check("date_heapint_to_strfmt", [](Scheme &s) {
        auto dc = s.add("DateConvertor"), m1 = s.add("hcMemory"), m2 = s.add("hcMemory"), m3 = s.add("hcMemory");
        auto fmt = s.add("FormatStr"), out = printer(s);
        s.set(dc, "Mode", "5");
        s.set(dc, "Format", "dd/MM/yyyy");
        s.set(m1, "Default", "String(2026)");
        s.set(m2, "Default", "String(1)");
        s.set(m3, "Default", "String(1)");
        s.set(fmt, "DataCount", "1");
        s.set(fmt, "Mask", "%1");
        s.reveal(dc, "DYear");
        s.reveal(dc, "DMonth");
        s.reveal(dc, "DDay");
        s.reveal(dc, "Result");
        s.link(dc, "DYear", m1, "Value");
        s.link(dc, "DMonth", m2, "Value");
        s.link(dc, "DDay", m3, "Value");
        s.link(fmt, "Str1", dc, "Result");
        s.start(dc, "doConvert");
        s.link(dc, "onResult", fmt, "doString");
        s.link(fmt, "onFString", out, "doPrint");
    }, "01/01/2026\n");
    check("date_real_to_vcl", [](Scheme &s) {
        auto dc = s.add("DateConvertor"), src = constant(s, "693595"), fmt = s.add("FormatStr"), out = printer(s);
        s.set(dc, "Mode", "6");
        s.set(fmt, "DataCount", "1");
        s.set(fmt, "Mask", "%1");
        s.reveal(dc, "Result");
        s.start(src, "doData");
        s.link(src, "onEventData", dc, "doConvert");
        s.link(dc, "onResult", fmt, "doString");
        s.link(fmt, "Str1", dc, "Result");
        s.link(fmt, "onFString", out, "doPrint");
    }, "1\n");
    check("date_vcl_to_real", [](Scheme &s) {
        auto dc = s.add("DateConvertor"), src = constant(s, "1"), fmt = s.add("FormatStr"), out = printer(s);
        s.set(dc, "Mode", "7");
        s.set(fmt, "DataCount", "1");
        s.set(fmt, "Mask", "%1");
        s.reveal(dc, "Result");
        s.start(src, "doData");
        s.link(src, "onEventData", dc, "doConvert");
        s.link(dc, "onResult", fmt, "doString");
        s.link(fmt, "Str1", dc, "Result");
        s.link(fmt, "onFString", out, "doPrint");
    }, "693595\n");
    check("date_unix_to_real", [](Scheme &s) {
        auto dc = s.add("DateConvertor"), src = constant(s, "0"), fmt = s.add("FormatStr"), out = printer(s);
        s.set(dc, "Mode", "18");
        s.set(fmt, "DataCount", "1");
        s.set(fmt, "Mask", "%1");
        s.reveal(dc, "Result");
        s.start(src, "doData");
        s.link(src, "onEventData", dc, "doConvert");
        s.link(dc, "onResult", fmt, "doString");
        s.link(fmt, "Str1", dc, "Result");
        s.link(fmt, "onFString", out, "doPrint");
    }, "719163\n");
    check("date_real_to_unix", [](Scheme &s) {
        auto dc = s.add("DateConvertor"), src = constant(s, "719163"), fmt = s.add("FormatStr"), out = printer(s);
        s.set(dc, "Mode", "19");
        s.set(fmt, "DataCount", "1");
        s.set(fmt, "Mask", "%1");
        s.reveal(dc, "Result");
        s.start(src, "doData");
        s.link(src, "onEventData", dc, "doConvert");
        s.link(dc, "onResult", fmt, "doString");
        s.link(fmt, "Str1", dc, "Result");
        s.link(fmt, "onFString", out, "doPrint");
    }, "0\n");
    // Write (C++17 line output): the incoming data, or its Data input when the event carries none.
    check("write_data", [](Scheme &s) {
        auto src = constant(s, "written"), w = s.add("Write");
        s.start(src, "doData");
        s.link(src, "onEventData", w, "doWrite");
    }, "written\n");
    check("write_linked", [](Scheme &s) {
        auto mem = s.add("Memory"), w = s.add("Write");
        s.set(mem, "Default", "String(kept)");
        s.link(mem, "Value", w, "Data");
        s.start(w, "doWrite");
    }, "kept\n");
    // If (C++17 conditional): onTrue or onFalse by comparing Left and Right; Type 0 is equality.
    check("if_equal", [](Scheme &s) {
        auto cmp = s.add("If"), yes = constant(s, "yes"), no = constant(s, "no");
        auto out1 = printer(s), out2 = printer(s);
        s.set(cmp, "Left", "3");
        s.set(cmp, "Right", "3");
        s.start(cmp, "doCompare");
        s.link(cmp, "onTrue", yes, "doData");
        s.link(cmp, "onFalse", no, "doData");
        s.link(yes, "onEventData", out1, "doPrint");
        s.link(no, "onEventData", out2, "doPrint");
    }, "yes\n");
    check("if_less_false", [](Scheme &s) {
        auto cmp = s.add("If"), yes = constant(s, "yes"), no = constant(s, "no");
        auto out1 = printer(s), out2 = printer(s);
        s.set(cmp, "Type", "1");
        s.set(cmp, "Left", "5");
        s.set(cmp, "Right", "3");
        s.start(cmp, "doCompare");
        s.link(cmp, "onTrue", yes, "doData");
        s.link(cmp, "onFalse", no, "doData");
        s.link(yes, "onEventData", out1, "doPrint");
        s.link(no, "onEventData", out2, "doPrint");
    }, "no\n");
    // The loop value must reach the writer through a hub (For -> Hub -> Write).
    check("for_hub_data", [](Scheme &s) {
        auto loop = s.add("For"), hub = s.add("Hub"), out = s.add("Write");
        s.set(loop, "Start", "1");
        s.set(loop, "End", "2");
        s.set(loop, "IncludeEnd", "0");
        s.start(loop, "doFor");
        s.link(loop, "onEvent", hub, "doEvent1");
        s.link(hub, "onEvent1", out, "doWrite");
    }, "1\n2\n");
}
