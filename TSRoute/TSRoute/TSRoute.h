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
#include "TSTrace.h"

#pragma once
#define SZSERVICENAME "TSROUTE"
#define MAX_PID 64

enum VerboseLevel
{
	None = 0,
	Information,
	TSPID,
	TSTimeStamp,
	TSPacket
};

enum Action
{
	Parse = 0,
	Stream,
	Receive,
	Check,
	Install,
	Uninstall,
	Start,
	Stop,
	Convert,
	Filter,
	MRoute,
	NoAction
};
enum Mode
{
	Command = 0,
	Service
};


	typedef struct Counter_Info 
	{
		double duration;
		double bitrate;
		WORD PID;
		WORD program_map_pid;
		WORD PCR_PID;
		WORD program_number;

		DWORD TimeStampPacketCount;

		__int64 IndexFirstTimeStampPacket;
		__int64 IndexLastTimeStampPacket;

		__int64 FirstTimeStamp;
		__int64 LastTimeStamp;

		__int64 delta;

		DWORD MinPacketBetweenTimeStamp;
		DWORD MaxPacketBetweenTimeStamp;

		DWORD   TimeStampDiscontinuity;
		DWORD   Discontinuity;
		DWORD   DiscontinuityIndicator;
		DWORD   TimeStampFixed;

		DWORD MinTimeStampBitrate;
		DWORD MaxTimeStampBitrate;
	} TIME_STAMP_COUNTERS;


	typedef struct Parse_Info 
	{
		double duration;
		double bitrate;
		double expectedduration;

		bool bError;
		__int64 PacketCount;
		__int64 Delta;
		int Count;
		TIME_STAMP_COUNTERS Counters[MAX_PID];
} PARSE_COUNTERS;

	typedef struct 
	{
		WORD  pid;
		DWORD counter;
		BOOL  bSetDicontinuityCounter;
	} PID_COUNTER;

	// April 6th 
	// Add continuity counter struc
	typedef struct 
	{
		WORD  pid;
		BYTE ContinuityCounter;
	} PID_INFORMATION;
	typedef struct 
	{
		WORD  pid;
		__int64 PCR;
		__int64 Counter;
		__int64 FirstPCR;
		__int64 FirstCounter;
	} PCR_INFORMATION;
	typedef struct 
	{
		WORD  pid;
		__int64 DTS;
	} DTS_INFORMATION;
	typedef struct 
	{
		WORD  pid;
		__int64 PTS;
	} PTS_INFORMATION;

typedef struct RecvStruct{
const char* recv_ts_file;
const char* udp_ip_address;
const char* udp_ip_address_interface;
int udp_port;
__int64 GlobalCounter;
bool bStop;
bool bStopped;
unsigned long  refresh_period;
bool bError;
string ErrorMessage;
} RECV_STR;

typedef struct RouteStruct{
const char* udp_ip_address;
const char* udp_ip_address_interface;
int udp_port;
bool rtp;
int ttl;
const char* input_udp_ip_address;
const char* input_udp_ip_address_interface;
int input_udp_port;
bool input_rtp;
__int64 GlobalCounter;
bool bStop;
bool bStopped;
unsigned long  refresh_period;
} ROUTE_STR;

typedef struct CheckStruct{
const char* recv_ts_file;
const char* udp_ip_address;
const char* udp_ip_address_interface;
int udp_port;
__int64 GlobalCounter;
bool bStop;
bool bStopped;
unsigned long  refresh_period;
long ContinuityErrorCounter;
long PCRAccuracyErrorCounter;
long UnderrunErrorCounter;
__int64 packetCounter;
__int64 bitrate;
} CHECK_STR;

