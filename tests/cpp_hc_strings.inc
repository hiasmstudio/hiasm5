// Cases for the Core string and array elements. Included by cpp_hc_test.cpp.

// DoData -> element.work; the element's result event feeds a printer.
static void chain(Scheme &s, const char *text, Element *e, const char *work, const char *event, Element *out) {
    auto src = constant(s, text);
    s.start(src, "doData");
    s.link(src, "onEventData", e, work);
    s.link(e, event, out, "doPrint");
}

static void checkStrings() {
    check("strpos", [](Scheme &s) {
        auto pos = s.add("hcStrPos"), out = printer(s), none = constant(s, "none"), out2 = printer(s);
        s.set(pos, "Target", "world");
        chain(s, "hello world", pos, "doSearch", "onSearch", out);
        s.link(pos, "onNotFound", none, "doData");
        s.link(none, "onEventData", out2, "doPrint");
    }, "6\n");
    check("copy", [](Scheme &s) {
        auto copy = s.add("hcCopy");
        s.set(copy, "Position", "2");
        s.set(copy, "Count", "3");
        chain(s, "Hello", copy, "doCopy", "onCopy", printer(s));
    }, "ell\n");
    check("copy_right_to_left", [](Scheme &s) {
        auto copy = s.add("hcCopy");
        s.set(copy, "Position", "1");
        s.set(copy, "Count", "2");
        s.set(copy, "Direction", "1");
        chain(s, "Hello", copy, "doCopy", "onCopy", printer(s));
    }, "lo\n");
    check("replace", [](Scheme &s) {
        auto rep = s.add("hcReplace");
        s.set(rep, "Source", "-");
        s.set(rep, "Dest", "+");
        chain(s, "a-b-c", rep, "doReplace", "onReplace", printer(s));
    }, "a+b+c\n");
    check("insert", [](Scheme &s) {
        auto ins = s.add("hcInsert");
        s.set(ins, "SubStr", "b");
        s.set(ins, "Position", "2");
        chain(s, "ac", ins, "doInsert", "onInsert", printer(s));
    }, "abc\n");
    check("delete", [](Scheme &s) {
        auto del = s.add("hcDelete");
        s.set(del, "Position", "2");
        s.set(del, "Count", "2");
        chain(s, "abcd", del, "doDelete", "onDelete", printer(s));
    }, "ad\n");
    check("strpart", [](Scheme &s) {
        auto part = s.add("hcStrPart"), left = printer(s);
        s.set(part, "Char", "=");
        chain(s, "k=v", part, "doSplit", "onSplit", printer(s));
        s.link(part, "onPart", left, "doPrint");
    }, "v\nk\n");
    check("formatstr", [](Scheme &s) {
        auto fmt = s.add("hcFormatStr"), a = s.add("hcMemory"), b = s.add("hcMemory");
        s.set(a, "Default", "String(x)");
        s.set(b, "Default", "String(y)");
        s.set(fmt, "Mask", "100%% %2-%1");
        s.start(fmt, "doString");
        s.link(a, "Value", fmt, "Str1");
        s.link(b, "Value", fmt, "Str2");
        s.link(fmt, "onFString", printer(s), "doPrint");
    }, "100% y-x\n");
    check("blockfind", [](Scheme &s) {
        auto find = s.add("hcBlockFind"), end = printer(s);
        s.set(find, "StartBlock", "[");
        s.set(find, "EndBlock", "]");
        chain(s, "a[b]c[d]", find, "doSearch", "onSearch", printer(s));
        s.link(find, "onEndSearch", end, "doPrint");
    }, "[b]\n[d]\na[b]c[d]\n");
    check("blockfind_inner", [](Scheme &s) {
        auto find = s.add("hcBlockFind"), end = printer(s);
        s.set(find, "StartBlock", "[");
        s.set(find, "EndBlock", "]");
        s.set(find, "IncludeBlock", "1");
        chain(s, "a[b]c[d]", find, "doSearch", "onSearch", printer(s));
        s.link(find, "onEndSearch", end, "doPrint");
    }, "b\nd\na[b]c[d]\n");
    check("blockfind_delete", [](Scheme &s) {
        auto find = s.add("hcBlockFind");
        s.set(find, "StartBlock", "[");
        s.set(find, "EndBlock", "]");
        s.set(find, "Delete", "0");
        s.set(find, "UserReplace", "1");
        s.set(find, "ReplaceStr", "X");
        chain(s, "a[b]c[d]", find, "doSearch", "onEndSearch", printer(s));
    }, "aXcX\n");
    check("blockfind_delete_inner", [](Scheme &s) {
        auto find = s.add("hcBlockFind");
        s.set(find, "StartBlock", "[");
        s.set(find, "EndBlock", "]");
        s.set(find, "IncludeBlock", "1");
        s.set(find, "Delete", "0");
        s.set(find, "UserReplace", "1");
        s.set(find, "ReplaceStr", "X");
        chain(s, "a[b]c[d]", find, "doSearch", "onEndSearch", printer(s));
    }, "a[X]c[X]\n");
    check("blockfind_notfound", [](Scheme &s) {
        auto find = s.add("hcBlockFind"), none = constant(s, "none"), out = printer(s), end = printer(s);
        s.set(find, "StartBlock", "[");
        s.set(find, "EndBlock", "]");
        s.reveal(find, "onNotFind");
        auto src = constant(s, "abc");
        s.start(src, "doData");
        s.link(src, "onEventData", find, "doSearch");
        s.link(find, "onNotFind", none, "doData");
        s.link(none, "onEventData", out, "doPrint");
        s.link(find, "onEndSearch", end, "doPrint");
    }, "none\nabc\n");
    check("strlist_array", [](Scheme &s) {
        auto list = s.add("hcStrList"), second = s.add("hcStrList"), en = s.add("hcArrayEnum"), rd = s.add("hcArrayRead");
        auto hub = s.add("hcHub"), out1 = printer(s), out2 = printer(s), out3 = printer(s);
        s.set(list, "Strings", "p\nq");
        s.set(second, "Strings", "p\nq");
        s.set(rd, "Index", "Integer(1)");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", en, "doEnum");
        s.link(hub, "onEvent2", rd, "doRead");
        s.link(en, "onItem", out1, "doPrint");
        s.link(rd, "onRead", out2, "doPrint");
        // A point owns one wire, so every reader gets its own list.
        s.link(en, "Array", list, "Array");
        s.link(rd, "Array", second, "Array");
        s.link(out3, "Text1", list, "Count");
        s.link(en, "onEndEnum", out3, "doPrint");
    }, "p\nq\n2\nq\n");
    check("strlist_add", [](Scheme &s) {
        auto list = s.add("hcStrList"), add = constant(s, "q"), out = printer(s);
        s.set(list, "Strings", "p");
        s.start(add, "doData");
        s.link(add, "onEventData", list, "doAdd");
        s.link(list, "onChange", out, "doPrint");
        s.link(out, "Text1", list, "Text");
    }, "p\nq\n");
    check("strlist_clear", [](Scheme &s) {
        auto list = s.add("hcStrList"), out = printer(s);
        s.set(list, "Strings", "p\nq");
        s.reveal(list, "doClear");
        s.start(list, "doClear");
        s.link(list, "onChange", out, "doPrint");
        s.link(out, "Text1", list, "Count");
    }, "0\n");
    check("strlist_text", [](Scheme &s) {
        auto list = s.add("hcStrList"), src = constant(s, "z"), out = printer(s);
        s.set(list, "Strings", "p");
        s.start(src, "doData");
        s.link(src, "onEventData", list, "doText");
        s.link(list, "onChange", out, "doPrint");
        s.link(out, "Text1", list, "Text");
    }, "z\n");
    check("strlist_save_load", [](Scheme &s) {
        auto list = s.add("hcStrList"), hub = s.add("hcHub"), out = printer(s);
        s.set(list, "Strings", "p\nq");
        s.set(list, "FileName", workDir + "/list.txt");
        s.set(hub, "OutCount", "4");
        s.reveal(list, "doClear");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", list, "doSave");
        s.link(hub, "onEvent2", list, "doClear");
        s.link(hub, "onEvent3", list, "doLoad");
        s.link(hub, "onEvent4", out, "doPrint");
        s.link(out, "Text1", list, "Text");
    }, "p\nq\n");
    check("copy_position_right", [](Scheme &s) {
        auto copy = s.add("hcCopy");
        s.set(copy, "Position", "2");
        s.set(copy, "Count", "2");
        s.set(copy, "Direction", "1");
        chain(s, "Hello", copy, "doCopy", "onCopy", printer(s));
    }, "ll\n");
    check("delete_right_to_left", [](Scheme &s) {
        auto del = s.add("hcDelete");
        s.set(del, "Position", "2");
        s.set(del, "Count", "2");
        s.set(del, "Direction", "1");
        chain(s, "abcde", del, "doDelete", "onDelete", printer(s));
    }, "abe\n");
    check("strpart_left_right", [](Scheme &s) {
        auto part = s.add("hcStrPart"), left = printer(s), right = printer(s);
        s.set(part, "Char", "=");
        s.reveal(part, "Left");
        s.reveal(part, "Right");
        auto src = constant(s, "k=v");
        s.start(src, "doData");
        s.link(src, "onEventData", part, "doSplit");
        s.link(part, "onSplit", right, "doPrint");
        s.link(part, "onPart", left, "doPrint");
        s.link(left, "Text1", part, "Left");
        s.link(right, "Text1", part, "Right");
    }, "v\nk\n");
}
