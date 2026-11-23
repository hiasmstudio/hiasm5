#ifndef HIASM_HINT_LAYOUT_H
#define HIASM_HINT_LAYOUT_H
#include <pango/pangocairo.h>
#include <algorithm>

// Measure and paint with the same Pango font, wrapping and Unicode fallback.
inline void drawLayerHint(DrawContext dc, const std::list<ustring> &info,
                         gdouble &x, gdouble &y, int &width, int &height,
                         Widget *parent, const TypePixbuf &icon = {}) {
    struct Row { PangoLayout *layout; int x, y; };
    std::vector<Row> rows;
    std::vector<int> separators;
    width = 32; height = 5;
    const int available = parent ? (std::max)(40, parent->get_allocated_width() - 12) : 480;
    const int maxWidth = (std::min)(480, available);
    bool heading = true;
    for(const auto &text : info) {
        if(text == "-") { separators.push_back(height + 2); height += 5; continue; }
        const int indent = heading && icon ? 24 : 6;
        PangoLayout *layout = pango_cairo_create_layout(dc->cobj());
        PangoFontDescription *font = pango_font_description_from_string(heading ? "Sans Bold" : "Sans");
        pango_font_description_set_absolute_size(font, 11 * PANGO_SCALE);
        pango_layout_set_font_description(layout, font);
        pango_font_description_free(font);
        pango_layout_set_text(layout, text.c_str(), text.bytes());
        pango_layout_set_width(layout, (std::max)(20, maxWidth - indent - 6) * PANGO_SCALE);
        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
        int w = 0, h = 0; pango_layout_get_pixel_size(layout, &w, &h);
        rows.push_back({layout, indent, height});
        width = (std::max)(width, indent + w + 6);
        height += (std::max)(h, heading && icon ? ICON_SIZE : 0) + 3;
        heading = false;
    }
    height += 3;
    if(parent && parent->get_allocated_width() > width)
        x = (std::max)(0.0, (std::min)(x, double(parent->get_allocated_width() - width - 1)));
    if(parent && parent->get_allocated_height() > height)
        y = (std::max)(0.0, (std::min)(y, double(parent->get_allocated_height() - height - 1)));
    dc->save(); dc->set_antialias(Cairo::ANTIALIAS_DEFAULT);
    auto style = parent ? parent->get_style_context() : Glib::RefPtr<Gtk::StyleContext>();
    auto bg = themeBackground(style); Gdk::RGBA fg;
    if(style) fg = themeText(style); else fg.set("#202020");
    const auto border = themeMix(bg, fg, 0.4);
    Gdk::Cairo::set_source_rgba(dc, bg); dc->rectangle(x, y, width, height); dc->fill();
    Gdk::Cairo::set_source_rgba(dc, border); dc->set_line_width(1);
    dc->rectangle(x + 0.5, y + 0.5, width - 1, height - 1); dc->stroke();
    for(int offset : separators) {
        dc->move_to(x + 3, y + offset + 0.5); dc->line_to(x + width - 3, y + offset + 0.5); dc->stroke();
    }
    if(icon) { Gdk::Cairo::set_source_pixbuf(dc, icon, x + 4, y + 5); dc->paint(); }
    Gdk::Cairo::set_source_rgba(dc, fg);
    for(const auto &row : rows) {
        dc->move_to(x + row.x, y + row.y);
        pango_cairo_show_layout(dc->cobj(), row.layout); g_object_unref(row.layout);
    }
    dc->restore();
}
#endif
