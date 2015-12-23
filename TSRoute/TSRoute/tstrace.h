//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************
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