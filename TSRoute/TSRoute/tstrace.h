#pragma once
#define TRACE_ERROR   0
#define TRACE_SUCCESS 1
class TSTrace
{
public:
	TSTrace(LPCTSTR pFile, DWORD dwMaxSize);
	DWORD TraceWrite(LPCTSTR pTrace);
	DWORD TraceHex(LPTSTR pTrace, DWORD TraceLen,BYTE *Buffer, DWORD Count);
private:
	string     m_FilePath;
	DWORD      m_dwFileSize;

};