

#define MAX_TIME_STAMP 4
#define TIME_STAMP_PCR  0
#define TIME_STAMP_OPCR 1
#define TIME_STAMP_DTS  2
#define TIME_STAMP_PTS  3
#define PACKET_SIZE 188
#define MAX_PID 64

typedef BYTE PACKET[PACKET_SIZE];

typedef struct PIDInfo
{
	WORD PID;
	BYTE Type;
} PID_INFO;
/* PCR Buffer
typedef struct PCRInfo
{
	DWORD Index;
	__int64 PCR;
} PCR_INFO;
*/

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


class TSFile
{
public:
	enum Parse_Mask{
		none = 0,
		pid = 1,
		timestamp = 2,
		ts = 4,
		all = 0xFFFFFFFF
	} ;

	TSFile(void);
	~TSFile(void);
	int Open(const TCHAR* ts_file);
	int Create(const TCHAR* ts_file);
	int Write(char* p, long len);
	int Close();
	__int64 Load(unsigned long buffersize=0);
/* PCR Buffer
	DWORD LoadPCRBuffer(WORD PCR_PID);
	int GetNextPCR(DWORD Index, __int64* lpPCR,DWORD* IndexPCR);
*/
	int GetPacketLength(void);
	DWORD ResetRead(void);
	DWORD Read204(LPBYTE buffer);
	DWORD Read188(LPBYTE buffer);

	PACKET* GetFirstPacket(void);
	PACKET* GetNextPacket(PACKET* pPacket);
	PACKET* GetLastPacket(void);
	PACKET* GetPrevPacket(PACKET* pPacket);

	LPCTSTR GetStreamType(BYTE st);
	int GetProgramMapTableID(PARSE_COUNTERS* pCounter);
	WORD GetPCR_PID(WORD PID,PID_INFO *pPIDInfo = NULL, DWORD dw = 0);
	int GetBitrate(TIME_STAMP_COUNTERS* pCounter, WORD PID, double& duration, double& bitrate, __int64& PacketCount);
	int GetPacketRange(WORD PCR_PID,  __int64& PacketStart,  __int64& PacketEnd);

	int GetPID(PACKET* pPacket, WORD& PID);
//	int GetPacketInformation(Parse_Mask Mask,PACKET* pPacket,string& s);





	int Unload(void);
	int SetLimit(__int64 start, __int64 end);
	bool IsCached(void);

	int GetNextPCR(char* p, __int64* lpPCR,WORD PID = 0xFFFF);
	int GetNextOPCR(char* p, __int64* lpOPCR,WORD PID = 0xFFFF);
	int GetNextPTS(char* p, __int64* lpPTS,WORD PID = 0xFFFF);
	int GetNextDTS(char* p, __int64* lpDTS,WORD PID = 0xFFFF);
	int GetPIDPCRfromPacket(char* p,WORD PID, __int64* lpPCR,bool* pbDiscontinuity);
	int UpdateTimeStamps(char* p,__int64 delta);
	int TestTimeStamps(__int64 delta,unsigned char ContinuityCounter);
	int GetPCRInAdaptationField(LPBYTE Packet, __int64* lpPCR);
	int SetPCRInAdaptationField(LPBYTE Packet, __int64 PCR);
	int GetDTSInAdaptationField(LPBYTE Packet, __int64* lpDTS);
	int SetDTSInAdaptationField(LPBYTE Packet, __int64 DTS);
	int GetOPCRInAdaptationField(LPBYTE Packet, __int64* lpOPCR);
	int SetOPCRInAdaptationField(LPBYTE Packet, __int64 OPCR);
	int GetRAIInAdaptationField(LPBYTE Packet);
	int GetDTSInPES(LPBYTE Packet, __int64* lpDTS);
	int SetDTSInPES(LPBYTE Packet, __int64 DTS);
	int GetPTSInPES(LPBYTE Packet, __int64* lpPTS);
	int SetPTSInPES(LPBYTE Packet, __int64 PTS);
	int IsDiscontinuityIndicator(LPBYTE Packet);
	int DiscontinuityIndicator(LPBYTE Packet);
	int SetDiscontinuityIndicator(LPBYTE Packet);

//	bool CheckContinuityCounter(LPBYTE Packet,__int64 count,string& s);
	int InitPIDInformation();
	BYTE GetLastContinuityCounter(WORD Pid);
	bool SetLastContinuityCounter(WORD Pid,BYTE Counter);
	bool GetLastPCR(WORD Pid,__int64& PCR);
	bool GetLastPCRCounter(WORD Pid,__int64& PCR, __int64& Counter);
	bool GetFirstPCRCounter(WORD Pid,__int64& PCR, __int64& Counter);
	bool SetLastPCR(WORD Pid,__int64 PCR);
	bool SetLastPCRCounter(WORD Pid,__int64 PCR,__int64 Counter);
	bool SetFirstPCRCounter(WORD Pid,__int64 PCR,__int64 Counter);
	__int64 GetLastDTS(WORD Pid);
	bool SetLastDTS(WORD Pid,__int64 DTS);
	__int64 GetLastPTS(WORD Pid);
	bool SetLastPTS(WORD Pid,__int64 PTS);
	bool UpdateContinuityCounter(LPBYTE Packet);


	int GetContinuityCounter(LPBYTE Packet, unsigned char* pCounter );
	int SetContinuityCounter(LPBYTE Packet, unsigned char Counter );


	int AddPIDDiscontinuity(WORD PID);
	int InitPIDDiscontinuity();
	int ResetPIDDiscontinuity();
	int IsPIDDiscontinuitySet(WORD PID);
	int SetPIDDiscontinuity(WORD PID);
	bool AreAllPIDDiscontinuitySet();

protected:
	DWORD Read(__int64 IndexPacket);

private:

	HANDLE  m_file_handle;
	TCHAR  m_file[MAX_PATH];
	// update type to support 4 GB files
	__int64   m_file_length;

	DWORD   m_buffer_size;
	HGLOBAL m_buffer_handle;
	LPBYTE  m_buffer_pointer;
	DWORD   m_buffer_index;
	DWORD   m_buffer_length;
/* PCR Buffer
	DWORD   m_pcr_buffer_size;
	HGLOBAL m_pcr_buffer_handle;
	PCR_INFO*  m_pcr_buffer_pointer;
	DWORD   m_pcr_buffer_index;
	DWORD   m_pcr_buffer_length;
*/
	__int64 m_start;
	__int64 m_end;
	static int GetPIDPCR(LPBYTE Packet, WORD PID, __int64* lpPCR,bool* pbDiscontinuity=NULL);
	static int GetPIDPCR(LPBYTE Packet, LPWORD lpPID, __int64* lpPCR,bool* pbDiscontinuity=NULL);
	unsigned long m_limit_start;
	unsigned long m_limit_end;
	unsigned short m_PCR_PID;
	PID_COUNTER m_arrayPID[MAX_PID];
	// April 6th 
	// Add continuity counter struc
	PID_INFORMATION m_arrayPIDInformation[MAX_PID];
	PCR_INFORMATION m_arrayPCRInformation[MAX_PID];
	PTS_INFORMATION m_arrayPTSInformation[MAX_PID];
	DTS_INFORMATION m_arrayDTSInformation[MAX_PID];

};
