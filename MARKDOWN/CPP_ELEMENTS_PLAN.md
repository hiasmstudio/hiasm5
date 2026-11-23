# План реализации элементов пакета «Приложение на C++»

Файл создан из `packs/cpp/elements.sql` командой `python tools/cpp_elements_plan.py`; правьте не его, а `packs/cpp/elements.sql`. Таблица `planned_elements` хранит имена, описания, группу и имя файла иконки (`packs/cpp/icon`). Палитра её не читает.

Группа WinCE не переносится: её элементы и иконки удалены. Частичные аналоги Delphi перечислены в `tools/audit_delphi_migration.py` (таблица PARTIAL) и в `MARKDOWN/DELPHI_MIGRATION.md`.

Реализовано элементов: 203; в плане: 654; из них с иконкой 614, без иконки 40.

## Итог по группам

| Группа | Реализовано | В плане |
|---|---:|---:|
| Widgets | 13 | 0 |
| Controls | 2 | 14 |
| Helpers | 3 | 14 |
| RTCG | 31 | 0 |
| Core | 57 | 11 |
| Tools | 80 | 25 |
| (скрытая группа) | 17 | 33 |
| Arrays | 0 | 2 |
| Cables | 0 | 8 |
| DataBase | 0 | 23 |
| Files | 0 | 40 |
| GameEngine | 0 | 11 |
| Graphics | 0 | 75 |
| Internet | 0 | 46 |
| KernelChip | 0 | 9 |
| Logic | 0 | 3 |
| Media | 0 | 38 |
| Misc | 0 | 11 |
| MultiThread | 0 | 16 |
| OpenGL | 0 | 48 |
| Strings | 0 | 5 |
| System | 0 | 40 |
| Type | 0 | 16 |
| WinControls | 0 | 73 |
| WinManControls | 0 | 47 |
| Windows | 0 | 46 |

## Только среда

В готовой программе не имеют смысла; остаются в плане, пока не решено, как проверять их в Core.

| Элемент | Описание | Иконка |
|---|---|---|
| `hcAddElement` | Вставка элемента |  |
| `hcCommanHandler` | Обработчик команд |  |
| `hcCommander` | Вызов команд |  |
| `hcConnectElement` | Соединение точек элементов |  |
| `hcMoveElement` | Перемещение элемента |  |
| `hcSDKEvents` | События схемы |  |

## Все элементы в плане

### Controls (14)

| Элемент | Описание | Иконка |
|---|---|---|
| `BaloonToolTips` | Всплывающая надпись | BaloonToolTips.ico |
| `ClipboardHook` | Буфер обмена | ClipboardHook.ico |
| `DragText` | Перетаскивание текста | DragText.ico |
| `DropFile` | Прием файлов | DropFile.ico |
| `HiAsmSDK` | Рабочее поле HiAsm | HiAsmSDK.ico |
| `IconAnimation` | Анимация картинок | IconAnimation.ico |
| `IconGraph` | График на иконке | IconGraph.ico |
| `MMTimer` | Независимый таймер | MMTimer.ico |
| `MinMaxSize` | Ограничение размеров формы | MinMaxSize.ico |
| `PopupMenu` | Всплывающее меню | PopupMenu.ico |
| `StyleXP` | XP стиль | StyleXP.ico |
| `TaskProgressBar` | ProgressBar на значках в панели задач | TaskProgressBar.ico |
| `TrayIcon` | Икoнкa в тpee | TrayIcon.ico |
| `UseActiveX` | Использование ActiveX (OCX) | UseActiveX.ico |

### Helpers (14)

| Элемент | Описание | Иконка |
|---|---|---|
| `AccessFTP` | Доступ к FTP | — |
| `Check` | Флажок | — |
| `ImageMulti` | Картинка-контейнер | — |
| `LineBreak` | Разрыв | — |
| `LineBreakEx` | Разрыв связи | — |
| `LinkTip` | Гиперссылка | — |
| `PictureTip` | Картинка в редакторе | — |
| `PointHint` | Сноска | — |
| `SDKBtn` | Кнопка | — |
| `ScriptEvents` | События скрипта | — |
| `Shape` | Фигура | — |
| `Translator` | Перевод | — |
| `Version` | Версия | Version.ico |
| `ViewSHA` | Отображение SHA | — |

### Core (11)

| Элемент | Описание | Иконка |
|---|---|---|
| `hcAddElement` | Вставка элемента | — |
| `hcButton` | Кнопка | — |
| `hcCommanHandler` | Обработчик команд | — |
| `hcCommander` | Вызов команд | — |
| `hcConnectElement` | Соединение точек элементов | — |
| `hcKeyHandler` | Обработчик клавиатуры | — |
| `hcListBox` | Список строк | — |
| `hcMouseHandler` | Обработчик мыши | — |
| `hcMoveElement` | Перемещение элемента | — |
| `hcSDKEvents` | События схемы | — |
| `hcVisualText` | Вставка текста | — |

### Tools (25)

| Элемент | Описание | Иконка |
|---|---|---|
| `CreateSignature` | Создание цифровой подписи | CreateSignature.ico |
| `CrossLine` | Пересечение отрезков | CrossLine.ico |
| `EnumKeyContainers` | Перечисление контейнеров ключей | EnumKeyContainers.ico |
| `ExportSessionKey` | Экспорт сессионного ключа | ExportSessionKey.ico |
| `FTCG_Tools` | Вложенная схема | FTCG_Tools.ico |
| `GenKeyPairs` | Генератор ключевых пар | GenKeyPairs.ico |
| `Gentee` | Gentee | Gentee.ico |
| `ImportSessionKey` | Импорт сессионного ключа | ImportSessionKey.ico |
| `Informer` | Информатор | Informer.ico |
| `InlineCode` | Вставка кода | InlineCode.ico |
| `KeyContainer` | Контейнер ключей | KeyContainer.ico |
| `MemFIFO` | Запоминает данные из потока в очередь FIFO | MemFifo.ico |
| `MultiElement` | Вложенная схема | MultiElement.ico |
| `MultiElementEx` | Вложенная схема | MultiElementEx.ico |
| `MultiSetParam` | Установка параметров выходов согласно списка | MultiSetParam.ico |
| `PolyGate` | Шлюз полиморфа | PolyGate.ico |
| `PolymorphMulti` | Полиморфный контейнер | PolymorphMulti.ico |
| `Project` | Проект | Project.ico |
| `RSRepair` | Восстановление данных RS-кодами | RSRepair.ico |
| `Rect` | Область | Rect.ico |
| `SampleDelta` | Разность сигнала | SampleDelta.ico |
| `VBJScript` | Visual Basic & Java Script | VBJScript.ico |
| `VBScript` | Visual Basic Script | VBScript.ico |
| `VerifySignature` | Проверка цифровой подписи | VerifySignature.ico |
| `VisualStrings` | Вставка текста | VisualStrings.ico |

