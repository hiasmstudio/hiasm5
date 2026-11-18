"""Read-only Delphi inventory; name matches never imply behavioral parity."""
import argparse
import json
import re
import sqlite3
from collections import Counter
from pathlib import Path


PARTIAL = {
    "MainForm": ("MainForm", "GtkFixed window, Caption/Resizable/onCreate/doShow/doClose/doCaption; Delphi BorderStyle, Icon, Position, transparency, minimize/restore remain"),
    "Button": ("Button", "Caption, Data, click, geometry/visibility/enabled/hint points; Flat, DefaultBtn, colour and font remain"),
    "Label": ("Label", "Caption, alignment, click and common points; AutoSize, Transparent, ColorHover, Underline remain"),
    "Edit": ("Edit", "Text, Password, ReadOnly, onChange/onEnter, doText2; selection, masks, colour and font remain"),
    "Memo": ("Memo", "Strings, ReadOnly, add/load/save/clear/replace selection, Text/Count; scrolling options and font remain"),
    "CheckBox": ("CheckBox", "Checked/onCheck/doCheck/caption; tri-state and styling remain"),
    "RadioButton": ("RadioButton", "Checked/onCheck; one group per form instead of per-parent groups"),
    "ListBox": ("ListBox", "Strings, add/insert/delete/select, Index/Text/Count/Array, onClick; multi-select, ItemData and owner draw remain"),
    "ComboBox": ("ComboBox", "Strings, add/clear/select, Index/EditText, onClick; uses deprecated GtkComboBoxText"),
    "ProgressBar": ("ProgressBar", "Text and Value in percent; Delphi Max, orientation and styling remain"),
    "TrackBar": ("TrackBar", "Min/Max/Position, onPosition, CurPos; ticks and page size remain"),
    "GroupBox": ("GroupBox", "Captioned frame that is not a parent: controls sit next to it, not inside"),
    "Panel": ("Panel", "Captioned frame and the parent of the nested scheme of ChildPanel (hidden in the palette); bevels, border styles and docking remain"),
    "ChildPanel": ("ChildPanel", "Container: its Panel holds the elements of the nested scheme; a double click enters it and events cross the frame both ways; Delphi colours and dynamic points remain"),
    "ChildForm": ("ChildForm", "Additional window owned by the main window: its elements live in the window, Visible shows it at start, hiding on close; Delphi position and docking remain"),
    "Timer": ("Timer", "Interval/Enable/AutoStop, doTimer/doStop/onTimer/onStop on the GLib main loop; OverCall remains"),
    "Message": ("Message", "Message/Caption/Type, response code in onMessage via GtkAlertDialog; Delphi icon types remain"),
    "DoData": ("hcDoData", "Constant or linked input; Delphi event fallback and conversion details differ"),
    "Math": ("hcMath", "All 44 operations, dynamic operands, result/default/clear, angle units and errors; point names/conversions differ"),
    "If_else": ("hcIf_else", "Dynamic numeric comparison; Delphi string and heterogeneous type semantics remain"),
    "Hub": ("hcHub", "Three fixed outputs; Delphi dynamic point count remains (hcHub has dynamic points)"),
    "For": ("hcFor", "Dynamic range, descending steps, stop and nested invocation; end is exclusive unlike Delphi"),
    "StrCat": ("hcStrCat", "Dynamic UTF-8 operands, stored result and clear; Delphi conversion details remain"),
    "Memory": ("hcMemory", "Typed scalar/string state, default, clear and event snapshots; MT/resource variants remain"),
    "Counter": ("Counter", "Signed 32-bit boundaries, direction, reset and dynamic setters; legacy string conversion differs"),
    "ArrayEnum": ("hcArrayEnum", "Enumerates an array view with stop, Item and Index"),
    "ArrayRead": ("hcArrayRead", "Reads one item by index"),
    "Convertor": ("hcConvertor", "Eleven conversions of the Core combo (IntToStr...StrToReal); stream, Roman and ternary modes remain"),
    "Copy": ("hcCopy", "UTF-8 aware, 1-based positions and RightToLeft as in Delphi"),
    "Delete": ("hcDelete", "UTF-8 aware, 1-based positions and RightToLeft as in Delphi"),
    "Insert": ("hcInsert", "UTF-8 aware, 1-based positions and RightToLeft as in Delphi"),
    "Replace": ("hcReplace", "Replaces every occurrence of Source"),
    "FormatStr": ("hcFormatStr", "%N and %% as in Delphi, dynamic StrN points"),
    "Random": ("hcRandom", "Integer range with a shared generator; Delphi real/gaussian modes and seeds remain"),
    "StrList": ("hcStrList", "Add/text/load/save/clear, Text/Count/Array; sort and search remain"),
    "TimeCounter": ("hcTimeCounter", "Start/Stop with elapsed microseconds"),
}


def decode_bytes(raw):
    try:
        return raw.decode("utf-8-sig")
    except UnicodeDecodeError:
        return raw.decode("cp1251")


