# HiAsm C++ package — example scheme audit

Audit date: 2026-11-07. READ-ONLY audit of `D:\Devs\Cpp\hiasm`.

## Method

- 16 `.sha` schemes across 4 folders (3 RTCG, 2 System/DLL, 6 KernelChip, 4 GameEngine), 578 `Add(...)` element instances total (lines inside `BEGIN_SDK` blocks included; `Make`/`Pos`/`link`/property lines ignored).
- Compilability verified against `src/Pack.cpp:218 Pack::compiles(name)`: for the cpp pack, a name compiles when `packs/cpp/code/hi<Name>.hws` exists **or** the element's conf class is `HubEx`/`GetDataEx`/`ITElement` (checked in `packs/cpp/conf` then `packs/_base/conf`).
- ALIAS: no compilable `<Name>` itself, but the `hc<Name>` counterpart is compilable (a `hihc<Name>.hws` code file exists, or the `hc<Name>` conf class is one of the three allowed). Conf files with non-allowed classes (e.g. `EditMulti` → `Class=EditMulti`) do NOT make a name compilable and are noted.
- PLANNED: name appears in `INSERT INTO "planned_elements"` rows of `packs/cpp/elements.sql` (759 planned names).
- MISSING: none of the above.

## Per-folder scheme tables

### packs/cpp/example/RTCG/ (3 schemes, none buildable)

| Scheme | Make | Instances | Distinct names | Element names |
|---|---|---|---|---|
| ReplaceChars.sha | cpp | 28 | 20 | MainForm, Button, Edit, TimeCounter, For, DoData, FTCG_Tools, EditMultiEx, FT_StringToArray, FT_For, FT_Length, StrArray, ArrayRead, ArraySize, GetDataEx, FT_If_else, ArrayWrite, FT_DoData, StrList, Hub |
| SpeedTest.sha | cpp | 22 | 15 | MainForm, RTCG_Tools, EditMultiEx, RT_StrCat, RT_For, RT_DoData, GetDataEx, Button, MultiElementEx, StrCat, For, DoData, TimeCounter, Message, Label |
| TechSpeedReport.sha | cpp | 70 | 27 | MainForm, Button, TimeCounter, For, ListBox, StrCat, Math, Hub, HubEx, MathParse, Application, Gentee, InlineCode, VBJScript, FastMathParse, RTCG_Tools, EditMultiEx, RT_Math, RT_Memory, RT_For, RT_DoData, GetDataEx, Memory, ProcInfo, FormatStr, MemoryInfo, Clipboard |

### packs/cpp/example/System/DLL/ (2 schemes, none buildable; no Make line in these files)

| Scheme | Make | Instances | Distinct names | Element names |
|---|---|---|---|---|
| TestDLL.sha | (none) | 3 | 3 | DLL, Message, DoData |
| UseDllTest.sha | (none) | 6 | 6 | MainForm, UseHiDLL, Button, ListBox, Edit, DoData |

### packs/cpp/example/KernelChip/ (6 schemes, none buildable)

| Scheme | Make | Instances | Distinct names | Element names |
|---|---|---|---|---|
| CheckDevice.sha | cpp | 21 | 15 | MainForm, KE_Connect, Button, Edit, Label, ListBox, FormatStr, HubEx, DoData, KE_SerialNumber, KE_DetectDevices, Message, If_else, ComboBox, Copy |
| DetectDevices.sha | cpp | 5 | 5 | MainForm, Button, KE_DetectDevices, FormatStr, ListBox |
| LineDirection.sha | cpp | 52 | 23 | MainForm, KE_Connect, Button, Edit, Label, ListBox, FormatStr, HubEx, DoData, CheckBox, KE_LineState, Hub, CharArray, ArrayEnum, IndexToChanel, KE_LineDirection, MT_ChanelToIndex, Convertor, KE_DetectDevices, Message, If_else, ComboBox, Copy |
| ReadADC.sha | cpp | 27 | 17 | MainForm, KE_Connect, Button, Edit, Label, ListBox, FormatStr, HubEx, DoData, Grapher, KE_ADC, Hub, KE_DetectDevices, Message, If_else, ComboBox, Copy |
| ReadLine.sha | cpp | 25 | 15 | MainForm, KE_Connect, Button, Edit, Label, ListBox, FormatStr, HubEx, DoData, KE_ReadLine, KE_DetectDevices, Message, If_else, ComboBox, Copy |
| WriteLine.sha | cpp | 28 | 16 | MainForm, KE_Connect, Button, Edit, Label, ListBox, FormatStr, HubEx, DoData, KE_WriteLine, RadioButton, KE_DetectDevices, Message, If_else, ComboBox, Copy |

