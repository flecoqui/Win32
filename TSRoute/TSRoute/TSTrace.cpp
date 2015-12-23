// TSTrace.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "tstrace.h"


#define BUFFER_TRACE_LENGTH 4096
#define MAX_LINE_LENGTH    	    300

TSTrace::TSTrace(LPCTSTR pFile, DWORD dwMaxSize)
{
	m_FilePath = pFile;
	m_dwFileSize = dwMaxSize;
}

DWORD TSTrace::TraceWrite(LPCTSTR pTrace)
{
	char BufferTrace[BUFFER_TRACE_LENGTH];
	char BufferTraceInt[BUFFER_TRACE_LENGTH];


	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);

		StringCchPrintf(BufferTrace, sizeof(BufferTrace),"%02d/%02d/%04d %02d:%02d:%02d %03d : %s\r\n",
		sysTime.wDay, sysTime.wMonth, sysTime.wYear, 
		sysTime.wHour,sysTime.wMinute,sysTime.wSecond,
		sysTime.wMilliseconds,
		pTrace);        
	
	
	int Len = lstrlen(BufferTrace);

	int lastlineFeed = 0;                                     
	int i = 0;
	while ((BufferTrace[i]!='\0') && (i <BUFFER_TRACE_LENGTH))
	{       
		switch (BufferTrace[i])
		{
    		case '\n':          
    		{
    			int delta = lstrlen(BufferTrace)-i-1;
    			if(delta>0)
    			{
    				StringCchCopy(BufferTraceInt, sizeof(BufferTraceInt), BufferTrace+i );
    				BufferTrace[i]='\r';
    				StringCchCopy(BufferTrace+i+1, sizeof(BufferTrace)-i-1 ,BufferTraceInt );
    				i++;
    			}
    			else
    			{
    				BufferTrace[i++]='\r';
    				BufferTrace[i]='\n';
    				BufferTrace[i+1]='\0';
    			}
    			lastlineFeed = i;
    		}
			break;	    		
			default:
			if((i - lastlineFeed) > MAX_LINE_LENGTH)
			{
    			int delta = lstrlen(BufferTrace)-i-1;
    			if(delta>0)
    			{
    				StringCchCopy(BufferTraceInt, sizeof(BufferTraceInt),BufferTrace+i );
    				BufferTrace[i++]='\r';
    				BufferTrace[i]='\n';
	    			lastlineFeed = i;
    				StringCchCopy(BufferTrace+i+1,  sizeof(BufferTrace)-i-1 ,BufferTraceInt );
    			}
			
			}
			break;	    		
		}                   
		i++;
	}
				

	BOOL bOpen = TRUE;
	
	// attempt file creation
	DWORD dwAccess = GENERIC_READ|GENERIC_WRITE;
	DWORD dwShareMode = FILE_SHARE_READ;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = 0;
	DWORD dwCreateFlag = OPEN_EXISTING;

	HANDLE hFile = ::CreateFile((LPCTSTR)m_FilePath.c_str(), 
		dwAccess, 
		dwShareMode, 
		&sa,
		dwCreateFlag, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		dwCreateFlag = CREATE_ALWAYS;

		hFile = ::CreateFile((LPCTSTR)m_FilePath.c_str(), 
							dwAccess, 
							dwShareMode, 
							&sa,
							dwCreateFlag, FILE_ATTRIBUTE_NORMAL, NULL);					
		if (hFile == INVALID_HANDLE_VALUE)
		{
			bOpen = FALSE;
		}
	}
	if(bOpen == TRUE)
	{
		DWORD dwNew = ::SetFilePointer((HANDLE)hFile, 0, NULL,  FILE_END );
		if(dwNew > (DWORD)m_dwFileSize)
		{
			TCHAR NewName[MAX_PATH];

			StringCchCopy(NewName, sizeof(NewName), (LPCTSTR)m_FilePath.c_str());
			int pos = - 1;
			int j = lstrlen(NewName);
			while ( j  > 0)
			{
				j--;
				if(NewName[j] == '.')
				{
					pos = j;
					break;
				}
			}
			if(pos == -1) 
				StringCchCat(NewName,sizeof(NewName),".old");
			if(pos >= (lstrlen(NewName)-4))
			{
				StringCchCopy(&NewName[pos+1],sizeof(NewName)-pos-1,"old");
			}
			CloseHandle(hFile);                
			DWORD dwAttribute = ::GetFileAttributes(NewName);
			if(dwAttribute != 0xFFFFFFFF)
				::DeleteFile(NewName);
			MoveFile((LPTSTR)m_FilePath.c_str(), (LPTSTR)NewName);

			dwCreateFlag = CREATE_ALWAYS;

			hFile = ::CreateFile((LPCTSTR)m_FilePath.c_str(), 
								dwAccess, 
								dwShareMode, 
								&sa,
								dwCreateFlag, FILE_ATTRIBUTE_NORMAL, NULL);					
			if (hFile == INVALID_HANDLE_VALUE)
			{
				return TRACE_ERROR;
			}
		}

		DWORD nWritten;
		::WriteFile((HANDLE)hFile, BufferTrace, lstrlen(BufferTrace), &nWritten, NULL);
		CloseHandle(hFile);
	}
	return TRACE_SUCCESS;
}

DWORD TSTrace::TraceHex(LPTSTR pTrace, DWORD TraceLen,BYTE *Buffer, DWORD Count)
{                    
	DWORD i;
	TCHAR Loc[30];
	TCHAR Line[30];
	LPTSTR pLine;
	StringCchCopy(Line,sizeof(Line),"\t -  ");
	pLine = Line;
	StringCchCopy(pTrace,TraceLen, "");
	
	for(i = 0; i < Count; i++ )
	{            
		StringCchPrintf(Loc,sizeof(Loc),"\t%02X", *(Buffer+i));
		StringCchCat(pTrace, TraceLen, Loc);
		if((*(Buffer+i)>0x20) && (*(Buffer+i)<0x80))
			StringCchPrintf(Loc,sizeof(Loc),"%c",*(Buffer+i));
		else
			StringCchPrintf(Loc,sizeof(Loc),".",*(Buffer+i));
		StringCchCat(pLine,sizeof(Line), Loc);
		if(i%16 == 15)
		{
			StringCchCat(pTrace,TraceLen, Line);
			StringCchCat(pTrace,TraceLen, "\n");
			StringCchCopy(Line,sizeof(Line),"\t -  ");
			if((DWORD)lstrlen(pTrace)>(DWORD)(TraceLen-69))
			{
				return TRACE_ERROR;
			}
		}
	}
	if(i%16 != 0)
	{
		StringCchCat(pTrace,TraceLen,"\t  ");
		for(; (i % 16) != 15 ; i++ )
		{
			StringCchCat(pTrace,TraceLen, "\t  ");
		}
		StringCchCat(pTrace,TraceLen, Line);
		StringCchCat(pTrace,TraceLen,"\n");
	}
//	StringCchCat(pTrace, "\n");
//	DWORD LEn = lstrlen(pTrace);
//	OutputDebugString(pTrace);
	return TRACE_SUCCESS;
}