### (скрытая группа) (33)

| Элемент | Описание | Иконка |
|---|---|---|
| `BASS_CD` | Управление CDROM | BASS_CD.ico |
| `BASS_ChlAttributes` | Параметры канала | BASS_ChlAttributes.ico |
| `BASS_ChlControl` | Управление каналами | BASS_ChlControl.ico |
| `BASS_ChlFX` | Эффекты | BASS_ChlFX.ico |
| `BASS_ChlFormat` | Формат позиции проигрывания | BASS_ChlFormat.ico |
| `BASS_ChlPosition` | Позиция проигрывания | BASS_ChlPosition.ico |
| `BASS_ChlState` | Состояние канала | BASS_ChlState.ico |
| `BASS_ChlVisibleOcilloScope` | Визуализация канала | BASS_ChlVisibleOcilloScope.ico |
| `BASS_ChlVisibleSpectrum` | Визуализация канала | BASS_ChlVisibleSpectrum.ico |
| `BASS_Global` | Общая настройка | BASS_Global.ico |
| `BASS_MusicFile` | Загрузка MO3/IT/XM/S3M/MTM/MOD/UMX | BASS_MusicFile.ico |
| `BASS_MusicPlayer` | Проигрыватель музыки | BASS_MusicPlayer.ico |
| `BASS_Record` | Запись звука | BASS_Record.ico |
| `BASS_SmpFile` | Загрузка MP3,WAV,OGG | BASS_SmpFile.ico |
| `BASS_SmpPlayer` | Проигрыватель семплов | BASS_SmpPlayer.ico |
| `BASS_StreamFile` | Поточный звук | BASS_StreamFile.ico |
| `BASS_StreamPlayer` | Проигрыватель потока | BASS_StreamPlayer.ico |
| `BASS_StreamSound` | Потоковый звук | BASS_StreamSound.ico |
| `CPL` | Панель Управления | CPL.ico |
| `Context` | Расширение Explorer | Context.ico |
| `CustomCode` | Вставка текста скрипта FTCG | CustomCode.ico |
| `DLL` | DLL | DLL.ico |
| `ImageLoader` | Загрузка и отрисовка изображений | ImageLoader.ico |
| `MRA_Wire` | Транслятор интерфейса Mail.ru агента между контейнерами | — |
| `NET` | Активность NET | NET.ico |
| `NTService` | Сервис NT | NTService.ico |
| `PagePanel` | Страница для контроллера страниц | PagePanel.ico |
| `PanelRush` | Панель | PanelRush.ico |
| `ScrollBox` | Панель прокрутки | ScrollBox.ico |
| `TCP_Connection` | TCP клиент | TCP_Connection.ico |
| `VisualInline` | Вставка кода | VisualInline.ico |
| `VisualText` | Вставка текста | VisualText.ico |
| `hiPlugs` | Плагины в HiAsm | hiPlugs.ico |

### Arrays (2)

| Элемент | Описание | Иконка |
|---|---|---|
| `Matrix` | Матрица данных | Matrix.ico |
| `MatrixRW` | Матрица запись/чтение | MatrixRW.ico |

### Cables (8)

| Элемент | Описание | Иконка |
|---|---|---|
| `CableData` | Объединяет потоки в кабель | CableData.ico |
| `CableDataNamed` | Объединяет потоки в кабель | CableDataNamed.ico |
| `CableEvent` | Выводит потоки из кабеля | CableEvent.ico |
| `CableEventNamed` | Выводит потоки из кабеля | CableEventNamed.ico |
| `CableVar` | Доступ к отдельным потокам в кабеле | CableVar.ico |
| `CableVarNamed` | Доступ к отдельным потокам в кабеле | CableVarNamed.ico |
| `CableWork` | Объединяет потоки в кабель | CableWork.ico |
| `CableWorkNamed` | Объединяет потоки в кабель | CableWorkNamed.ico |

### DataBase (23)

| Элемент | Описание | Иконка |
|---|---|---|
| `DSC_Exec` | Запрос на выполнение | DSC_Exec.ico |
| `DSC_Query` | Запрос на получение данных | DSC_Query.ico |
| `DSC_QueryScalar` | Запрос значения ячейки | DSC_QueryScalar.ico |
| `DS_MySQL` | Источник данных MySQL | DS_MySQL.ico |
| `DS_ODBC` | Источник данных ODBC | DS_ODBC.ico |
| `DS_OLEDB` | Источник данных OLEDB | DS_OLEDB.ico |
| `DS_SQLite` | Источник данных SQLite | DS_SQLite.ico |
| `DataGrid` | Таблица | DataGrid.ico |
| `MSSQL` | MSSQL | MSSQL.ico |
| `ODBC` | ODBC Access | ODBC.ico |
| `OLEdb` | База данных OLE | OLEDb.ico |
| `OLEdb_Query` | Запрос к OLE базе данных | OLEDb_Query.ico |
| `OLEdb_Session` | Сессия OLE БД | OLEDb_Session.ico |
| `SQLite_DB` | База данных SQLite | SQLite_DB.ico |
| `SQLite_Exec` | Запрос на выполнение | SQLite_Exec.ico |
| `SQLite_Query` | Запрос к базе данных | SQLite_Query.ico |
| `SQLite_QueryScalar` | Запрос к базе данных | SQLite_QueryScalar.ico |
| `dbMySQL` | MySQL | dbMySQL.ico |
| `dbMySQL_Databases` | Базы данных | dbMySQL_Databases.ico |
| `dbMySQL_Exec` | MySQL Execute | dbMySQL_Exec.ico |
| `dbMySQL_Query` | Запрос к MySQL базе данных | dbMySQL_Query.ico |
| `dbMySQL_ShowQuery` | Показ результата запроса | dbMySQL_ShowQuery.ico |
| `dbMySQL_Tables` | Таблицы | dbMySQL_Tables.ico |

### Files (40)

