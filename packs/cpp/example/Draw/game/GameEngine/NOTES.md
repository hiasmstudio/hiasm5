# GameEngine примеры — заметки (2026-11-07)

Четыре схемы этой папки (Balls, Demo, life, StarWars) сейчас **не собираются**:
ни одного GE_* элемента в C++-пакете ещё нет. Полные блокировки по схемам и
приоритетный список портирования — в `MARKDOWN/CPP_EXAMPLES_AUDIT.md` (раздел
GameEngine). Здесь — только самодостаточные заметки по этому каталогу.

## Что есть

| Схема | Элементов | Особенности |
|---|---|---|
| Balls | 49 | спрайты-круги, `GE_SpriteArray`, `MT_Add/MT_Get/MT_MultiMem`, `Case`, `SafeMode`, `MathParse`, `EditPolyMulti` |
| Demo | 79 | анимация `GE_SpriteAnimation`, `Keyboard`, `ChangeMon`, `InfoTip`, `MultiElementEx` |
| StarWars | 91 | самая богатая: `GE_GameTick`, `Print_Text/Print_Shape/Print_GradientRect` (документы), `PC_Text/PC_Size`, `Switch`, `StrList` |
| life | 72 | клеточный автомат: `MatrixRW`, `IntegerArray`, `ArrayRW`, `MultiElement`, `EditMulti` |

Общее ядро во всех четырёх: `GE_Scene` (корень), `GE_Background`,
`GE_SpriteCollection`, `PaintBox` (поверхность), `MMTimer` (такт), `Hub/HubEx`.

## Порядок портирования, чтобы схемы заработали

1. **Data/logic (без GUI):** `GE_Scene`, `GE_Sprite`, `GE_SpriteCollection`,
   `GE_Background`, `GE_GridSpace`, `GE_SpriteMove`, `GE_SpriteArray`,
   `GE_SpritePosition`, `GE_SpriteAnimation`, `GE_GameTick` — чистые структуры
   и математика, портятся первыми и дают весь каркас движка.
2. **Таймер и данные:** `MMTimer` (маленький), `MT_AddData/MT_Add/MT_Get/MT_MultiMem`
   (мультипоточные данные), для life — `MatrixRW/IntegerArray/ArrayRW`.
3. **GUI:** `PaintBox` — точка отрисовки; без него движок есть, а показываться
   негде. За ним `MathParse` и ветвление `Case`/`Switch`.
4. **Редакторы:** `EditMultiEx` (самый дешёвый «большой разблокировщик» —
   конф exists, но Class не из разрешённых; один фикс снимает блокировку 5
   схем сразу, включая 3 RTCG-схемы), `EditMulti`, `EditPolyMulti`,
   `MultiElementEx`.
5. **Остальное по StarWars:** `DocumentTemplate` + `Print_*` (документы),
   `PC_Text/PC_Size` (свойства), `SafeMode`, `Keyboard`, `ChangeMon`.

Когда GE_ядра + `MMTimer` + `PaintBox` будут готовы, все четыре схемы можно
собрать по шагам: сначала `life` (меньше всего имён), затем `Balls`, `Demo`,
`StarWars`.

## Зависимости вне GameEngine

- `MultiElementEx`, `EditMultiEx`, `StrList`, `InfoTip`, `Counter`, `Random`,
  `Copy`, `IndexToChanel`, `Convertor` — ALIAS (`hihc<Name>.hws` есть); если
  нужен собственный `Name` без `hc`, достаточно добавить короткие алиасы.
- Windows-only в этом каталоге: ничего чисто Windows-ного, кроме `Grapher`
  (GUI) — но он не используется в GameEngine.