### packs/cpp/example/Draw/game/GameEngine/ (4 schemes, none buildable)

| Scheme | Make | Instances | Distinct names | Element names |
|---|---|---|---|---|
| Balls.sha | cpp | 49 | 27 | PaintBox, GE_Scene, GE_SpriteCollection, MainForm, GE_Background, DoData, MMTimer, MT_Add, GE_SpriteArray, EditPolyMulti, GE_Sprite, GE_SpriteMove, MT_MultiMem, MathParse, GE_GridSpace, Hub, Math, Memory, GE_SpritePosition, MT_Get, MT_AddData, HubEx, Case, Random, For, Timer, SafeMode |
| Demo.sha | cpp | 79 | 26 | GE_Background, MainForm, PaintBox, GE_Scene, Timer, GE_GridSpace, GE_SpriteCollection, GE_Sprite, GE_SpriteMove, Hub, Keyboard, Math, ChangeMon, IndexToChanel, HubEx, DoData, If_else, GetDataEx, MMTimer, GE_SpriteAnimation, Memory, Case, MultiElementEx, EditMultiEx, InfoTip, Counter |
| StarWars.sha | cpp | 91 | 36 | MainForm, PaintBox, Hub, MMTimer, GE_Scene, GE_SpriteCollection, GE_Background, GE_Sprite, GE_SpriteMove, GE_SpriteArray, EditPolyMulti, GE_SpritePosition, Math, Switch, If_else, Counter, MultiElementEx, EditMultiEx, PC_Text, Timer, DocumentTemplate, Print_Text, Print_GradientRect, Print_Shape, GetDataEx, Random, DoData, HubEx, MT_AddData, StrList, ArrayRW, GE_GameTick, InfoTip, MT_Get, MT_Add, PC_Size |
| life.sha | cpp | 72 | 22 | PaintBox, GE_Scene, GE_SpriteCollection, MainForm, GE_Background, DoData, GE_GameTick, MMTimer, GE_GridSpace, For, MatrixRW, MultiElement, EditMulti, Memory, IntegerArray, ArrayRW, Math, Hub, GetDataEx, If_else, HubEx, MT_AddData |

## Element classification (102 distinct names)

Legend: COMPILABLE = builds now; ALIAS = `hc<Name>` counterpart is compilable; PLANNED = in `planned_elements`; MISSING = nothing available. "Blocked" = number of schemes for which this name is a blocker.

### COMPILABLE (19)

| Name | Basis | Schemes using |
|---|---|---|
| MainForm | MainForm.hws | 14 |
| HubEx | conf class HubEx | 10 |
| Button | hiButton.hws | 10 |
| Hub | hiHub.hws | 8 |
| Edit | hiEdit.hws | 7 |
| Message | hiMessage.hws | 7 |
| Label | hiLabel.hws | 6 |
| ListBox | hiListBox.hws | 8 |
| Math | hiMath.hws | 5 |
| For | hiFor.hws | 5 |
| ComboBox | hiComboBox.hws | 5 |
| Memory | hiMemory.hws | 4 |
| Timer | hiTimer.hws | 3 |
| StrCat | hiStrCat.hws | 2 |
| GetDataEx | conf class GetDataEx | 6 |
| InfoTip | conf class ITElement | 2 |
| Counter | hiCounter.hws | 2 |
| CheckBox | hiCheckBox.hws | 1 |
| RadioButton | hiRadioButton.hws | 1 |

### ALIAS — `hc<Name>` is compilable (12)

| Name | Compilable counterpart | Schemes using |
|---|---|---|
| DoData | hihcDoData.hws (Class HCDoData) | 13 |
| If_else | hihcIf_else.hws (HCIfElse) | 8 |
| FormatStr | hihcFormatStr.hws (HCFormatStr) | 7 |
| TimeCounter | hihcTimeCounter.hws (HCTimeCounter) | 3 |
| StrList | hihcStrList.hws (HCStrList) | 2 |
| Copy | hihcCopy.hws (HCCopy) | 5 |
| Random | hihcRandom.hws (HCRandom) | 2 |
| IndexToChanel | hihcIndexToChanel.hws | 2 |
| Convertor | hihcConvertor.hws | 1 |
| Clipboard | hihcClipboard.hws | 1 |
| ArrayRead | hihcArrayRead.hws | 1 |
| ArrayEnum | hihcArrayEnum.hws | 1 |

