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
// TSRoute.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "tsroute.h"
#include "tsxml.h"
HRESULT GetStringValue(CComPtr<IXMLDOMNode> spXMLNode, CComBSTR& bstr, CComVariant* pvarValue)
{
	CComPtr<IXMLDOMNode> spXMLLocalNode;
	HRESULT hr = spXMLNode->selectSingleNode(bstr, &spXMLLocalNode);
	if ( FAILED(hr) ) return S_FALSE;
	if ( spXMLLocalNode.p == NULL ) return S_FALSE;
	
	pvarValue->Clear();
	pvarValue->ChangeType(VT_EMPTY);
	hr = spXMLLocalNode->get_nodeTypedValue(pvarValue);
	if ( FAILED(hr) ) return S_FALSE;
	if ( pvarValue->vt != VT_BSTR ) return S_FALSE;
	return S_OK;
}
int GetArgCount(TCHAR* p)
{
	bool bFound = false;
	int argc = 0;

	for(unsigned int i = 0; i < strlen(p); i++)
	{
		if(isspace(p[i])) 
		{
			bFound = false;
		}
		else
		{
			if(bFound == false)
			{
				argc++;
				bFound = true;
			}
		}
	}
	return argc;
}
int FeedArgv(TCHAR* p, int argc, _TCHAR** argv)
{
	int a = 0;
	if(argv)
	{
		argv[a++] = "TSROUTE";
		bool bFound = false;
		for(unsigned int i = 0; (i < strlen(p)) && (a<argc+1); i++)
		{
			if(isspace(p[i])) 
			{
				p[i] = '\0';
				bFound = false;
			}
			else
			{
				if(bFound == false)
				{
					argv[a++] = &p[i];
					bFound = true;
				}
			}
		}
	}
	return a;
}
int SetString(CComPtr<IXMLDOMDocument> spXMLDOM,CComPtr<IXMLDOMNode> spXMLNode,LPCWSTR FieldName, LPCTSTR Value )
{
	HRESULT hr;
	CComPtr<IXMLDOMNode> spInsertedNode;
	CComBSTR bstr(FieldName);
	hr = spXMLNode->selectSingleNode(bstr, &spInsertedNode);
	if (( FAILED(hr) ) || ( spInsertedNode.p == NULL ))
	{
		CComPtr<IXMLDOMNode> spXMLChildNode;
		hr = spXMLDOM->createNode(CComVariant(NODE_ELEMENT),CComBSTR(FieldName),NULL,&spXMLChildNode);
		if ( FAILED(hr) ) return 0;
		if ( spXMLChildNode.p == NULL ) return 0;

		hr = spXMLNode->appendChild(spXMLChildNode,&spInsertedNode);
		if ( FAILED(hr) ) return 0;
		if ( spInsertedNode.p == NULL ) return 0;
	}

	// Add the attribute. Note we do this through the IXMLDOMElement
	// interface, so we need to do the QI().
	CComVariant varValue = Value;
	hr = spInsertedNode->put_nodeTypedValue(varValue);
	if ( FAILED(hr) ) return 0;
	return 1;
}
int SetDWORD(CComPtr<IXMLDOMDocument> spXMLDOM,CComPtr<IXMLDOMNode> spXMLNode,LPCWSTR FieldName, DWORD Value )
{
	ostringstream oss;
	oss << Value ;
	return SetString(spXMLDOM,spXMLNode,FieldName, oss.str().c_str());
}
int SetINT64(CComPtr<IXMLDOMDocument> spXMLDOM,CComPtr<IXMLDOMNode> spXMLNode,LPCWSTR FieldName, __int64 Value )
{
	ostringstream oss;
	oss << Value ;
	return SetString(spXMLDOM,spXMLNode,FieldName, oss.str().c_str());
}
int SetDOUBLE(CComPtr<IXMLDOMDocument> spXMLDOM,CComPtr<IXMLDOMNode> spXMLNode,LPCWSTR FieldName, double Value )
{
	ostringstream oss;
	oss << Value ;
	return SetString(spXMLDOM,spXMLNode,FieldName, oss.str().c_str());
}