typedef struct SendStruct{
bool bPIP;
unsigned long loopCounter;
struct SendStruct* pMainStream;
const char* send_ts_file;
const char* udp_ip_address;
const char* udp_ip_address_interface;
int udp_port;
int ttl;
const char* input_udp_ip_address;
const char* input_udp_ip_address_interface;
int input_udp_port;
bool input_rtp;
unsigned long buffersize;


PARSE_COUNTERS Counters;
__int64 packetStart;
__int64 packetEnd;
unsigned long forcedbitrate;
bool bupdatetimestamp;
long timeStart;
long timeEnd;
int nloop;

double currentbitrate;
double expectedbitrate;
double averagebitrate;
double maxbitrate;
double minbitrate;
double duration;
__int64 GlobalCounter;
__int64 LastGlobalCounter;
DWORD LocalCounter;
bool bStart;
bool bReadytoStart;
bool bStop;
bool bStopped;
bool bError;
string ErrorMessage;

__int64 FirstPCR;
__int64 LastPCR;
double interval;

HANDLE hThread;
DWORD dwThreadID;
double lastduration;
unsigned long  refresh_period;
} SEND_STR;

typedef struct GlobalParameterStruct{
Mode           mode;
Action         action;
VerboseLevel   console_trace_level;
VerboseLevel   trace_level;
string         trace_file;
unsigned long  trace_max_size;
TSTrace*       pLog;
bool bSyncTransmission;
} GLOBAL_PARAMETER;
typedef struct StreamParameterStruct{
string    name;
string    xml_file;
string    ts_file;
string    udp_ip_address;
string    udp_ip_address_interface;
unsigned long  udp_port;
bool      input_rtp;
string    input_udp_ip_address;
string    input_udp_ip_address_interface;
unsigned long  input_udp_port;
bool      inputrtp;
unsigned long  buffersize;
unsigned long  forcedbitrate;
unsigned long  ttl;
unsigned long  nloop;
bool           bupdatetimestamps;
int            pid;
unsigned long  packetStart;
unsigned long  packetEnd;
unsigned long  timeStart;
unsigned long  timeEnd;

string    pip_ts_file;
string    pip_udp_ip_address;
string    pip_udp_ip_address_interface;
unsigned long  pip_udp_port;
unsigned long  pip_buffersize;
unsigned long  pip_forcedbitrate;

string         output_file;
unsigned long  refresh_period;

SEND_STR Maindata;
SEND_STR PIPdata;
string input_file;
string output_188_file;
WORD program_number;
} STREAM_PARAMETER;

#define XML_TSROUTE_INPUT_PARAMETERS       "TSROUTE.InputParameters"

#define XML_TSROUTE_TRACE_FILE              L"TraceFile"
#define XML_TSROUTE_TRACE_MAX_SIZE          L"TraceMaxSize"
#define XML_TSROUTE_TRACE_LEVEL             L"TraceLevel"
#define XML_TSROUTE_CONSOLE_TRACE_LEVEL     L"ConsoleTraceLevel"
#define XML_TSROUTE_SYNC_TRANSMISSION       L"Sync"


#define XML_TSROUTE_STREAM                 "Stream"
#define XML_TSROUTE_STREAM_NAME            L"Name"
#define XML_TSROUTE_STREAM_OUTPUT_FILE     L"OutputFile"
#define XML_TSROUTE_STREAM_REFRESH_PERIOD  L"RefreshPeriod"

#define XML_TSROUTE_STREAM_TS_FILE                   L"TSFile"
#define XML_TSROUTE_STREAM_UDP_IP_ADDRESS            L"UdpIpAddress"
#define XML_TSROUTE_STREAM_UDP_IP_ADDRESS_INTERFACE  L"UdpIpAddressInterface"
#define XML_TSROUTE_STREAM_UDP_PORT                  L"UdpPort"
#define XML_TSROUTE_STREAM_FORCED_BITRATE            L"ForcedBitrate"
#define XML_TSROUTE_STREAM_BUFFER_SIZE               L"BufferSize"

#define XML_TSROUTE_STREAM_PIP_TS_FILE                   L"PIPTSFile"
#define XML_TSROUTE_STREAM_PIP_UDP_IP_ADDRESS            L"PIPUdpIpAddress"
#define XML_TSROUTE_STREAM_PIP_UDP_IP_ADDRESS_INTERFACE  L"PIPUdpIpAddressInterface"
#define XML_TSROUTE_STREAM_PIP_UDP_PORT                  L"PIPUdpPort"
#define XML_TSROUTE_STREAM_PIP_FORCED_BITRATE            L"PIPForcedBitrate"
#define XML_TSROUTE_STREAM_PIP_BUFFER_SIZE               L"PIPBufferSize"