| Элемент | Описание | Иконка |
|---|---|---|
| `Browse` | Обзор | Browse.ico |
| `ChangeFileSecurity` | Безопасность файла | ChangeFileSecurity.ico |
| `DataToFile` | Данные в файл | DataToFile.ico |
| `DirTools` | Операции с папками | DirTools.ico |
| `DirectoryChanges` | Мониторинг изменений структуры папок | DirectoryChanges.ico |
| `FSOwner` | Владелец файла | FSOwner.ico |
| `FileAttributes` | Атрибуты файла | FileAttributes.ico |
| `FileSearch` | Поиск файлов | FileSearch.ico |
| `FileSecurity` | Безопасность файла | FileSecurity.ico |
| `FileStream` | Файловый поток | FileStream.ico |
| `FilesContextMenu` | Контекстное меню файлов | FilesContextMenu.ico |
| `GVarContainer` | Контейнер переменных | GVarContainer.ico |
| `Ini` | Работа с INI файлами | INI.ico |
| `LZH` | LZH компрессия | LZH.ico |
| `LZMA` | LZMA компрессия | LZMA.ico |
| `MemoryStream` | Данные в памяти | MemoryStream.ico |
| `ODialog` | Открытие файла | ODialog.ico |
| `SDialog` | Сохранение файла | SDialog.ico |
| `SIDConvertor` | Конвертор SID | SIDConvertor.ico |
| `SIDInfo` | Информация о SID | SIDInfo.ico |
| `SStorage_DS` | Структуированное хранилище | SStorage_DS.ico |
| `STG_AddFileTo` | Добавление файла | STG_AddFileTo.ico |
| `STG_CopyTo` | Копирование | STG_CopyTo.ico |
| `STG_CreateFolder` | Создание папки | STG_CreateFolder.ico |
| `STG_Delete` | Удаление | STG_Delete.ico |
| `STG_Enumerator` | Перечисление элементов | STG_Enumerator.ico |
| `STG_Exists` | Проверка наличия | STG_Exists.ico |
| `STG_ExtractFileFrom` | Извлечение файла | STG_ExtractFileFrom.ico |
| `STG_GetSize` | Получение размера | STG_GetSize.ico |
| `STG_Merge` | Объединение хранилищ | STG_Merge.ico |
| `STG_MoveTo` | Пересылка | STG_MoveTo.ico |
| `STG_Pack` | Упаковка хранилища | STG_Pack.ico |
| `STG_Rename` | Переименование | STG_Rename.ico |
| `SharedStream` | Проекция файла | SharedStream.ico |
| `ShortCut` | Ярлык файла | Shortcut.ico |
| `StreamArray` | Содержит массив стримов | StreamArray.ico |
| `StreamCopy` | Копирование данных | StreamCopy.ico |
| `StreamPack` | Упаковка данных | StreamPack.ico |
| `StreamUnPack` | Распаковка данных | StreamUnPack.ico |
| `ZLIB` | ZIP компрессия | ZLIB.ico |

### GameEngine (11)

| Элемент | Описание | Иконка |
|---|---|---|
| `GE_Background` | Фон игрового пространства | GE_Background.ico |
| `GE_GameTick` | Счетчик тиков | GE_GameTick.ico |
| `GE_GridSpace` | Клетчатое поле | GE_GridSpace.ico |
| `GE_Scene` | Сцена | GE_Scene.ico |
| `GE_Sprite` | Спрайт | GE_Sprite.ico |
| `GE_SpriteAnimation` | Анимация спрайта | GE_SpriteAnimation.ico |
| `GE_SpriteArray` | Массив спрайтов | GE_SpriteArray.ico |
| `GE_SpriteCollection` | Коллекция скинов | GE_SpriteCollection.ico |
| `GE_SpriteHit` | Столкновение спрайтов | GE_SpriteHit.ico |
| `GE_SpriteMove` | Перемещение спрайта | GE_SpriteMove.ico |
| `GE_SpritePosition` | Позиция спрайта | GE_SpritePosition.ico |

### Graphics (75)

| Элемент | Описание | Иконка |
|---|---|---|
| `AS_Control` | Элемент управления | AS_Control.ico |
| `AS_DrawRect` | Отрисовка области | AS_DrawRect.ico |
| `ActionSkin` | Скин | ActionSkin.ico |
| `AlphaBitmap` | Хранение Alpha-картинки в памяти | AlphaBitmap.ico |
| `AlphaCopy` | Копирование Alpha-картинки | AlphaCopy.ico |
| `AlphaResize` | Изменение размера Alpha-картинки | AlphaResize.ico |
| `AlphaRotate` | Поворот Alpha-картинки | AlphaRotate.ico |
| `AlphaShadow` | Получение Alpha-картинки с тенью | AlphaShadow.ico |
| `Bitmap` | Картинка | Bitmap.ico |
| `BitmapArray` | Картинки | BitmapArray.ico |
| `BitmapStream` | Картинка-данные | BitmapStream.ico |
| `ColorDialog` | Выбор цвета | ColorDialog.ico |
| `ColorShade` | Формирование оттенков цвета | ColorShade.ico |
| `DocumentTemplate` | Шаблон документа | DocumentTemplate.ico |
| `Font` | Установка шрифта | Font.ico |
| `FontsList` | Список системных шрифтов | FontsList.ico |
| `Gamma` | Гамма | Gamma.ico |
| `Gif` | Картинка GIF | Gif.ico |
| `Glow` | Генерация свечения картинки | Glow.ico |
| `GrayScale` | Градация серого | GrayScale.ico |
| `Icon` | Иконка | Icon.ico |
| `Img_AlphaBmp` | Отрисовка картинки с прозрачностью | Img_AlphaBmp.ico |
| `Img_Arc` | Дуга | Img_Arc.ico |
| `Img_Bmp` | Отрисовка картинки | Img_Bmp.ico |
| `Img_ButEff` | Эффект кнопки | Img_ButEff.ico |
| `Img_Copy` | Копирование картинки | Img_Copy.ico |
| `Img_Diagram` | Диаграмма | Img_Diagram.ico |
| `Img_Edge` | Граница прямоугольника | Img_Edge.ico |
| `Img_Ellipse` | Элипс | Img_Ellipse.ico |
| `Img_FloodFill` | Заливка области | Img_FloodFill.ico |
| `Img_GetPoint` | Цвет пикселя | Img_GetPoint.ico |
| `Img_Gradient` | Градиентный прямоугольник | Img_Gradient.ico |
| `Img_Line` | Рисование линии | Img_Line.ico |
| `Img_Loader` | Загрузка изображений различных форматов | Img_Loader.ico |
| `Img_MultiText` | Отрисовка многострочного текста | Img_MultiText.ico |
| `Img_Point` | Рисование точки | Img_Point.ico |
| `Img_Polygon` | Многоугольник | Img_Polygon.ico |
| `Img_Rectangle` | Прямоугольник | Img_Rectangle.ico |
| `Img_Rotate` | Поворот картинки | Img_Rotate.ico |
| `Img_SetTransform` | Установка трансформации | Img_SetTransform.ico |
| `Img_Shift` | Сдвиг картинки | Img_Shift.ico |
| `Img_Text` | Рисование текста | Img_Text.ico |
| `Img_TextInRect` | Рисование текста в заданом прямоугольнике | Img_TextInRect.ico |
| `Img_Tween` | Наложение картинок | Img_Tween.ico |
| `Img_UseFlip` | Отражение | Img_UseFlip.ico |
| `Img_UseOffset` | Смещение | Img_UseOffset.ico |
| `Img_UseRotate` | Поворот | Img_UseRotate.ico |
| `Img_UseScale` | Маштаб | Img_UseScale.ico |
| `Img_UseSkew` | Скос | Img_UseSkew.ico |
| `Img_UseTransform` | Трансформация | Img_UseTransform.ico |
| `Jpeg` | Картинка Jpeg | Jpeg.ico |
| `Negative` | Негатив | Negative.ico |
| `PBlur` | Размытие | PBlur.ico |
| `PC_Image` | Контроллер картинок | PC_Image.ico |
| `PC_Position` | Контроллер положения | PC_Position.ico |
| `PC_Size` | Контроллер габаритов | PC_Size.ico |
| `PC_TableCellStyle` | Управление стилем ячеек | PC_TableCellStyle.ico |
| `PC_TableRow` | Управление ячейками | PC_TableRow.ico |
| `PC_TableSetCell` | Установка текста ячейки | PC_TableSetCell.ico |
| `PC_Text` | Контроллер текста | PC_Text.ico |
| `PC_TextParameters` | Контроллер текстовых параметров | PC_TextParameters.ico |
| `PNG` | Картинка в формате PNG | PNG.ico |
| `PictureStream` | Файл картинки | PictureStream.ico |
| `Print_GradientRect` | Печать градиента | Print_GradientRect.ico |
| `Print_Image` | Печать картинки | Print_Image.ico |
| `Print_Shape` | Печать фигуры | Print_Shape.ico |
| `Print_Table` | Печать таблицы | Print_Table.ico |
| `Print_Text` | Печать текста | Print_Text.ico |
| `Resize` | Размер | Resize.ico |
| `SelectColor` | Выбор цвета | SelectColor.ico |
| `Sharpened` | Резкость | Sharpened.ico |
| `TempFont` | Временный шрифт | TempFont.ico |
| `TempFontProc` | Управление временными шрифтами | TempFontProc.ico |
| `VectorFields` | Векторные поля | VectorFields.ico |
| `WaveProcessor` | Волновой процессор | WaveProcessor.ico |

