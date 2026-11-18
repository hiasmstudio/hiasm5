"""Builds MARKDOWN/CPP_ELEMENTS_PLAN.md from packs/cpp/elements.sql.

`elements.sql` is the source: it is loaded into an in-memory SQLite database here, so the
plan cannot be built from a stale packs/cpp/elements.db. The database has two tables that
matter for the plan:

- `elements(id, name, info, tab, pos, hash)`: what the C++ package implements; `tab` is a
  group id from `groups`;
- `planned_elements(name, info, group_name, icon)`: Delphi elements that are not implemented
  yet. Their configurations may already exist; the implementation is what is missing.

When an element gets C++ code, move its row from `planned_elements` into `elements` in
packs/cpp/elements.sql and run

    python tools/cpp_elements_plan.py

Elements that exist only in the IDE are listed in IDE_ONLY below; the list follows
MARKDOWN/DELPHI2CPP_ELEMENTS_RULES.md, section 2. Partial analogues are not listed here:
they belong to the PARTIAL table of tools/audit_delphi_migration.py.
"""
import sqlite3
import sys
from collections import OrderedDict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "packs" / "cpp" / "elements.sql"
OUT = ROOT / "MARKDOWN" / "CPP_ELEMENTS_PLAN.md"

IDE_ONLY = ["hcCommander", "hcCommanHandler", "hcSDKEvents", "hcAddElement", "hcMoveElement", "hcConnectElement"]
HIDDEN = "(скрытая группа)"


def cell(text):
    return (text or "").replace("|", "\\|").replace("\n", " ")


def load():
    db = sqlite3.connect(":memory:")
    db.executescript(SOURCE.read_text(encoding="utf-8"))
    return db


def main():
    db = load()
    groups = list(db.execute("SELECT id, name, info, pos FROM groups ORDER BY pos"))
    kept = {gid: db.execute("SELECT count(*) FROM elements WHERE tab=? AND name NOT LIKE '*%'", (gid,)).fetchone()[0]
            for gid, _, _, _ in groups}
    planned = list(db.execute("SELECT name, info, group_name, icon FROM planned_elements ORDER BY group_name, name"))

    by_group = OrderedDict((name or HIDDEN, []) for _, name, _, _ in groups)
    for row in planned:
        by_group.setdefault(row[2] or HIDDEN, []).append(row)
    group_names = list(by_group.keys())
    implemented = {(name or HIDDEN): kept[gid] for gid, name, _, _ in groups}

    with_icon = sum(1 for r in planned if r[3])
    text = ["# План реализации элементов пакета «Приложение на C++»", "",
            "Файл создан из `packs/cpp/elements.sql` командой `python tools/cpp_elements_plan.py`; правьте не его, а "
            "`packs/cpp/elements.sql`. Таблица `planned_elements` хранит имена, описания, группу и имя файла иконки "
            "(`packs/cpp/icon`). Палитра её не читает.", "",
            "Группа WinCE не переносится: её элементы и иконки удалены. Частичные аналоги Delphi перечислены в "
            "`tools/audit_delphi_migration.py` (таблица PARTIAL) и в `MARKDOWN/DELPHI_MIGRATION.md`.", "",
            f"Реализовано элементов: {sum(kept.values())}; в плане: {len(planned)}; "
            f"из них с иконкой {with_icon}, без иконки {len(planned) - with_icon}.", "",
            "## Итог по группам", "", "| Группа | Реализовано | В плане |", "|---|---:|---:|"]
    for label in group_names:
        text.append(f"| {cell(label)} | {implemented.get(label, 0)} | {len(by_group[label])} |")

    ide = [r for r in planned if r[0] in IDE_ONLY]
    text += ["", "## Только среда", "",
             "В готовой программе не имеют смысла; остаются в плане, пока не решено, как проверять их в Core.", "",
             "| Элемент | Описание | Иконка |", "|---|---|---|"]
    for name, info, group, icon in ide:
        text.append(f"| `{name}` | {cell(info)} | {cell(icon)} |")

    text += ["", "## Все элементы в плане", ""]
    for label, rows in by_group.items():
        if not rows:
            continue
        text += [f"### {label} ({len(rows)})", "", "| Элемент | Описание | Иконка |", "|---|---|---|"]
        for name, info, group, icon in rows:
            text.append(f"| `{name}` | {cell(info)} | {cell(icon) or '—'} |")
        text.append("")
    OUT.write_text("\n".join(text) + "\n", encoding="utf-8", newline="\n")
    print(f"{OUT.relative_to(ROOT)}: {len(planned)} planned, {sum(kept.values())} implemented, {len(ide)} IDE-only")


if __name__ == "__main__":
    sys.exit(main())