### PLANNED (in `planned_elements`, 62 distinct names used by schemes)

GE_Scene, GE_Background, GE_SpriteCollection, GE_Sprite, GE_SpriteMove, GE_SpriteArray, GE_SpriteAnimation, GE_SpritePosition, GE_GameTick, GE_GridSpace,
MMTimer, MT_Add, MT_AddData, MT_Get, MT_MultiMem, MT_ChanelToIndex,
Case, Switch, SafeMode, InlineCode, Keyboard, MultiElementEx, MultiElement,
MathParse, FastMathParse, MatrixRW, IntegerArray, ArrayRW, ArraySize, ArrayWrite, StrArray, CharArray,
KE_DetectDevices, KE_Connect, KE_ADC, KE_ReadLine, KE_WriteLine, KE_LineDirection, KE_LineState, KE_SerialNumber,
PaintBox, Grapher, Print_Text, Print_Shape, Print_GradientRect, PC_Text, PC_Size,
DLL, UseHiDLL, VBJScript, Application, MemoryInfo, ProcInfo, Gentee, DocumentTemplate,
ChangeMon, FTCG_Tools, FT_DoData, FT_For, FT_If_else, FT_Length, FT_StringToArray.

### MISSING (no code, no allowed class, no planned row)

| Name | Conf status | Schemes using | Blocked |
|---|---|---|---|
| EditMultiEx | conf exists, Class=EditMultiEx (not HubEx/GetDataEx/ITElement) | 5 | 5 |
| RTCG_Tools | no conf, no code, not planned | 2 | 2 |
| RT_StrCat | no conf, not planned | 1 | 1 |
| RT_For | no conf, not planned | 2 | 2 |
| RT_DoData | no conf, not planned | 2 | 2 |
| RT_Math | no conf, not planned | 1 | 1 |
| RT_Memory | no conf, not planned | 1 | 1 |
| EditPolyMulti | no conf, not planned | 2 | 2 |
| EditMulti | conf exists, Class=EditMulti (not an allowed class) | 1 | 1 |

Note: the six `RT_*` names and `RTCG_Tools` are Delphi-side RTCG (virtual-code RT) elements with no conf file at all in either pack — they are not even registered as planned. `EditMulti`/`EditMultiEx` have confs in `packs/_base/conf` but their classes fall outside the three allowed, so `Pack::compiles` rejects them; promoting their class or adding a `.hws` would fix all 5 schemes that use `EditMultiEx` in one stroke.

## Windows-only elements (flagged)

- `KE_*` (KE_DetectDevices, KE_Connect, KE_ADC, KE_ReadLine, KE_WriteLine, KE_LineDirection, KE_LineState, KE_SerialNumber): KernelChip hardware — needs the device/driver, Windows-only. All 6 KernelChip schemes are effectively Windows-only.
- `DLL`, `UseHiDLL`: Windows DLL loading / HiAsm DLL interop (planned). Both System/DLL schemes blocked on Windows-only pieces.
- `VBJScript`, `InlineCode`: VBScript/Windows COM scripting (planned).
- `MemoryInfo`, `ProcInfo`, `Application`: Windows API introspection (planned).
- `Grapher`: GUI chart widget (planned) — only used by ReadADC.

## Prioritized port list (top 15)

Ordered by number of schemes advanced (the "Blocked" count above), ties broken by simplicity (pure data/logic before GUI/hardware).

| # | Element | Schemes advanced | Class | Reason |
|---|---|---|---|---|
| 1 | KE_DetectDevices | 6 | hardware (Windows) | Blocks every one of the 6 KernelChip schemes; one shared device-enumeration entry point, natural first hardware piece. |
| 2 | EditMultiEx | 5 | GUI widget | Single conf class fix or hws unblocks ReplaceChars, SpeedTest, TechSpeedReport, Demo, StarWars; cheapest "big unlock" of all. |
| 3 | KE_Connect | 5 | hardware (Windows) | Needed by 5 of 6 KernelChip schemes; logically paired with #1. |
| 4 | GE_Scene | 4 | data/logic | Root of the game engine used by all 4 GameEngine schemes; pure engine-structure logic, no GUI of its own. |
| 5 | MMTimer | 4 | data/logic | Thread-safe timer primitive in all 4 GameEngine schemes; small, pure-logic port. |
| 6 | GE_Background | 4 | data/logic | Simple canvas data (sprite/image ref) in all 4 game schemes; trivial once PaintBox drawing exists. |
| 7 | PaintBox | 4 | GUI | Drawing surface required by all 4 game schemes; GUI work but the anchor of the whole folder. |
| 8 | GE_SpriteCollection | 4 | data/logic | Sprite container shared by all 4 game schemes; plain data management. |
| 9 | MT_AddData | 3 | data/logic | Pure multithreaded-data op in Balls, StarWars, life; small and reusable. |
| 10 | GE_SpriteMove | 3 | data/logic | Movement/position math for Balls, Demo, StarWars. |
| 11 | GE_Sprite | 3 | data/logic | Core sprite record in Balls, Demo, StarWars. |
| 12 | GE_GridSpace | 3 | data/logic | Grid/coordinate abstraction in Balls, Demo, StarWars. |
| 13 | MultiElementEx | 3 | GUI widget | Multi-value editor widget in SpeedTest, Demo, StarWars. |
| 14 | Case | 2 | data/logic | Simple branching control-flow element in Balls and Demo; classic control-flow port. |
| 15 | MathParse | 2 | data/logic | Expression parser in Balls and TechSpeedReport; pure logic, unlocks large TechSpeedReport body. |