### Internet (46)

| Элемент | Описание | Иконка |
|---|---|---|
| `DynDNS` | DynDNS | DynDNS.ico |
| `FTPC_DeleteFile` | Удаление файлов FTP | FTPC_DeleteFile.ico |
| `FTPC_DirAction` | Действия над папками FTP | FTPC_DirAction.ico |
| `FTPC_FileSearch` | Поиск файлов на FTP-сервере | FTPC_FileSearch.ico |
| `FTPC_ReadWrite` | Чтение/запись FTP | FTPC_ReadWrite.ico |
| `FTPC_Rename` | Переименование файлов и папок FTP | FTPC_Rename.ico |
| `FTP_Client` | FTP-клиент через WinAPI | FTP_Client.ico |
| `HTTP_FileUpload` | Отправка файла на сервер | HTTP_FileUpload.ico |
| `HTTP_Get` | Закачка по HTTP | HTTP_Get.ico |
| `HTTP_PostBuilder` | Построитель POST запроса | HTTP_PostBuilder.ico |
| `Host` | Определение HostName и IP-адреса | Host.ico |
| `JB_Base` | Jabber клиент | JB_Base.ico |
| `JB_Messages` | Сообщения | JB_Messages.ico |
| `JB_RosterList` | Список контактов | JB_RosterList.ico |
| `JB_Subscribe` | Подписка | JB_Subscribe.ico |
| `JB_XMLConsole` | XML консоль | JB_XMLConsole.ico |
| `MRA_Authorize` | Авторизация Mail.ru агента | MRA_Authorize.ico |
| `MRA_Base` | Mail.ru agent | MRA_Base.ico |
| `MRA_ChangeStatus` | Смена статуса | MRA_ChangeStatus.ico |
| `MRA_ContactAdd` | Добавление контакта | MRA_ContactAdd.ico |
| `MRA_ContactDelete` | Удаление контакта | MRA_ContactDelete.ico |
| `MRA_Contacts` | Контакты Mail.ru агента | MRA_Contacts.ico |
| `MRA_GroupAdd` | Добавление группы | MRA_GroupAdd.ico |
| `MRA_GroupDelete` | Удаление группы | MRA_GroupDelete.ico |
| `MRA_Groups` | Группы Mail.ru агента | MRA_Groups.ico |
| `MRA_MessageReciever` | Сообщения от Mail.ru агента | MRA_MessageReciever.ico |
| `MRA_Messages` | Сообщения Mail.ru agent | MRA_Messages.ico |
| `MRA_NewMail` | Нотификация о приходе почты | MRA_NewMail.ico |
| `MRA_SendSMS` | Отправка SMS | MRA_SendSMS.ico |
| `ModemDial` | Управление модемом | ModemDial.ico |
| `NetInterfaceInfo` | Информация об интерфейсе | NetInterfaceInfo.ico |
| `NetInterfaces` | Сетевые интерфейсы | NetInterfaces.ico |
| `NetworkLocator` | Локатор сети | NetworkLocator.ico |
| `POP3` | POP3 | POP3.ico |
| `POP3_Parse` | POP3 парсер | POP3_parse.ico |
| `Ping` | Пинг | Ping.ico |
| `Proxy` | Прокси | Proxy.ico |
| `SMTP` | SMTP | SMTP.ico |
| `TCPStat` | Информация по TCP и UDP протоколам | TCPStat.ico |
| `TCP_Client` | TCP клиент | TCP_Client.ico |
| `TCP_Server` | TCP сервер | TCP_Server.ico |
| `TCP_ServerEx` | TCP Server | TCP_ServerEx.ico |
| `TimeOnHost` | Определение даты и времени на удаленном компьютере | TimeOnHost.ico |
| `UDP` | UDP | UDP.ico |
| `URLBuilder` | Конструктор строки | URLBuilder.ico |
| `WinFTP` | FTP | WinFTP.ico |

### KernelChip (9)

| Элемент | Описание | Иконка |
|---|---|---|
| `KE_ADC` | Значение АЦП | KE_ADC.ico |
| `KE_Connect` | Соединение с устройством | KE_Connect.ico |
| `KE_DetectDevices` | Поиск устройств KernelChip | KE_DetectDevices.ico |
| `KE_LineDirection` | Направление линии | KE_LineDirection.ico |
| `KE_LineState` | Статус линий ввода/вывода | KE_LineState.ico |
| `KE_ReadLine` | Чтение значения | KE_ReadLine.ico |
| `KE_Reset` | Сброс параметров устройства | KE_Reset.ico |
| `KE_SerialNumber` | Серийный номер | KE_SerialNumber.ico |
| `KE_WriteLine` | Запись значения | KE_WriteLine.ico |

### Logic (3)

| Элемент | Описание | Иконка |
|---|---|---|
| `FastMathParse` | Компилятор математики | FastMathParse.ico |
| `MathParse` | Математический парсер | MathParse.ico |
| `Random` | Случайное число | Random.ico |

### Media (38)