#define XML_TSROUTE_STREAM_TTL                       L"TTL" 
#define XML_TSROUTE_STREAM_LOOP                      L"Loop"
#define XML_TSROUTE_STREAM_UPDATE_TIMESTAMPS         L"UpdateTimeStamp"
#define XML_TSROUTE_STREAM_PACKET_START              L"PacketStart" 
#define XML_TSROUTE_STREAM_PACKET_END                L"PacketEnd" 
#define XML_TSROUTE_STREAM_TIME_START                L"TimeStart" 
#define XML_TSROUTE_STREAM_TIME_END                  L"TimeEnd" 


#define XML_TSROUTE_STREAM_INPUT_UDP_IP_ADDRESS            L"InputUdpIpAddress"
#define XML_TSROUTE_STREAM_INPUT_UDP_IP_ADDRESS_INTERFACE  L"InputUdpIpAddressInterface"
#define XML_TSROUTE_STREAM_INPUT_UDP_PORT                  L"InputUdpPort"
#define XML_TSROUTE_STREAM_INPUT_RTP                       L"InputRTP" 

#define XML_TSROUTE_COUNTER       "TSROUTE.Counters"

#define XML_TSROUTE_COUNTER_NAME                     L"Name"
#define XML_TSROUTE_COUNTER_FILE                     L"File"
#define XML_TSROUTE_COUNTER_DATE                     L"Date"
#define XML_TSROUTE_COUNTER_LOOP                     L"Loop"
#define XML_TSROUTE_COUNTER_FIRST_PCR                L"FirstPCR"
#define XML_TSROUTE_COUNTER_LAST_PCR                 L"LastPCR"
#define XML_TSROUTE_COUNTER_DURATION                 L"Duration"
#define XML_TSROUTE_COUNTER_BITRATE                  L"Bitrate"
#define XML_TSROUTE_COUNTER_MINBITRATE               L"MINBitrate"
#define XML_TSROUTE_COUNTER_MAXBITRATE               L"MAXBitrate"
#define XML_TSROUTE_COUNTER_PACKET_TRANSMIT          L"PacketTransmit"
#define XML_TSROUTE_COUNTER_EXPECTED_DURATION        L"ExpectedDuration"
#define XML_TSROUTE_COUNTER_EXPECTED_BITRATE         L"ExpectedBitrate"
#define XML_TSROUTE_COUNTER_ERROR					L"LastErrorMessage"

#define XML_TSROUTE_COUNTER_PIP_FILE                     L"PIPFile"
#define XML_TSROUTE_COUNTER_PIP_FIRST_PCR                L"PIPFirstPCR"
#define XML_TSROUTE_COUNTER_PIP_LAST_PCR                 L"PIPLastPCR"
#define XML_TSROUTE_COUNTER_PIP_DURATION                 L"PIPDuration"
#define XML_TSROUTE_COUNTER_PIP_BITRATE                  L"PIPBitrate"
#define XML_TSROUTE_COUNTER_PIP_MINBITRATE               L"MINPIPBitrate"
#define XML_TSROUTE_COUNTER_PIP_MAXBITRATE               L"MAXPIPBitrate"
#define XML_TSROUTE_COUNTER_PIP_PACKET_TRANSMIT          L"PIPPacketTransmit"
#define XML_TSROUTE_COUNTER_PIP_EXPECTED_DURATION        L"PIPExpectedDuration"
#define XML_TSROUTE_COUNTER_PIP_EXPECTED_BITRATE         L"PIPExpectedBitrate"
#define XML_TSROUTE_COUNTER_PIP_ERROR					L"PIPLastErrorMessage"
#define XML_TSROUTE_COUNTER_ADDRESS						L"InputAddress"
#define XML_TSROUTE_COUNTER_PORT							L"InputUdpPort"
#define XML_TSROUTE_COUNTER_INTERFACE					L"InputInterface"

int ParseCommandLine(int argc, _TCHAR* argv[],GLOBAL_PARAMETER* pGlobal,STREAM_PARAMETER* pParam, LPDWORD lpdwParamLen,  string& error_string);
int GetProcessDirectory(int Size, LPTSTR Path);
bool GetTraceLevel(LPCTSTR lp, VerboseLevel& vb);
int GetFullPath(string& s,LPCTSTR path);


