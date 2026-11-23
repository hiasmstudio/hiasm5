"""Exercise the public compiler interface with CRLF and non-ASCII paths."""
import os
import re
import sqlite3
import zlib
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory


NESTED = """Make(cpp)
Add(Entry,1,0,0)
{
 link(onStart,2:doWork1,[])
}
Add(MultiElement,2,1,1)
{
 link(onEvent1,4:doPrint,[])
}
BEGIN_SDK
Add(EditMulti,3,0,0)
{
 WorkCount=1
 EventCount=1
 link(doWork1,5:doData,[])
}
Add(DoData,5,0,0)
{
 Data=String(nested-ok)
 link(onEventData,3:onEvent1,[])
}
END_SDK
Add(hcConsole,4,2,2)
{
 Count=1
}
"""


def main():
    app, root = map(Path, sys.argv[1:])
    catalog = sqlite3.connect(":memory:")
    catalog.executescript((root / "packs/cpp/elements.sql").read_text(encoding="utf-8-sig"))
    sections = catalog.execute("SELECT name FROM elements WHERE tab=1 AND name LIKE '*%' ORDER BY pos").fetchall()
    assert [name for (name,) in sections] == ["*Widgets_" + section for section in (
        "Rush", "Graphics", "Dynamic", "Extended", "Managers", "Layouts", "Experiments",
        "TupleTree", "MTTable", "Plotting", "Commands")]
    assert catalog.execute("SELECT tab, pos FROM elements WHERE name='MTStrTbl'").fetchone() == (1, 901)
    assert catalog.execute("SELECT tab FROM elements WHERE name='StringTable'").fetchall() == [(1,)]
    assert catalog.execute("SELECT tab, pos FROM elements WHERE name='StringTableMT'").fetchall() == [(1, 403)]
    assert catalog.execute("SELECT tab, pos FROM elements WHERE name='HilightMemo'").fetchall() == [(1, 38)]
    assert catalog.execute("SELECT name FROM elements WHERE tab=1 AND pos IN (504,505,506) ORDER BY pos").fetchall() == [
        ("IndexManager",), ("IconsManager",), ("WorkIconsManager",)]
    assert catalog.execute("SELECT name FROM elements WHERE tab=1 AND pos BETWEEN 601 AND 605 ORDER BY pos").fetchall() == [
        ("HBoxLayout",), ("VBoxLayout",), ("ToolBarLayout",), ("GridLayout",), ("LayoutSpacer",)]
    assert catalog.execute("SELECT name FROM elements WHERE tab=1 AND pos BETWEEN 101 AND 106 ORDER BY pos").fetchall() == [
        ("CheckBoxRush",), ("RadioButtonRush",), ("ButtonRush",), ("ProgressBarRush",), ("TrackBarRush",), ("ChildPanelRush",)]
    env = dict(os.environ)
    env["PATH"] = str(app.parent) + os.pathsep + env.get("PATH", "")

    def run(*command):
        result = subprocess.run(command, capture_output=True, timeout=120, env=env)
        if result.returncode:
            raise RuntimeError(f"{command}: {result.returncode}\n"
                               + (result.stdout + result.stderr).decode("utf-8", "replace"))
        return result.stdout.decode("utf-8").splitlines()

    with TemporaryDirectory(prefix="HiAsm проекты ") as folder:
        work = Path(folder)
        scheme = work / "проект.sha"
        output = work / ("test.exe" if os.name == "nt" else "test")
        scheme.write_bytes(NESTED.replace("\n", "\r\n").encode())
        run(app, "--build", scheme, output)
        assert run(output) == ["nested-ok"]
        run(app, "--build", root / "packs/cpp/example/math_functions.sha", output)
        assert run(output) == ["1", "3", "domain error", "7"]

        named = (NESTED.replace("MultiElement,", "MultiElementEx,").replace("EditMulti,", "EditMultiEx,")
                 .replace("doWork1", "enter").replace("onEvent1", "done")
                 .replace("WorkCount=1", "WorkCount=#5:enter|").replace("EventCount=1", "EventCount=#4:done|"))
        scheme.write_text(named, encoding="utf-8")
        run(app, "--build", scheme, output)
        assert run(output) == ["nested-ok"]

        if (app.parent / "platforms").is_dir():
            env["QT_PLUGIN_PATH"] = str(app.parent)
        env["QT_QPA_PLATFORM"] = "offscreen"
        run(app, "--build", root / "packs/cpp/example/Info.sha", output)
        scheme.write_text('Make(cpp)\nAdd(MainForm,1,0,0)\n{\n Backend=1\n}\n'
                          'Add(Button,2,80,20)\n{\n Hint=#5:First|6:Second|\n}\n', encoding="utf-8")
        tooltip_source = work / "tooltip.cpp"
        run(app, "--generate", scheme, tooltip_source)
        assert 'button->setToolTip(qt_text("First\\nSecond"));' in tooltip_source.read_text(encoding="utf-8")
        for example in sorted((root / "packs/web/example").rglob("*.sha")):
            target = work / "web.txt"
            run(app, "--generate", example, target)
            source = target.read_text(encoding="utf-8")
            assert source.strip(), example
            if example.stem == "cURL_demo":
                for marker in ["<?php", "curl_init(", "curl_setopt_array($", "curl_exec($", "CURLINFO_HTTP_CODE"]:
                    assert marker in source, (example, marker)
                assert "echo ." not in source

        run(app, "--build", root / "tests/core_qt.sha", output)
        run(output)  # onCreate crosses the child expander twice and closes the root form.

        panel = root / "tests/qt_child_panel.sha"
        run(app, "--generate", panel, work / "panel.cpp")
        source = (work / "panel.cpp").read_text(encoding="utf-8")
        assert source.count("new QLineEdit(widgetParent)") == 2 and "QWidget *widgetParent = panel;" in source
        assert "new HiasmClickable<HiasmPanel>(widgetParent)" in source
        assert 'new HiasmClickable<QGroupBox>(qt_text("Options"), widgetParent)' in source
        assert "group->setFlat(true);" in source
        assert "new HiasmForm(widgetParent->window(), Qt::Window)" in source
        assert "QWidget *widgetParent = child;" in source
        assert 'qt_widget<HiasmForm>(form, "MainForm12")->close();' in source
        run(app, "--build", panel, output)
        run(output)  # Closing the child triggers the panel edit, which reads the outer label and closes the root.

        containers = panel.read_text(encoding="utf-8").replace("Add(ChildPanel,", "Add(ChildPanelEx,")
        containers = containers.replace("Add(ChildForm,", "Add(ChildFormEx,")
        containers = containers.replace("Add(EditMulti,", "Add(EditMultiEx,")
        for name in ("WorkCount", "EventCount", "DataCount"):
            prefix = {"WorkCount": "doWork", "EventCount": "onEvent", "DataCount": "Data"}[name]
            containers = re.sub(rf"\b{name}=1\b", f"{name}=#{len(prefix)+1}:{prefix}1|", containers)
        scheme.write_text(containers, encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        rush_panel = containers.replace("Add(ChildPanelEx,", "Add(ChildPanelRush,").replace("Add(Panel,", "Add(PanelRush,")
        scheme.write_text(rush_panel, encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        group_box = panel.read_text(encoding="utf-8").replace("Add(ChildPanel,", "Add(ChildGroupBox,")
        scheme.write_text(group_box.replace("Add(Panel,", "Add(GroupBox,"), encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        scroll_box = panel.read_text(encoding="utf-8").replace("Add(ChildPanel,", "Add(ChildScrollBox,")
        scheme.write_text(scroll_box.replace("Add(Panel,", "Add(ScrollBox,"), encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)

        for fixture in ("tests/fixtures/cpp_basic_widgets.sha", "tests/qt_cpp_list_widgets.sha",
                        "packs/cpp/example/Widgets/FontBox.sha", "packs/cpp/example/Widgets/DatePicker.sha",
                        "packs/cpp/example/Widgets/StatusBarQt.sha", "packs/cpp/example/Widgets/TabControlQt.sha",
                        "packs/cpp/example/Widgets/LedIndicatorsQt.sha", "packs/cpp/example/Widgets/GrapherQt.sha",
                        "packs/cpp/example/Widgets/SplitterDockingQt.sha", "packs/cpp/example/Widgets/DropManagersQt.sha",
                        "packs/cpp/example/Widgets/MenuQt.sha", "packs/cpp/example/Widgets/PopupMenuQtRtCg.sha",
                        "packs/cpp/example/Widgets/PageControlPolymorphQt.sha", "packs/cpp/example/Widgets/BoxLayoutsQt.sha",
                        "packs/cpp/example/Widgets/GridLayoutQt.sha", "packs/cpp/example/Widgets/ToolBarLayoutQt.sha",
                        "packs/cpp/example/Widgets/RushProgressQt.sha", "packs/cpp/example/Widgets/ToolBarQt.sha",
                        "packs/cpp/example/Widgets/TreeViewQt.sha", "packs/cpp/example/Widgets/TreeViewExQt.sha",
                        "packs/cpp/example/Widgets/StringTableQt.sha", "packs/cpp/example/Widgets/StringTableMTQt.sha",
                        "packs/cpp/example/Widgets/HilightMemoQt.sha", "packs/cpp/example/Widgets/MenuExQt.sha",
                        "packs/cpp/example/Widgets/CommandFamilyQt.sha", "packs/cpp/example/Widgets/TreeViewTrainQt.sha",
                        "packs/cpp/example/Widgets/MSTManagementQt.sha", "packs/cpp/example/Widgets/ChildFormPolyQt.sha",
                        "packs/cpp/example/Widgets/ChildPanelPolyQt.sha"):
            run(app, "--build", root / fixture, output)
        highlight = (root / "packs/cpp/example/Widgets/HilightMemoQt.sha").read_text(encoding="utf-8")
        html_output = work / "table.html"
        array_output = work / "table-columns.txt"
        copied_output = work / "table-copy.txt"
        scheme.write_text(f'''Make(cpp)
Add(MainForm,1,0,0)
{{
 Backend=1
 link(onCreate,2:doEvent1,[])
}}
Add(Hub,2,40,0)
{{
 OutCount=15
 link(onEvent1,4:doData,[])
 link(onEvent2,12:doSelect,[])
 link(onEvent3,13:doData,[])
 link(onEvent4,6:doTabToHTML,[])
 link(onEvent5,7:doSave,[])
 link(onEvent6,8:doEditEnd,[])
 link(onEvent7,9:doBkColor,[])
 link(onEvent8,14:doData,[])
 link(onEvent9,15:doEnum,[])
 link(onEvent10,18:doEnum,[])
 link(onEvent11,20:doSave,[])
 link(onEvent12,22:doSave,[])
 link(onEvent13,23:doNextPage,[])
 link(onEvent14,26:doSave,[])
 link(onEvent15,1:doClose,[])
}}
Add(MTStrTbl,3,80,0)
{{
 Name="Table"
 Columns=#4:Name|
 DrawManager="Style"
}}
Add(DoData,4,40,40)
{{
 Data=String(A & B < C)
 link(onEventData,5:doAddRow,[])
}}
Add(MST_DB,5,80,40)
{{
 MSTControl="Table"
}}
Add(MST_HTML,6,120,40)
{{
 MSTControl="Table"
 TitleText="Inventory"
 link(onTabToHTML,7:doText,[])
}}
Add(Memo,7,160,40)
{{
 FileName="{html_output.as_posix()}"
}}
Add(MST_EditEnd,8,200,40)
{{
 MSTControl="Table"
}}
Add(MST_SetColors,9,240,40)
{{
 MSTControl="Table"
 Point(doBkColor)
}}
Add(MST_DrawManager,10,280,40)
{{
 Name="Style"
 Point(doGradient)
}}
Add(MST_UseEditCtrl,11,320,40)
{{
 MSTControl="Table"
}}
Add(MST_SelectRow,12,360,40)
{{
 MSTControl="Table"
}}
Add(DoData,13,400,40)
{{
 Data=String(Edited & <value>)
 link(onEventData,11:doSetData,[])
}}
Add(DoData,14,440,40)
{{
 Data=Integer(1)
 link(onEventData,10:doGradient,[])
}}
Add(ArrayEnum,15,480,40)
{{
 link(Array,16:Strings,[])
 link(onItem,17:doAddRow,[])
}}
Add(MST_ArrayRows,16,520,40)
{{
 MSTControl="Table"
}}
Add(MST_DB,17,560,40)
{{
 MSTControl="Copy"
}}
Add(ArrayEnum,18,600,40)
{{
 link(Array,19:ColumnArray,[])
 link(onItem,20:doAdd,[])
}}
Add(MST_ArrayCols,19,640,40)
{{
 MSTControl="Copy"
}}
Add(Memo,20,680,40)
{{
 FileName="{array_output.as_posix()}"
}}
Add(MTStrTbl,21,720,40)
{{
 Name="Copy"
 Columns=#11:Copied=1350|
}}
Add(MST_Save,22,760,40)
{{
 MSTControl="Copy"
 FileName="{copied_output.as_posix()}"
}}
Add(MST_Virtual,23,800,40)
{{
 MSTControl="Virtual"
 link(VirtualData,25:Text,[])
 link(onChangeVirtIdx,26:doText,[])
}}
Add(MTStrTbl,24,840,40)
{{
 Name="Virtual"
 VirtualTab=0
 Columns=#4:Name|
}}
Add(Edit,25,880,40)
{{
 Text="Virtual row"
}}
Add(Memo,26,920,40)
{{
 FileName="{(work / 'page.txt').as_posix()}"
}}
''', encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        html = html_output.read_text(encoding="utf-8")
        assert "<h2>Inventory</h2>" in html and "<th>Name</th>" in html
        assert "<td>Edited &amp; &lt;value&gt;</td>" in html, html
        assert array_output.read_text(encoding="utf-8").splitlines()[0].startswith("Copied=1350="), array_output
        assert "Edited & <value>" in copied_output.read_text(encoding="utf-8").splitlines()
        assert int((work / "page.txt").read_text(encoding="utf-8")) > 0
        plot_example = root / "packs/cpp/example/Draw/gui/Plotter/PlotterQt.sha"
        run(app, "--build", plot_example, output)
        plot_output = work / "plot.bmp"
        plot = plot_example.read_text(encoding="utf-8")
        plot = plot.replace(' Caption="Plotter and line series"\n',
                            ' Caption="Plotter and line series"\n link(onCreate,7:doEvent1,[])\n')
        plot = plot.replace(' FileName="PlotterQt.bmp"', f' FileName="{plot_output.as_posix()}"')
        plot = plot.replace(' link(onPosition,4:doAdd,[])\n', '').replace(' link(onClick,2:doSaveToFile,[])\n', '')
        scheme.write_text(plot + '''Add(Hub,7,21,252)
{
 OutCount=3
 link(onEvent1,8:doData,[])
 link(onEvent2,2:doSaveToFile,[])
 link(onEvent3,1:doClose,[])
}
Add(DoData,8,84,252)
{
 Data=Integer(25)
 link(onEventData,4:doAdd,[])
}
''', encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        bmp = plot_output.read_bytes()
        assert bmp.startswith(b"BM")
        assert int.from_bytes(bmp[18:22], "little") == 520
        assert int.from_bytes(bmp[22:26], "little", signed=True) == 240
        plot_output.unlink()
        scatter = scheme.read_text(encoding="utf-8").replace("Add(PlotLines,4,", "Add(PlotPoints,4,")
        scatter = scatter.replace(" Step=0.5\n", " Step=0.5\n Shape=2\n Size=5\n").replace(" Size=2\n", "")
        scheme.write_text(scatter, encoding="utf-8")
        run(app, "--generate", scheme, work / "scatter.cpp")
        assert "series->marker = 2;" in (work / "scatter.cpp").read_text(encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        scatter_bmp = plot_output.read_bytes()
        assert scatter_bmp.startswith(b"BM") and scatter_bmp != bmp
        plot_output.unlink()
        histogram = scatter.replace("Add(PlotPoints,4,", "Add(PlotHistogram,4,").replace(" Shape=2\n", ' Axis="Baseline"\n')
        histogram += '''Add(PlotAxis,9,250,252)
{
 Grapher="Chart"
 Name="Baseline"
 Y=24.7
}
Add(PlotMouseValues,10,300,252)
{
 Grapher="Chart"
 link(onMouseMove,11:doAxis,[])
}
Add(PlotMarker,11,350,252)
{
 Grapher="Chart"
 AxisX="Baseline"
 X=0.4
 Y=24.8
 Text="Sample"
}
Add(PlotSeriesMarker,12,400,252)
{
 Grapher="Chart"
 Series="Slider"
 TextList=#6:Sample|
}
'''
        scheme.write_text(histogram, encoding="utf-8")
        run(app, "--generate", scheme, work / "histogram.cpp")
        histogram_source = (work / "histogram.cpp").read_text(encoding="utf-8")
        assert 'series->baselineAxis = qt_text("PlotAxis9");' in histogram_source
        assert 'listener->moved = [&form]' in histogram_source
        assert 'marker->horizontalAxis = qt_text("PlotAxis9");' in histogram_source
        assert 'marker->seriesTarget = qt_text("PlotHistogram4");' in histogram_source
        run(app, "--build", scheme, output)
        run(output)
        assert plot_output.read_bytes().startswith(b"BM") and plot_output.read_bytes() != scatter_bmp
        stream_plot = histogram.replace(' link(onEvent1,8:doData,[])\n', ' link(onEvent1,14:doSaveToStream,[])\n')
        stream_plot += '''Add(MTStrTbl,13,350,300)
{
 Name="Table"
 Columns=#5:Value|
}
Add(MST_Save,14,400,300)
{
 MSTControl="Table"
 Point(doSaveToStream)
 Point(onSaveToStream)
 link(onSaveToStream,15:doPlotStream,[])
}
Add(PlotStream,15,450,300)
{
 Series="Slider"
 Step=1
 link(onPlotStream,18:doDiff,[])
}
Add(PlotLines,16,450,340)
{
 Grapher="Chart"
 Name="Difference"
}
Add(PlotDiffSeries,18,450,380)
{
 Series1="Slider"
 Series2="Slider"
 ResultSeries="Difference"
 Mask="(%2-%1)*2+1"
}
'''
        scheme.write_text(stream_plot, encoding="utf-8")
        run(app, "--generate", scheme, work / "stream-plot.cpp")
        stream_source = (work / "stream-plot.cpp").read_text(encoding="utf-8")
        assert 'if (qt_plot_stream(' in stream_source
        assert 'result->add(( second - first ) * double(2) + double(1)' in stream_source
        run(app, "--build", scheme, output)
        run(output)
        run(app, "--build", root / "packs/cpp/example/Draw/gui/Plotter/PlotPointsQt.sha", output)
        mail = (root / "packs/cpp/example/Internet/Mail.sha").read_text(encoding="utf-8-sig", errors="replace")
        bitmap = re.search(r"(?m)^\s*Bitmap=(\[ZIP[0-9A-F]+\])$", mail).group(1)
        popup = (root / "packs/cpp/example/Widgets/PopupMenuExQt.sha").read_text(encoding="utf-8")
        popup = popup.replace(' Menu=#7:Actions|1:(|4:Open|1:-|4:Save|1:)|5:Close|\n',
            f" IconByIndex=0\n Bitmaps=['Open'={bitmap}]\n Menu=#7:Actions|1:(|7:<0>Open|1:-|4:Save|1:)|5:Close|\n")
        popup_output = work / "popup-enabled.txt"
        popup = popup.replace(' Caption="Extended popup menu"\n', ' Caption="Extended popup menu"\n link(onCreate,5:doEvent1,[])\n')
        popup = popup.replace(' Point(MenuItemName)\n', ' Point(MenuItemName)\n Point(doIdxEnabled)\n Point(doEnablIdx)\n Point(doEnabledItems)\n Point(onEnabledItems)\n link(onEnabledItems,8:doAdd,[])\n')
        scheme.write_text(popup + f'''Add(Hub,5,28,140)
{{
 OutCount=5
 link(onEvent1,6:doData,[])
 link(onEvent2,9:doData,[])
 link(onEvent3,2:doEnabledItems,[])
 link(onEvent4,8:doSave,[])
 link(onEvent5,1:doClose,[])
}}
Add(DoData,6,84,140)
{{
 Data=Integer(1)
 link(onEventData,7:doGetCol_MT,[])
}}
Add(StringTableMT,7,140,140)
{{
 Columns=#5:Spare|1:0|
 Point(doGetCol_MT)
 Point(onMT_GetCol)
 link(onMT_GetCol,2:doIdxEnabled,[])
}}
Add(Memo,8,250,140)
{{
 FileName="{popup_output.as_posix()}"
}}
Add(DoData,9,84,196)
{{
 Data=Integer(4)
 link(onEventData,2:doEnablIdx,[])
}}
''', encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        enabled_items = popup_output.read_text(encoding="utf-8").splitlines()
        assert enabled_items == ["0", "3"], enabled_items
        popup_output.unlink()
        main_menu = scheme.read_text(encoding="utf-8").replace("Add(PopupMenuEx,2,", "Add(MenuEx,2,")
        main_menu = main_menu.replace(" link(onClick,2:doPopupHere,[])\n", "")
        scheme.write_text(main_menu, encoding="utf-8")
        run(app, "--generate", scheme, work / "menu-ex.cpp")
        menu_source = (work / "menu-ex.cpp").read_text(encoding="utf-8")
        assert 'menu->attachBar();' in menu_source
        assert 'menu->chosen = [&form]' in menu_source
        assert 'menu->indexedIcons = 1;' in menu_source
        assert bitmap in menu_source and '<0>Open' in menu_source
        run(app, "--build", scheme, output)
        run(output)
        enabled_items = popup_output.read_text(encoding="utf-8").splitlines()
        assert enabled_items == ["0", "3"], enabled_items
        palette_example = (root / "packs/cpp/example/Widgets/CtrlPaletteQt.sha").read_text(encoding="utf-8")
        palette_output = work / "palette-items.txt"
        palette_example = palette_example.replace(' Caption="Link palette"\n', ' Caption="Link palette"\n link(onCreate,4:doEvent1,[])\n')
        palette_example = palette_example.replace(' Name="selection"\n', ' Name="selection"\n Point(doGradient)\n')
        palette_example = palette_example.replace(' Strings=#5:First|6:Second|\n',
            f' Strings=#5:First|6:Second|\n FileName="{palette_output.as_posix()}"\n Point(doStrings)\n Point(doAdd)\n Point(doSave)\n')
        scheme.write_text(palette_example + '''Add(Hub,4,28,140)
{
 OutCount=5
 link(onEvent1,6:doData,[])
 link(onEvent2,5:doData,[])
 link(onEvent3,2:doSave,[])
 link(onEvent4,8:doData,[])
 link(onEvent5,1:doClose,[])
}
Add(DoData,5,84,140)
{
 Data=String(Third)
 link(onEventData,2:doAdd,[])
}
Add(DoData,6,84,196)
{
 Data=String(Replacement)
 link(onEventData,2:doStrings,[])
}
Add(DoData,8,84,252)
{
 Data=Integer(0)
 link(onEventData,7:doGradient,[])
}
''', encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        items = palette_output.read_text(encoding="utf-8").splitlines()
        assert items == ["Replacement", "Third"], items
        data_example = (root / "packs/cpp/example/Widgets/StaticDataQt.sha").read_text(encoding="utf-8")
        data_output = work / "enumerated.txt"
        read_output = work / "read.txt"
        data_example = data_example.replace("Add(Memo,6,250,140)\n{\n", f'Add(Memo,6,250,140)\n{{\n FileName="{data_output.as_posix()}"\n')
        data_example = data_example.replace("Add(Hub,2,100,28)\n{\n", "Add(Hub,2,100,28)\n{\n OutCount=6\n")
        data_example = data_example.replace(" link(onEvent2,5:doEnum,[])\n", " link(onEvent2,5:doEnum,[])\n link(onEvent3,4:doRead,[])\n link(onEvent4,6:doSave,[])\n link(onEvent5,8:doSave,[])\n link(onEvent6,1:doClose,[])\n")
        data_example = data_example.replace(" link(Value,7:Text,[])\n", " link(Value,7:Text,[])\n link(onRead,8:doText,[])\n")
        data_example += f'Add(Memo,8,340,140)\n{{\n FileName="{read_output.as_posix()}"\n}}\n'
        scheme.write_text(data_example, encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        roles = data_output.read_text(encoding="utf-8").splitlines()
        assert roles == ["Analyst", "Compiler pioneer"], roles
        role = read_output.read_text(encoding="utf-8")
        assert role == "Compiler pioneer", role
        user_data = (root / "packs/cpp/example/Widgets/UserDataQt.sha").read_text(encoding="utf-8")
        user_data_output = work / "user-data.txt"
        user_data = user_data.replace(' Name="input"\n', ' Name="input"\n link(onInit,8:doData,[])\n link(onClose,7:doSave,[])\n')
        user_data = user_data.replace('Add(Memo,7,305,28)\n{\n',
                                      f'Add(Memo,7,305,28)\n{{\n FileName="{user_data_output.as_posix()}"\n')
        user_data = user_data.replace(' link(onCreate,3:doEnum,[])\n', ' link(onCreate,9:doEvent1,[])\n')
        scheme.write_text(user_data + '''Add(DoData,8,100,100)
{
 Data=String(Before rows)
 link(onEventData,7:doText,[])
}
Add(Hub,9,100,150)
{
 link(onEvent1,3:doEnum,[])
 link(onEvent2,1:doClose,[])
}
''', encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        user_rows = user_data_output.read_text(encoding="utf-8").splitlines()
        assert user_rows == ["Before rows", "Analyst", "Analyst"], user_rows
        list_example = (root / "packs/cpp/example/Widgets/MListBoxQt.sha").read_text(encoding="utf-8")
        scheme.write_text(list_example + '''Add(Timer,5,28,140)
{
 Interval=20
 link(onTimer,1:doClose,[])
}
''', encoding="utf-8")
        run(app, "--generate", scheme, work / "data-list.cpp")
        data_list_source = (work / "data-list.cpp").read_text(encoding="utf-8")
        assert '->refresh(dynamic_cast<HiasmStaticData *>(form.findChild<QObject *>(qt_text("DS_StaticData2"))))' in data_list_source
        assert 'list->captionField = qt_text("Name");' in data_list_source
        assert 'list->dataField = qt_text("Role");' in data_list_source
        run(app, "--build", scheme, output)
        run(output)
        scheme.write_text(highlight + '''Add(Timer,4,28,140)
{
 Interval=20
 link(onTimer,1:doClose,[])
}
''', encoding="utf-8")
        run(app, "--generate", scheme, work / "highlight.cpp")
        highlight_source = (work / "highlight.cpp").read_text(encoding="utf-8")
        assert 'highlighter->addRules(qt_text("HiAsm=Green=B\\nQt=Blue=U"));' in highlight_source
        assert '->addRules(qt_text("Widgets=Purple=C"));' in highlight_source
        run(app, "--build", scheme, output)
        run(output)
        table_input = work / "table-input.txt"
        table_output = work / "table-output.txt"
        table_input.write_bytes(b"Name=140;Value=90\r\nTen;10\r\nTwo;2\r\n")
        scheme.write_text(f'''Make(cpp)
Add(MainForm,1,0,0)
{{
 Backend=1
 link(onCreate,4:doEvent1,[])
}}
Add(StringTable,2,100,0)
{{
 SaveWidth=0
 Point(doSortDigit)
}}
Add(DoData,3,50,0)
{{
 Data=String({table_input.as_posix()})
 link(onEventData,2:doLoad,[])
}}
Add(Hub,4,0,20)
{{
 OutCount=4
 link(onEvent1,3:doData,[])
 link(onEvent2,5:doData,[])
 link(onEvent3,7:doData,[])
 link(onEvent4,1:doClose,[])
}}
Add(DoData,5,50,20)
{{
 Data=Integer(1)
 link(onEventData,2:doSortDigit,[])
}}
Add(DoData,7,50,40)
{{
 Data=String({table_output.as_posix()})
 link(onEventData,2:doSave,[])
}}
''', encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        rows = table_output.read_text(encoding="utf-8").splitlines()
        assert rows == ["Name=140;Value=90", "Two;2", "Ten;10"], rows
        mt_example = (root / "packs/cpp/example/Widgets/StringTableMTQt.sha").read_text(encoding="utf-8")
        mt_output = work / "mt-output.txt"
        mt_example = mt_example.replace(" link(onCreate,2:doGetCol_MT,[])\n", " link(onCreate,4:doEvent1,[])\n")
        mt_example = mt_example.replace(" Point(doGetCol_MT)\n", " Point(doGetCol_MT)\n Point(doMT_WidthCol)\n")
        mt_example = mt_example.replace(" Point(doMT_Add)\n", " Point(doMT_Add)\n Point(doSelect)\n Point(onSelect)\n Point(Select)\n link(onSelect,6:doAdd,[])\n")
        scheme.write_text(mt_example + f'''Add(Hub,4,0,20)
{{
 OutCount=6
 link(onEvent1,10:doData,[])
 link(onEvent2,9:doGetCol_MT,[])
 link(onEvent3,2:doGetCol_MT,[])
 link(onEvent4,3:doSelect,[])
 link(onEvent5,6:doSave,[])
 link(onEvent6,1:doClose,[])
}}
Add(Memo,6,50,20)
{{
 FileName="{mt_output.as_posix()}"
 link(Str,3:Select,[])
}}
Add(StringTableMT,9,100,40)
{{
 Point(doMT_AddCols)
 Point(doGetCol_MT)
 Point(onMT_GetCol)
 link(onMT_GetCol,2:doMT_WidthCol,[])
}}
Add(DoData,10,100,60)
{{
 Data=String(96_120_-1_0)
 link(onEventData,9:doMT_AddCols,[])
}}
''', encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        rows = mt_output.read_text(encoding="utf-8").splitlines()
        assert rows == ["0;Name;96;-1;2"], rows
        tree = (root / "packs/cpp/example/Widgets/TreeViewQt.sha").read_text(encoding="utf-8")
        tree_output = work / "tree.txt"
        scheme.write_text(tree.replace('Caption="Tree view"', 'Caption="Tree view"\n link(onCreate,2:doSave,[])')
                          .replace('Strings=#', f'FileName="{tree_output.as_posix()}"\n Strings=#')
                          + '''Add(Timer,4,28,140)
{
 Interval=20
 link(onTimer,1:doClose,[])
}
''', encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        assert tree_output.read_text(encoding="utf-8").splitlines() == ["Folder", "(", "First", "Second", ")", "Root"]
        tree_output.unlink()
        extended_tree = tree.replace('Caption="Tree view"', 'Caption="Extended tree"\n link(onCreate,5:doTimer,[])')
        extended_tree = extended_tree.replace('Add(TreeView,', 'Add(TreeViewEx,').replace(
            ' Strings=#', f' FileName="{tree_output.as_posix()}"\n IconByIndex=0\n Point(doAddChild)\n'
            ' Point(doInsert)\n Point(Str)\n link(Str,9:Text,[])\n link(onChange,2:doSave,[])\n Strings=#')
        extended_tree = extended_tree.replace('5:First|', '14:<0,1,2,3>First|')
        scheme.write_text(extended_tree + '''Add(Timer,4,28,140)
{
 Interval=80
 link(onTimer,1:doClose,[])
}
Add(Timer,5,28,196)
{
 Enable=1
 Interval=5
 AutoStop=1
 link(onTimer,6:doData,[])
}
Add(DoData,6,84,196)
{
 link(onEventData,2:doAddChild,[])
}
Add(Timer,7,28,252)
{
 Interval=30
 AutoStop=1
 link(onTimer,8:doData,[])
}
Add(DoData,8,84,252)
{
 Data=Integer(1)
 link(onEventData,2:doInsert,[])
}
Add(Edit,9,252,140)
{
 Text="Third"
}
''', encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        assert tree_output.read_text(encoding="utf-8").splitlines() == [
            "Folder", "(", "<0,1,2,3>First", "Third", "Second", "Third", ")", "Root"]
        rush = (root / "packs/cpp/example/Widgets/RushProgressQt.sha").read_text(encoding="utf-8")
        scheme.write_text(rush.replace('Caption="Rush progress"', 'Caption="Rush progress"\n link(onCreate,4:doCheck,[])')
                          + '''Add(Timer,6,224,196)
{
 Interval=20
 link(onTimer,1:doClose,[])
}
''', encoding="utf-8")
        run(app, "--generate", scheme, work / "rush.cpp")
        rush_source = (work / "rush.cpp").read_text(encoding="utf-8")
        assert 'qt_widget<HiasmRushCheckBox>(form, "CheckBoxRush4")' in rush_source
        assert 'qt_widget<HiasmRushRadio>(form, "RadioButtonRush5")' in rush_source
        assert 'choice->isChecked()' in rush_source
        assert 'qt_widget<QSlider>(form, "TrackBarRush7")' in rush_source
        assert 'qt_widget<HiasmRushProgress>(form, "ProgressBarRush3")' in rush_source
        run(app, "--build", scheme, output)
        run(output)
        hint_source = work / "hint.cpp"
        run(app, "--generate", root / "packs/cpp/example/Widgets/DropManagersQt.sha", hint_source)
        hint_text = hint_source.read_text(encoding="utf-8")
        assert 'const auto targets = qt_text("Label2")' in hint_text
        assert 'label->setToolTip(qt_text("Drop plain text here"));' in hint_text

        toolbar = (root / "packs/cpp/example/Widgets/ToolBarQt.sha").read_text(encoding="utf-8")
        toolbar = toolbar.replace("'First=Disabled at startup'=[]", f"'First=Disabled at startup'={bitmap}")
        toolbar = toolbar.replace(" link(onClick,1:doClose,[])\n", "")
        scheme.write_text(toolbar + '''Add(Timer,4,28,196)
{
 Interval=50
 link(onTimer,1:doClose,[])
}
''', encoding="utf-8")
        toolbar_source = work / "toolbar.cpp"
        run(app, "--generate", scheme, toolbar_source)
        toolbar_text = toolbar_source.read_text(encoding="utf-8")
        assert bitmap in toolbar_text
        assert 'qt_widget<HiasmToolBar>(form, "ToolBar2")->enable(0, false);' in toolbar_text
        run(app, "--build", scheme, output)
        run(output)
        palette = (root / "packs/cpp/example/Widgets/Palette.sha").read_text(encoding="utf-8-sig", errors="replace")
        icon = re.search(r"(?m)^ Icons=\['Icon'=(\[ZIP[0-9A-F]+\])", palette).group(1)
        scheme.write_text(f'''Make(cpp)
Add(MainForm,1,0,0)
{{
 Backend=1
 TransparentManager="transparency"
 link(onCreate,5:doDisk,[])
}}
Add(BitBtn,2,0,20)
{{
 Width=150
 Caption="Bitmap button"
 Bitmap={bitmap}
 link(onClick,1:doClose,[])
}}
Add(Image,3,0,40)
{{
 Picture={bitmap}
}}
Add(Image,4,0,60)
{{
 link(onClick,4:doLoad,[])
 link(Bitmap,3:ImageBitmap,[])
}}
Add(DriveBox,5,0,80)
{{
 Disk="*"
 Point(doDisk)
}}
Add(PaintBox,6,0,100)
{{
 Width=20
 Height=20
 link(onBeforeDraw,7:doLoad,[])
}}
Add(AlphaImage,7,0,120)
{{
 AlphaBlendValue=128
 link(AlphaBitmap,6:Bitmap,[])
}}
Add(ColorBox,8,0,140)
{{
}}
Add(ImgBtn,9,0,160)
{{
 Normal={bitmap}
 Point(Normal)
}}
Add(TransparentManager,10,0,180)
{{
 Name="transparency"
 Picture={bitmap}
 AlphaBlendValue=128
}}
Add(ListBox,11,0,200)
{{
 Strings=#5:First|6:Second|
 IconsManager="icons"
 IndexManager="indexer"
}}
Add(ComboBox,12,0,220)
{{
 Strings=#5:First|6:Second|
 IconsManager="icons"
 IndexManager="indexer"
}}
Add(IconsManager,13,0,240)
{{
 Name="icons"
 Icons=['Icon'={icon}]
}}
Add(IndexManager,14,0,260)
{{
 Name="indexer"
 Index=['skip'=-1,'show'=0]
}}
Add(WorkIconsManager,15,0,280)
{{
 IconsManager="icons"
 Point(doGetIcon)
 Point(doDeleteIcon)
 Point(onGetIcon)
 link(onGetIcon,18:doLoad,[])
}}
Add(Button,16,0,300)
{{
 Data=Integer(0)
 link(onClick,15:doGetIcon,[])
}}
Add(Button,17,0,320)
{{
 Data=Integer(0)
 link(onClick,15:doDeleteIcon,[])
}}
Add(Image,18,0,340)
{{
}}
''', encoding="utf-8")
        generated = work / "bitbtn.cpp"
        run(app, "--generate", scheme, generated)
        image_source = generated.read_text(encoding="utf-8")
        assert bitmap in image_source
        assert 'manager->target = qt_widget<HiasmForm>(form, "MainForm1");' in image_source
        assert 'manager->opacity = 128;' in image_source
        assert icon in image_source
        assert image_source.count('qt_list_icons(target, form, qt_text("IconsManager13"), qt_text("IndexManager14"))') == 2
        assert 'form.findChild<QObject *>(qt_text("IconsManager13"))' in image_source
        assert 'manager->icons.removeAt(index);' in image_source
        run(app, "--build", scheme, output)

        input_icon = work / "input.ico"
        saved_icon = work / "saved.ico"
        input_icon.write_bytes(zlib.decompress(bytes.fromhex(icon[4:-1])[4:]))
        scheme.write_text(f'''Make(cpp)
Add(MainForm,1,0,0)
{{
 Backend=1
 link(onCreate,3:doLoadIcon,[])
}}
Add(IconsManager,2,0,20)
{{
 Name="icons"
}}
Add(WorkIconsManager,3,0,40)
{{
 IconsManager="icons"
 IconFileName="{input_icon.as_posix()}"
 Point(doLoadIcon)
 link(onChangeImgLst,4:doSaveIcon,[])
}}
Add(WorkIconsManager,4,0,60)
{{
 IconsManager="icons"
 IconFileName="{saved_icon.as_posix()}"
 Point(doSaveIcon)
}}
Add(Timer,5,0,80)
{{
 Interval=20
 link(onTimer,1:doClose,[])
}}
''', encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        assert saved_icon.read_bytes().startswith(b"\x00\x00\x01\x00")

        icon_list = work / "list.ico"
        roundtrip = work / "roundtrip.ico"
        scheme.write_text(f'''Make(cpp)
Add(MainForm,1,0,0)
{{
 Backend=1
 link(onCreate,3:doSaveIList,[])
}}
Add(IconsManager,2,0,20)
{{
 Name="icons"
 Icons=['First'={icon},'Second'={icon}]
}}
Add(WorkIconsManager,3,0,40)
{{
 IconsManager="icons"
 IListFileName="{icon_list.as_posix()}"
 Point(doSaveIList)
}}
Add(WorkIconsManager,4,0,60)
{{
 IconsManager="icons"
 IListFileName="{icon_list.as_posix()}"
 Point(doLoadIList)
 link(onChangeImgLst,6:doSaveIList,[])
}}
Add(Timer,5,0,80)
{{
 Interval=20
 AutoStop=1
 Point(onStop)
 link(onTimer,4:doLoadIList,[])
 link(onStop,1:doClose,[])
}}
Add(WorkIconsManager,6,0,100)
{{
 IconsManager="icons"
 IListFileName="{roundtrip.as_posix()}"
 Point(doSaveIList)
}}
''', encoding="utf-8")
        run(app, "--build", scheme, output)
        run(output)
        for path in (icon_list, roundtrip):
            header = path.read_bytes()[:6]
            assert header == b"\x00\x00\x01\x00\x02\x00", (path, header)

        if os.name == "nt":
            resource_library = Path(os.environ["SystemRoot"]) / "System32/shell32.dll"
            resource_icons = work / "resources.ico"
            scheme.write_text(f'''Make(cpp)
Add(MainForm,1,0,0)
{{
 Backend=1
 link(onCreate,3:doLoadPakIcons,[])
}}
Add(IconsManager,2,0,20)
{{
 Name="icons"
}}
Add(WorkIconsManager,3,0,40)
{{
 IconsManager="icons"
 IconsFileName="{resource_library.as_posix()}"
 Point(doLoadPakIcons)
 link(onChangeImgLst,4:doSaveIList,[])
}}
Add(WorkIconsManager,4,0,60)
{{
 IconsManager="icons"
 IListFileName="{resource_icons.as_posix()}"
 Point(doSaveIList)
}}
Add(Timer,5,0,80)
{{
 Interval=20
 link(onTimer,1:doClose,[])
}}
''', encoding="utf-8")
            run(app, "--build", scheme, output)
            run(output)
            header = resource_icons.read_bytes()[:6]
            assert header[:4] == b"\x00\x00\x01\x00" and int.from_bytes(header[4:], "little") > 1, header

        scheme.write_text('''Make(cpp)
Add(MainForm,1,0,0)
{
 Backend=1
 link(onCreate,8:PageInsert,[])
}
Add(UpDown,2,0,20)
{
 Position=5
 link(onPosition,3:doPosition,[])
}
Add(ScrollBarEx,3,0,40)
{
 Position=10
 link(onPosition,4:doOn,[])
}
Add(LED,4,0,60)
{
 link(onChange,5:doIndexText,[])
}
Add(StatusBar,5,0,80)
{
 Panels=#9:Status=80|
 Strings=#5:Ready|4:Done|
 link(onText,7:doTimer,[])
}
Add(ProgressBarRush,6,0,100)
{
 Max=42
 Kind=1
}
Add(GProgressBar,10,0,110)
{
 Max=42
}
Add(Timer,7,0,120)
{
 Enable=1
 Interval=5
 AutoStop=2
 Point(onStop)
 link(onTimer,18:doPosPBar,[])
 link(onStop,1:doClose,[])
}
Add(TabControl,8,0,140)
{
 link(onChange,2:doPosition,[])
}
Add(LedLadder,9,0,160)
{
 Point(doMax)
}
Add(Grapher,11,0,180)
{
 Name="chart"
 MaxValues=5
 LeftMargin=12
 PenWidth=2
}
Add(RedrawManager,12,0,200)
{
 ControlManager="chart"
 Point(doBeginUpdate)
 Point(doEndUpdate)
}
Add(Button,13,0,220)
{
 link(onClick,12:doBeginUpdate,[])
}
Add(Button,14,0,240)
{
 link(onClick,12:doEndUpdate,[])
}
Add(RichEdit,15,0,260)
{
 Point(doFormatSel)
}
Add(Button,16,0,280)
{
 Data=String(Rich text)
 link(onClick,15:doAdd,[])
}
Add(Button,17,0,300)
{
 link(onClick,15:doFormatSel,[])
}
Add(StatusBarEx,18,0,320)
{
 Panels=#9:Build=100|
 EnablePBar=0
 PanelPBar=0
 VisiblePBar=0
 Point(doPosPBar)
}
Add(ButtonRush,19,0,340)
{
 Data=Integer(37)
 link(onClick,11:doAdd,[])
}
Add(FontManager,20,0,360)
{
 ControlManager="chart"
 Font=[Arial,12,3,255,1]
 Point(doFont)
}
Add(Button,21,0,380)
{
 link(onClick,20:doFont,[])
}
Add(UseExtCursor,22,0,400)
{
 ControlManager="chart"
 FileCurName="cursor.cur"
 link(onSetCursor,23:doAnimation,[])
}
Add(UseWinAnimation,23,0,420)
{
 ControlManager="chart"
 ActivationMode=1
 Time=0
}
Add(Button,24,0,440)
{
 link(onClick,22:doSetCursor,[])
}
Add(IndexManager,25,0,460)
{
 Index=['zero'=0,'skip'=-1,'two'=2]
 Point(doDelete)
}
Add(Button,26,0,480)
{
 Data=Integer(1)
 link(onClick,25:doDelete,[])
}
Add(Timer,27,0,500)
{
 Interval=1
 AutoStop=1
 link(onTimer,6:doPosition,[])
}
''', encoding="utf-8")
        generated = work / "grapher.cpp"
        run(app, "--generate", scheme, generated)
        graph_source = generated.read_text(encoding="utf-8")
        assert "graph->margins = QMargins(12, 15, 15, 15);" in graph_source
        assert "graph->penWidth = 2;" in graph_source
        assert 'qt_widget<QWidget>(form, "Grapher11")' in graph_source
        assert 'cursor.insertText(text, format);' in graph_source
        assert 'cursor.mergeCharFormat(format);' in graph_source
        assert 'status->setProgressPanel(0);' in graph_source
        assert 'new HiasmRushButton' in graph_source and 'button->setSkin(3,' in graph_source
        assert 'qt_apply_font(qt_widget<QWidget>(form, "Grapher11"), qt_text("[Arial,12,3,255,1]"));' in graph_source
        assert '->load(qt_widget<QWidget>(form, "Grapher11"), qt_text("cursor.cur"))' in graph_source
        assert '->run(qt_widget<QWidget>(form, "Grapher11"));' in graph_source
        assert 'manager->load(qt_text("[' in graph_source and "'skip'=-1" in graph_source
        assert 'QStringLiteral("IndexManager25")))->remove(qt_text(1).toInt());' in graph_source
        run(app, "--build", scheme, output)
        run(output)

        cases = [
            ("web", "HtmlEntryPoint", "Echo", "doEcho", ' Message="Привет 世界"', ["Привет 世界", "<!--"]),
            ("web", "JavaEntryPoint", "Echo", "doEcho", ' Message="Привет 世界"', ["alert(", "Привет 世界"]),
            ("web", "EntryPoint", "Echo", "doEcho", ' Message="Привет 世界"', ["<?php", "echo ", "Привет 世界", "?>"]),
            ("web", "CSSEntryPoint", "CSS", "doPrint", ' Tag="body"\n Color="red"', ["body", "red"]),
            ("modules", "HISEntryPoint", "Message", "doMessage", ' Text="RTCG"', ["sys.message(", "RTCG"]),
            ("modules", "MakeEntryPoint", "Message", "doMessage", ' Text="RTCG"', ["MessageBoxA(", "RTCG"]),
        ]
        for pack, entry, node, port, properties, markers in cases:
            scheme.write_bytes((f"Make({pack})\nAdd({entry},1,0,0)\n{{\n link(onStart,2:{port},[])\n}}\n"
                                f"Add({node},2,70,70)\n{{\n{properties}\n}}\n").encode("utf-8"))
            target = work / "result.txt"
            run(app, "--build", scheme, target)
            source = target.read_text(encoding="utf-8")
            for marker in markers:
                assert marker in source, (entry, marker, source)


if __name__ == "__main__":
    main()
