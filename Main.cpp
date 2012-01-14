
#include <windows.h>

#include <cstdio>
#include <string>

typedef HRESULT (STDAPICALLTYPE  *FindMimeFromDataFunc)(LPBC, LPCWSTR, LPVOID, DWORD, LPCWSTR, DWORD, LPWSTR*, DWORD);

std::string WStrToStr(const std::wstring &src, UINT codePage = CP_ACP)
{
	std::string ret;

	int size = WideCharToMultiByte(codePage, 0, src.c_str(), src.size() + 1, 0, 0, 0,0);
	if (size)
	{
		ret.resize(size - 1);

		size = WideCharToMultiByte(codePage, 0, src.c_str(), src.size() + 1, &ret[0], size, 0, 0);
		if (!size)
		{
			ret.clear();
		}
	}

	return ret;
}

std::string RegReadValue(HKEY rootKey, const std::string &subKey, const std::string &name)
{
	HKEY key;

	LONG ret = RegOpenKeyExA(rootKey, subKey.c_str(), 0, KEY_READ, &key);
	if (ret != ERROR_SUCCESS)
		return "";

	const DWORD bufferSize = 1024;
	char buffer[bufferSize];
	DWORD size = bufferSize;
	DWORD type = REG_SZ;

	ret = RegQueryValueExA(key, name.c_str(), 0, &type, (LPBYTE)buffer, &size);
	if (ret != ERROR_SUCCESS)
		return "";

	RegCloseKey(key);

	return buffer;
}

std::string GetMimeTypeFromFile(const std::string &fileName)
{
	HMODULE dll = LoadLibraryA("urlmon.dll");
	if(!dll)
		return "";

	FindMimeFromDataFunc pFindMimeFromData = (FindMimeFromDataFunc)GetProcAddress(dll,"FindMimeFromData");
	if (!pFindMimeFromData)
		return "";

	FILE* file = fopen(fileName.c_str(), "rb");
	if (!file)
		return "";

	const int bufferSize = 256;
	BYTE buffer[bufferSize] ;
	int readSize = fread(buffer, 1, bufferSize, file);
	fclose(file);

	PWSTR mime = NULL ;
	if (NOERROR != pFindMimeFromData(NULL, NULL, buffer, readSize, NULL, 0, &mime, 0))
		return "";

	FreeLibrary(dll);

	std::string ret = WStrToStr(mime);
	
	CoTaskMemFree(mime);

	return ret;
}

std::string GetExtentionFromMimeType(const std::string &mimeType)
{
	std::string subKey = "MIME\\Database\\Content Type\\" + mimeType;
	return RegReadValue(HKEY_CLASSES_ROOT, subKey, "Extension");
}

std::string GetOpenCommandFromExtention(const std::string &ext)
{
	std::string subKey = ext;
	std::string app = RegReadValue(HKEY_CLASSES_ROOT, subKey, "");
	
	if (app.empty())
		return "";

	subKey = app + "\\shell\\open\\command";
	return RegReadValue(HKEY_CLASSES_ROOT, subKey, "");
}

int main()
{
	std::string mimeType = GetMimeTypeFromFile("Penguins");
	if (mimeType.empty())
		return 0;
	printf(mimeType.c_str());

	std::string ext = GetExtentionFromMimeType(mimeType);
	if (ext.empty())
		return 0;
	printf(ext.c_str());

	std::string command = GetOpenCommandFromExtention(ext);
	if (command.empty())
		return 0;
	printf(command.c_str());

	return 0;
}
