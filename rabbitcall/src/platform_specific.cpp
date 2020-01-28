#include "pch.h"

#if defined (_MSC_VER)
#include <SDKDDKVer.h>
#include <windows.h>
#endif

void setProcessPriorityToLow() {
#if defined (_MSC_VER)
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
#endif
}

FILE * openFileOrThrow(const filesystem::path &path, const string &mode, int64_t *fileSizeOut) {
	FILE *file = nullptr;

#if defined (_MSC_VER)

	// Windows does not support UTF-8 file names in fopen() => use the wchar_t version.
	u16string modeUtf16 = convertUtf8ToUtf16(mode);
	if (_wfopen_s(&file, path.wstring().c_str(), wstring(modeUtf16.begin(), modeUtf16.end()).c_str()) != 0 || !file) {
		EXC(sb() << "Error opening file: " << path.u8string());
	}

	if (fileSizeOut) {
		_fseeki64(file, 0, SEEK_END);
		*fileSizeOut = _ftelli64(file);
		_fseeki64(file, 0, SEEK_SET);
	}

#else

	file = fopen(path.u8string().c_str(), mode.c_str());
	if (!file) EXC(sb() << "Error opening file: " << path.u8string());

	if (fileSizeOut) {
		*fileSizeOut = filesystem::file_size(path);
	}

#endif

	return file;
}

