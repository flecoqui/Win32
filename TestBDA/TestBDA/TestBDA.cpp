// TestBDA.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "graph.h"
#include "tsfile.h"
#include "strsafe.h"
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
int main()
{
	// Initialize COM library
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
	{
		
		MsgDbg(TEXT("Failed to initialize COM library!\0"));
		return 0;
	}
	// Create the BDA filter graph and initialize its components
	g_pfg = new CBDAFilterGraph();
	// If the graph failed to build, don't go any further.
	if (!g_pfg)
	{
		MsgDbg(TEXT("Failed to create the filter graph!"));
		return 0;
	}
	if ((g_pfg->SetDVBT(
		546000, -1, -1,
		8, -1, -1, -1
		))<0)
	{
		MsgDbg(TEXT("Could not Build the DVB BDA FilterGraph."));
		return 0;
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
				while (g_pfg->GlobalCounter < 100000000)
					Sleep(1);
				g_pfg->bStop = true;
				while (g_pfg->bStopped == false)
					Sleep(1);

			}

		}
	}

	// Release the BDA components and clean up
	delete g_pfg;

	CoUninitialize();
    return 0;
}

