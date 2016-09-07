// TestFindFirstFile.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <strsafe.h>
#include <string.h>
#include <iostream>
#include <string>
#include "NewFindFile.h"



std::string wstrtostr(const std::wstring &wstr)
{
	// Convert a Unicode string to an ASCII string
	std::string strTo;
	char *szTo = new char[wstr.length() + 1];
	szTo[wstr.size()] = '\0';
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, szTo, (int)wstr.length(), NULL, NULL);
	strTo = szTo;
	delete[] szTo;
	return strTo;
}

std::wstring strtowstr(const std::string &str)
{
	// Convert an ASCII string to a Unicode String
	std::wstring wstrTo;
	wchar_t *wszTo = new wchar_t[str.length() + 1];
	wszTo[str.size()] = L'\0';
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wszTo, (int)str.length());
	wstrTo = wszTo;
	delete[] wszTo;
	return wstrTo;
}
std::wstring towstr(LPCSTR str)
{
	// Convert an ASCII string to a Unicode String
	std::wstring wstrTo;
	int wchars_num = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	wchar_t *wszTo = new wchar_t[wchars_num+1];
	wszTo[wchars_num] = L'\0';
	MultiByteToWideChar(CP_UTF8, 0, str, -1, wszTo, wchars_num);
	wstrTo = wszTo;
	delete[] wszTo;
	return wstrTo;
}
#define BUFSIZE MAX_PATH



int main(int argc, char *argv[])
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError;


	// Check for command-line parameter; otherwise, print usage.
	if (argc != 2)
	{
		printf("Usage: Test <dir>\n");
		return 2;
	}

	std::wstring ws = strtowstr(argv[1]);
	if (ws.at(ws.length()-1) != L'\\' )
		ws += L"\\*";
	else
		ws += L"*";

	// Find the first file in the directory.
	hFind = FindFirstFile(ws.c_str(), &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		printf("Invalid file handle. Error is %u.\n", GetLastError());
		return (-1);
	}
	else
	{
		printf("First file name is %ls.\n", FindFileData.cFileName);

		// List all the other files in the directory.
		while (FindNextFile(hFind, &FindFileData) != 0)
		{
			printf("Next file name is %ls.\n", FindFileData.cFileName);
		}

		dwError = GetLastError();
		FindClose(hFind);
		if (dwError != ERROR_NO_MORE_FILES)
		{
			printf("FindNextFile error. Error is %u.\n", dwError);
			return (-1);
		}
	}
	// Find the first file in the directory.
	std::wstring folder = strtowstr(argv[1]);
	hFind = NewFirstFind(folder.c_str(), &FindFileData);

		
	if (hFind == INVALID_HANDLE_VALUE)
	{
		printf("NewFindFirstFile Invalid file handle. Error is %u.\n", GetLastError());
		return (-1);
	}
	else
	{
		if(FindFileData.dwFileAttributes&0x10)
			printf("NewFindFirstFile First directory name is %ls.\n", FindFileData.cFileName);
		else
			printf("NewFindFirstFile First file name is %ls.\n", FindFileData.cFileName);

		// List all the other files in the directory.
		while (NewFindNextFile(hFind, &FindFileData) != 0)
		{
			if (FindFileData.dwFileAttributes & 0x10)
				printf("NewFindNextFile First directory name is %ls.\n", FindFileData.cFileName);
			else
				printf("NewFindNextFile First file name is %ls.\n", FindFileData.cFileName);
		}

		dwError = GetLastError();
		NewFindClose(hFind);
		if (dwError != ERROR_NO_MORE_FILES)
		{
			printf("FindNextFile FindNextFile error. Error is %u.\n", dwError);
			return (-1);
		}
	}

	return (0);
}
