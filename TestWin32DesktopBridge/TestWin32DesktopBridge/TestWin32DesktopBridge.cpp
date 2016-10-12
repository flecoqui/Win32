// TestWin32DesktopBridge.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "TestWin32DesktopBridge.h"


#include <iostream>
#include <iomanip>
#include <roapi.h>
//#include <windows.globalization.h>
//#include <Windows.System.Profile.h>

using namespace std;
//using namespace ABI::Windows::Globalization;
//using namespace ABI::Windows::System::Profile;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TESTWIN32DESKTOPBRIDGE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTWIN32DESKTOPBRIDGE));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTWIN32DESKTOPBRIDGE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TESTWIN32DESKTOPBRIDGE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}
BOOL
IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion, WORD wServicePackMajor)
{
	OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0,{ 0 }, 0, 0 };
	DWORDLONG        const dwlConditionMask = VerSetConditionMask(
		VerSetConditionMask(
			VerSetConditionMask(
				0, VER_MAJORVERSION, VER_GREATER_EQUAL),
			VER_MINORVERSION, VER_GREATER_EQUAL),
		VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

	osvi.dwMajorVersion = wMajorVersion;
	osvi.dwMinorVersion = wMinorVersion;
	osvi.wServicePackMajor = wServicePackMajor;

	return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) != FALSE;
}
BOOL IsWindows10OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN10), LOBYTE(_WIN32_WINNT_WIN10), 0);
}
BOOL IsWindows8OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN8), LOBYTE(_WIN32_WINNT_WIN8), 0);
}
BOOL IsWindows7OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7), 0);
}
BOOL IsWindowsVistaOrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 0);
}
BOOL IsWindowsXPOrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 0);
}
BOOL IsWindows2KOrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN2K), LOBYTE(_WIN32_WINNT_WIN2K), 0);
}
BOOL IsWindowsNTOrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_NT4), LOBYTE(_WIN32_WINNT_NT4), 0);
}