def parse(path):
    sections = {}
    section = ""
    for line in decode_bytes(path.read_bytes()).splitlines():
        line = line.strip()
        if not line or line.startswith((";", "//", "##")):
            continue
        if line.startswith("[") and line.endswith("]"):
            section = line[1:-1].casefold()
        elif "=" in line:
            key, value = line.split("=", 1)
            sections.setdefault(section, {})[key] = value
    return sections


def members(sections, name):
    return {key.lstrip("+@*"): {"declaration": key, "definition": value}
            for key, value in sections.get(name, {}).items()}


def audit(root, native):
    configs = {p.stem.casefold(): (p.stem, parse(p)) for p in sorted((root / "conf").glob("*.ini"), key=lambda p: p.name.casefold())}
    if not configs:
        raise ValueError("Legacy conf directory is empty or missing")
    resolved = {}

    def resolve(key, stack=()):
        if key in resolved:
            return resolved[key]
        if key in stack:
            raise ValueError("Inheritance cycle: " + " -> ".join((*stack, key)))
        if key not in configs:
            raise ValueError("Missing inherited configuration: " + key)
        _, config = configs[key]
        props, methods = {}, {}
        inherit = config.get("type", {}).get("Inherit", "")
        for parent in filter(None, (s.strip().casefold() for s in re.split(r"[,;]", inherit))):
            inherited = resolve(parent, (*stack, key))
            props.update(inherited[0])
            methods.update(inherited[1])
        for prop, declaration in members(config, "property").items():
            if prop in props:
                # ElementConfig preserves inherited type/info/flags and only
                # replaces the default value of a redeclared property.
                inherited = dict(props[prop])
                inherited["default_override"] = declaration["definition"]
                inherited["override_source"] = configs[key][0]
                props[prop] = inherited
            else:
                props[prop] = {**declaration, "source": configs[key][0]}
        for method, declaration in members(config, "methods").items():
            methods[method] = {**declaration, "source": configs[key][0]}
        resolved[key] = props, methods
        return props, methods

    group_by_name = {}
    database = root / "Elements.db"
    if database.exists():
        with sqlite3.connect(database.resolve().as_uri() + "?mode=ro", uri=True) as db:
            db.text_factory = decode_bytes
            for name, group in db.execute("SELECT e.name,g.name FROM elements e LEFT JOIN groups g ON g.id=e.tab"):
                group_by_name[name.casefold()] = group or "Unassigned"
    rows = []
    for key, (name, config) in configs.items():
        props, methods = resolve(key)
        native_name, note = PARTIAL.get(name, ("", "No reviewed native C++ implementation"))
        if native_name and not ((native / "conf" / (native_name + ".ini")).exists() or
                                (native.parent / "_base" / "conf" / (native_name + ".ini")).exists()):
            # The application looks for a configuration in the package first, then in Core (packs/_base).
            raise ValueError("Missing mapped native configuration: " + native_name)
        rows.append({"name": name, "group": group_by_name.get(key, config.get("type", {}).get("Tab", "") or "Unassigned"),
                     "class": config.get("type", {}).get("Class", ""), "inherit": config.get("type", {}).get("Inherit", ""),
                     "properties": props, "methods": methods,
                     "native": native_name, "status": "partial" if native_name else "not_ported", "note": note})
    return rows


def write_report(rows, output):
    output.mkdir(parents=True, exist_ok=True)
    summary = Counter(row["status"] for row in rows)
    (output / "DELPHI_MIGRATION.json").write_text(json.dumps({"format": 1, "summary": dict(summary), "elements": rows}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    text = ["# Поэлементный перенос Delphi → C++17", "", "Источник: установленный Delphi-пакет HiAsm 4, только чтение. Перегенерация: `python tools/audit_delphi_migration.py --legacy-pack ПУТЬ --output MARKDOWN`.", "",
            f"Конфигураций: {len(rows)}; частичных функциональных аналогов: {summary['partial']}; без проверенного C++-аналога: {summary['not_ported']}; полностью совместимых: 0.", "",
            "Сопоставление частичных аналогов выполнено явно; одинаковое имя метода не считается доказательством совместимости. Все строки открыты до проверки свойств, событий, данных, сохранения и рабочих примеров на целевых ОС. JSON содержит эффективные объявления с наследованием; исходные определения доступны в INI.", ""]
    for group in sorted({r["group"] for r in rows}, key=str.casefold):
        text += ["## " + group, ""]
        for row in rows:
            if row["group"] != group:
                continue
            mapped = f"; частичный аналог `{row['native']}`: {row['note']}" if row["native"] else "; C++-аналога пока нет"
            text.append(f"- [ ] `{row['name']}` — {len(row['properties'])} свойств, {len(row['methods'])} объявленных методов/точек{mapped}.")
        text.append("")
    (output / "DELPHI_MIGRATION.md").write_text("\n".join(text), encoding="utf-8")
    print(json.dumps({"configurations": len(rows), **summary}, ensure_ascii=False))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--legacy-pack", required=True, type=Path)
    parser.add_argument("--native-pack", type=Path, default=Path(__file__).resolve().parent.parent / "packs/cpp")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    write_report(audit(args.legacy_pack, args.native_pack), args.output)


if __name__ == "__main__":
    main()
