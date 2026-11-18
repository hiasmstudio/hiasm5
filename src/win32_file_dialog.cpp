/*
 * Standard Windows file dialogs (IFileOpenDialog / IFileSaveDialog).
 *
 * On Windows the GTK4 build runs these instead of the GTK file chooser (see dialog_run()
 * in gtkcompat.h), so open/save/folder windows look like every other Windows window.
 * The file is compiled to nothing on other platforms.
 */

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shobjidl.h>
#include <string>
#include <vector>

namespace {

std::wstring to_wide(const std::string &text) {
	if(text.empty())
		return std::wstring();
	int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), NULL, 0);
	std::wstring wide(size, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), &wide[0], size);
	return wide;
}

std::string to_utf8(const wchar_t *wide) {
	if(!wide)
		return std::string();
	int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
	if(size <= 1)
		return std::string();
	std::string text(size - 1, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wide, -1, &text[0], size, NULL, NULL);
	return text;
}

// shell item for an existing path; NULL when it cannot be resolved
IShellItem *shell_item(const std::string &path) {
	IShellItem *item = NULL;
	if(FAILED(SHCreateItemFromParsingName(to_wide(path).c_str(), NULL, IID_PPV_ARGS(&item))))
		return NULL;
	return item;
}

} // namespace

// kind: 0 = open file, 1 = save file, 2 = select folder
bool win32_file_dialog(int kind, const std::string &title, const std::string &initial_name,
		const std::string &initial_folder, const std::vector<std::string> &places,
		void *parent, std::string &result) {
	bool com_started = SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));

	const CLSID &clsid = (kind == 1) ? CLSID_FileSaveDialog : CLSID_FileOpenDialog;
	IFileDialog *dialog = NULL;
	bool ok = false;
	if(SUCCEEDED(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))) && dialog) {
		DWORD options = 0;
		dialog->GetOptions((FILEOPENDIALOGOPTIONS *)&options);
		options |= FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR;
		if(kind == 2)
			options |= FOS_PICKFOLDERS;
		dialog->SetOptions((FILEOPENDIALOGOPTIONS)options);

		if(!title.empty())
			dialog->SetTitle(to_wide(title).c_str());
		if(kind != 2) {
			COMDLG_FILTERSPEC all_files = { L"All files (*.*)", L"*.*" };
			dialog->SetFileTypes(1, &all_files);
			dialog->SetFileTypeIndex(1);
		}
		if(!initial_folder.empty()) {
			if(IShellItem *folder = shell_item(initial_folder)) {
				dialog->SetFolder(folder);
				folder->Release();
			}
		}
		if(!initial_name.empty())
			dialog->SetFileName(to_wide(initial_name).c_str());
		for(size_t i = 0; i < places.size(); i++) {
			if(IShellItem *item = shell_item(places[i])) {
				dialog->AddPlace(item, FDAP_BOTTOM);
				item->Release();
			}
		}

		if(SUCCEEDED(dialog->Show((HWND)parent))) {
			IShellItem *item = NULL;
			if(SUCCEEDED(dialog->GetResult(&item)) && item) {
				LPWSTR path = NULL;
				if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path) {
					result = to_utf8(path);
					CoTaskMemFree(path);
					ok = !result.empty();
				}
				item->Release();
			}
		}
		dialog->Release();
	}

	if(com_started)
		CoUninitialize();
	return ok;
}

#endif