BOOL GetUWPVersion(unsigned int* pv1, unsigned int* pv2, unsigned int* pv3, unsigned int* pv4)
{
	BOOL bResult = FALSE;
	HMODULE h = ::LoadLibrary(L"UWPBridge.DLL");
	if (h > 0)
	{
		BOOL(WINAPI * pGetUWPVersion)(unsigned int* pv1, unsigned int* pv2, unsigned int* pv3, unsigned int* pv4);
		pGetUWPVersion = (BOOL (WINAPI*) (unsigned int* pv1, unsigned int* pv2, unsigned int* pv3, unsigned int* pv4)) ::GetProcAddress(h, "GetUWPVersion");
		if (pGetUWPVersion != NULL)
		{
			bResult = pGetUWPVersion(pv1, pv2, pv3, pv4);
		}
		::FreeModule(h);
	}
	return bResult;
}
HRESULT __stdcall CallbackActivated()
{
	::MessageBox(NULL, L"User clicked on Toast", L"Toast", MB_OK);
	return S_OK;
}
BOOL IsRunningWithinCentennial()
{
	BOOL bResult = FALSE;
	HMODULE h = ::LoadLibrary(L"UWPBridge.DLL");
	if (h > 0)
	{
		BOOL(WINAPI * pIsRunningWithinCentennial)();
		pIsRunningWithinCentennial = (BOOL(WINAPI*) ()) ::GetProcAddress(h, "InContainer");
		if (pIsRunningWithinCentennial != NULL)
		{
			/// To be fixed
			bResult = pIsRunningWithinCentennial();
		}
		::FreeModule(h);
	}
	return bResult;

}
BOOL ShowUWPToast(LPCWSTR pLogo, LPCWSTR pTitle, LPCWSTR pMessage, LPCWSTR pAppId)
{
	BOOL bResult = FALSE;
	HMODULE h = ::LoadLibrary(L"UWPBridge.DLL");
	if (h > 0)
	{
		BOOL(WINAPI * pShowUWPToast)(LPCWSTR pLogo, LPCWSTR pTitle, LPCWSTR pMessage, LPCWSTR pAppId, HRESULT(__stdcall * callbackActivated)());
		pShowUWPToast = (BOOL(WINAPI*) (LPCWSTR pLogo, LPCWSTR pTitle, LPCWSTR pMessage, LPCWSTR pAppId, HRESULT(__stdcall * callbackActivated)())) ::GetProcAddress(h, "ShowUWPToast");
		if (pShowUWPToast != NULL)
		{
			bResult = pShowUWPToast(pLogo, pTitle, pMessage, pAppId, CallbackActivated);
		}
		::FreeModule(h);
	}
	return bResult;
}
/*
BOOL GetWindows10Version(unsigned int* pv1, unsigned int* pv2, unsigned int* pv3, unsigned int* pv4)
{
	HRESULT hr;
	HSTRING hs;
	hr = ::WindowsCreateString(L"Foo", 3, &hs);

	hr = ::RoInitialize(RO_INIT_MULTITHREADED);
	if (SUCCEEDED(hr))
	{
		HSTRING hClassName;
		wstring className(L"Windows.System.Profile.AnalyticsInfo");
		hr = ::WindowsCreateString(className.c_str(), className.size(), &hClassName);
		if (SUCCEEDED(hr))
		{
			IInspectable* pInst;
			IAnalyticsInfoStatics* pInfo;
			///hr = ::RoActivateInstance(hClassName, &pInst);
			hr = ::RoGetActivationFactory(hClassName, __uuidof(IAnalyticsInfoStatics), (void**)&pInfo);
			if (SUCCEEDED(hr))
			{
				IAnalyticsVersionInfo* pVersionInfo;
				hr = pInfo->get_VersionInfo(&pVersionInfo);
				if (SUCCEEDED(hr))
				{

					hr = pVersionInfo->get_DeviceFamilyVersion(&hs);
					if (SUCCEEDED(hr))
					{
						HSTRING hVersion;
						hr = ::WindowsCreateString(L"10", 3, &hVersion);
						if (SUCCEEDED(hr))
						{
							UINT32 len = 0;
							PCWSTR p = ::WindowsGetStringRawBuffer(hs, NULL);
							unsigned long long v = wcstoull(p, NULL, 10);
							*pv1 = (v & 0xFFFF000000000000L) >> 48;
							*pv2 = (v & 0x0000FFFF00000000L) >> 32;
							*pv3 = (v & 0x00000000FFFF0000L) >> 16;
							*pv4 = (v & 0x000000000000FFFFL);
							::WindowsDeleteString(hClassName);
							::WindowsDeleteString(hVersion);
							pVersionInfo->Release();
							pInfo->Release();
							::RoUninitialize();
							return TRUE;
						}
					}
					pVersionInfo->Release();
				}
				pInfo->Release();
			}
			::WindowsDeleteString(hClassName);
		}
		::RoUninitialize();
	}
	return FALSE;
}
*/
//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	OSVERSIONINFO osvi;
	BOOL bIsWindowsXPorLater;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {

            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
			case ID_TESTDESKTOPBRIDGE_VERSION:
				if (IsWindows8OrGreater())
				{
					unsigned int v1 = 0;
					unsigned int v2 = 0;
					unsigned int v3 = 0;
					unsigned int v4 = 0;
					if (GetUWPVersion(&v1, &v2, &v3, &v4))
					{
						int const arraysize = 300;
						WCHAR pszDest[arraysize];
						size_t cbDest = arraysize * sizeof(WCHAR);

						HRESULT hr = StringCbPrintfW(pszDest, cbDest, L"Windows 10 version: %d.%d.%d.%d", v1, v2, v3, v4);
						if (hr == S_OK) 
							::MessageBox(hWnd, pszDest, L"GetVersion", MB_OK);
						else
							::MessageBox(hWnd, L"Windows 10 or later", L"GetVersion", MB_OK);
					}
					else
					
						::MessageBox(hWnd, L"Version: Windows 8 or later", L"GetVersion", MB_OK);

				}
				else if (IsWindows7OrGreater())
				{
					::MessageBox(hWnd, L"Version: Windows 7 or later", L"GetVersion", MB_OK);

				}
				else if (IsWindowsVistaOrGreater())
				{
					::MessageBox(hWnd, L"Version: Windows Vista or later", L"GetVersion", MB_OK);

				}
				else if (IsWindowsXPOrGreater())
				{
					::MessageBox(hWnd, L"Version: Windows XP or later", L"GetVersion", MB_OK);

				}
				else if (IsWindows2KOrGreater())
				{
					::MessageBox(hWnd, L"Version: Windows 2K or later", L"GetVersion", MB_OK);

				}
				else if (IsWindowsNTOrGreater())
				{
					::MessageBox(hWnd, L"Version: Windows NT4 or later", L"GetVersion", MB_OK);

				}
				else 
				{
					::MessageBox(hWnd, L"Version: below Windows NT4 ", L"GetVersion", MB_OK);
				}
				break;
			case ID_TESTDESKTOPBRIDGE_TILE:
				break;
			case ID_TESTDESKTOPBRIDGE_TOAST:
				ShowUWPToast(_wfullpath(nullptr, L"uwp-logo.png", MAX_PATH),L"Title Content",L"Message Content",L"TestWin32DesktopBridge.Toast");
				break;
			case ID_TESTDESKTOPBRIDGE_CORTANA:
				break;
			case ID_TESTDESKTOPBRIDGE_RUNNINGINACONTAINER:
				if (IsRunningWithinCentennial())
				{
					::MessageBox(hWnd, L"The Application is running within a Desktop Bridge container", L"GetVersion", MB_OK);
				}
				else
				{
					::MessageBox(hWnd, L"The Application is not running within a Desktop Bridge container", L"GetVersion", MB_OK);
				}

				break;
			default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
