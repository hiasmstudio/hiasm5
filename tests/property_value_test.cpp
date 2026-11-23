#include "ElementProperty.h"
#include "Data.h"
#include <iostream>

#define CHECK(expr) do { if(!(expr)) { std::cerr << __LINE__ << ": " << #expr << std::endl; return 1; } } while(0)

int main(int argc, char **argv) {
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.property-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    ustring value = "unchanged";
    for(const char *bad : {"#1", "#-1:x|", "#999999999999999999999999999:x|", "#3:ab|", "#1:x", "#a:x|", "\""}) {
        CHECK(!DataValue::parse(bad, data_str, &value));
        CHECK(value == "unchanged");
    }
    CHECK(DataValue::parse("", data_str, &value) && value.empty());
    CHECK(DataValue::parse(u8"#6:Привет|0:|2:世界|", data_str, &value));
    CHECK(value == u8"Привет\n\n世界");
    const ustring saved = DataValue::serialize(data_str, &value);
    ustring restored;
    CHECK(DataValue::parse(saved, data_str, &restored) && restored == value);

    FontRecord font;
    CHECK(DataValue::parse("[Sans,12,1,255,0]", data_font, &font));
    FontRecord before = font;
    for(const char *bad : {"", "[", "[Sans,12]", "[Sans,12,1,255,x]", "[Sans,12,1,255,0,2]"}) {
        CHECK(!DataValue::parse(bad, data_font, &font));
        CHECK(font == before);
    }
    int number = 42;
    for(const char *bad : {"", "12x", "99999999999999999999"}) {
        CHECK(!DataValue::parse(bad, data_int, &number) && number == 42);
    }
    TData data;
    CHECK(!DataValue::parse("String", data_data, &data));
    CHECK(DataValue::parse("String(a,b)", data_data, &data) && data.toStr() == "a,b");
    for(const char *bad : {"Integer(x)", "Integer(999999999999999)", "Real(1.2x)", "Real(nan)"}) {
        CHECK(!DataValue::parse(bad, data_data, &data) && data.toStr() == "a,b");
    }
    double real = 2.5;
    for(const char *bad : {"", "nan", "inf", "1e9999", "1.2x"})
        CHECK(!DataValue::parse(bad, data_real, &real) && real == 2.5);
    CHECK(DataValue::parse("-1.25e2", data_real, &real) && real == -125);
    TypeColor color;
    CHECK(DataValue::parse("#123456", data_color, &color));
    TypeColor savedColor = color;
    CHECK(!DataValue::parse("invalid color", data_color, &color) && color == savedColor);
    // Native colors stay RGBA: Delphi syntax is rejected, CGT gets $00BBGGRR.
    for(const char *delphi : {"clBtnFace", "-16777201", "$00FF00"})
        CHECK(!DataValue::parse(delphi, data_color, &color) && color == savedColor);
    CHECK(!color.isDelphi() && color.toDelphi() == 0x563412);
    CHECK(DataValue::serialize(data_color, &color) == color.to_string());

    // External HiAsm 4 packages keep the Delphi TColor (values from HiAsm.exe).
    const struct { const char *text; int value; } legacyColors[] = {
        {"clBtnFace", -16777201}, {"clWindow", -16777211}, {"clWindowText", -16777208},
        {"clWhite", 0xFFFFFF}, {"clSkyBlue", 0xF0CAA6}, {"clNone", 0x1FFFFFFF}, {"clDefault", 0x20000000},
        {"-16777201", -16777201}, {"16711680", 0xFF0000}, {"$00FF00", 0x00FF00}, {"$FF00000F", -16777201},
        {"", 0}, {"0 ", 0}, {"clbtnface", 0}, {"rgb(1,2,3)", 0x030201}, {"#123456", 0x563412},
    };
    for(const auto &legacy : legacyColors) {
        TypeColor parsed;
        if(!DataValue::parse(legacy.text, data_color, &parsed, true) || !parsed.isDelphi() || parsed.toDelphi() != legacy.value)
            std::cerr << "legacy color '" << legacy.text << "' -> " << parsed.toDelphi() << std::endl;
        CHECK(parsed.isDelphi() && parsed.toDelphi() == legacy.value);
        CHECK(DataValue::serialize(data_color, &parsed) == int_to_str(legacy.value));
        TypeColor restored;
        CHECK(DataValue::parse(DataValue::serialize(data_color, &parsed), data_color, &restored, true));
        CHECK(restored == parsed && restored.toDelphi() == legacy.value);
    }
    TypeColor btnFace, window;
    CHECK(DataValue::parse("clBtnFace", data_color, &btnFace, true) && DataValue::parse("clWindow", data_color, &window, true));
    CHECK(btnFace.get_alpha() == 1 && btnFace.get_red_u() >> 8 == 0xF0 && !(btnFace == window));
    TypeColor sameRgb = btnFace;
    sameRgb.set("rgb(240,240,240)");	// rewriting the unchanged preview keeps the TColor
    CHECK(sameRgb.toDelphi() == -16777201);
    sameRgb.set("rgb(1,2,3)");	// property editor edits the RGBA in place
    CHECK(sameRgb.isDelphi() && sameRgb.toDelphi() == 0x030201 && !(sameRgb == btnFace));
    CHECK(DataValue::serialize(data_color, &sameRgb) == "197121");
    TypeColor plain = btnFace;
    plain = Gdk::RGBA("rgb(240,240,240)");	// assignment from a color dialog drops the TColor
    CHECK(!plain.isDelphi() && plain.toDelphi() == 0xF0F0F0);
    ArrayValue colors;
    colors.type = data_color;
    CHECK(DataValue::parse("['a'=clBtnFace,'b'=255]", data_array, &colors, true) && colors.size() == 2);
    CHECK(((TypeColor*)colors[0].data)->toDelphi() == -16777201 && ((TypeColor*)colors[1].data)->toDelphi() == 255);

    FontRecord legacyFont;
    CHECK(DataValue::parse("", data_font, &legacyFont, true));	// empty default: HiAsm 4 IDE font
    CHECK(legacyFont.name == "MS Sans Serif" && legacyFont.size == 8 && legacyFont.style == 0 && legacyFont.color == 0 && legacyFont.charset == 1);
    CHECK(DataValue::parse(" Tahoma,8,0,0,1", data_font, &legacyFont, true));	// INI default after '|'
    CHECK(legacyFont.name == "Tahoma" && legacyFont.size == 8 && legacyFont.charset == 1);
    CHECK(DataValue::parse("[Arial,8,0,0]", data_font, &legacyFont, true));	// v4 scheme without charset
    CHECK(legacyFont.name == "Arial" && legacyFont.size == 8 && legacyFont.color == 0 && legacyFont.charset == 0);
    CHECK(DataValue::parse(u8"[Шрифт,10,1,8388608,204]", data_font, &legacyFont, true));
    CHECK(legacyFont.name == u8"Шрифт" && legacyFont.size == 10 && legacyFont.style == 1 && legacyFont.color == 8388608 && legacyFont.charset == 204);
    FontRecord legacyRestored;
    CHECK(DataValue::parse(DataValue::serialize(data_font, &legacyFont), data_font, &legacyRestored, true) && legacyRestored == legacyFont);
    CHECK(!DataValue::parse(" Tahoma,8,0,0,1", data_font, &legacyRestored) && legacyRestored == legacyFont);

    ArrayValue array;
    array.type = data_str;
    const ustring arrayText = u8"['first'=#6:a,b]世界|,'second'=#0:|]";
    CHECK(DataValue::parse(arrayText, data_array, &array) && array.size() == 2);
    CHECK(*(ustring*)array[0].data == u8"a,b]世界");
    for(const char *bad : {"[", "['a'", "['a'=", "['a'=#500:x|]", "['a'=x,]"}) {
        CHECK(!DataValue::parse(bad, data_array, &array) && array.size() == 2);
        CHECK(*(ustring*)array[0].data == u8"a,b]世界");
    }
    ArrayValue copy(array), assigned;
    assigned = array;
    CHECK(copy[0].data != array[0].data && assigned[0].data != array[0].data);
    *(ustring*)copy[0].data = "copy";
    array.clear();
    CHECK(*(ustring*)assigned[0].data == u8"a,b]世界");
    CHECK(*(ustring*)copy[0].data == "copy");
    CHECK(DataValue::parse("[]", data_array, &assigned) && assigned.empty());
    array.type = data_font;
    CHECK(DataValue::parse("['font'=[Sans,12,1,255,0],'other'=[Serif,9,0,0,0]]", data_array, &array));
    CHECK(array.size() == 2 && ((FontRecord*)array[1].data)->name == "Serif");
    DataValue image(data_pixbuf);
    for(DataType type : {data_icon, data_script, data_stream, data_bitmap, data_wave, data_matr, data_jpeg, data_menu, data_code}) {
        DataValue resource(type), copy(type);
        const ustring original = u8"[ICON[00FF]Привет|#opaque]";
        CHECK(DataValue::parse(original, type, resource.value));
        DataValue::copyValue(type, copy.value, resource.value);
        CHECK(DataValue::serialize(type, copy.value).raw() == original.raw());
        CHECK(DataValue::parse("", type, resource.value));
        CHECK(DataValue::serialize(type, resource.value).empty());
        CHECK(DataValue::serialize(type, copy.value).raw() == original.raw());
    }
    CHECK(DataValue::serialize(data_pixbuf, image.value).empty());
    CHECK(!DataValue::parse("not an image", data_pixbuf, image.value));
    std::cout << "OK property bounds, Unicode lists, array ownership, malformed font/image, HiAsm 4 colors/fonts" << std::endl;
    return 0;
}