(Next up at the 2-scheme tier: RT_For, RT_DoData, RTCG_Tools, MT_Add, MT_Get, Switch, ArrayRW, EditPolyMulti, GE_GameTick, GE_SpritePosition, GE_SpriteArray.)

## Scheme-level buildability

**Fully buildable today: 0 of 16.**

Blocked schemes and their remaining blockers:

| Scheme | Blocking names |
|---|---|
| ReplaceChars.sha | FTCG_Tools, FT_StringToArray, FT_For, FT_Length, FT_If_else, FT_DoData, StrArray, ArraySize, ArrayWrite, **EditMultiEx** (MISSING ×7, 1) |
| SpeedTest.sha | **RTCG_Tools**, **RT_StrCat**, **RT_For**, **RT_DoData**, **MultiElementEx**(PLANNED), **EditMultiEx** |
| TechSpeedReport.sha | MathParse, Application, Gentee, InlineCode, VBJScript, FastMathParse, RT_Math, RT_Memory, RT_For, RT_DoData, RTCG_Tools, ProcInfo, MemoryInfo, EditMultiEx |
| TestDLL.sha | **DLL** |
| UseDllTest.sha | **UseHiDLL** |
| CheckDevice.sha | KE_Connect, KE_SerialNumber, KE_DetectDevices |
| DetectDevices.sha | KE_DetectDevices |
| LineDirection.sha | KE_Connect, KE_LineState, KE_LineDirection, KE_DetectDevices, CharArray, MT_ChanelToIndex |
| ReadADC.sha | KE_Connect, KE_ADC, KE_DetectDevices, Grapher |
| ReadLine.sha | KE_Connect, KE_ReadLine, KE_DetectDevices |
| WriteLine.sha | KE_Connect, KE_WriteLine, KE_DetectDevices |
| Balls.sha | PaintBox, GE_×9, MMTimer, MT_Add, MT_AddData, MT_Get, MT_MultiMem, MathParse, EditPolyMulti, Case, SafeMode |
| Demo.sha | GE_×6, PaintBox, MMTimer, Keyboard, ChangeMon, Case, MultiElementEx, EditMultiEx |
| StarWars.sha | GE_×9, MMTimer, PaintBox, MT_×3, Switch, Case, MultiElementEx, EditMultiEx, EditPolyMulti, PC_Text, PC_Size, DocumentTemplate, Print_×3, ArrayRW, GE_GameTick |
| life.sha | GE_×5, PaintBox, MMTimer, GE_GameTick, MatrixRW, MultiElement, EditMulti, IntegerArray, ArrayRW, MT_AddData |

Closest-to-unblocked: DetectDevices.sha (1 blocker), TestDLL.sha (1), CheckDevice.sha (3), ReadLine.sha / WriteLine.sha / UseDllTest.sha.

## Summary counts

- 16 schemes: 3 RTCG + 2 System/DLL + 6 KernelChip + 4 GameEngine; 578 element instances, 102 distinct names.
- 19 names COMPILABLE, 12 ALIAS (hcXxx), 62 PLANNED, 9 MISSING (EditMultiEx, EditMulti, EditPolyMulti, RTCG_Tools, RT_StrCat, RT_For, RT_DoData, RT_Math, RT_Memory).
- Buildable today: 0. Best single-leverage fixes: EditMultiEx (5 schemes, a widget), the GE_ engine core + MMTimer + PaintBox (4 schemes each), KE_DetectDevices + KE_Connect (6 / 5 KernelChip schemes, Windows hardware).
