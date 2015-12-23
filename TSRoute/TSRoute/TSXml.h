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
int ParseCommandLinesInXmlFile(LPCTSTR xmlfile,GLOBAL_PARAMETER* pGParam,STREAM_PARAMETER* pParam, LPDWORD lpdwParamLen, string& error_string);
int UpdateResultXmlFile(STREAM_PARAMETER* pParam, string& error_string);