| Элемент | Описание | Иконка |
|---|---|---|
| `BASS_CPU` | Загрузка CPU | BASS_CPU.ico |
| `BASS_ChannelAttribute` | Аттрибуты канала | BASS_ChannelAttribute.ico |
| `BASS_ChannelEnumTags` | Получение тегов | BASS_ChannelEnumTags.ico |
| `BASS_ChannelFX_Eq` | Эквалайзер | BASS_ChannelFX_Eq.ico |
| `BASS_ChannelPause` | Пауза канала | BASS_ChannelPause.ico |
| `BASS_ChannelPlay` | Проигрывание потока | BASS_ChannelPlay.ico |
| `BASS_ChannelPosition` | Позиция проигрывания | BASS_ChannelPosition.ico |
| `BASS_ChannelStop` | Останов потока | BASS_ChannelStop.ico |
| `BASS_ChannelVisibleOcilloScope` | Визуализация канала | BASS_ChannelVisibleOcilloScope.ico |
| `BASS_ChannelVisibleSpectrum` | Визуализация канала | BASS_ChannelVisibleSpectrum.ico |
| `BASS_Device` | Устройство вывода звука | BASS_Device.ico |
| `BASS_Init` | Инициализация BASS | BASS_Init.ico |
| `BASS_InputControl` | Управление устройством ввода | BASS_InputControl.ico |
| `BASS_MusicLoad` | Загрузка музыки форматов MO3/IT/XM/S3M/MTM/MOD/UMX | BASS_MusicLoad.ico |
| `BASS_RecordCenter` | Центр звукозаписи | BASS_RecordCenter.ico |
| `BASS_RecordDevices` | Устройства записи | BASS_RecordDevices.ico |
| `BASS_RecordInput` | Уствойство ввода | BASS_RecordInput.ico |
| `BASS_RecordStart` | Управление записью | BASS_RecordStart.ico |
| `BASS_StreamCreate` | Потоковый звук | BASS_StreamCreate.ico |
| `BASS_StreamCreateFile` | Создание потока | BASS_StreamCreateFile.ico |
| `BASS_StreamCreateURL` | Создание потока по URL | BASS_StreamCreateURL.ico |
| `BASS_Version` | Версия BASS | BASS_Version.ico |
| `BASS_Volume` | Звук BASS | BASS_Volume.ico |
| `DirectShowPlayer` | Медиа плеер | DirectShowPlayer.ico |
| `MP3_Info` | Информация об MP3 | MP3_Info.ico |
| `MediaPlayer` | Медиа плеер | MediaPlayer.ico |
| `PlayMIDI` | Проигрывание MIDI-файлов | PlayMIDI.ico |
| `PlaySound` | Проигрывание звука | PlaySound.ico |
| `Record` | Запись звука | Record.ico |
| `SoundBuffer` | Звуковой буфер | SoundBuffer.ico |
| `VolControl` | Контролер звука | VolControl.ico |
| `Volume` | Громкость | Volume.ico |
| `VolumeComparator` | Сравнение звука | VolumeComparator.ico |
| `VolumeDetector` | Детектор звука | VolumeDetector.ico |
| `VolumeEdge` | Огибающая звукового потока | VolumeEdge.ico |
| `VolumeSmoothing` | Сглаживание звукового потока | VolumeSmoothing.ico |
| `VolumeSummator` | Усреднение звукового потока | VolumeSummator.ico |
| `WaveArray` | Массив звуков | WaveArray.ico |

### Misc (11)

| Элемент | Описание | Иконка |
|---|---|---|
| `COM` | COM порт | COM.ico |
| `COMEX` | Асинхронный COM порт | COMEX.ico |
| `CallDLL` | Обращение к DLL | CallDLL.ico |
| `DynamicHiDLL` | Динамический DLL HiAsm | DynamicHiDLL.ico |
| `EnumCOM` | Перечисление COM-портов | EnumCOM.ico |
| `GamePort` | Игровой порт | GamePort.ico |
| `KeyHook` | Перехват клавиш | KeyHook.ico |
| `LPT` | LPT порт | LPT.ico |
| `PortIO` | Ввод\вывод в порт | PortIO.ico |
| `UseHiDLL` | DLL HiAsm | UseHiDLL.ico |
| `UseLoadDLL` | Управление загрузкой DLL | UseLoadDLL.ico |

### MultiThread (16)

| Элемент | Описание | Иконка |
|---|---|---|
| `MT_Add` | Добавление элемента | MT_Add.ico |
| `MT_AddData` | Добавляет данных в МТ | MT_AddData.ico |
| `MT_Array` | MT массив | MT_Array.ico |
| `MT_ChanelToIndex` | Упаковывает несколько каналов в один | MT_ChanelToIndex.ico |
| `MT_Enum` | Перебор данных MT | MT_Enum.ico |
| `MT_Get` | Чтение данных | MT_Get.ico |
| `MT_IndexToChanel` | Распаковывает один канал в несколько | MT_IndexToChanel.ico |
| `MT_MTArray` | Массив MT-потоков | MT_MTArray.ico |
| `MT_MTArrayEnum` | Перебор всех элементов массива MT-потоков | MT_MTArrayEnum.ico |
| `MT_MTArrayRW` | Запись и чтение из массива MT-потоков | MT_MTArrayRW.ico |
| `MT_Memory` | Сохранение потока | MT_Memory.ico |
| `MT_MultiData` | Разложение MT-потока на элементы | MT_MultiData.ico |
| `MT_MultiMem` | Запоминает данные из MT | MT_MultiMem.ico |
| `MT_Part` | Разбиение многомерного потока | MT_Part.ico |
| `MT_Stack` | MT-стек | MT_Stack.ico |
| `MT_String` | MT строка | MT_String.ico |

### OpenGL (48)

