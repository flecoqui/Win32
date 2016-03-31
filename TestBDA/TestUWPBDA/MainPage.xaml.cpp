//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "graphwinrt.h"

#include "graph.h"
#include "tsfile.h"
#include "strsafe.h"

using namespace TestUWPBDA;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();
}

CBDAFilterGraph* g_pfg;
void MsgDbg(LPCTSTR  psz_format, ...)
{
	va_list args;
	va_start(args, psz_format);
	int const arraysize = 300;
	TCHAR pszDest[arraysize];
	size_t cbDest = arraysize * sizeof(TCHAR);

	HRESULT hr = StringCchVPrintf(pszDest, cbDest, psz_format, args);
	if (hr == S_OK) {
		OutputDebugString(pszDest);
		OutputDebugString(TEXT("\r\n"));
	}
	va_end(args);
};

DWORD FAR PASCAL RecvProc(LPSTR lpData)
{
	int buffersize = PACKET_SIZE * 7000;
	char buffer[70 * PACKET_SIZE];
	CBDAFilterGraph* p = (CBDAFilterGraph*)lpData;
	if (p == NULL)
		return 0;

	HANDLE thisThread = GetCurrentThread();
	SetThreadPriority(thisThread, THREAD_PRIORITY_TIME_CRITICAL);

	//
	__int64 PCR = -1;
	__int64 FirstPCR = -1;

	double deltamax = -1;
	double deltamin = -1;

	// Load TS file
	TSFile file;
	if (file.Create(TEXT("c:\\temp\\test.ts")))
	{
		int len;
		p->GlobalCounter = 0;

		len = sizeof(buffer);
		while (((len = p->Receive(buffer, len)) >= 0) && (p->bStop == false))
		{
			if (len != 0)
			{
				file.Write(buffer, len);
				p->GlobalCounter += len;
			}
			else
				Sleep(1);
			len = sizeof(buffer);
		}
		file.Close();
	}
	p->bStopped = true;
	return 1;
}

void TestUWPBDA::MainPage::DVB_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	// Initialize COM library
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
	{

		MsgDbg(TEXT("Failed to initialize COM library!\0"));
		return ;
	}
	// Create the BDA filter graph and initialize its components
	g_pfg = new CBDAFilterGraph();
	// If the graph failed to build, don't go any further.
	if (!g_pfg)
	{
		MsgDbg(TEXT("Failed to create the filter graph!"));
		return ;
	}
	if ((g_pfg->SetDVBT(
		546000, -1, -1,
		8, -1, -1, -1
		))<0)
	{
		MsgDbg(TEXT("Could not Build the DVB BDA FilterGraph."));
		return ;
	}
	else
	{
		if (g_pfg->SubmitTuneRequest() == VLC_SUCCESS)
		{

			g_pfg->bStop = FALSE;
			g_pfg->bStopped = FALSE;
			g_pfg->GlobalCounter = 0;
			HANDLE hThread;
			DWORD dwThreadID;
			if (NULL != (hThread =
				CreateThread((LPSECURITY_ATTRIBUTES)NULL,
					0,
					(LPTHREAD_START_ROUTINE)RecvProc,
					(LPVOID)g_pfg,
					0, &dwThreadID)))
			{
			}

		}
	}

	// Release the BDA components and clean up
	delete g_pfg;

	CoUninitialize();
	return ;

}