int GetString(CComPtr<IXMLDOMNode> XMLNode,LPCWSTR FieldName, string& value, LPCTSTR DefaultValue = NULL  )
{
	CComVariant varValue(VT_EMPTY);
	CComBSTR bstr(FieldName);
	USES_CONVERSION;
	if(DefaultValue!=NULL) 
		value = DefaultValue;
	else
		value = "";
	HRESULT hr = GetStringValue(XMLNode,bstr,&varValue);
	if ( hr != S_OK)
	{
		return 0;
	}
	value = W2T(varValue.bstrVal);
	return 1;
}
int GetDWORD(CComPtr<IXMLDOMNode> XMLNode, LPCWSTR FieldName, DWORD& value,DWORD defaultvalue = 0)
{
	CComVariant varValue(VT_EMPTY);
	CComBSTR bstr(FieldName);
	USES_CONVERSION;
	value = defaultvalue;
	HRESULT hr = GetStringValue(XMLNode,bstr,&varValue);
	if ( hr != S_OK)
	{
		return 0;
	}
	TCHAR* stop = "\0";
	value = strtoul(W2T(varValue.bstrVal),&stop,10);
	return 1;
}
int ParseCommandLinesInXmlFile(LPCTSTR xmlfile,GLOBAL_PARAMETER* pGParam, STREAM_PARAMETER* pParam, LPDWORD lpdwParamLen, string& error_string)
{
	try {
			// Start COM
			CoInitialize(NULL);

			// Create an instance of the parser
			CComPtr<IXMLDOMDocument> spXMLDOM;
			HRESULT hr = spXMLDOM.CoCreateInstance(__uuidof(DOMDocument));
			if ( FAILED(hr) ) throw "Unable to create XML parser object";
			if ( spXMLDOM.p == NULL ) throw "Unable to create XML parser object";

			// Load the XML document file...
			VARIANT_BOOL bSuccess = false;
	
			// Test if file exist to avoid exception
			ifstream ifs(xmlfile);
			if(!ifs.is_open())
				return 0;

			hr = spXMLDOM->load(CComVariant(xmlfile),&bSuccess);
			if ( FAILED(hr) ) return 0;
			if ( !bSuccess ) return 0;

			DWORD i = 0;
			char strDir[1024]; 
			GetProcessDirectory(sizeof(strDir),strDir);

			string s = XML_TSROUTE_INPUT_PARAMETERS;

			CComBSTR bstrSS1(s.c_str());
			CComPtr<IXMLDOMNode> spXMLNode1;
			hr = spXMLDOM->selectSingleNode(bstrSS1,&spXMLNode1);
			if ( FAILED(hr) ) throw "Unable to locate XML node";
			if ( spXMLNode1.p == NULL ) throw "Unable to locate XML node";


			s = strDir;
			s += "\\TSROUTE.LOG";
			string r="";
			GetString(spXMLNode1,XML_TSROUTE_TRACE_FILE,r, s.c_str());
			GetFullPath(pGParam->trace_file, r.c_str());

			GetDWORD(spXMLNode1,XML_TSROUTE_TRACE_MAX_SIZE,pGParam->trace_max_size, 300000);
			string v;
			GetString(spXMLNode1,XML_TSROUTE_TRACE_LEVEL,v, "information");
			if(GetTraceLevel(v.c_str(), pGParam->trace_level)==false)
				pGParam->trace_level = Information;
			GetString(spXMLNode1,XML_TSROUTE_CONSOLE_TRACE_LEVEL,v, "information");
			if(GetTraceLevel(v.c_str(), pGParam->console_trace_level)==false)
				pGParam->console_trace_level = Information;

			DWORD dwSync = 0;
			GetDWORD(spXMLNode1,XML_TSROUTE_SYNC_TRANSMISSION,dwSync, 0);
			if(dwSync)
				pGParam->bSyncTransmission = true;
			else
				pGParam->bSyncTransmission = false;

			s = XML_TSROUTE_INPUT_PARAMETERS;
			s += "/";
			s += XML_TSROUTE_STREAM;

			CComBSTR bstrSS(s.c_str());
			CComPtr<IXMLDOMNode> spXMLNode;
			hr = spXMLDOM->selectSingleNode(bstrSS,&spXMLNode);
			if ( FAILED(hr) ) throw "Unable to locate XML node";
			if ( spXMLNode.p == NULL ) throw "Unable to locate XML node";

			
			while ((spXMLNode.p != NULL) && (i < *lpdwParamLen))
			{
				// Pull the enclosed text and display
				CComVariant varValue(VT_EMPTY);

				USES_CONVERSION;

				// Get Name Value
				ostringstream oss;
				string sname; 
				string s; 

				if( (GetString(spXMLNode,XML_TSROUTE_STREAM_TS_FILE,r)) &&
					(r.length()>0))
				{
					GetFullPath(pParam[i].ts_file, r.c_str());

					oss << "Stream" << i+1 ;
					sname = oss.str().c_str();
					GetString(spXMLNode,XML_TSROUTE_STREAM_NAME,pParam[i].name,sname.c_str());
					
					s = strDir;
					s += "\\";
					s += sname.c_str();
					s += ".xml";
					GetString(spXMLNode,XML_TSROUTE_STREAM_OUTPUT_FILE,r,s.c_str());
					GetFullPath(pParam[i].output_file, r.c_str());
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_REFRESH_PERIOD,pParam[i].refresh_period, 5);
					



					GetString(spXMLNode,XML_TSROUTE_STREAM_UDP_IP_ADDRESS,pParam[i].udp_ip_address);
					GetString(spXMLNode,XML_TSROUTE_STREAM_UDP_IP_ADDRESS_INTERFACE,pParam[i].udp_ip_address_interface);
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_UDP_PORT,pParam[i].udp_port);

					GetString(spXMLNode,XML_TSROUTE_STREAM_INPUT_UDP_IP_ADDRESS,pParam[i].input_udp_ip_address);
					GetString(spXMLNode,XML_TSROUTE_STREAM_INPUT_UDP_IP_ADDRESS_INTERFACE,pParam[i].input_udp_ip_address_interface);
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_INPUT_UDP_PORT,pParam[i].input_udp_port);
					DWORD dw;
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_INPUT_RTP,dw,1);
					if(dw>0)
						pParam[i].inputrtp= true;
					else
						pParam[i].inputrtp= false;


					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_FORCED_BITRATE,pParam[i].forcedbitrate,0);

					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_BUFFER_SIZE,pParam[i].buffersize,4096*1024);

					GetString(spXMLNode,XML_TSROUTE_STREAM_PIP_TS_FILE,r);
					GetFullPath(pParam[i].pip_ts_file, r.c_str());

					GetString(spXMLNode,XML_TSROUTE_STREAM_PIP_UDP_IP_ADDRESS,pParam[i].pip_udp_ip_address);
					GetString(spXMLNode,XML_TSROUTE_STREAM_PIP_UDP_IP_ADDRESS_INTERFACE,pParam[i].pip_udp_ip_address_interface);
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_PIP_UDP_PORT,pParam[i].pip_udp_port);
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_PIP_FORCED_BITRATE,pParam[i].pip_forcedbitrate,0);
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_PIP_BUFFER_SIZE,pParam[i].pip_buffersize,4096*1024);

					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_TTL,pParam[i].ttl,1);
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_LOOP,pParam[i].nloop,-1);

					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_UPDATE_TIMESTAMPS,dw,1);
					if(dw>0)
						pParam[i].bupdatetimestamps= true;
					else
						pParam[i].bupdatetimestamps= false;
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_PACKET_START,pParam[i].packetStart,0);
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_PACKET_END,pParam[i].packetEnd,0xFFFFFFFF);
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_TIME_START,pParam[i].timeStart,0);
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_TIME_END,pParam[i].timeEnd,0xFFFFFFFF);
					// Next Node
					i++;
				}
				else if( (GetString(spXMLNode,XML_TSROUTE_STREAM_INPUT_UDP_IP_ADDRESS,r)) &&
					(r.length()>0))
				{

					oss << "Stream" << i+1 ;
					sname = oss.str().c_str();
					GetString(spXMLNode,XML_TSROUTE_STREAM_NAME,pParam[i].name,sname.c_str());
					
					s = strDir;
					s += "\\";
					s += sname.c_str();
					s += ".xml";
					GetString(spXMLNode,XML_TSROUTE_STREAM_OUTPUT_FILE,r,s.c_str());
					GetFullPath(pParam[i].output_file, r.c_str());
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_REFRESH_PERIOD,pParam[i].refresh_period, 5);
					



					GetString(spXMLNode,XML_TSROUTE_STREAM_UDP_IP_ADDRESS,pParam[i].udp_ip_address);
					GetString(spXMLNode,XML_TSROUTE_STREAM_UDP_IP_ADDRESS_INTERFACE,pParam[i].udp_ip_address_interface);
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_UDP_PORT,pParam[i].udp_port);
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_TTL,pParam[i].ttl,1);

					GetString(spXMLNode,XML_TSROUTE_STREAM_INPUT_UDP_IP_ADDRESS,pParam[i].input_udp_ip_address);
					GetString(spXMLNode,XML_TSROUTE_STREAM_INPUT_UDP_IP_ADDRESS_INTERFACE,pParam[i].input_udp_ip_address_interface);
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_INPUT_UDP_PORT,pParam[i].input_udp_port);
					DWORD dw;
					GetDWORD(spXMLNode,XML_TSROUTE_STREAM_INPUT_RTP,dw,1);
					if(dw>0)
						pParam[i].inputrtp= true;
					else
						pParam[i].inputrtp= false;


					// Next Node
					i++;
				}

				CComPtr<IXMLDOMNode> spXMLNodeNext;
				hr = spXMLNode->get_nextSibling(&spXMLNodeNext);
				if ( FAILED(hr) ) throw "Unable to go to the next XML node";
				if ( spXMLNode.p == NULL ) throw "Unable to locate the next XML node";
				spXMLNode = spXMLNodeNext;
			}
			*lpdwParamLen = i;

	} // try
	catch(...) {
		// Stop COM
		CoUninitialize();
		return 0;
	}
	// Stop COM
	CoUninitialize();
	return 1;
}
bool GetLocalTime(string& s)
{
	TCHAR Buffer[256];
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);

	StringCchPrintf(Buffer, sizeof(Buffer),"%02d/%02d/%04d %02d:%02d:%02d %03d",
	sysTime.wDay, sysTime.wMonth, sysTime.wYear, 
	sysTime.wHour,sysTime.wMinute,sysTime.wSecond,
	sysTime.wMilliseconds);        
	s = Buffer;
	return true;
}
int UpdateResultXmlFile(STREAM_PARAMETER* pParam, string& error_string)
{
	try {
			if(pParam==NULL) return 0;
			if(pParam->output_file.length()==0) return 0;
			// Start COM
			CoInitialize(NULL);

			// Create an instance of the parser
			CComPtr<IXMLDOMDocument> spXMLDOM;
			HRESULT hr = spXMLDOM.CoCreateInstance(__uuidof(DOMDocument));
			if ( FAILED(hr) ) throw "Unable to create XML parser object";
			if ( spXMLDOM.p == NULL ) throw "Unable to create XML parser object";

			// Load the XML document file...
			VARIANT_BOOL bSuccess = false;
	
			// Test if file exist to avoid exception
			ifstream ifs(pParam->output_file.c_str());
			if(!ifs.is_open())
			{
				ofstream ofs(pParam->output_file.c_str());
				ofs << "<" << XML_TSROUTE_COUNTER << ">" << endl;
				ofs << "</" << XML_TSROUTE_COUNTER << ">" << endl;
			}
			hr = spXMLDOM->load(CComVariant(pParam->output_file.c_str()),&bSuccess);
			if ( FAILED(hr) ) return 0;
			if ( !bSuccess ) return 0;

			string s = XML_TSROUTE_COUNTER;

			CComBSTR bstrSS(s.c_str());
			CComPtr<IXMLDOMNode> spXMLNode;
			hr = spXMLDOM->selectSingleNode(bstrSS,&spXMLNode);
			if ( FAILED(hr) ) throw "Unable to locate XML node";
			if ( spXMLNode.p == NULL ) throw "Unable to locate XML node";

			DWORD i = 0;
			
			if(spXMLNode.p != NULL)
			{
				// Pull the enclosed text and display
				CComVariant varValue(VT_EMPTY);

				USES_CONVERSION;

				// Get Name Value
				ostringstream oss;
				string sname; 
				string s; 





				
				
				SetString(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_NAME,pParam->name.c_str());
				if(pParam->ts_file.length()>0)
				{
					SetString(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_FILE,pParam->ts_file.c_str());
					GetLocalTime(s);
					SetString(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_DATE,s.c_str());
					
					SetINT64(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PACKET_TRANSMIT,pParam->Maindata.GlobalCounter);
					SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_DURATION,pParam->Maindata.duration);
					SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_BITRATE,pParam->Maindata.currentbitrate);
					SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_MINBITRATE,pParam->Maindata.minbitrate);
					SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_MAXBITRATE,pParam->Maindata.maxbitrate);
					SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_EXPECTED_DURATION,pParam->Maindata.Counters.expectedduration);
					SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_EXPECTED_BITRATE,pParam->Maindata.Counters.bitrate);
					SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_FIRST_PCR,(double)pParam->Maindata.FirstPCR/27000000);
					SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_LAST_PCR,(double)pParam->Maindata.LastPCR/27000000);
					SetString(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_ERROR,pParam->Maindata.ErrorMessage.c_str());


					if(pParam->pip_ts_file.length())
					{
						SetString(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PIP_FILE,pParam->pip_ts_file.c_str());
						SetINT64(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PIP_PACKET_TRANSMIT,pParam->PIPdata.GlobalCounter);
						SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PIP_DURATION,pParam->PIPdata.duration);
						SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PIP_BITRATE,pParam->PIPdata.currentbitrate);
						SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PIP_MINBITRATE,pParam->PIPdata.minbitrate);
						SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PIP_MAXBITRATE,pParam->PIPdata.maxbitrate);
						SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PIP_EXPECTED_DURATION,pParam->PIPdata.Counters.expectedduration);
						SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PIP_EXPECTED_BITRATE,pParam->PIPdata.Counters.bitrate);
						SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PIP_FIRST_PCR,(double)pParam->PIPdata.FirstPCR/27000000);
						SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PIP_LAST_PCR,(double)pParam->PIPdata.LastPCR/27000000);
						SetString(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PIP_ERROR,pParam->PIPdata.ErrorMessage.c_str());
					}

					hr = spXMLDOM->save(CComVariant(pParam->output_file.c_str()));
				}				
				else if(pParam->input_udp_ip_address.length()>0)
				{
					SetString(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_ADDRESS,pParam->input_udp_ip_address.c_str());
					SetDWORD(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PORT,pParam->input_udp_port);
					SetString(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_INTERFACE,pParam->input_udp_ip_address_interface.c_str());
					GetLocalTime(s);
					SetString(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_DATE,s.c_str());
					
					SetINT64(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_PACKET_TRANSMIT,pParam->Maindata.GlobalCounter);
					SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_DURATION,pParam->Maindata.duration);
					SetDOUBLE(spXMLDOM,spXMLNode,XML_TSROUTE_COUNTER_BITRATE,pParam->Maindata.currentbitrate);

					hr = spXMLDOM->save(CComVariant(pParam->output_file.c_str()));
				}				
			}

	} // try
	catch(...) {
		// Stop COM
		CoUninitialize();
		return 0;
	}
	// Stop COM
	CoUninitialize();
	return 1;
}