| Элемент | Описание | Иконка |
|---|---|---|
| `GL_BindTexture` | Вызов текстурного списка | GL_BindTexture.ico |
| `GL_BlendFunc` | Смешивание значений цвета в формате RGBA | GL_BlendFunc.ico |
| `GL_CallList` | Вызов дисплейного списка | GL_CallList.ico |
| `GL_ClipPlane` | Отсечение объектов плоскостями | GL_ClipPlane.ico |
| `GL_Color` | Цвет вершин | GL_Color.ico |
| `GL_Cube` | Куб | GL_Cube.ico |
| `GL_Cube2` | Паралеллепипед | GL_Cube2.ico |
| `GL_CullFace` | Отсечение сторон | GL_CullFace.ico |
| `GL_Cylinder` | Цилиндр | GL_Cylinder.ico |
| `GL_Disk` | Диск | GL_Disk.ico |
| `GL_Dodecahedron` | Додекаэдр | GL_Dodecahedron.ico |
| `GL_FlyHandler` | Симулятор полета | GL_FlyHandler.ico |
| `GL_Fog` | Туман | GL_Fog.ico |
| `GL_GTools` | Графический инструментарий | GL_GTools.ico |
| `GL_GenTextures` | Текстура с применением mipmap уровней | GL_GenTextures.ico |
| `GL_GenTexturesTga` | Текстура с применением mipmap уровней (Loads 24 and 32bpp TGA textures) | GL_GenTexturesTga.ico |
| `GL_GluTools` | Настройки GLU расширения | Gl_GluTools.ico |
| `GL_Light` | Источник света | GL_Light.ico |
| `GL_Main` | GL main | GL_Main.ico |
| `GL_Material` | Св-ва материала | GL_Material.ico |
| `GL_Normal` | Нормаль к поверхности | Gl_Normal.ico |
| `GL_Options` | Настройки вывода | GL_Options.ico |
| `GL_PartialDisk` | Часть диска | GL_PartialDisk.ico |
| `GL_Perspective` | Перспектива | GL_Perspective.ico |
| `GL_Plane` | Плоскость | GL_PLane.ico |
| `GL_Point3D` | Точка (X,Y,Z) | GL_Point3D.ico |
| `GL_PolygonMode` | Режим вывода полигонов | GL_PolygonMode.ico |
| `GL_PrimSizes` | Размеры примитивов | GL_PrimSizes.ico |
| `GL_Rotate` | Поворот | GL_Rotate.ico |
| `GL_Scissor` | Прямоугольник отсечения | GL_Scissor.ico |
| `GL_Sphere` | Сфера | GL_Sphere.ico |
| `GL_StencilFunc` | Функция трафарета | GL_StencilFunc.ico |
| `GL_StencilOp` | Операция трафарета | GL_StencilOp.ico |
| `GL_Teapot` | Чайник | GL_Teapot.ico |
| `GL_TexEnvMode` | Режим наложения текстур | GL_TexEnvMode.ico |
| `GL_Texture` | Текстура | GL_Texture.ico |
| `GL_Torus` | Тор | GL_Torus.ico |
| `GL_Vertex3D` | Вершина | Gl_Vertex3D.ico |
| `GL_VertexTexture` | Натягивание текстур | GL_VertexTexture.ico |
| `Gl_Axis` | Оси | Gl_Axis.ico |
| `Gl_Begin` | Начало блока точек | GL_Begin.ico |
| `Gl_BeginList` | Начало дисплейного списка | GL_BeginList.ico |
| `Gl_End` | Конец блока точек | GL_End.ico |
| `Gl_EndList` | Конец дисплейного списка | GL_EndList.ico |
| `Gl_Particles` | Частицы | Gl_Particles.ico |
| `Gl_Scale` | Масштаб | GL_Scale.ico |
| `Gl_Text` | Текст | GL_Text.ico |
| `Gl_Translate` | Перенос | Gl_Translate.ico |

### Strings (5)

| Элемент | Описание | Иконка |
|---|---|---|
| `MultiBlockFind` | Множественный цепочный поиск | MultiBlockFind.ico |
| `Position` | Позиция | Position.ico |
| `RE_Check` | Проверка строки на совпадение | RE_Check.ico |
| `RE_Replace` | Замена по регулярному выражению | RE_Replace.ico |
| `RE_Search` | Поиск вхождений регулярного выражения | RE_Search.ico |

### System (40)

| Элемент | Описание | Иконка |
|---|---|---|
| `AdapterInfo` | Cетевые адаптеры | AdapterInfo.ico |
| `BenchmarkCPU` | Тест CPU | BenchmarkCPU.ico |
| `BenchmarkHDD` | Тест HDD | BenchmarkHDD.ico |
| `BenchmarkRAM` | Тест RAM | BenchmarkRAM.ico |
| `CDROM` | СД-РОМ | CDROM.ico |
| `Clipboard` | Буфер обмена | Clipboard.ico |
| `DDEClient` | DDE клиент | DDEClient.ico |
| `DDEServer` | DDE сервер | DDEServer.ico |
| `DSKSwitch` | Перключатель рабочих столов | DSKSwitch.ico |
| `DeferredEvent` | Отложенное событие | DeferredEvent.ico |
| `DefragMemory` | Дефрагментация памяти | DefragMemory.ico |
| `Desktop` | Рабочий стол | Desktop.ico |
| `DeviceInfo` | Информация об устройствах | DeviceInfo.ico |
| `DiskInfo` | Логические диски | DiskInfo.ico |
| `EnumProcess` | Список процессов | EnumProcess.ico |
| `Events` | События | Events.ico |
| `FSMonitor` | Мониторинг ФС | FSMonitor.ico |
| `GHotKey` | Горячая клавиша | GHotKey.ico |
| `HardDisk` | Диски | HardDisk.ico |
| `KeyEvent` | Эмуляция нажатия клавиш | KeyEvent.ico |
| `KeyboardLayout` | Переключатель языков | KeyboardLayout.ico |
| `LedKey` | Клавиши-лампочки | LedKey.ico |
| `MailSlot_Client` | MailSlot клиент | MailSlot_Client.ico |
| `MailSlot_Server` | MailSlot сервер | MailSlot_Server.ico |
| `MemoryInfo` | Физическая память | MemoryInfo.ico |
| `MonthCalendar` | Месячный календарь | MonthCalendar.ico |
| `MotherBoardInfo` | Материнская плата | MotherBoardInfo.ico |
| `MouseEvent` | Эмулятор мыши | MouseEvent.ico |
| `MutexThread` | Синхронизируемый поток | MutexThread.ico |
| `Printer` | Принтер | Printer.ico |
| `ProcInfo` | Процессоры | ProcInfo.ico |
| `ProcessInfo` | Запущенные процессы | ProcessInfo.ico |
| `SCR` | Управление скринсейвером | SCR.ico |
| `SafeMode` | Защита ресурсов мьютексом | SafeMode.ico |
| `ServiceTools` | Управление сервисами | ServiceTools.ico |
| `Synchronize` | Вызов события в главном потоке из параллельного потока | Synchronize.ico |
| `TWAIN` | Управление сканером | TWAIN.ico |
| `Thread` | Поток | Thread.ico |
| `WebCam` | Захват изображения с WEB-камеры | WebCam.ico |
| `Zipper` | Zip архивы | Zipper.ico |

### Type (16)

