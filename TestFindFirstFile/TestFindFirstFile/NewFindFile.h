#pragma once
#include <Windows.h>

HANDLE
WINAPI
NewFirstFind(
	_In_ LPCWSTR lpFileName,
	_Out_ WIN32_FIND_DATA* pData
);
BOOL
WINAPI
NewFindNextFile(
	_In_ HANDLE hFindFile,
	_Out_ LPWIN32_FIND_DATAW lpFindFileData
);
BOOL
WINAPI
NewFindClose(
	_Inout_ HANDLE hFindFile
);