| Элемент | Описание | Иконка |
|---|---|---|
| `Type_Add` | Добавляет переменные в тип | Type_Add.ico |
| `Type_Array` | Работа с типом как с массивом | Type_Array.ico |
| `Type_Combine` | Объединение типов | Type_Combine.ico |
| `Type_Create` | Создает новый тип | Type_Create.ico |
| `Type_CreateFromList` | Создание типа | Type_CreateFromList.ico |
| `Type_Delete` | Удаляет переменные из типа | Type_Delete.ico |
| `Type_FastCreate` | Создание быстрого типа | Type_FastCreate.ico |
| `Type_FastMem` | Быстрый доступ к типу | Type_FastMem.ico |
| `Type_If` | Сравнение типов | Type_If.ico |
| `Type_Load` | Загрузка типа | Type_Load.ico |
| `Type_MultiData` | Раскладывает переменные в поток | Type_MultiData.ico |
| `Type_MultiMem` | Извлекает значения из типа | Type_MultiMem.ico |
| `Type_RW` | Чтение\запись переменных | Type_RW.ico |
| `Type_Save` | Сохранение типа | Type_Save.ico |
| `Type_Set` | Изменяет значение переменных | Type_Set.ico |
| `Type_TypeMemory` | Содержит тип в памяти | Type_TypeMemory.ico |

### WinControls (73)

| Элемент | Описание | Иконка |
|---|---|---|
| `AlphaImage` | Отображение Alpha-картинок | AlphaImage.ico |
| `BitBtn` | Кнопка с картинкой | BitBtn.ico |
| `BoxDrawManager` | Менеджер отрисовки | BoxDrawManager.ico |
| `ButtonRush` | Кнопка | ButtonRush.ico |
| `CheckBoxRush` | Флажок | CheckBoxRush.ico |
| `ChildForm` | Дочерняя форма | ChildForm.ico |
| `ChildFormEx` | Дочерняя форма | ChildFormEx.ico |
| `ChildFormPoly` | Полиморфная форма | ChildFormPoly.ico |
| `ChildGroupBox` | Группа элементов | ChildGroupBox.ico |
| `ChildPanel` | Панель | ChildPanel.ico |
| `ChildPanelEx` | Панель | ChildPanelEx.ico |
| `ChildPanelPoly` | Полиморфная Панель | ChildPanelPoly.ico |
| `ChildPanelRush` | Панель | ChildPanelRush.ico |
| `ChildScrollBox` | Панель прокрутки | ChildScrollBox.ico |
| `ColorBox` | Выпадающий список цветов | ColorBox.ico |
| `CtrlPalette` | Палитра ссылок | CtrlPalette.ico |
| `DS_StaticData` | Статические данные | DS_StaticData.ico |
| `DS_UserData` | Пользовательские данные | DS_UserData.ico |
| `DatePicker` | Выпадающий календарь | DatePicker.ico |
| `DriveBox` | Список дисков | DriveBox.ico |
| `DropFileManager` | Приём сброшенных объектов | DropFileManager.ico |
| `DropTextManager` | Приём сброшенного текста | DropTextManager.ico |
| `Flash` | Flash-проигрыватель | Flash.ico |
| `FontBox` | Список шрифтов | FontBox.ico |
| `FontManager` | Менеджер параметров шрифта | FontManager.ico |
| `GLabel` | Градиентная Надпись | GLabel.ico |
| `GProgressBar` | Панель прогресса | GProgressBar.ico |
| `Grapher` | График | Grapher.ico |
| `GridLayout` | Слой решетка | GridLayout.ico |
| `HBoxLayout` | Горизонтальный слой | HBoxLayout.ico |
| `HilightMemo` | Редактор с подсветкой синтаксиса | HiLightMemo.ico |
| `HintManager` | Менеджер подсказки | HintManager.ico |
| `IconsManager` | Менеджер иконок | IconsManager.ico |
| `Image` | Изображение | Image.ico |
| `ImgBtn` | Кнопка-картинка | ImgBtn.ico |
| `IndexManager` | Менеджер индексов | IndexManager.ico |
| `LED` | Светодиод | LED.ico |
| `LayoutSpacer` | Отступ в слоях | LayoutSpacer.ico |
| `LedLadder` | Светодиодный индикатор | LedLadder.ico |
| `LedNumber` | Цифровое табло | LedNumber.ico |
| `MEnumerator` | Перечисление данных | MEnumerator.ico |
| `MListBox` | Список строк | MListBox.ico |
| `MReadWrite` | Запись и чтение данных | MReadWrite.ico |
| `Menu` | Главное меню | Menu.ico |
| `MenuEx` | Расширенное главное меню | MenuEx.ico |
| `PageControl` | Страницы | PageControl.ico |
| `PaintBox` | Рисование | PaintBox.ico |
| `PopupMenuEx` | Расширенное вcплывaющee мeню | PopupMenuEx.ico |
| `ProgressBarRush` | Отображает значение параметра | ProgressBarRush.ico |
| `RadioButtonRush` | Пepeключaтeль | RadioButtonRush.ico |
| `RedrawManager` | Менеджер перерисовки | RedrawManager.ico |
| `RichEdit` | Расширенный рeдaктop тeкcтa | RichEdit.ico |
| `ScrollBarEx` | Скролинг | ScrollBarEx.ico |
| `Splitter` | Сплиттер | Splitter.ico |
| `StatusBar` | Строка состояния | StatusBar.ico |
| `StatusBarEx` | Строка состояния c панелью прогресса | StatusBarEx.ico |
| `StringTable` | Таблица строк | StringTable.ico |
| `StringTableMT` | Таблица строк с поддержкой MT | StringTableMT.ico |
| `TabControl` | Контроллёр страниц | TabControl.ico |
| `ToolBar` | Панель инструментов | ToolBar.ico |
| `ToolBarLayout` | Инструментальный слой | ToolBarLayout.ico |
| `TrackBarRush` | ТрекБар | TrackBarRush.ico |
| `TransparentManager` | Менеджер прозрачности формы | TransparentManager.ico |
| `TreeView` | Дерево элементов | TreeView.ico |
| `TreeViewEx` | Расширенное дерево элементов | TreeViewEx.ico |
| `UpDown` | Счетчик | UpDown.ico |
| `UseExtCursor` | Установка внешнего курсора | UseExtCursor.ico |
| `UseWinAnimation` | Управление анимацией | UseWinAnimation.ico |
| `UserHintManager` | Пользовательский менеджер подсказки | UserHintManager.ico |
| `VBoxLayout` | Вертикальный слой | VBoxLayout.ico |
| `VisualShape` | Shape | VisualShape.ico |
| `WebBrowser` | Интернет браузер | WebBrowser.ico |
| `WorkIconsManager` | Управление списком иконок | WorkIconsManager.ico |

### WinManControls (47)

| Элемент | Описание | Иконка |
|---|---|---|
| `CMD_PopupMenu` | Контекстное меню | CMD_PopupMenu.ico |
| `CMD_State` | Статус команды | CMD_State.ico |
| `CMD_ToolBar` | Панель инструментов | CMD_ToolBar.ico |
| `CommandCenter` | Коммандный центр | CommandCenter.ico |
| `MST_ArrayCols` | Массив столбцов | MST_ArrayCols.ico |
| `MST_ArrayRows` | Массив строк | MST_ArrayRows.ico |
| `MST_CheckBoxes` | Управление флажками состояния | MST_CheckBoxes.ico |
| `MST_Clear` | Очистка | MST_Clear.ico |
| `MST_ColAction` | Действия над столбцами | MST_ColAction.ico |
| `MST_DB` | Добавление строк и стобцов | MST_DB.ico |
| `MST_DrawManager` | Пользовательская отрисовка | MST_DrawManager.ico |
| `MST_EditEnd` | Управление редактированием | MST_EditEnd.ico |
| `MST_FindText` | Поиск текста | MST_FindText.ico |
| `MST_HTML` | Конвертор таблицы в формат HTML | MST_HTML.ico |
| `MST_Load` | Загрузка из файла | MST_Load.ico |
| `MST_Matrix` | Матрица строк | MST_Matrix.ico |
| `MST_RowAction` | Действия над строками | MST_RowAction.ico |
| `MST_Save` | Сохранение в файл | MST_Save.ico |
| `MST_SelectRow` | Выделение строки | MST_SelectRow.ico |
| `MST_Selected` | Управление выбором строк | MST_Selected.ico |
| `MST_SetColors` | Управление цветом | MST_SetColors.ico |
| `MST_SortInCol` | Сортировка строк в столбце | MST_SortInCol.ico |
| `MST_UseEditCtrl` | Управление внешними Edit | MST_UseEditCtrl.ico |
| `MST_Virtual` | Виртуальная таблица | MST_Virtual.ico |
| `MTStrTbl` | MT таблица строк | MTStrTbl.ico |
| `PlotAxis` | Ось на графике | PlotAxis.ico |
| `PlotDiffSeries` | Сравнение серий | PlotDiffSeries.ico |
| `PlotHistogram` | Гистограмма | PlotHistogram.ico |
| `PlotLines` | Линейный график | PlotLines.ico |
| `PlotMarker` | Маркер на графике | PlotMarker.ico |
| `PlotMouseValues` | Значение графика в точке | PlotMouseValues.ico |
| `PlotPoints` | Точечный график | PlotPoints.ico |
| `PlotSeriesMarker` | Маркеры значений графика | PlotSeriesMarker.ico |
| `PlotStream` | Вывод потока | PlotStream.ico |
| `Plotter` | График | Plotter.ico |
| `TVT_AddNode` | Добавление элемента | TVT_AddNode.ico |
| `TVT_ChangeNode` | Изменение элемента | TVT_ChangeNode.ico |
| `TVT_DataSource` | Источник данных дерева | TVT_DataSource.ico |
| `TVT_DeleteNode` | Удаление элемента | TVT_DeleteNode.ico |
| `TVT_EnumNodes` | Перебор элементов дерева | TVT_EnumNodes.ico |
| `TVT_FirstNode` | Получение первого элемента | TVT_FirstNode.ico |
| `TVT_GetNode` | Получение элемента | TVT_GetNode.ico |
| `TVT_ItemPath` | Полный путь к элементу | TVT_ItemPath.ico |
| `TVT_LoadTree` | Загрузка дерева | TVT_LoadTree.ico |
| `TVT_SaveTree` | Сохранение дерева | TVT_SaveTree.ico |
| `TVT_SelectNode` | Выделение элемента | TVT_SelectNode.ico |
| `TreeViewTrain` | Дерево элементов | TreeViewTrain.ico |

### Windows (46)

| Элемент | Описание | Иконка |
|---|---|---|
| `CeCallDLL` | Вызов функций из DLL | — |
| `CeDeviceInfo` | Некоторая информация об устройстве | — |
| `CeDir` | Путь к некоторым папкам | — |
| `CeDirTools` | Операции с папками | — |
| `CeDiskSpace` | Информация о дисковом пространстве | — |
| `CeFileAttributes` | Аттрибуты файла/папки | — |
| `CeFileSearch` | Пoиcк файлов | — |
| `CeFileStream` | Пoтoк данных файла | — |
| `CeFileTools` | Операции с фaйлaми | — |
| `CeInit` | Соединение с ActiveSync | — |
| `CeMemoryStatus` | Информация о памяти устройства | — |
| `CeNotify` | Сигнал о подключении устройств к ActiveSync | — |
| `CeOSVersion` | Информация о версии ОС | — |
| `CeRegistry` | Работа с реестром | — |
| `CeWinExec` | Запуск программ | — |
| `MoveWindow` | Перемещение окна | MoveWindow.ico |
| `PosWindow` | Положение окна | PosWindow.ico |
| `RGN_Array` | Массив регионов | RGN_Array.ico |
| `RGN_Collision` | Коллизия регионов | RGN_Collision.ico |
| `RGN_Combine` | Комбинирует два региона | RGN_Combine.ico |
| `RGN_Convertor` | Конвертирование региона | RGN_Convertor.ico |
| `RGN_Draw` | Рисование региона | RGN_Draw.ico |
| `RGN_Elliptic` | Эллиптический регион | RGN_Elliptic.ico |
| `RGN_File` | Сохранение и загрузка региона | RGN_File.ico |
| `RGN_Form` | Создание региона на основе контролов формы или панели | RGN_Form.ico |
| `RGN_GetData` | Данные о регионе | RGN_GetData.ico |
| `RGN_InRect` | Проверка нахождения региона в прямоугольнике | RGN_InRect.ico |
| `RGN_MultiCombine` | Комбинирует много регионов | RGN_MultiCombine.ico |
| `RGN_Offset` | Перемещение региона | RGN_Offset.ico |
| `RGN_OutlinePicture` | Контурный регион | RGN_OutlinePicture.ico |
| `RGN_Path` | Регион из траектории | RGN_Path.ico |
| `RGN_PointXY` | Проверка нахождения точки в регионе | RGN_PointXY.ico |
| `RGN_Rect` | Прямоугольный регион | RGN_Rect.ico |
| `RGN_Rotate` | Поворот регион | RGN_Rotate.ico |
| `RGN_RoundRect` | Прямоугольный регион с закругленными углами | RGN_RoundRect.ico |
| `RGN_Script` | Создание региона по скрипту | RGN_Script.ico |
| `RGN_SetToWindow` | Применить регион к окну | RGN_SetToWindow.ico |
| `RGN_Text` | Регион из строки | RGN_Text.ico |
| `RGN_Transform` | Трансформация региона | RGN_Transform.ico |
| `ScreenShort` | Снятие скриншотов с окон и с экрана | ScreenShort.ico |
| `SendMessage` | Посылка сообщений окну | SendMessage.ico |
| `SetParent` | Переназеачение родительского окна | SetParent.ico |
| `SizeWindow` | Размер окна | SizeWindow.ico |
| `SysMenu` | Системное меню формы | SysMenu.ico |
| `WinEnum` | Перебор окон | WinEnum.ico |
| `WindowAnimation` | Анимация окон | WindowAnimation.ico |

