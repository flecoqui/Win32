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
#include "stdafx.h"
#include "TSRoute.h"
#include "TSFile.h"


TSFile::TSFile(void)
{
	m_buffer_handle = NULL;
	m_buffer_pointer = NULL;
    m_file = "";
	m_file_length = 0;
	m_start = 0;
	m_end = -1;
	m_limit_start = 0;
	m_limit_end = -1;
	m_buffer_index = 0;
	m_buffer_length = 0;
	m_PCR_PID = 0xFFFF;


}
TSFile::~TSFile(void)
{
	Unload();
}
int TSFile::Open(const char* ts_file)
{
	m_file_handle = CreateFile(ts_file,           // open MYFILE.TXT 
					GENERIC_READ,              // open for reading 
					FILE_SHARE_READ,           // share for reading 
					NULL,                      // no security 
					OPEN_EXISTING,             // existing file only 
					FILE_ATTRIBUTE_NORMAL,     // normal file 
					NULL);                     // no attr. template 
	 
	if (m_file_handle != INVALID_HANDLE_VALUE)
	{
		m_file = ts_file;
		m_buffer_length = 0;
		m_buffer_index = 0xFFFFFFFF;
		return 1;
	}
	return 0;
}
int TSFile::Create(const char* ts_file)
{
	m_file_handle = CreateFile(ts_file,           // open MYFILE.TXT 
							GENERIC_WRITE,                // open for writing 
							0,                            // do not share 
							NULL,                         // no security 
							CREATE_ALWAYS,                // overwrite existing 
							FILE_ATTRIBUTE_NORMAL,        // normal file 
							NULL);      	 
	if (m_file_handle != INVALID_HANDLE_VALUE)
	{
		m_file = ts_file;
		return 1;
	}
	return 0;
}
int TSFile::Write(char* pMsg, long Len)
{
	if((pMsg) && (Len))
	{
		if(m_file_handle!=NULL)
		{
			DWORD nBytesWritten = 0;

			// Attempt a synchronous write operation. 
			BOOL bResult = WriteFile(m_file_handle, pMsg, Len, &nBytesWritten, NULL) ; 
			if((bResult) && (Len == nBytesWritten))
			{ 
				return 1;
			} 
			else
			{
				DWORD dwErrorCode = GetLastError();
			}
		}
	}
	return 0;
}
int TSFile::Close(void)
{
	if(m_file_handle)
	{
        CloseHandle(m_file_handle);
		m_file_handle = NULL;
		m_file = "";
		return 1;
	}
	return 0;
}
DWORD TSFile::ResetRead(void)
{	
	DWORD dwPtr = SetFilePointer(m_file_handle,0,NULL,FILE_BEGIN);
	if (dwPtr == INVALID_SET_FILE_POINTER) // Test for failure
	{ 
		// Obtain the error code. 
		DWORD dwError = GetLastError() ; 
		return 0;	 
	} // End of error handler 
	return 1;
}

DWORD TSFile::Read204(LPBYTE buffer)
{	
	DWORD nBytesRead = 0;
	DWORD dwError = 0;
	DWORD dwLen = 204;
	if(buffer)
	{
		BOOL bResult = ReadFile(m_file_handle, buffer, dwLen, &nBytesRead, NULL) ; 
		// Check for end of file. 
		if(bResult)
		{ 

			if(nBytesRead!=204)
			{
				return 0;
			}
			return 1;
		} 
	}
	return 0;
}
DWORD TSFile::Read188(LPBYTE buffer)
{	
	DWORD nBytesRead = 0;
	DWORD dwError = 0;
	DWORD dwLen = 188;
	if(buffer)
	{
		BOOL bResult = ReadFile(m_file_handle, buffer, dwLen, &nBytesRead, NULL) ; 
		// Check for end of file. 
		if(bResult)
		{ 

			if(nBytesRead!=188)
			{
				return 0;
			}
			return 1;
		} 
	}
	return 0;
}
/*
DWORD TSFile::Read(__int64 IndexPacket)
{	
	DWORD nBytesRead = 0;
	DWORD dwError = 0;
	DWORD dwLen = m_buffer_size;

	// if already loaded
	if(IsCached() == true)
	{
		if((IndexPacket == m_buffer_index)&& (m_buffer_length > 0))
			return m_buffer_length;
	}
	
	DWORD dwPtr = SetFilePointer(m_file_handle,(LONG) (IndexPacket*PACKET_SIZE),NULL,FILE_BEGIN);
	if (dwPtr == INVALID_SET_FILE_POINTER) // Test for failure
	{ 
		// Obtain the error code. 
		dwError = GetLastError() ; 
		return 0;	 
	} // End of error handler 


	BOOL bResult = ReadFile(m_file_handle, m_buffer_pointer, dwLen, &nBytesRead, NULL) ; 
	// Check for end of file. 
	if((bResult)&& (nBytesRead%PACKET_SIZE==0)&& (((int)(nBytesRead/PACKET_SIZE))>=m_step) )
	{ 

		if(nBytesRead!=m_buffer_size)
		{
			nBytesRead = ((nBytesRead/PACKET_SIZE)/m_step)*m_step*PACKET_SIZE;
		}

		m_buffer_index = (DWORD) IndexPacket;
		m_buffer_length = nBytesRead;
		return nBytesRead;
	} 
	return 0;

}
*/
// new function to support file with a size over 2 GB
__int64 FileSeek (HANDLE hf, __int64 distance, DWORD MoveMethod)
{
   LARGE_INTEGER li;

   li.QuadPart = distance;

   li.LowPart = SetFilePointer (hf, 
                                li.LowPart, 
                                &li.HighPart, 
                                MoveMethod);

   if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() 
       != NO_ERROR)
   {
      li.QuadPart = -1;
   }

   return li.QuadPart;
}

DWORD TSFile::Read(__int64 IndexPacket)
{	
	DWORD nBytesRead = 0;
	DWORD dwError = 0;
	DWORD dwLen = m_buffer_size;

	// if already loaded
	if(IsCached() == true)
	{
		if((IndexPacket == m_buffer_index)&& (m_buffer_length > 0))
			return m_buffer_length;
	}
	if(FileSeek (m_file_handle, IndexPacket*PACKET_SIZE, FILE_BEGIN)==-1)
	{ 
		// Obtain the error code. 
		dwError = GetLastError() ; 
		return 0;	 
	} // End of error handler 
	
	/*
	DWORD dwPtr = SetFilePointer(m_file_handle,(LONG) (IndexPacket*PACKET_SIZE),NULL,FILE_BEGIN);
	if (dwPtr == INVALID_SET_FILE_POINTER) // Test for failure
	{ 
		dwError = GetLastError() ; 
		return 0;	 
	} 
	*/ 


	BOOL bResult = ReadFile(m_file_handle, m_buffer_pointer, dwLen, &nBytesRead, NULL) ; 
	// Check for end of file. 
	if((bResult)&& (nBytesRead%PACKET_SIZE==0) )
	{ 

		if(nBytesRead!=m_buffer_size)
		{
			nBytesRead = (nBytesRead/PACKET_SIZE)*PACKET_SIZE;
		}

		m_buffer_index = (DWORD) IndexPacket;
		m_buffer_length = nBytesRead;
		return nBytesRead;
	} 
	return 0;

}
int TSFile::GetPacketLength(void)
{
	unsigned long localloop = 7;

	if(m_buffer_pointer)
	{
		if(Read(0))
		{
			unsigned long i = 0;
			while (i<localloop)
			{
				if( (m_buffer_pointer[i*PACKET_SIZE]==0x47) && (m_buffer_length > (i+1)*PACKET_SIZE))
					i++;
				else
					break;
			}
			if(i >= localloop)
				return PACKET_SIZE;
			i = 0;
			while (i<localloop)
			{
				if( (m_buffer_pointer[i*(PACKET_SIZE+16)]==0x47) && (m_buffer_length > (i+1)*(PACKET_SIZE+16)))
					i++;
				else
					break;
			}
			if(i >= localloop)
				return PACKET_SIZE+16;
		}
	}
	return 0;
}
/*
DWORD TSFile::Load(unsigned long buffersize)
{	 

	if (m_file_handle != NULL)
	{
		LARGE_INTEGER l;
		if(::GetFileSizeEx(m_file_handle, &l))
		{
			if(l.u.HighPart == 0)
			{
				if(buffersize == 0)
					m_buffer_size = l.u.LowPart;
				else
					m_buffer_size = ((buffersize/PACKET_SIZE)/m_step)*PACKET_SIZE*m_step;
				m_buffer_handle = GlobalAlloc(GHND, m_buffer_size);
				if(m_buffer_handle)
				{
					m_buffer_pointer = (LPBYTE) GlobalLock(m_buffer_handle);
					if(m_buffer_pointer)
					{
						Read(0);
						InitPIDDiscontinuity();
						InitPIDInformation();
						m_file_length = l.u.LowPart;
						return m_file_length;
					}
				}
			}
		}
	}
	return 0;
}*/
__int64 TSFile::Load(unsigned long buffersize)
{	 

	if (m_file_handle != NULL)
	{
		LARGE_INTEGER l;
		if(::GetFileSizeEx(m_file_handle, &l))
		{
			if(l.u.HighPart == 0)
			{
				m_buffer_size = l.u.LowPart;
				// fix bug:
				// if the buffersize is over the file size the streamer will fail at the end of the first loop.
				if((buffersize != 0) && (buffersize < l.u.LowPart))
					m_buffer_size = (buffersize/PACKET_SIZE)*PACKET_SIZE;
			}
			else
			{
				m_buffer_size = 0x80000000;
				// fix bug:
				// if the buffersize is over the file size the streamer will fail at the end of the first loop.
				if((buffersize != 0) && (buffersize < 0x80000000))
					m_buffer_size = (buffersize/PACKET_SIZE)*PACKET_SIZE;
			}


			m_buffer_handle = GlobalAlloc(GHND, m_buffer_size);
			if(m_buffer_handle)
			{
				m_buffer_pointer = (LPBYTE) GlobalLock(m_buffer_handle);
				if(m_buffer_pointer)
				{
					Read(0);
					InitPIDDiscontinuity();
					InitPIDInformation();
					// update file length
					m_file_length = l.QuadPart;
						//l.u.LowPart;
					return m_file_length;
				}
			}
		}
	}
	return 0;
}
int TSFile::Unload(void)
{
	if(m_buffer_handle)
	{
		GlobalUnlock(m_buffer_handle);
		GlobalFree(m_buffer_handle);
		m_buffer_handle = NULL;
		m_buffer_pointer = NULL;
		/* PCR Buffer
		GlobalUnlock(m_pcr_buffer_handle);
		GlobalFree(m_pcr_buffer_handle);
		m_pcr_buffer_handle = NULL;
		m_pcr_buffer_pointer = NULL;
		*/
		m_file_length = 0;
		return 1;
	}
	m_buffer_pointer = NULL;
	return 0;
}

PACKET* TSFile::GetFirstPacket(void)
{
	if(Read(m_start)>0)
	{
		return (PACKET*)m_buffer_pointer;
	}
	return NULL;
}
PACKET* TSFile::GetLastPacket(void)
{
	__int64 Index;
	if(m_end == -1)
	{
		if(m_file_length/PACKET_SIZE >= m_buffer_size/PACKET_SIZE)
			Index = m_file_length/PACKET_SIZE - m_buffer_size/PACKET_SIZE;
		else
			Index = 0;
	}
	else
	{
		if(m_end <= m_file_length/PACKET_SIZE)
		{
		if(m_end >= m_buffer_size/PACKET_SIZE)
			Index = (DWORD)m_end - m_buffer_size/PACKET_SIZE;
		else
			Index = 0;
		}
		else
			return NULL;
	}
	if(Read(Index)>0)
	{
		return (PACKET*)((char*)m_buffer_pointer+(m_buffer_length-PACKET_SIZE));
	}
	return NULL;
}
PACKET* TSFile::GetNextPacket(PACKET* pPacket)
{
	if ( ((char*)pPacket >= (char *) m_buffer_pointer) && 
		 ((char*)pPacket < (char *) m_buffer_pointer + m_buffer_length) )
	{
		 if((m_end != -1) && ( m_buffer_index + (((LPBYTE)pPacket - m_buffer_pointer)/PACKET_SIZE) + 1) > m_end)
			 return NULL;
		 if((char*)pPacket+PACKET_SIZE>=(char *)m_buffer_pointer + m_buffer_length)
		 {
			 if(m_buffer_length==m_file_length)
				pPacket = NULL;
			 else
			 {
				 if(Read(m_buffer_index+ m_buffer_length/PACKET_SIZE)==0)
					pPacket = NULL;
				 else
					pPacket = (PACKET *)m_buffer_pointer;
			 }
		 }
		 else
			 pPacket = (PACKET*)((char *)pPacket + PACKET_SIZE); 	
		return pPacket;
	}
	return NULL;
}
PACKET* TSFile::GetPrevPacket(PACKET* pPacket)
{
	if ( ((char*)pPacket >= (char *) m_buffer_pointer) && 
		 ((char*)pPacket < (char *) m_buffer_pointer + m_buffer_length) )
	{
		 if(((char*)pPacket-PACKET_SIZE)<(char*)m_buffer_pointer )
		 {

			 if(m_buffer_index == 0)
			 {
				pPacket = NULL;
				return NULL;
			 }
			 DWORD Index = 0;
			 if(m_buffer_index > m_buffer_size/PACKET_SIZE)
				Index = m_buffer_index - m_buffer_size/PACKET_SIZE;
			 else
				m_buffer_index = 0;
			 
			 if(Read(Index)==0)
				pPacket = NULL;
			 else
				pPacket = (PACKET*)((char*)m_buffer_pointer+(m_buffer_length-PACKET_SIZE));
		 }
		 else
			 pPacket = (PACKET*)((char *)pPacket - PACKET_SIZE); 	
		return pPacket;
	}
	return NULL;
}

typedef	struct {
	BYTE Min;
	BYTE Max;
	LPCSTR s;
} STREAM_TYPE_STRING;

STREAM_TYPE_STRING Array[] = {
0,0, "ITU-T | ISO/IEC reserved",
1,1, "ISO/IEC 11172-2 Video",
2,2, "ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream",
3,3, "ISO/IEC 11172-3 Audio",
4,4, "ISO/IEC 13818-3 Audio",
5,5, "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 private_sections",
6,6, "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 PES packets containing private data",
7,7, "ISO/IEC 13522 MHEG",
8,8, "Annex A – DSM CC",
9,9, "ITU-T Rec. H.222.1",
10,10, "ISO/IEC 13818-6 type A",
11,11, "ISO/IEC 13818-6 type B",
12,12, "ISO/IEC 13818-6 type C",
13,13, "ISO/IEC 13818-6 type D",
14,14, "ISO/IEC 13818-1 auxiliary",
15,127, "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 reserved",
128,255,"User private"};

LPCTSTR TSFile::GetStreamType(BYTE st)
{
	int i = 0;
	while(Array[i].Max != 0xFF)
	{
		if ( (st >= Array[i].Min) && 
			 (st <= Array[i].Max) )
			 return Array[i].s;
		i++;
	}
	return Array[i].s;
}

WORD TSFile::GetPCR_PID(WORD PID,PID_INFO *pPIDInfo, DWORD dw )
{
	WORD PCR_PID = 0xFFFF;
	int Offset = 4;
	int OffsetLimit = 0;
	int OffsetSectionLengthLimit = 0;
	int Count = 0;
	if(pPIDInfo!=NULL)
	{
		for(DWORD k = 0; k < dw; k++)
		{
			pPIDInfo[k].PID = 0xFFFF;
			pPIDInfo[k].Type = 0xFF;
		}
	}

	PACKET* Packet = GetFirstPacket();
	while(Packet)
	{
		if((Packet[0][0]==0x47)&& ((Packet[0][1]&0x80)==0))
		{
			WORD locPID = MAKEWORD(Packet[0][2], Packet[0][1]&0x1F);
			//PID only
			if(locPID == PID)
			{
				Offset = 4;
				if((Packet[0][3]&0x20)!=0)
					Offset += (Packet[0][4]+1);
				if((Packet[0][3]&0x10)!=0)
				{
					//NOT PES
					if( (Packet[0][Offset]!=0x00) ||
						(Packet[0][Offset+1]!=0x00) ||
						(Packet[0][Offset+2]!=0x01))
					{
						if((Packet[0][1]&0x40)!=0) //paylod unit start indicator
												//pointer is there
							Offset += Packet[0][Offset]+1;

						WORD transport_stream_id = 0;
						BYTE section_number = 0;
						BYTE last_section_number = 0;
						WORD section_length = 0; 
						WORD network_pid = 0;
						WORD program_map_pid = 0;
						WORD program_number = 0;
						WORD program_info_length = 0;
						BYTE stream_type = 0 ;
						WORD elementary_PID = 0;
						WORD ES_info_length = 0;
						BYTE descriptor_tag = 0;
						BYTE descriptor_length = 0;
						int i = 0;
						//PSI
						section_length = MAKEWORD(Packet[0][Offset+2], Packet[0][Offset+1]&0x0F);
						transport_stream_id = MAKEWORD(Packet[0][Offset+4], Packet[0][Offset+3]);
						switch (Packet[0][Offset])
						{
						case 0x02: //program association section
							OffsetSectionLengthLimit = Offset-1+section_length;
							PCR_PID = MAKEWORD(Packet[0][Offset+9], Packet[0][Offset+8]&0x1F);
							program_info_length = MAKEWORD(Packet[0][Offset+11], Packet[0][Offset+10]&0x0F);
							Offset = Offset + 12;
							OffsetLimit = Offset+program_info_length;
							while(Offset<OffsetLimit)
							{
								descriptor_tag = Packet[0][Offset];
								descriptor_length = Packet[0][Offset+1];
								Offset += descriptor_length+2;
							}
							//while(Offset<section_length-1)
							while(Offset<OffsetSectionLengthLimit)
							{
								stream_type = Packet[0][Offset];
								elementary_PID = MAKEWORD(Packet[0][Offset+2], Packet[0][Offset+1]&0x1F);
								ES_info_length = MAKEWORD(Packet[0][Offset+4], Packet[0][Offset+3]&0x0F);
								if(pPIDInfo!=NULL)
								{
									for(DWORD k = 0; k < dw; k++)
									{
										if(pPIDInfo[k].PID == 0xFFFF)
										{
											pPIDInfo[k].PID = elementary_PID;
											pPIDInfo[k].Type = stream_type;
											break;
										}
										if(pPIDInfo[k].PID == elementary_PID)
										{
											pPIDInfo[k].Type = stream_type;
											break;
										}
									}
								}


								Offset += 5;
								OffsetLimit = Offset+ES_info_length;
								while(Offset<OffsetLimit)
								{
									descriptor_tag = Packet[0][Offset];
									descriptor_length = Packet[0][Offset+1];
									Offset += descriptor_length+2;
								}
							}
						    return PCR_PID;
							break;
						default:
							break;
						}
					}
				}
			}
		}
		Packet = GetNextPacket(Packet);
	}
	return 0xFFFF;
}
int TSFile::GetProgramMapTableID(PARSE_COUNTERS* p)
{
	WORD PID = 0xFFFF;
	int Offset = 4;
	int Count = 0;
	PACKET* Packet = GetFirstPacket();
	if(p == 0)
		return 0;
	while(Packet)
	{
		if((Packet[0][0]==0x47)&& ((Packet[0][1]&0x80)==0))
		{
			WORD locPID = MAKEWORD(Packet[0][2], Packet[0][1]&0x1F);
			//PAT Only
			if(locPID == 0)
			{
				if((Packet[0][3]&0x20)!=0)
					Offset += (Packet[0][4]+1);
				if((Packet[0][3]&0x10)!=0)
				{
					//NOT PES
					if( (Packet[0][Offset]!=0x00) ||
						(Packet[0][Offset+1]!=0x00) ||
						(Packet[0][Offset+2]!=0x01))
					{
						if((Packet[0][1]&0x40)!=0) //paylod unit start indicator
												//pointer is there
							Offset += Packet[0][Offset]+1;

						WORD transport_stream_id = 0;
						BYTE section_number = 0;
						BYTE last_section_number = 0;
						WORD section_length = 0; 
						WORD network_pid = 0;
						WORD program_map_pid = 0;
						WORD program_number = 0;
						int i = 0;
						//PSI
						section_length = MAKEWORD(Packet[0][Offset+2], Packet[0][Offset+1]&0x0F);
						transport_stream_id = MAKEWORD(Packet[0][Offset+4], Packet[0][Offset+3]);
						switch (Packet[0][Offset])
						{
						case 0x00: //program association section
							section_number = Packet[0][Offset+6];
							last_section_number = Packet[0][Offset+7];

							while(8+i*4+4<section_length)
							{
								program_number = MAKEWORD(Packet[0][Offset+4*i+9], Packet[0][Offset+4*i+8]);
								if(program_number == 0)
									network_pid = MAKEWORD(Packet[0][Offset+4*i+11], Packet[0][Offset+4*i+10]&0x1F);
								else
								{
									program_map_pid = MAKEWORD(Packet[0][Offset+4*i+11], Packet[0][Offset+4*i+10]&0x1F);

									if(Count<(sizeof(p->Counters)/sizeof(TIME_STAMP_COUNTERS)))
									{
										p->Counters[Count].program_map_pid = p->Counters[Count].PID = program_map_pid;
										p->Counters[Count++].program_number = program_number;
										p->Count = Count;
									}
								}
								i++;
							}
							if(Count>0)
							{
								return Count;
							}

							break;
						default:
							break;
						}
					}
				}
			}
		}
		Packet = GetNextPacket(Packet);
	}
	return 0;
}

int TSFile::AddPIDDiscontinuity(WORD PID)
{
	for(int i = 0; i< sizeof(m_arrayPID)/sizeof(PID_COUNTER);i++)
	{
		if(m_arrayPID[i].pid == PID)
		{
			m_arrayPID[i].counter++;
			return 1;
		}
		if(m_arrayPID[i].pid == 0xFFFF)
		{
			m_arrayPID[i].pid = PID;
			m_arrayPID[i].counter = 0;
			return 1;
		}
	}
	return 0;
}
bool TSFile::UpdateContinuityCounter(LPBYTE Packet)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{

		 WORD PID = MAKEWORD(Packet[2], Packet[1]&0x1F);
		 BYTE Counter = GetLastContinuityCounter(PID);
		 if(Counter == 0xFF)
		 {
			 Counter = Packet[3]&0x0F;
			 SetLastContinuityCounter(PID,Counter);
			 return 0;
		 }
		 if( ((Packet[3]&0x30)==0x00) || ((Packet[3]&0x30)==0x20))
			 // Discontinuity
			 Packet[3] = (Packet[3]&0xF0)|(Counter&0x0F);	  		 
		 else
			 Packet[3] = (Packet[3]&0xF0)|(++Counter&0x0F);
		 SetLastContinuityCounter(PID,Counter);
	}
	return 0;
}
bool TSFile::CheckContinuityCounter(LPBYTE Packet,__int64 count, string& s)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{

		 WORD PID = MAKEWORD(Packet[2], Packet[1]&0x1F);
		 int Counter = GetLastContinuityCounter(PID);
		 if(Counter == 0xFF)
		 {
			 Counter = Packet[3]&0x0F;
			 SetLastContinuityCounter(PID,Counter);
			 return 0;
		 }
		 if( ((Packet[3]&0x30)==0x00) || ((Packet[3]&0x30)==0x20))
		 {
			 // Discontinuity
				 Counter = Packet[3]&0x0F;
				 SetLastContinuityCounter(PID,Counter);
/*			 if( Counter != (Packet[3]&0x0F))
			 {
 				ostringstream oss;
				int c = Packet[3]&0x0F;
				 oss << "Continuity Error for packet " << count << " PID: "  << PID << " Counter: " << c << " Expected Counter: " << Counter << endl;
				 s = oss.str().c_str();
				 Counter = Packet[3]&0x0F;
				 SetLastContinuityCounter(PID,Counter);
				 return 1;				
			 }
			 */
		 }
		 else
		 {
			 Counter = ++Counter & 0x0F;
			 if( Counter != (Packet[3]&0x0F))
			 {
				 ostringstream oss;
				int c = Packet[3]&0x0F;
				 oss << "Continuity Error for packet " << count << " PID: "  << PID << " Counter: " << c << " Expected Counter: " << Counter << endl;
				 s = oss.str().c_str();
				 Counter = Packet[3]&0x0F;
				 SetLastContinuityCounter(PID,Counter);
				 return 1;				
			 }
		 }
		 SetLastContinuityCounter(PID,Counter);
	}
	return 0;
}

int TSFile::InitPIDInformation()
{
	for(int i = 0; i< sizeof(m_arrayPIDInformation)/sizeof(PID_INFORMATION);i++)
	{
		m_arrayPIDInformation[i].pid = 0xFFFF;
		m_arrayPIDInformation[i].ContinuityCounter = 0;
	}
	for(int i = 0; i< sizeof(m_arrayPCRInformation)/sizeof(PCR_INFORMATION);i++)
	{
		m_arrayPCRInformation[i].pid = 0xFFFF;
		m_arrayPCRInformation[i].PCR = 0;
	}	
	for(int i = 0; i< sizeof(m_arrayPTSInformation)/sizeof(PTS_INFORMATION);i++)
	{
		m_arrayPTSInformation[i].pid = 0xFFFF;
		m_arrayPTSInformation[i].PTS = 0;
	}	
	for(int i = 0; i< sizeof(m_arrayDTSInformation)/sizeof(DTS_INFORMATION);i++)
	{
		m_arrayDTSInformation[i].pid = 0xFFFF;
		m_arrayDTSInformation[i].DTS = 0;
	}	
	
	return 1;
}
__int64 TSFile::GetLastPTS(WORD Pid)
{
	for(int i = 0; i< sizeof(m_arrayPTSInformation)/sizeof(PTS_INFORMATION);i++)
	{
		if( m_arrayPTSInformation[i].pid == 0xFFFF)
		{
			m_arrayPTSInformation[i].pid = Pid;
			m_arrayPTSInformation[i].PTS = 0;
		}
		if( m_arrayPTSInformation[i].pid == Pid)
			return m_arrayPTSInformation[i].PTS;

	}
	return 0;
}
__int64 TSFile::GetLastDTS(WORD Pid)
{
	for(int i = 0; i< sizeof(m_arrayDTSInformation)/sizeof(DTS_INFORMATION);i++)
	{
		if( m_arrayDTSInformation[i].pid == 0xFFFF)
		{
			m_arrayDTSInformation[i].pid = Pid;
			m_arrayDTSInformation[i].DTS = 0;
		}
		if( m_arrayDTSInformation[i].pid == Pid)
			return m_arrayDTSInformation[i].DTS;

	}
	return 0;
}
bool TSFile::GetLastPCR(WORD Pid,__int64& PCR)
{
	for(int i = 0; i< sizeof(m_arrayPCRInformation)/sizeof(PCR_INFORMATION);i++)
	{
		if( m_arrayPCRInformation[i].pid == Pid)
		{
			PCR = m_arrayPCRInformation[i].PCR;
			return true;
		}
	}
	return false;
}
bool TSFile::GetLastPCRCounter(WORD Pid,__int64& PCR,__int64& Counter)
{
	for(int i = 0; i< sizeof(m_arrayPCRInformation)/sizeof(PCR_INFORMATION);i++)
	{
		if( m_arrayPCRInformation[i].pid == Pid)
		{
			PCR = m_arrayPCRInformation[i].PCR;
			Counter = m_arrayPCRInformation[i].Counter;
			return true;
		}
	}
	return false;
}
bool TSFile::GetFirstPCRCounter(WORD Pid,__int64& PCR,__int64& Counter)
{
	for(int i = 0; i< sizeof(m_arrayPCRInformation)/sizeof(PCR_INFORMATION);i++)
	{
		if( m_arrayPCRInformation[i].pid == Pid)
		{
			PCR = m_arrayPCRInformation[i].FirstPCR;
			Counter = m_arrayPCRInformation[i].FirstCounter;
			return true;
		}
	}
	return false;
}
BYTE TSFile::GetLastContinuityCounter(WORD Pid)
{
	for(int i = 0; i< sizeof(m_arrayPIDInformation)/sizeof(PID_INFORMATION);i++)
	{
		if( m_arrayPIDInformation[i].pid == 0xFFFF)
		{
			m_arrayPIDInformation[i].pid = Pid;
			m_arrayPIDInformation[i].ContinuityCounter = 0;
			return 0xFF;
		}
		if( m_arrayPIDInformation[i].pid == Pid)
			return m_arrayPIDInformation[i].ContinuityCounter ;

	}
	return 0;
}
bool TSFile::SetLastContinuityCounter(WORD Pid,BYTE Counter)
{
	for(int i = 0; i< sizeof(m_arrayPIDInformation)/sizeof(PID_INFORMATION);i++)
	{
		if( m_arrayPIDInformation[i].pid == 0xFFFF)
		{
			m_arrayPIDInformation[i].pid = Pid;
			m_arrayPIDInformation[i].ContinuityCounter = 0x0F & Counter;
			return true;
		}
		if( m_arrayPIDInformation[i].pid == Pid)
		{
			m_arrayPIDInformation[i].ContinuityCounter = 0x0F & Counter;
			return true;
		}

	}
	return false;
}
bool TSFile::SetLastPCR(WORD Pid,__int64 PCR)
{
	for(int i = 0; i< sizeof(m_arrayPCRInformation)/sizeof(PCR_INFORMATION);i++)
	{
		if( m_arrayPCRInformation[i].pid == 0xFFFF)
		{
			m_arrayPCRInformation[i].pid = Pid;
//			m_arrayPCRInformation[i].PCR = 0x3FFFFFFFFFF & PCR;
			m_arrayPCRInformation[i].PCR = (0x1FFFFFFFF & (PCR/300))*300 + (PCR%300);
			return true;
		}
		if( m_arrayPCRInformation[i].pid == Pid)
		{
//			m_arrayPCRInformation[i].PCR = 0x3FFFFFFFFFF & PCR;
			m_arrayPCRInformation[i].PCR = (0x1FFFFFFFF & (PCR/300))*300 + (PCR%300);
			return true;
		}

	}
	return false;
}
bool TSFile::SetLastPCRCounter(WORD Pid,__int64 PCR,__int64 Counter)
{
	for(int i = 0; i< sizeof(m_arrayPCRInformation)/sizeof(PCR_INFORMATION);i++)
	{
		if( m_arrayPCRInformation[i].pid == 0xFFFF)
		{
			m_arrayPCRInformation[i].pid = Pid;
//			m_arrayPCRInformation[i].PCR = 0x3FFFFFFFFFF & PCR;
			m_arrayPCRInformation[i].PCR = (0x1FFFFFFFF & (PCR/300))*300 + (PCR%300);
			m_arrayPCRInformation[i].Counter = Counter;
			return true;
		}
		if( m_arrayPCRInformation[i].pid == Pid)
		{
//			m_arrayPCRInformation[i].PCR = 0x3FFFFFFFFFF & PCR;
			m_arrayPCRInformation[i].PCR = (0x1FFFFFFFF & (PCR/300))*300 + (PCR%300);
			m_arrayPCRInformation[i].Counter = Counter;
			return true;
		}

	}
	return false;
}
bool TSFile::SetFirstPCRCounter(WORD Pid,__int64 PCR,__int64 Counter)
{
	for(int i = 0; i< sizeof(m_arrayPCRInformation)/sizeof(PCR_INFORMATION);i++)
	{
		if( m_arrayPCRInformation[i].pid == 0xFFFF)
		{
			m_arrayPCRInformation[i].pid = Pid;
//			m_arrayPCRInformation[i].PCR = 0x3FFFFFFFFFF & PCR;
			m_arrayPCRInformation[i].PCR = (0x1FFFFFFFF & (PCR/300))*300 + (PCR%300);
			m_arrayPCRInformation[i].Counter = Counter;
//			m_arrayPCRInformation[i].FirstPCR = 0x3FFFFFFFFFF & PCR;
			m_arrayPCRInformation[i].FirstPCR = (0x1FFFFFFFF & (PCR/300))*300 + (PCR%300);
			m_arrayPCRInformation[i].FirstCounter = Counter;
			return true;
		}
		if( m_arrayPCRInformation[i].pid == Pid)
		{
//			m_arrayPCRInformation[i].FirstPCR = 0x3FFFFFFFFFF & PCR;
			m_arrayPCRInformation[i].FirstPCR = (0x1FFFFFFFF & (PCR/300))*300 + (PCR%300);
			m_arrayPCRInformation[i].FirstCounter = Counter;
			return true;
		}

	}
	return false;
}

bool TSFile::SetLastDTS(WORD Pid,__int64 DTS)
{
	for(int i = 0; i< sizeof(m_arrayDTSInformation)/sizeof(DTS_INFORMATION);i++)
	{
		if( m_arrayDTSInformation[i].pid == 0xFFFF)
		{
			m_arrayDTSInformation[i].pid = Pid;
			m_arrayDTSInformation[i].DTS = 0x1FFFFFFFF & DTS;
			return true;
		}
		if( m_arrayDTSInformation[i].pid == Pid)
		{
			m_arrayDTSInformation[i].DTS = 0x1FFFFFFFF &  DTS;
			return true;
		}

	}
	return false;
}
bool TSFile::SetLastPTS(WORD Pid,__int64 PTS)
{
	for(int i = 0; i< sizeof(m_arrayPTSInformation)/sizeof(PTS_INFORMATION);i++)
	{
		if( m_arrayPTSInformation[i].pid == 0xFFFF)
		{
			m_arrayPTSInformation[i].pid = Pid;
			m_arrayPTSInformation[i].PTS = 0x1FFFFFFFF & PTS;
			return true;
		}
		if( m_arrayPTSInformation[i].pid == Pid)
		{
			m_arrayPTSInformation[i].PTS = 0x1FFFFFFFF & PTS;
			return true;
		}

	}
	return false;
}

int TSFile::InitPIDDiscontinuity()
{
	for(int i = 0; i< sizeof(m_arrayPID)/sizeof(PID_COUNTER);i++)
	{
		m_arrayPID[i].pid = 0xFFFF;
		m_arrayPID[i].bSetDicontinuityCounter = false;
	}
	return 1;
}
bool TSFile::AreAllPIDDiscontinuitySet()
{
	bool bcrt = false;
	for(int i = 0; i< sizeof(m_arrayPID)/sizeof(PID_COUNTER);i++)
	{
		if(m_arrayPID[i].pid != 0xFFFF)
		{
			if(m_arrayPID[i].bSetDicontinuityCounter == false)
				return false;
		}
		else
			return true;
	}
	return true;
}
int TSFile::ResetPIDDiscontinuity()
{
	for(int i = 0; i< sizeof(m_arrayPID)/sizeof(PID_COUNTER);i++)
	{
		m_arrayPID[i].bSetDicontinuityCounter = false;
	}
	return 1;
}
int TSFile::IsPIDDiscontinuitySet(WORD PID)
{
	for(int i = 0; i< sizeof(m_arrayPID)/sizeof(PID_COUNTER);i++)
	{
		if(m_arrayPID[i].pid == PID)
			return m_arrayPID[i].bSetDicontinuityCounter;
	}
	return false;
}
int TSFile::SetPIDDiscontinuity(WORD PID)
{
	for(int i = 0; i< sizeof(m_arrayPID)/sizeof(PID_COUNTER);i++)
	{
		if(m_arrayPID[i].pid == PID)
		{
			m_arrayPID[i].bSetDicontinuityCounter = true;
			return 1;
		}
	}
	return 0;
}
/*
int TSFile::GetBitrate(TIME_STAMP_COUNTERS* pCounter, WORD PID, double& duration, double& bitrate, __int64& PacketCounter)
{

	__int64 PCR;
	__int64 DeltaPCR = 0;
	__int64 DeltaTempo = 0;
	PacketCounter = 0;

	if(pCounter==0)
		return 0;

	ResetPIDDiscontinuity();

	PACKET* Packet = GetFirstPacket();
	while(Packet)
	{
		if((Packet[0][0]==0x47)&& ((Packet[0][1]&0x80)==0))
		{
			WORD locPID = MAKEWORD(Packet[0][2], Packet[0][1]&0x1F);
			AddPIDDiscontinuity(locPID);
			//PID Only
			if(locPID == pCounter->PCR_PID)
			{
				int bDiscontinuity = IsDiscontinuityIndicator(Packet[0]);
				if((Packet[0][3]&0x20)!=0)
				{
					if(GetPCRInAdaptationField(Packet[0],&PCR))
					{
						if(bDiscontinuity)
							pCounter->TimeStampDiscontinuity++;

						pCounter->TimeStampPacketCount++;
						if(pCounter->FirstTimeStamp == -1)
						{
							pCounter->IndexLastTimeStampPacket = pCounter->IndexFirstTimeStampPacket = PacketCounter;
							pCounter->FirstTimeStamp = PCR;
							pCounter->LastTimeStamp = PCR;
						}
						if(PCR>=pCounter->LastTimeStamp)
							DeltaTempo = PCR - pCounter->LastTimeStamp;
						else
							DeltaTempo = (0x200000000 - (pCounter->LastTimeStamp - PCR));


						pCounter->IndexLastTimeStampPacket = PacketCounter;

						if(bDiscontinuity == 0) 
							DeltaPCR += DeltaTempo;

						pCounter->LastTimeStamp = PCR;
					}
				}
			}
			PacketCounter++;
		}
		Packet = GetNextPacket(Packet);
	}

	__int64 locbitrate = (pCounter->IndexLastTimeStampPacket-pCounter->IndexFirstTimeStampPacket)*PACKET_SIZE*8;
//	bitrate = PacketCounter*PACKET_SIZE*8;
	bitrate = (double) (locbitrate / DeltaPCR);
	bitrate = bitrate * 90000;
	duration = (double)(PacketCounter*8*PACKET_SIZE);
	duration = duration/bitrate;
	pCounter->bitrate = bitrate;
	pCounter->duration = duration;
	return 1;
}
*/
/* PCR Buffer
int TSFile::GetNextPCR(DWORD Index, __int64* lpPCR,DWORD* lpIndexPCR)
{
	if(m_pcr_buffer_pointer)
	{
		DWORD start = 0;
		if(m_pcr_buffer_pointer[m_pcr_buffer_index].Index<=Index)
			start = m_pcr_buffer_index;
		for(DWORD i = start;i < m_pcr_buffer_length;i++)
		{
			if(m_pcr_buffer_pointer[i].Index>Index)
			{
				*lpIndexPCR = m_pcr_buffer_pointer[i].Index;
				*lpPCR = m_pcr_buffer_pointer[i].PCR;
				m_pcr_buffer_index = i;
				return 1;
			}
		}
	}
	return 0;
}

DWORD TSFile::LoadPCRBuffer(WORD PCR_PID)
{

	__int64 PCR;
	DWORD PacketCounter = 0;
	m_pcr_buffer_index = 0;
	m_pcr_buffer_length = 0;

	PACKET* Packet = GetFirstPacket();
	while(Packet)
	{
		if((Packet[0][0]==0x47)&& ((Packet[0][1]&0x80)==0))
		{
			WORD locPID = MAKEWORD(Packet[0][2], Packet[0][1]&0x1F);
			if(locPID == PCR_PID)
			{
				if((Packet[0][3]&0x20)!=0)
				{
					if(GetPCRInAdaptationField(Packet[0],&PCR))
					{
						if(m_pcr_buffer_index<m_pcr_buffer_size)
						{
							m_pcr_buffer_pointer[m_pcr_buffer_index].Index = PacketCounter;
							m_pcr_buffer_pointer[m_pcr_buffer_index].PCR = PCR;
							m_pcr_buffer_index++;
						}
						else
							return 0;
					}
				}
			}
			PacketCounter++;
		}
		Packet = GetNextPacket(Packet);
	}
	m_pcr_buffer_length = m_pcr_buffer_index;
	m_pcr_buffer_index = 0;
	return m_pcr_buffer_length;
}
*/
int TSFile::GetBitrate(TIME_STAMP_COUNTERS* pCounter, WORD PID, double& duration, double& bitrate, __int64& PacketCounter)
{

	__int64 PCR;
	__int64 DeltaPCR = 0;
	__int64 DeltaTempo = 0;
	PacketCounter = 0;

	if(pCounter==0)
		return 0;

	ResetPIDDiscontinuity();

	PACKET* Packet = GetFirstPacket();
	while(Packet)
	{
		if((Packet[0][0]==0x47)&& ((Packet[0][1]&0x80)==0))
		{
			WORD locPID = MAKEWORD(Packet[0][2], Packet[0][1]&0x1F);
			AddPIDDiscontinuity(locPID);
			//PID Only
			if(locPID == pCounter->PCR_PID)
			{
				int bDiscontinuity = IsDiscontinuityIndicator(Packet[0]);
				if((Packet[0][3]&0x20)!=0)
				{
					if(GetPCRInAdaptationField(Packet[0],&PCR))
					{
						pCounter->IndexFirstTimeStampPacket = PacketCounter;
						pCounter->FirstTimeStamp = PCR;
						break;
					}
				}
			}
			PacketCounter++;
		}
		Packet = GetNextPacket(Packet);
	}
	PacketCounter = m_file_length/PACKET_SIZE;
	Packet = GetLastPacket();
	while(Packet)
	{
		PacketCounter--;
		if((Packet[0][0]==0x47)&& ((Packet[0][1]&0x80)==0))
		{
			WORD locPID = MAKEWORD(Packet[0][2], Packet[0][1]&0x1F);
			AddPIDDiscontinuity(locPID);
			//PID Only
			if(locPID == pCounter->PCR_PID)
			{
				int bDiscontinuity = IsDiscontinuityIndicator(Packet[0]);
				if((Packet[0][3]&0x20)!=0)
				{
					if(GetPCRInAdaptationField(Packet[0],&PCR))
					{
						pCounter->IndexLastTimeStampPacket = PacketCounter;
						pCounter->LastTimeStamp = PCR;
						break;
					}
				}
			}
			
		}
		Packet = GetPrevPacket(Packet);
	}

/*
	if(pCounter->LastTimeStamp>=pCounter->FirstTimeStamp)
		DeltaPCR = pCounter->LastTimeStamp - pCounter->FirstTimeStamp;
	else
		DeltaPCR = (0x40000000000 - (pCounter->FirstTimeStamp - pCounter->LastTimeStamp));
*/
	//	DeltaPCR = (0x25800000000 - (pCounter->FirstTimeStamp - pCounter->LastTimeStamp));
	if (((pCounter->LastTimeStamp & 0x20000000000) == 0) && ((pCounter->FirstTimeStamp & 0x20000000000) != 0))
		DeltaPCR = (0x257FFFFFFFF - pCounter->FirstTimeStamp) + pCounter->LastTimeStamp;
	else
		DeltaPCR = pCounter->LastTimeStamp - pCounter->FirstTimeStamp;

	__int64 locbitrate = (pCounter->IndexLastTimeStampPacket-pCounter->IndexFirstTimeStampPacket)*PACKET_SIZE*8;
//	bitrate = PacketCounter*PACKET_SIZE*8;
	if(DeltaPCR>0)
		bitrate = (double) (locbitrate*27000000 / DeltaPCR);
	else
		bitrate = 0;
	PacketCounter = m_file_length/PACKET_SIZE;
	if(m_end != -1)
		PacketCounter = m_end - m_start;
	duration = (double)(PacketCounter*8*PACKET_SIZE);
	if(bitrate>0)
		duration = duration/bitrate;
	else
		duration = 0;
	pCounter->bitrate = bitrate;
	pCounter->duration = duration;


	__int64 IndexFirstPacket = m_start;
	__int64 IndexLastPacket = m_end;
	if(IndexLastPacket == -1)
		IndexLastPacket = m_file_length/PACKET_SIZE -1;

	pCounter->delta = 0;
/*	if((pCounter->LastTimeStamp-pCounter->FirstTimeStamp)>=0)
		pCounter->delta = pCounter->LastTimeStamp - pCounter->FirstTimeStamp ;
	else
		pCounter->delta = (0x200000000 - (pCounter->FirstTimeStamp - pCounter->LastTimeStamp));
*/
	if (((pCounter->LastTimeStamp & 0x20000000000) == 0) && ((pCounter->FirstTimeStamp & 0x20000000000) != 0))
		pCounter->delta = (0x257FFFFFFFF - pCounter->FirstTimeStamp) + pCounter->LastTimeStamp;
	else
		pCounter->delta = pCounter->LastTimeStamp - pCounter->FirstTimeStamp;


	pCounter->delta += (long long) (((pCounter->IndexFirstTimeStampPacket - IndexFirstPacket) + (IndexLastPacket - pCounter->IndexLastTimeStampPacket) +1)*8*PACKET_SIZE*27000000/pCounter->bitrate);
	return 1;
}




int TSFile::GetPacketRange(WORD PCR_PID,  __int64& PacketStart,  __int64& PacketEnd)
{

	__int64 PCR;
	__int64 DeltaPCR = 0;
	__int64 DeltaTempo = 0;
	__int64 PacketCounter = 0;


	PacketStart = -1;
	PacketEnd = -1;

	PACKET* Packet = GetFirstPacket();
	while(Packet)
	{
		if((Packet[0][0]==0x47)&& ((Packet[0][1]&0x80)==0))
		{
			WORD locPID = MAKEWORD(Packet[0][2], Packet[0][1]&0x1F);
			if(locPID == 0)
				PacketStart = PacketCounter;
			//PID Only
			if(locPID == PCR_PID)
			{
				if((Packet[0][3]&0x20)!=0)
				{
					if(GetPCRInAdaptationField(Packet[0],&PCR))
					{
						if(PacketStart != -1)
							break;
					}
				}
			}
			PacketCounter++;
		}
		Packet = GetNextPacket(Packet);
	}
	PacketCounter = m_file_length/PACKET_SIZE;
	Packet = GetLastPacket();
	while(Packet)
	{
		PacketCounter--;
		if((Packet[0][0]==0x47)&& ((Packet[0][1]&0x80)==0))
		{
//			WORD locPID = MAKEWORD(Packet[0][2], Packet[0][1]&0x1F);
			//PID Only
//			if(locPID == PCR_PID)
//			if(Packet[0][1]&0x40 != 0)
			{
				if( GetRAIInAdaptationField((LPBYTE)Packet)
/*					&&
					(GetDTSInAdaptationField((LPBYTE)Packet,&PCR)||
						GetDTSInPES((LPBYTE)Packet,&PCR) ||
						GetPTSInPES((LPBYTE)Packet,&PCR))
*/						)
				{
					PacketEnd = --PacketCounter;
					break;
				}
			}			
		}
		Packet = GetPrevPacket(Packet);
	}
	return 1;
}








int TSFile::GetContinuityCounter(LPBYTE Packet, unsigned char* pCounter )
{
	if((pCounter)&&(Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		*pCounter = Packet[3]&0x0F;
		return 1;
	}
	return 0;
}
int TSFile::SetContinuityCounter(LPBYTE Packet, unsigned char Counter )
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		Counter = 0x0F & Counter;
		Packet[3] = (Packet[3]&0xF0)|(((Packet[3]&0x0F)+ Counter)&0x0F);
//		Packet[3] = (Packet[3]&0xF0)|Counter;
//		Packet[3] = Packet[3]|Counter;
		return 1;
	}
	return 0;
}

int TSFile::GetPIDPCR(LPBYTE Packet, WORD PID, __int64* lpPCR,bool* pbDiscontinuity)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if( ((Packet[3]&0x20)!=0) && (Packet[4]!=0))
		 {
			 if(pbDiscontinuity)
			 {
				 if(Packet[5]&0x80)
					 *pbDiscontinuity = true;
				 else
					 *pbDiscontinuity = false;
			 }
			 if(Packet[5]&0x10)
			 {
				if(PID == MAKEWORD(Packet[2], Packet[1]&0x1F))
				{
					__int64 pcr_base = (((Packet[6] << 24) | (Packet[7] << 16) | (Packet[8] << 8) | (Packet[9])) & 0xFFFFFFFF);
					pcr_base = pcr_base << 1;

					if((Packet[10] & 0x80)== 0x80)
						pcr_base = (pcr_base) | 0x000000001;
					else
						pcr_base = (pcr_base) & 0xFFFFFFFFE;

					__int64 pcr_ext =  ( ((Packet[10] & 0x01) == 0x01)? 0x100 : 0x00);
					pcr_ext = pcr_ext | Packet[11];

					*lpPCR = pcr_base*300 + pcr_ext;
					return 1;
				}
			 }
		 }
	}
	return 0;
}

int TSFile::GetPIDPCR(LPBYTE Packet, LPWORD lpPID, __int64* lpPCR,bool* pbDiscontinuity)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if( ((Packet[3]&0x20)!=0) && (Packet[4]!=0))
		 {
			 if(pbDiscontinuity)
			 {
				 if(Packet[5]&0x80)
					 *pbDiscontinuity = true;
				 else
					 *pbDiscontinuity = false;
			 }
			 if(Packet[5]&0x10)
			 {
				*lpPID= MAKEWORD(Packet[2], Packet[1]&0x1F);
				__int64 pcr_base = (((Packet[6] << 24) | (Packet[7] << 16) | (Packet[8] << 8) | (Packet[9])) & 0xFFFFFFFF);
				pcr_base = pcr_base << 1;

				if((Packet[10] & 0x80)== 0x80)
					pcr_base = (pcr_base) | 0x000000001;
				else
					pcr_base = (pcr_base) & 0xFFFFFFFFE;

				__int64 pcr_ext =  ( ((Packet[10] & 0x01) == 0x01)? 0x100 : 0x00);
				pcr_ext = pcr_ext | Packet[11];

				*lpPCR = pcr_base*300 + pcr_ext;
				return 1;
			 }
		 }
	}
	return 0;
}
int TSFile::SetPCRInAdaptationField(LPBYTE Packet, __int64 PCR)
{
	__int64 pcr_base = PCR / 300;
	__int64 pcr_ext = PCR % 300;
	if((pcr_base & 0x0000000000000001)== 1)
		Packet[10] = Packet[10] | 0x80;
	else
		Packet[10] = Packet[10] & 0x7F;
	pcr_base = pcr_base >> 1;
	char* pPCR = (char *) &pcr_base;
	Packet[6] = pPCR[3];
	Packet[7] = pPCR[2];
	Packet[8] = pPCR[1];
	Packet[9] = pPCR[0];

	if((pcr_ext & 0x100)== 0x100)
		Packet[10] = Packet[10] | 0x01;
	else
		Packet[10] = Packet[10] & 0xFE;
	Packet[11] = 0x0FF & pcr_ext;

	return 1;
}
int TSFile::GetOPCRInAdaptationField(LPBYTE Packet, __int64* lpOPCR)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if( ((Packet[3]&0x20)!=0) && (Packet[4]!=0))
		 {
			 if(Packet[5]&0x08)
			 {
				int Offset = 2;
				if(Packet[5]&0x10)
					Offset += 6;

				__int64 t = (((Packet[4+Offset] << 24) | (Packet[5+Offset] << 16) | (Packet[6+Offset] << 8) | (Packet[7+Offset])) & 0xFFFFFFFF);
				t = t << 1;

				*lpOPCR = (t) | ((Packet[8+Offset] & 0x80) >> 7);
				return 1;
			 }
		 }
	}
	return 0;

}
int TSFile::SetOPCRInAdaptationField(LPBYTE Packet, __int64 OPCR)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if( ((Packet[3]&0x20)!=0) && (Packet[4]!=0))
		 {
			 if(Packet[5]&0x08)
			 {
				int Offset = 2;
				if(Packet[5]&0x10)
					Offset += 6;

				if(OPCR & 0xFFFFFFFFFFFFFFFE)
					Packet[8+Offset] = Packet[8+Offset] | 0x80;
				else
					Packet[8+Offset] = Packet[8+Offset] & 0x7F;
				OPCR = OPCR >> 1;
				char* pPCR = (char *) &OPCR;
				Packet[4+Offset] = pPCR[3];
				Packet[5+Offset] = pPCR[2];
				Packet[6+Offset] = pPCR[1];
				Packet[7+Offset] = pPCR[0];
				return 1;
			 }
		 }
	}
	return 0;
}
int TSFile::IsDiscontinuityIndicator(LPBYTE Packet)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if( ((Packet[3]&0x20)!=0) && (Packet[4]!=0))
		 {
			 if(Packet[5]&0x80)
			 {
				return 1;
			 }
		 }
	}
	return 0;
}
int TSFile::SetDiscontinuityIndicator(LPBYTE Packet)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if( ((Packet[3]&0x20)!=0) && (Packet[4]!=0))
		 {
			 WORD PID = MAKEWORD(Packet[2], Packet[1]&0x1F);
			 if(IsPIDDiscontinuitySet(PID) == false)
			 {
				Packet[5] = Packet[5]|0x80;
				SetPIDDiscontinuity(PID);
				return 1;
			}
		 }
	}
	return 0;
}
int TSFile::GetPCRInAdaptationField(LPBYTE Packet, __int64* lpPCR)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if( ((Packet[3]&0x20)!=0) && (Packet[4]!=0))
		 {
			 if(Packet[5]&0x10)
			 {
				__int64 pcr_base = (((Packet[6] << 24) | (Packet[7] << 16) | (Packet[8] << 8) | (Packet[9])) & 0xFFFFFFFF);
				pcr_base = pcr_base << 1;

				if((Packet[10] & 0x80)== 0x80)
					pcr_base = (pcr_base) | 0x000000001;
				else
					pcr_base = (pcr_base) & 0xFFFFFFFFE;

				__int64 pcr_ext =  ( ((Packet[10] & 0x01) == 0x01)? 0x100 : 0x00);
				pcr_ext = pcr_ext | Packet[11];

				*lpPCR = pcr_base*300 + pcr_ext;
				return 1;
			 }
		 }
	}
	return 0;
}
/*int TSFile::IsOPCRPresentInAdaptationField(LPBYTE Packet)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if((Packet[3]&0x20)!=0)
		 {
			 if(Packet[5]&0x08)
			 {
				 return 1;
			 }
		 }
	}
	return 0;
}
int TSFile::IsDTSPresentInAdaptationField(LPBYTE Packet)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if((Packet[3]&0x20)!=0)
		 {
			 if(Packet[5]&0x01)
			 {
				 int Offset = 2;
				 if(Packet[5]&0x10)
					 Offset += 6;
				 if(Packet[5]&0x08)
					 Offset += 6;
				 if(Packet[5]&0x04)
					 Offset += 1;
				 if(Packet[5]&0x02)
					 Offset += 2;
				 if(Packet[5]&0x01)
				 {
					 Offset += 2;
					 if(Packet[4 + Offset - 1] & 0x80)
						 Offset += 2;
					 if(Packet[4 + Offset - 1] & 0x40)
						 Offset += 3;
					 if(Packet[4 + Offset - 1] & 0x20)
					 {
						 return 1;
					 }
				 }

			 }
		 }
	}
	return 0;
}
*/
/*
int TSFile::GetPCR_PID(WORD& PCR_PID)
{
	WORD PID;
	PCR_PID=0xFFFF;
	if(Read(0)>0)
	{
		char* p = (char *)m_buffer_pointer;
		while(p!=NULL)
		{
			if(GetPCR_PID((LPBYTE)p, PID))
			{
				if(PCR_PID!=PID)
					PCR_PID=PID;
			}
			 if(p+PACKET_SIZE>(char *)m_buffer_pointer + m_buffer_length)
			 {

				 if(Read(m_buffer_index+ m_buffer_length/PACKET_SIZE)==0)
					p = NULL;
				 else
					p = (char *)m_buffer_pointer;
			 }
			 else
				 (char *)(p) += PACKET_SIZE; 	

		}
	}
	if(PCR_PID != 0xFFFF)
		return 1;
	return 0;
}*/

DWORD DumpHexBuffer(char *pTrace, DWORD TraceLen,BYTE *Buffer, DWORD Count)
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
		StringCchPrintf(Loc,sizeof(Loc)," %02X", *(Buffer+i));
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
				return 0;
			}
		}
	}
	if(i%16 != 0)
	{
		StringCchCat(pTrace,TraceLen,"   ");
		for(; (i % 16) != 15 ; i++ )
		{
			StringCchCat(pTrace,TraceLen, "   ");
		}
		StringCchCat(pTrace,TraceLen, Line);
		StringCchCat(pTrace,TraceLen,"\n");
	}
//	StringCchCat(pTrace, "\n");
//	DWORD LEn = lstrlen(pTrace);
//	OutputDebugString(pTrace);
	return 1;
}
/*
int TSFile::GetPCR_PID(LPBYTE Packet, WORD& PID)
{
	int Offset = 4;
	PID = 0xFFFF;
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		WORD locPID = MAKEWORD(Packet[2], Packet[1]&0x1F);
		//PAT Only
		if(locPID == 0)
		{
			if((Packet[3]&0x20)!=0)
				Offset += (Packet[4]+1);
			if((Packet[3]&0x10)!=0)
			{
				if( (Packet[Offset]==0x00) &&
					(Packet[Offset+1]==0x00) &&
					(Packet[Offset+2]==0x01))
				{
					//PES
				}
				else
				{
					if((Packet[1]&0x40)!=0) //paylod unit start indicator
											//pointer is there
						Offset += Packet[Offset]+1;

					WORD transport_stream_id = 0;
					BYTE section_number = 0;
					BYTE last_section_number = 0;
					WORD section_length = 0; 
					WORD network_pid = 0;
					WORD program_map_pid = 0;
					WORD program_number = 0;
					int i = 0;
					//PSI
					section_length = MAKEWORD(Packet[Offset+2], Packet[Offset+1]&0x0F);
					transport_stream_id = MAKEWORD(Packet[Offset+4], Packet[Offset+3]);
					switch (Packet[Offset])
					{
					case 0x00: //program association section
						section_number = Packet[Offset+6];
						last_section_number = Packet[Offset+7];

						while(8+i*4+4<section_length)
						{
							program_number = MAKEWORD(Packet[Offset+4*i+9], Packet[Offset+4*i+8]);
							if(program_number == 0)
								network_pid = MAKEWORD(Packet[Offset+4*i+11], Packet[Offset+4*i+10]&0x1F);
							else
								program_map_pid = MAKEWORD(Packet[Offset+4*i+11], Packet[Offset+4*i+10]&0x1F);
							i++;
						}
						break;
					case 0x02: //TS program map section
						PID = MAKEWORD(Packet[Offset+9], Packet[Offset+8]&0x1F);
						return 1;
						break;
					default:
						break;
					}


				}
			}
		}
	}

	return 0;
}
*/
int TSFile::GetDTSInPES(LPBYTE Packet, __int64* lpDTS)
{
	int Offset = 4;
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if((Packet[3]&0x20)!=0)
			Offset += (Packet[4]+1);
		if((Packet[3]&0x10)!=0)
		{
			if( (Packet[Offset]==0x00) &&
				(Packet[Offset+1]==0x00) &&
				(Packet[Offset+2]==0x01))
			{
				if( (Packet[Offset+3]!=0xBC) && //program stream map
					(Packet[Offset+3]!=0xBE) && // padding stream
					(Packet[Offset+3]!=0xBF) && // private stream
					(Packet[Offset+3]!=0xF0) && // ECM
					(Packet[Offset+3]!=0xF1) && // EMM
					(Packet[Offset+3]!=0xFF) && // program stream directroy
					(Packet[Offset+3]!=0xF2) && // DSMCC stream
					(Packet[Offset+3]!=0xF8))   // Type E stream
				{
					if((Packet[Offset+7]& 0xC0) == 0xC0)
					{
						//Packet[Offset+9] // PTS
						//Packet[Offset+14] // PTS
//						 __int64 t = ((Packet[14 + Offset]& 0x0E) << 29)&     0x00000001C0000000;
						 __int64 t = Packet[14 + Offset]& 0x0E;
						 t = (t << 29)&     0x00000001C0000000;
						 t |=        ((Packet[14 + Offset + 1]& 0xFF) << 22)& 0x000000003FC00000;
						 t |=        ((Packet[14 + Offset + 2]& 0xFE) << 14)& 0x00000000003F8000;
						 t |=        ((Packet[14 + Offset + 3]& 0xFF) << 7) & 0x0000000000007F80;
						 t |=        ((Packet[14 + Offset + 4]& 0xFE) >> 1) & 0x000000000000007F;
						*lpDTS = t;
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

int TSFile::SetDTSInPES(LPBYTE Packet, __int64 DTS)
{
	int Offset = 4;
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if((Packet[3]&0x20)!=0)
			Offset += (Packet[4]+1);
		if((Packet[3]&0x10)!=0)
		{
			if( (Packet[Offset]==0x00)&&
				(Packet[Offset+1]==0x00)&&
				(Packet[Offset+2]==0x01))
			{
				if( (Packet[Offset+3]!=0xBC) && //program stream map
					(Packet[Offset+3]!=0xBE) && // padding stream
					(Packet[Offset+3]!=0xBF) && // private stream
					(Packet[Offset+3]!=0xF0) && // ECM
					(Packet[Offset+3]!=0xF1) && // EMM
					(Packet[Offset+3]!=0xFF) && // program stream directroy
					(Packet[Offset+3]!=0xF2) && // DSMCC stream
					(Packet[Offset+3]!=0xF8))   // Type E stream
				{
					if((Packet[Offset+7]& 0xC0) == 0xC0)
					{
						 __int64 t = DTS >> 29;
						 Packet[14 + Offset] = (Packet[14 + Offset]&0xF1) | (0x0E & (BYTE)t);
						 t = DTS >> 22;
						 Packet[14 + Offset + 1] =  (0xFF & (BYTE)t);
						 t = DTS >> 14;
						 Packet[14 + Offset + 2] = (Packet[14 + Offset + 2]&0x01) | (0xFE & (BYTE)t);
						 t = DTS >> 7;
						 Packet[14 + Offset + 3] = (0xFF & (BYTE)t);
						 t = DTS << 1;
						 Packet[14 + Offset + 4] = (Packet[14 + Offset + 4]&0x01) | (0xFE & (BYTE)t);
						 return 1;
					}
				}
			}
		}
	}
	return 0;
}

int TSFile::GetPTSInPES(LPBYTE Packet, __int64* lpPTS)
{
	int Offset = 4;
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if((Packet[3]&0x20)!=0)
			Offset += (Packet[4]+1);
		if((Packet[3]&0x10)!=0)
		{
			if( (Packet[Offset]==0x00)&&
				(Packet[Offset+1]==0x00)&&
				(Packet[Offset+2]==0x01))
			{
				if( (Packet[Offset+3]!=0xBC) && //program stream map
					(Packet[Offset+3]!=0xBE) && // padding stream
					(Packet[Offset+3]!=0xBF) && // private stream
					(Packet[Offset+3]!=0xF0) && // ECM
					(Packet[Offset+3]!=0xF1) && // EMM
					(Packet[Offset+3]!=0xFF) && // program stream directroy
					(Packet[Offset+3]!=0xF2) && // DSMCC stream
					(Packet[Offset+3]!=0xF8))   // Type E stream
				{
					if((Packet[Offset+7]& 0x80) == 0x80)
					{
						//Packet[Offset+9] // PTS
						
//						 __int64 t = ((Packet[9 + Offset]& 0x0E) << 29)&     0x00000001C0000000;
						 __int64 t = (Packet[9 + Offset]& 0x0E);
						 t = (t << 29)&     0x00000001C0000000;
						 t |=        ((Packet[9 + Offset + 1]& 0xFF) << 22)& 0x000000003FC00000;
						 t |=        ((Packet[9 + Offset + 2]& 0xFE) << 14)& 0x00000000003F8000;
						 t |=        ((Packet[9 + Offset + 3]& 0xFF) << 7) & 0x0000000000007F80;
						 t |=        ((Packet[9 + Offset + 4]& 0xFE) >> 1) & 0x000000000000007F;
						*lpPTS = t;
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

int TSFile::SetPTSInPES(LPBYTE Packet, __int64 PTS)
{
	int Offset = 4;
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if((Packet[3]&0x20)!=0)
			Offset += (Packet[4]+1);
		if((Packet[3]&0x10)!=0)
		{
			if( (Packet[Offset]==0x00)&&
				(Packet[Offset+1]==0x00)&&
				(Packet[Offset+2]==0x01))
			{
				if( (Packet[Offset+3]!=0xBC) && //program stream map
					(Packet[Offset+3]!=0xBE) && // padding stream
					(Packet[Offset+3]!=0xBF) && // private stream
					(Packet[Offset+3]!=0xF0) && // ECM
					(Packet[Offset+3]!=0xF1) && // EMM
					(Packet[Offset+3]!=0xFF) && // program stream directroy
					(Packet[Offset+3]!=0xF2) && // DSMCC stream
					(Packet[Offset+3]!=0xF8))   // Type E stream
				{
					if((Packet[Offset+7]& 0x80) == 0x80)
					{
						 __int64 t = PTS >> 29;
						 Packet[9 + Offset] = (Packet[9 + Offset]&0xF1 ) | (0x0E & (BYTE)t);
						 t = PTS >> 22;
						 Packet[9 + Offset + 1] =  (0xFF & (BYTE)t);
						 t = PTS >> 14;
						 Packet[9 + Offset + 2] = (Packet[9 + Offset + 2]&0x01 ) | (0xFE & (BYTE)t);
						 t = PTS >> 7;
						 Packet[9 + Offset + 3] =  (0xFF & (BYTE)t);
						 t = PTS << 1;
						 Packet[9 + Offset + 4] = (Packet[9 + Offset + 4]&0x01 ) | (0xFE & (BYTE)t);
						 return 1;

					}
				}
			}
		}
	}
	return 0;
}

int TSFile::GetRAIInAdaptationField(LPBYTE Packet)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if( ((Packet[3]&0x20)!=0) && (Packet[4]!=0))
		 {
//			 if(Packet[5]&0x01)
			 {
				 if(Packet[5]&0x40)
					 return 1;
			 }
		}
	}
	return 0;
}
int TSFile::GetDTSInAdaptationField(LPBYTE Packet, __int64* lpDTS)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if( ((Packet[3]&0x20)!=0) && (Packet[4]!=0))
		 {
			 if(Packet[5]&0x01)
			 {
				 int Offset = 2;
				 if(Packet[5]&0x10)
					 Offset += 6;
				 if(Packet[5]&0x08)
					 Offset += 6;
				 if(Packet[5]&0x04)
					 Offset += 1;
				 if(Packet[5]&0x02)
					 Offset += 2;
				 if(Packet[5]&0x01)
				 {
					 Offset += 2;
					 if(Packet[4 + Offset - 1] & 0x80)
						 Offset += 2;
					 if(Packet[4 + Offset - 1] & 0x40)
						 Offset += 3;
					 if(Packet[4 + Offset - 1] & 0x20)
					 {
//						 __int64 t = ((Packet[4 + Offset]& 0x0E) << 29)&     0x00000001C0000000;
						 __int64 t = Packet[4 + Offset]& 0x0E;
						 t = (t << 29)&     0x00000001C0000000;
						 t |=        ((Packet[4 + Offset + 1]& 0xFF) << 22)& 0x000000003FC00000;
						 t |=        ((Packet[4 + Offset + 2]& 0xFE) << 14)& 0x00000000003F8000;
						 t |=        ((Packet[4 + Offset + 3]& 0xFF) << 7) & 0x0000000000007F80;
						 t |=        ((Packet[4 + Offset + 4]& 0xFE) >> 1) & 0x000000000000007F;
						*lpDTS = t;
						 return 1;
					 }
				 }

			 }
		 }
	}
	return 0;
}
int TSFile::SetDTSInAdaptationField(LPBYTE Packet, __int64 DTS)
{
	if((Packet[0]==0x47)&& ((Packet[1]&0x80)==0))
	{
		if( ((Packet[3]&0x20)!=0) && (Packet[4]!=0))
		 {
			 if(Packet[5]&0x01)
			 {
				 int Offset = 2;
				 if(Packet[5]&0x10)
					 Offset += 6;
				 if(Packet[5]&0x08)
					 Offset += 6;
				 if(Packet[5]&0x04)
					 Offset += 1;
				 if(Packet[5]&0x02)
					 Offset += 2;
				 if(Packet[5]&0x01)
				 {
					 Offset += 2;
					 if(Packet[4 + Offset - 1] & 0x80)
						 Offset += 2;
					 if(Packet[4 + Offset - 1] & 0x40)
						 Offset += 3;
					 if(Packet[4 + Offset - 1] & 0x20)
					 {
						 __int64 t = DTS >> 29;
						 Packet[4 + Offset] = (Packet[4 + Offset] & 0xF1)| (0x0E & (BYTE)t);
						 t = DTS >> 22;
						 Packet[4 + Offset + 1] = (0xFF & (BYTE)t);
						 t = DTS >> 14;
						 Packet[4 + Offset + 2] = (Packet[4 + Offset + 2] &0x01 )| (0xFE & (BYTE)t);
						 t = DTS >> 7;
						 Packet[4 + Offset + 3] =  (0xFF & (BYTE)t);
						 t = DTS << 1;
						 Packet[4 + Offset + 4] = (Packet[4 + Offset + 4]  &0x01 )| (0xFE & (BYTE)t);
						 return 1;
					 }
				 }

			 }
		 }
	}
	return 0;
}
/*
int TSFile::ParsePID(
	PARSE_COUNTERS* pCounter
)
{
	if(pCounter)
	{
		pCounter->PID = 0xFFFF;
		WORD PID ;
		DWORD CurrentIndex = 0;
		while(Read(CurrentIndex)>0)
		{
			LPBYTE p = m_buffer_pointer;


			while ((DWORD)(p-m_buffer_pointer)<m_buffer_length)
			{

				if(p[0]==0x47)
				{
					if((p[1]&0x80)==0)
					{
						PID = MAKEWORD(p[2], p[1]&0x1F);
						for(int i = 0; i < sizeof(pCounter->TablePID)/sizeof(PARSE_PID_COUNTERS); i++)
						{
							if(pCounter->TablePID[i].PID == PID)
							{
								pCounter->TablePID[i].Counter++;
								break;
							}
							if(pCounter->TablePID[i].PID == 0xFFFF) 
							{
								pCounter->TablePID[i].PID = PID;
								pCounter->TablePID[i].Counter++;
								break;
							}
						}
					}
				}
				p += PACKET_SIZE;
			}
			CurrentIndex += m_buffer_length/PACKET_SIZE; 
		}
		return 1;
	}
	return 0;
}

*/
/*
int TSFile::Parse(
	Parse_Mask Mask,
	PARSE_COUNTERS* pCounter,
	long packetStart,long packetEnd, int pid
)
{

	if(pCounter)
	{
		pCounter->bError = false;

		pCounter->bitrate = 0;
		pCounter->duration = 0;
		for(int i = 0; i < sizeof(pCounter->TablePID)/sizeof(PARSE_PID_COUNTERS);i++)
		{
			pCounter->TablePID[i].PID = 0xFFFF;
			for (int k=0; k<MAX_TIME_STAMP;k++)
			{
				pCounter->TablePID[i].Counters[k].TimeStampPacketCount = 0;

				pCounter->TablePID[i].Counters[k].IndexFirstTimeStampPacket = -1;
				pCounter->TablePID[i].Counters[k].IndexLastTimeStampPacket = -1;

				pCounter->TablePID[i].Counters[k].FirstTimeStamp = -1;
				pCounter->TablePID[i].Counters[k].LastTimeStamp = -1;


				pCounter->TablePID[i].Counters[k].MinPacketBetweenTimeStamp = -1;
				pCounter->TablePID[i].Counters[k].MaxPacketBetweenTimeStamp = -1;

				for(int j = 0; j < sizeof(pCounter->TablePID[i].Counters[k].TimeStampPID)/sizeof(WORD);j++)
						pCounter->TablePID[i].Counters[k].TimeStampPID[j] = 0xFFFF;

				pCounter->TablePID[i].Counters[k].TimeStampDiscontinuity = 0;
				pCounter->TablePID[i].Counters[k].DiscontinuityIndicator = 0;

				pCounter->TablePID[i].Counters[k].MinTimeStampBitrate = -1;
				pCounter->TablePID[i].Counters[k].MaxTimeStampBitrate = -1;
			}
		}

		//ParsePID(pCounter);



		for(int i = 0; i < sizeof(pCounter->TablePID)/sizeof(PARSE_PID_COUNTERS);i++)
		{
			if(pCounter->TablePID[i].PID != 0xFFFF)
			{

				// parsing PCR, OPCR, PTS and DTS for each PID
				WORD PID ;
				DWORD CurrentIndex = 0;
				pCounter->PacketCount = 0;

				while(Read(CurrentIndex)>0)
				{

					LPBYTE p = m_buffer_pointer;
					

					while ((DWORD)(p-m_buffer_pointer)<m_buffer_length)
					{
						if(p[0]==0x47)
						{
							if((p[1]&0x80)==0)
							{
								PID = MAKEWORD(p[2], p[1]&0x1F);
								if(pCounter->TablePID[i].PID == PID)
								{
									__int64 TimeStamp[MAX_TIME_STAMP];

									for(int k=0;k<MAX_TIME_STAMP;k++)
										TimeStamp[k] = -1;
									// if adaption field
									if((p[3]&0x20)!=0)
									{
    									if(GetPCRInAdaptationField(p,&TimeStamp[TIME_STAMP_PCR]))
											pCounter->TablePID[i].Counters[TIME_STAMP_PCR].TimeStampPacketCount++;
										if(IsDiscontinuityIndicator(p))
											pCounter->TablePID[i].Counters[TIME_STAMP_PCR].DiscontinuityIndicator++;
										if(GetOPCRInAdaptationField(p,&TimeStamp[TIME_STAMP_OPCR]))
											pCounter->TablePID[i].Counters[TIME_STAMP_OPCR].TimeStampPacketCount++;
										if(GetDTSInAdaptationField(p,&TimeStamp[TIME_STAMP_DTS]))
											pCounter->TablePID[i].Counters[TIME_STAMP_DTS].TimeStampPacketCount++;
									}
									// if payload
									if((p[3]&0x10)!=0)
									{
										if(TimeStamp[TIME_STAMP_DTS] == -1)
											if(GetDTSInPES(p,&TimeStamp[TIME_STAMP_DTS]))
												pCounter->TablePID[i].Counters[TIME_STAMP_DTS].TimeStampPacketCount++;
										if(GetPTSInPES(p,&TimeStamp[TIME_STAMP_PTS]))
											pCounter->TablePID[i].Counters[TIME_STAMP_PTS].TimeStampPacketCount++;
									}

				//					if((PCR!=-1)||(OPCR!=-1)||(DTS!=-1)||(PTS!=-1))  
									for(int k = 0; k < MAX_TIME_STAMP; k++)
									{
										if(TimeStamp[k]!=-1)
										{
											for(int j = 0; j < sizeof(pCounter->TablePID[i].Counters[k].TimeStampPID)/sizeof(WORD); j++)
											{
												if(pCounter->TablePID[i].Counters[k].TimeStampPID[j]==PID) 
													break;
												if(pCounter->TablePID[i].Counters[k].TimeStampPID[j]==0xFFFF) 
												{
													pCounter->TablePID[i].Counters[k].TimeStampPID[j]=PID;
													break;
												}
											}

											if(pCounter->TablePID[i].Counters[k].FirstTimeStamp == -1)
											{
												pCounter->TablePID[i].Counters[k].LastTimeStamp = pCounter->TablePID[i].Counters[k].FirstTimeStamp = TimeStamp[k];
												pCounter->TablePID[i].Counters[k].IndexLastTimeStampPacket = pCounter->TablePID[i].Counters[k].IndexFirstTimeStampPacket = pCounter->PacketCount;
											}
											else
											{
												DWORD DeltaIndex = pCounter->PacketCount - pCounter->TablePID[i].Counters[k].IndexLastTimeStampPacket;
												if((pCounter->TablePID[i].Counters[k].MaxPacketBetweenTimeStamp == -1) &&
													(pCounter->TablePID[i].Counters[k].MinPacketBetweenTimeStamp == -1))
												{
													pCounter->TablePID[i].Counters[k].MaxPacketBetweenTimeStamp = DeltaIndex;
													pCounter->TablePID[i].Counters[k].MinPacketBetweenTimeStamp = DeltaIndex;
												}

												if((DeltaIndex)> pCounter->TablePID[i].Counters[k].MaxPacketBetweenTimeStamp)
													pCounter->TablePID[i].Counters[k].MaxPacketBetweenTimeStamp = DeltaIndex;
												if((DeltaIndex)< pCounter->TablePID[i].Counters[k].MinPacketBetweenTimeStamp)
													pCounter->TablePID[i].Counters[k].MinPacketBetweenTimeStamp = DeltaIndex;


												if(TimeStamp[k] < pCounter->TablePID[i].Counters[k].LastTimeStamp)
												{
													pCounter->TablePID[i].Counters[k].TimeStampDiscontinuity++;
													if(IsDiscontinuityIndicator(p))
														pCounter->TablePID[i].Counters[k].TimeStampDiscontinuity--;
													if(bFix==true)
													{
														__int64 NextTimeStamp=-1;
														switch(k)
														{
															case TIME_STAMP_PCR:
																GetNextPCR((char* )p,&NextTimeStamp,PID);
																break;

															case TIME_STAMP_DTS:
																GetNextDTS((char* )p,&NextTimeStamp,PID);
																break;
															case TIME_STAMP_PTS:
																GetNextPTS((char* )p,&NextTimeStamp,PID);
																break;
															default:
																break; 
														}
														if(NextTimeStamp!=-1)
														{
															if(NextTimeStamp >= pCounter->TablePID[i].Counters[k].LastTimeStamp)
															{
																TimeStamp[k] = pCounter->TablePID[i].Counters[k].LastTimeStamp + (NextTimeStamp - pCounter->TablePID[i].Counters[k].LastTimeStamp)/2;
																switch(k)
																{
																	case TIME_STAMP_PCR:
																		SetPCRInAdaptationField(p,TimeStamp[k]);
																		break;

																	case TIME_STAMP_DTS:
																		SetDTSInAdaptationField(p,TimeStamp[k]);
																		SetDTSInPES(p,TimeStamp[k]);
																		break;
																	case TIME_STAMP_PTS:
																		SetPTSInPES(p,TimeStamp[k]);
																		break;
																	default:
																		break; 
																}													
																pCounter->TablePID[i].Counters[k].TimeStampFixed++;
															}
														}

													}
												}
												else
												{
													__int64 Delta;
													DWORD DeltaIndex = pCounter->PacketCount - pCounter->TablePID[i].Counters[k].IndexLastTimeStampPacket;

													if((TimeStamp[k]-pCounter->TablePID[i].Counters[k].LastTimeStamp)>=0)
													{
														Delta = TimeStamp[k] - pCounter->TablePID[i].Counters[k].LastTimeStamp;
														// Delta < 1 second
														// fixme
														if(Delta<900000)
														{
															if((DeltaIndex!=0) && (Delta!=0))
															{
																double bitrate = ((DeltaIndex)*PACKET_SIZE*8);
																bitrate = bitrate/Delta;
																bitrate = bitrate*90000;
																
																//(DWORD) ((Delta*PACKET_SIZE*8)/((PCR-LastPCR)/90000));
																if( (pCounter->TablePID[i].Counters[k].MinTimeStampBitrate == -1)&&
																	(pCounter->TablePID[i].Counters[k].MaxTimeStampBitrate == -1))
																{
																	pCounter->TablePID[i].Counters[k].MinTimeStampBitrate  = (DWORD)bitrate;
																	pCounter->TablePID[i].Counters[k].MaxTimeStampBitrate  = (DWORD)bitrate;
																}

																if(bitrate<pCounter->TablePID[i].Counters[k].MinTimeStampBitrate)
																	pCounter->TablePID[i].Counters[k].MinTimeStampBitrate  = (DWORD)bitrate;
																if(bitrate>pCounter->TablePID[i].Counters[k].MaxTimeStampBitrate)
																	pCounter->TablePID[i].Counters[k].MaxTimeStampBitrate  = (DWORD)bitrate;
															}
														}
														else
														{
															pCounter->TablePID[i].Counters[k].TimeStampDiscontinuity++;
															if(IsDiscontinuityIndicator(p))
																pCounter->TablePID[i].Counters[k].TimeStampDiscontinuity--;

															if(bFix==true)
															{
																__int64 NextTimeStamp=-1;
																switch(k)
																{
																	case TIME_STAMP_PCR:
																		GetNextPCR((char* )p,&NextTimeStamp,PID);
																		break;

																	case TIME_STAMP_DTS:
																		GetNextDTS((char* )p,&NextTimeStamp,PID);
																		break;
																	case TIME_STAMP_PTS:
																		GetNextPTS((char* )p,&NextTimeStamp,PID);
																		break;
																	default:
																		break; 
																}
																if(NextTimeStamp !=-1)
																{
																	Delta = NextTimeStamp - pCounter->TablePID[i].Counters[k].LastTimeStamp;
																	if(Delta<180000)
																	{
																		TimeStamp[k] = pCounter->TablePID[i].Counters[k].LastTimeStamp + (NextTimeStamp - pCounter->TablePID[i].Counters[k].LastTimeStamp)/2;
																		switch(k)
																		{
																			case TIME_STAMP_PCR:
																				SetPCRInAdaptationField(p,TimeStamp[k]);
																				break;

																			case TIME_STAMP_DTS:
																				SetDTSInAdaptationField(p,TimeStamp[k]);
																				SetDTSInPES(p,TimeStamp[k]);
																				break;
																			case TIME_STAMP_PTS:
																				SetPTSInPES(p,TimeStamp[k]);
																				break;
																			default:
																				break; 
																		}													
																		pCounter->TablePID[i].Counters[k].TimeStampFixed++;
																	}
																}
															}
														}
													}
												}
												pCounter->TablePID[i].Counters[k].IndexLastTimeStampPacket = pCounter->PacketCount;
												pCounter->TablePID[i].Counters[k].LastTimeStamp = TimeStamp[k];
											}
										}
									}
								}
							}
						}
						else
						{
							ostringstream oss;
							oss << "Error: Invalid file, TS opening flag not found " << endl;
							pCounter->bError = true;
							return 0;
						}
						pCounter->PacketCount++;
						p += PACKET_SIZE;
					}
					CurrentIndex += m_buffer_length/PACKET_SIZE; 
				}
			}
		}
		for(int i = 0; i < sizeof(pCounter->TablePID)/sizeof(PARSE_PID_COUNTERS);i++)
		{
			if(pCounter->TablePID[i].PID != 0xFFFF)
			{
				if(pCounter->TablePID[i].Counters[TIME_STAMP_PCR].TimeStampPacketCount>0)
				{
					__int64 Delta = 0;
					if((pCounter->TablePID[i].Counters[TIME_STAMP_PCR].LastTimeStamp-pCounter->TablePID[i].Counters[TIME_STAMP_PCR].FirstTimeStamp)>=0)
						Delta = pCounter->TablePID[i].Counters[TIME_STAMP_PCR].LastTimeStamp - pCounter->TablePID[i].Counters[TIME_STAMP_PCR].FirstTimeStamp;
					else
						Delta = 0x200000000 - (pCounter->TablePID[i].Counters[TIME_STAMP_PCR].FirstTimeStamp - pCounter->TablePID[i].Counters[TIME_STAMP_PCR].LastTimeStamp);
					double bitrate = (pCounter->TablePID[i].Counters[TIME_STAMP_PCR].IndexLastTimeStampPacket- pCounter->TablePID[i].Counters[TIME_STAMP_PCR].IndexFirstTimeStampPacket)*PACKET_SIZE*8;
					bitrate = bitrate / Delta;
					bitrate = bitrate * 90000;
					if(bitrate > pCounter->bitrate)
					{
						pCounter->bitrate = bitrate ;
					
						pCounter->duration = (double)(pCounter->PacketCount*8*PACKET_SIZE);
						pCounter->duration = pCounter->duration/pCounter->bitrate;
					}
				}
			}
			else
				break;
		}
	}

	if( ( (int)(Mask & ts) == (int)ts) ||
		( (int)(Mask & timestamp ) == (int)timestamp))
	{
		LPBYTE p = m_buffer_pointer ;
		WORD PID ;
		pCounter->PacketCount = 0;
		DWORD PacketCount = 0;
		char bufferDump[PACKET_SIZE*5];
		DWORD len;
		DWORD end = m_file_length;
		if(packetEnd == -1)
			end = m_file_length;
		else
		{
			end = (packetEnd+1)*PACKET_SIZE;
			if(end > m_file_length)
				end = m_file_length;
		}

		while ((DWORD)(p-m_buffer_pointer)<m_file_length)
		{
			if(p[0]==0x47)
			{
				if((p>=m_buffer_pointer+packetStart*PACKET_SIZE)&&
					((DWORD)(p-m_buffer_pointer)<end))
				{
					if((p[1]&0x80)==0)
					{
						 __int64 l;
						if( ( (int)(Mask & ts) == (int)ts) ||
							 GetPCRInAdaptationField(p,&l) || 
							 GetOPCRInAdaptationField(p,&l) ||
							 GetDTSInAdaptationField(p,&l) ||
							 GetDTSInPES(p,&l) ||
							 GetPTSInPES(p,&l) )


						{
							 bool transport_error = ( p[1]&0x80? true:false) ;
							 bool payload_unit_start_indicator= ( p[1]&0x40? true:false) ;
							 bool transport_priority = ( p[1]&0x20? true:false) ; 
							 
							 BYTE scrambling_control= ( p[3]&0xC0 )>>6;
							 BYTE adaption_field_control=( p[3]&0x30 )>>4;
							 BYTE Continuity_Counter =( p[3]&0x0F );
							 ostringstream oss;
							 string s;
  	 						 PID = MAKEWORD(p[2], p[1]&0x1F);
							 if( (pid==-1) || (pid == PID))
							 {
								 oss << endl << "Packet : " << PacketCount << endl;
								 oss << "transport_error                 :"<< transport_error << endl;
								 oss << "payload_unit_start_indicator    :"<< payload_unit_start_indicator << endl;
								 oss << "transport_priority              :"<< transport_priority << endl;
								 oss << "PID                             :"<< PID << endl;
								 oss << "scrambling_control              :"<< (int) scrambling_control << endl;
								 oss << "adaption_field_control          :"<< (int) adaption_field_control << endl;
								 oss << "Continuity_Counter              :"<< (int) Continuity_Counter  << endl;
								 if( (int)(Mask & timestamp ) == (int)timestamp)
								 {
									 if(GetPCRInAdaptationField(p,&l))
										 oss << "PCR                             :"<< (int) l  << endl;
									 if(GetOPCRInAdaptationField(p,&l))
										 oss << "OPCR                            :"<< (int) l  << endl;
									 if(GetDTSInAdaptationField(p,&l))
										 oss << "DTS in Adaptation field         :"<< (int) l   << endl;
									 if(GetDTSInPES(p,&l))
										 oss << "DTS in PES                      :"<< (int) l   << endl;
									 if(GetPTSInPES(p,&l))
										 oss << "PTS in PES                      :"<< (int) l   << endl;
								 }						 
								 len = sizeof(bufferDump);
								 DumpHexBuffer(bufferDump, len,p, PACKET_SIZE);
								 oss  << bufferDump << endl;
 								 cout << oss.str().c_str();
							 }
							 Sleep(1);
						 }
					 }
				}
			}

			PacketCount++;
			p += PACKET_SIZE;
		}
	}
	return 1;

}
	*/
/*
int TSFile::FixTimeStamps(PARSE_COUNTERS* pCounter)
{
	return Parse( none,pCounter,true);
}*/

/*
	if(pCounter)
	{
		pCounter->bError = false;

		pCounter->PacketCount = 0;

		for (int k=0; k<MAX_TIME_STAMP;k++)
		{
			pCounter->Counter[k].TimeStampPacketCount = 0;

			pCounter->Counter[k].IndexFirstTimeStampPacket = -1;
			pCounter->Counter[k].IndexLastTimeStampPacket = -1;

			pCounter->Counter[k].FirstTimeStamp = -1;
			pCounter->Counter[k].LastTimeStamp = -1;


			pCounter->Counter[k].MinPacketBetweenTimeStamp = -1;
			pCounter->Counter[k].MaxPacketBetweenTimeStamp = -1;

			for(int i = 0; i < sizeof(pCounter->Counter[k].TimeStampPID)/sizeof(WORD);i++)
					pCounter->Counter[k].TimeStampPID[i] = 0xFFFF;

			pCounter->Counter[k].TimeStampDiscontinuity = 0;
			pCounter->Counter[k].TimeStampFixed = 0;

			pCounter->Counter[k].MinTimeStampBitrate = -1;
			pCounter->Counter[k].MaxTimeStampBitrate = -1;
		}
		LPBYTE p = m_buffer_pointer;
		WORD PID ;


		while ((DWORD)(p-m_buffer_pointer)<m_file_length)
		{

			if(p[0]==0x47)
			{
				if((p[1]&0x80)==0)
				{
					PID = MAKEWORD(p[2], p[1]&0x1F);
					__int64 TimeStamp[MAX_TIME_STAMP];

					for(int k=0;k<MAX_TIME_STAMP;k++)
						TimeStamp[k] = -1;
					// if adaption field
					if((p[3]&0x20)!=0)
					{
    					if(GetPCRInAdaptationField(p,&TimeStamp[TIME_STAMP_PCR]))
							pCounter->Counter[TIME_STAMP_PCR].TimeStampPacketCount++;
						if(GetOPCRInAdaptationField(p,&TimeStamp[TIME_STAMP_OPCR]))
							pCounter->Counter[TIME_STAMP_OPCR].TimeStampPacketCount++;
						if(GetDTSInAdaptationField(p,&TimeStamp[TIME_STAMP_DTS]))
							pCounter->Counter[TIME_STAMP_DTS].TimeStampPacketCount++;
					}
					// if payload
					if((p[3]&0x10)!=0)
					{
						if(TimeStamp[TIME_STAMP_DTS] == -1)
							if(GetDTSInPES(p,&TimeStamp[TIME_STAMP_DTS]))
								pCounter->Counter[TIME_STAMP_DTS].TimeStampPacketCount++;
						if(GetPTSInPES(p,&TimeStamp[TIME_STAMP_PTS]))
							pCounter->Counter[TIME_STAMP_PTS].TimeStampPacketCount++;
					}

//					if((PCR!=-1)||(OPCR!=-1)||(DTS!=-1)||(PTS!=-1))  
					for(int k = 0; k < MAX_TIME_STAMP; k++)
					{
						if(TimeStamp[k]!=-1)
						{
							for(int i = 0; i < sizeof(pCounter->Counter[k].TimeStampPID)/sizeof(WORD); i++)
							{
								if(pCounter->Counter[k].TimeStampPID[i]==PID) 
									break;
								if(pCounter->Counter[k].TimeStampPID[i]==0xFFFF) 
								{
									pCounter->Counter[k].TimeStampPID[i]=PID;
									break;
								}
							}

							if(pCounter->Counter[k].FirstTimeStamp == -1)
							{
								pCounter->Counter[k].LastTimeStamp = pCounter->Counter[k].FirstTimeStamp = TimeStamp[k];
								pCounter->Counter[k].IndexLastTimeStampPacket = pCounter->Counter[k].IndexFirstTimeStampPacket = pCounter->PacketCount;
							}
							else
							{
								DWORD DeltaIndex = pCounter->PacketCount - pCounter->Counter[k].IndexLastTimeStampPacket;
								if((pCounter->Counter[k].MaxPacketBetweenTimeStamp == -1) &&
									(pCounter->Counter[k].MinPacketBetweenTimeStamp == -1))
								{
									pCounter->Counter[k].MaxPacketBetweenTimeStamp = DeltaIndex;
									pCounter->Counter[k].MinPacketBetweenTimeStamp = DeltaIndex;
								}

								if((DeltaIndex)> pCounter->Counter[k].MaxPacketBetweenTimeStamp)
									pCounter->Counter[k].MaxPacketBetweenTimeStamp = DeltaIndex;
								if((DeltaIndex)< pCounter->Counter[k].MinPacketBetweenTimeStamp)
									pCounter->Counter[k].MinPacketBetweenTimeStamp = DeltaIndex;


								if(TimeStamp[k] < pCounter->Counter[k].LastTimeStamp)
								{
									pCounter->Counter[k].TimeStampDiscontinuity++;
									
									__int64 NextTimeStamp=-1;
									switch(k)
									{
										case TIME_STAMP_PCR:
											GetNextPCR((char* )p,&NextTimeStamp);
											break;
										case TIME_STAMP_OPCR:
											GetNextOPCR((char* )p,&NextTimeStamp);
											break;
										case TIME_STAMP_DTS:
											GetNextDTS((char* )p,&NextTimeStamp);
											break;
										case TIME_STAMP_PTS:
											GetNextPTS((char* )p,&NextTimeStamp);
											break;
										default:
											break; 
									}
									if(NextTimeStamp!=-1)
									{
										if(NextTimeStamp < pCounter->Counter[k].LastTimeStamp)
											pCounter->Counter[k].TimeStampDiscontinuity++;
										else
										{
											TimeStamp[k] = pCounter->Counter[k].LastTimeStamp + (NextTimeStamp - pCounter->Counter[k].LastTimeStamp)/2;
											switch(k)
											{
												case TIME_STAMP_PCR:
													SetPCRInAdaptationField(p,TimeStamp[k]);
													break;
												case TIME_STAMP_OPCR:
													SetOPCRInAdaptationField(p,TimeStamp[k]);
													break;
												case TIME_STAMP_DTS:
													SetDTSInAdaptationField(p,TimeStamp[k]);
													SetDTSInPES(p,TimeStamp[k]);
													break;
												case TIME_STAMP_PTS:
													SetPTSInPES(p,TimeStamp[k]);
													break;
												default:
													break; 
											}													
											pCounter->Counter[k].TimeStampFixed++;
										}
									}
									
								}
								else
								{
									__int64 Delta;
									DWORD DeltaIndex = pCounter->PacketCount - pCounter->Counter[k].IndexLastTimeStampPacket;

									if((TimeStamp[k]-pCounter->Counter[k].LastTimeStamp)>=0)
									{
										Delta = TimeStamp[k] - pCounter->Counter[k].LastTimeStamp;
										// Delta < 1 second
										if(Delta<90000)
										{
											if((DeltaIndex!=0) && (Delta!=0))
											{
												double bitrate = ((DeltaIndex)*PACKET_SIZE*8);
												bitrate = bitrate/Delta;
												bitrate = bitrate*90000;
												
												//(DWORD) ((Delta*PACKET_SIZE*8)/((PCR-LastPCR)/90000));
												if( (pCounter->Counter[k].MinTimeStampBitrate == -1)&&
													(pCounter->Counter[k].MaxTimeStampBitrate == -1))
												{
													pCounter->Counter[k].MinTimeStampBitrate  = (DWORD)bitrate;
													pCounter->Counter[k].MaxTimeStampBitrate  = (DWORD)bitrate;
												}

												if(bitrate<pCounter->Counter[k].MinTimeStampBitrate)
													pCounter->Counter[k].MinTimeStampBitrate  = (DWORD)bitrate;
												if(bitrate>pCounter->Counter[k].MaxTimeStampBitrate)
													pCounter->Counter[k].MaxTimeStampBitrate  = (DWORD)bitrate;
											}
										}
										else
										{
											pCounter->Counter[k].TimeStampDiscontinuity++;
											
											__int64 NextTimeStamp=-1;
											switch(k)
											{
												case TIME_STAMP_PCR:
													GetNextPCR((char* )p,&NextTimeStamp);
													break;
												case TIME_STAMP_OPCR:
													GetNextOPCR((char* )p,&NextTimeStamp);
													break;
												case TIME_STAMP_DTS:
													GetNextDTS((char* )p,&NextTimeStamp);
													break;
												case TIME_STAMP_PTS:
													GetNextPTS((char* )p,&NextTimeStamp);
													break;
												default:
													break; 
											}
											if(NextTimeStamp !=-1)
											{
												Delta = NextTimeStamp - pCounter->Counter[k].LastTimeStamp;
												if(Delta<180000)
												{
													TimeStamp[k] = pCounter->Counter[k].LastTimeStamp + (NextTimeStamp - pCounter->Counter[k].LastTimeStamp)/2;
													switch(k)
													{
														case TIME_STAMP_PCR:
															SetPCRInAdaptationField(p,TimeStamp[k]);
															break;
														case TIME_STAMP_OPCR:
															SetOPCRInAdaptationField(p,TimeStamp[k]);
															break;
														case TIME_STAMP_DTS:
															SetDTSInAdaptationField(p,TimeStamp[k]);
															SetDTSInPES(p,TimeStamp[k]);
															break;
														case TIME_STAMP_PTS:
															SetPTSInPES(p,TimeStamp[k]);
															break;
														default:
															break; 
													}													
													pCounter->Counter[k].TimeStampFixed++;
												}
												else
													pCounter->Counter[k].TimeStampDiscontinuity++;
											}
											
										}
									}
								}
								pCounter->Counter[k].IndexLastTimeStampPacket = pCounter->PacketCount;
								pCounter->Counter[k].LastTimeStamp = TimeStamp[k];
							}
						}
					}
				}
			}
			else
			{
				ostringstream oss;
				oss << "Error: Invalid file, TS opening flag not found " << endl;
				pCounter->bError = true;
				return 0;
			}
			pCounter->PacketCount++;
			p += PACKET_SIZE;
		}
	}

	return 1;

}*/
/*
int TSFile::UpdateTimeStamps(char* p,__int64 delta)
{
	if(m_buffer_pointer)
	{
		char* Packet = (char *)p;
		__int64 PCR ;
		
*/
//		DWORD Len;
//		if(m_end == 0xFFFFFFFF)
//			Len = m_file_length;
//		else
//			Len = m_end*PACKET_SIZE;

// flecoqui 21/12/2006
//		bool bDisc = AreAllPIDDiscontinuitySet();

//		unsigned char counter = 0;
//		GetContinuityCounter((LPBYTE)Packet,&counter);
//		unsigned char delta = ++ContinuityCounter - counter;
/*
		while ( ((DWORD)(Packet-(char *)p)<(DWORD)(m_step*PACKET_SIZE)) &&
			((DWORD)(Packet-(char *)m_buffer_pointer)<m_buffer_length))
		{
		*/
// flecoqui 21/12/2006
//			SetContinuityCounter((LPBYTE)Packet,delta)
//			if(bDisc!=true) (
//				SetDiscontinuityIndicator((LPBYTE)Packet);
/*			
			UpdateContinuityCounter((LPBYTE)Packet);
			if(GetPCRInAdaptationField((LPBYTE)Packet,&PCR))
			{
				PCR += delta;
				SetPCRInAdaptationField((LPBYTE)Packet,PCR);
			}
			*/

		/*	if(GetOPCRInAdaptationField((LPBYTE)Packet,&PCR))
			{
				PCR += delta;
				SetOPCRInAdaptationField((LPBYTE)Packet,PCR);
			}
			*/
/*
			if(GetDTSInAdaptationField((LPBYTE)Packet,&PCR))
			{
				PCR += delta;
				SetDTSInAdaptationField((LPBYTE)Packet,PCR);
			}
			if(GetDTSInPES((LPBYTE)Packet,&PCR))
			{
				PCR += delta;
				SetDTSInPES((LPBYTE)Packet,PCR);
			}
			if(GetPTSInPES((LPBYTE)Packet,&PCR))
			{
				PCR += delta;
				SetPTSInPES((LPBYTE)Packet,PCR);
			}
			Packet += PACKET_SIZE;
		}
		return 1;
	}
	return 0;
}
*/
bool CheckTimestampAdvancesPCR(__int64 NewTimestamp, __int64 OldTimestamp)
{
		__int64 delta = 0;

		if (((NewTimestamp & 0x20000000000) == 0) && ((OldTimestamp & 0x20000000000) != 0))
			delta = (0x257FFFFFFFF - OldTimestamp) + NewTimestamp;
		else
			delta = NewTimestamp - OldTimestamp;
		if (delta < 0)
			return false;
		return true;
}
bool CheckTimestampAdvances(__int64 NewTimestamp, __int64 OldTimestamp)
{
		__int64 delta = 0;

		if (((NewTimestamp & 0x100000000) == 0) && ((OldTimestamp & 0x100000000) != 0))
		{
			// Wrap case.
			delta = (0x1FFFFFFFF - OldTimestamp) + NewTimestamp;
		}
		else
		{
			delta = NewTimestamp - OldTimestamp;
		}
		if (delta < 0)
			return false;
		return true;
}
int TSFile::UpdateTimeStamps(char* p,__int64 delta)
{
	if(m_buffer_pointer)
	{
		char* Packet = (char *)p;
		__int64 PCR ;
		

		if( ((DWORD)(Packet-(char *)m_buffer_pointer)>=0) &&
			((DWORD)(Packet-(char *)m_buffer_pointer)<m_buffer_length))
		{
			
			UpdateContinuityCounter((LPBYTE)Packet);

			WORD PID = MAKEWORD(Packet[2], Packet[1]&0x1F);

			if(GetPCRInAdaptationField((LPBYTE)Packet,&PCR))
			{
				PCR += delta;
				__int64 LastPCR;

				if(GetLastPCR(PID,LastPCR)==false)
					LastPCR=0;

				if((LastPCR==0) || (CheckTimestampAdvancesPCR(PCR, LastPCR)==true))
				{
					SetPCRInAdaptationField((LPBYTE)Packet,PCR);
					SetLastPCR(PID, PCR);
				}
				else
					SetPCRInAdaptationField((LPBYTE)Packet,LastPCR);

			}
			if(GetDTSInAdaptationField((LPBYTE)Packet,&PCR))
			{
				PCR += delta/300;
				__int64 LastPCR = GetLastDTS(PID);
				if((LastPCR==0) || (CheckTimestampAdvances(PCR, LastPCR)==true))
				{
					SetDTSInAdaptationField((LPBYTE)Packet,PCR);
					SetLastDTS(PID, PCR);
				}
				else
					SetDTSInAdaptationField((LPBYTE)Packet,LastPCR);
			}
			if(GetDTSInPES((LPBYTE)Packet,&PCR))
			{
				PCR += delta/300;
				__int64 LastPCR = GetLastDTS(PID);
				if((LastPCR==0) || (CheckTimestampAdvances(PCR, LastPCR)==true))
				{
					SetDTSInPES((LPBYTE)Packet,PCR);
					SetLastDTS(PID, PCR);
				}
				else
					SetDTSInPES((LPBYTE)Packet,LastPCR);
			}
			if(GetPTSInPES((LPBYTE)Packet,&PCR))
			{
				PCR += delta/300;
				SetPTSInPES((LPBYTE)Packet,PCR);
			}
			Packet += PACKET_SIZE;
		}
		return 1;
	}
	return 0;
}


int TSFile::TestTimeStamps(__int64 delta,unsigned char ContinuityCounter)
{
	if(m_buffer_pointer)
	{
//		char* Packet = (char *)m_buffer_pointer+(m_start*PACKET_SIZE);
		char* Packet = (char *)m_buffer_pointer;
		__int64 PCR ;
		__int64 oPCR ;
		

		__int64 Len;
//		if(m_end == 0xFFFFFFFF)
			Len = m_file_length;
//		else
//			Len = m_end*PACKET_SIZE;

//		unsigned char counter = 0;
//		GetContinuityCounter((LPBYTE)Packet,&counter);
//		unsigned char delta = ++ContinuityCounter - counter;
		__int64 i = 0;
		bool bPCRInAdaptationField = false;
		bool bDTSInAdaptationField = false;
		bool bDTSInPES= false;
		bool bPTSInPES= false;

			Packet = (char *)m_buffer_pointer;
			while ( ((DWORD)(Packet-(char *)m_buffer_pointer)<Len) && 
				(
				    (bPCRInAdaptationField == false) || 
					( bDTSInAdaptationField == false) ||
					(bDTSInPES== false) ||
					(bPTSInPES== false)))
			{
	//			SetContinuityCounter((LPBYTE)Packet,delta);
				if( ( bPCRInAdaptationField == false)&& (GetPCRInAdaptationField((LPBYTE)Packet,&oPCR)))
				{
					i = 0;
					while (i++*delta < 0x40000000000)
					{
						PCR = oPCR;
						PCR += delta;
						SetPCRInAdaptationField((LPBYTE)Packet,PCR);
						if(GetPCRInAdaptationField((LPBYTE)Packet,&oPCR))
							if(oPCR!= PCR)
							{
								if( oPCR != (PCR & 0x3FFFFFFFFFF))
									return 0;
							}
					}
					bPCRInAdaptationField = true;
				}
			/*	if(GetOPCRInAdaptationField((LPBYTE)Packet,&PCR))
				{
					PCR += delta;
					SetOPCRInAdaptationField((LPBYTE)Packet,PCR);
				}
				*/
				if( (bDTSInAdaptationField == false) && GetDTSInAdaptationField((LPBYTE)Packet,&oPCR))
				{
					i = 0;
					while (i++*delta < 0x200000000)
					{
						PCR = oPCR;
						PCR += delta;
						SetDTSInAdaptationField((LPBYTE)Packet,PCR);
						if(GetDTSInAdaptationField((LPBYTE)Packet,&oPCR))
							if(oPCR!= PCR)
							{
								if( oPCR != (PCR & 0x1FFFFFFFF))
									return 0;
							}
					}
					bDTSInAdaptationField = true;
				}
				if( (bDTSInPES == false) && GetDTSInPES((LPBYTE)Packet,&oPCR))
				{
					i = 0;
					while (i++*delta < 0x200000000)
					{
						PCR = oPCR;
						PCR += delta;
						SetDTSInPES((LPBYTE)Packet,PCR);
						if(GetDTSInPES((LPBYTE)Packet,&oPCR))
							if(oPCR!= PCR)
							{
								if( oPCR != (PCR & 0x1FFFFFFFF))
									return 0;
							}
					}
					bDTSInPES = true;


				}
				if( (bPTSInPES == false) && GetPTSInPES((LPBYTE)Packet,&oPCR))
				{
					i = 0;
					while (i++*delta < 0x200000000)
					{
						PCR = oPCR;
						PCR += delta;
						SetPTSInPES((LPBYTE)Packet,PCR);
						if(GetPTSInPES((LPBYTE)Packet,&oPCR))
							if(oPCR!= PCR)
							{
								if( oPCR != (PCR & 0x1FFFFFFFF))
									return 0;
							}
					}
					bPTSInPES = true;
				}
				Packet += PACKET_SIZE;
			}

				    if((bPCRInAdaptationField == true) && 
					( bDTSInAdaptationField == true) &
					(bDTSInPES== true) &&
					(bPTSInPES== true))
			return 1;
	}
	return 0;
}
int TSFile::GetPIDPCRfromPacket(char* p,WORD PID, __int64* lpPCR, bool* pbDiscontinuity)
{
	if((p)&&(lpPCR))
	{
		*lpPCR = -1;
		int i = 0;
		if(pbDiscontinuity)
			*pbDiscontinuity = false;
		bool bDisc;
		if(GetPIDPCR((LPBYTE)p,PID,lpPCR,&bDisc ))
		{	
			if(bDisc == true)
			{
				if(pbDiscontinuity)
					*pbDiscontinuity = true;
			}
		}
		if(*lpPCR!=-1)
			return 1;
	}
	return 0;
}


int TSFile::SetLimit(__int64 start, __int64 end)
{
	if((start)*PACKET_SIZE<m_file_length)
		m_start = start;
	else
		m_start = 0;
	if((end*PACKET_SIZE)<m_file_length)
		m_end = end;
	else
		m_end = -1;
	return 1;

}
bool TSFile::IsCached(void)
{
	return (m_buffer_size>=m_file_length?true:false);
}
int TSFile::GetNextPCR(char* p, __int64* lpPCR,WORD PID)
{

	if((p)&&(lpPCR))
	{
		if ( (p >= (char *) m_buffer_pointer) && 
			 (p < (char *) m_buffer_pointer + m_buffer_length) )
		{
			DWORD   old_current_index = -1;

		


			while(p!=NULL)
			{
				 if(p+PACKET_SIZE>=(char *)m_buffer_pointer + m_buffer_length)
				 {
					 //end of the buffer
					 // load next buffer
					 if(old_current_index == -1)
						 old_current_index = m_buffer_index;
					 if(Read(m_buffer_index+ m_buffer_length/PACKET_SIZE)==0)
					 {
						*p = NULL;
						if(old_current_index!=-1)
							Read(m_buffer_index);
						return 0;
					 }
					 p = (char *)m_buffer_pointer;
				 }
				 else
					 (char *)(p) += PACKET_SIZE; 	

				if( (PID == 0xFFFF) ||
					(PID == MAKEWORD(p[2], p[1]&0x1F)))
					if(GetPCRInAdaptationField((LPBYTE)p,lpPCR))
					{
						if(old_current_index!=-1)
							Read(m_buffer_index);
						return 1;
					}
			}
		}
	}
	return 0;
}
int TSFile::GetNextOPCR(char* p, __int64* lpOPCR,WORD PID)
{
	if((p)&&(lpOPCR))
	{
		if ( (p >= (char *) m_buffer_pointer) && 
			 (p < (char *) m_buffer_pointer + m_file_length) )
		{
			while(p + PACKET_SIZE  < (char *) m_buffer_pointer + m_file_length)
			{
				p += PACKET_SIZE;
				if( (PID == 0xFFFF) ||
					(PID == MAKEWORD(p[2], p[1]&0x1F)))
					if(GetOPCRInAdaptationField((LPBYTE)p,lpOPCR))
					{
						return 1;
					}
			}
		}
	}
	return 0;
}
int TSFile::GetNextPTS(char* p, __int64* lpPTS,WORD PID)
{
	if((p)&&(lpPTS))
	{
		if ( (p >= (char *) m_buffer_pointer) && 
			 (p < (char *) m_buffer_pointer + m_file_length) )
		{
			while(p + PACKET_SIZE  < (char *) m_buffer_pointer + m_file_length)
			{
				p += PACKET_SIZE;
				if( (PID == 0xFFFF) ||
					(PID == MAKEWORD(p[2], p[1]&0x1F)))
					if(GetPTSInPES((LPBYTE)p, lpPTS))
					{
						return 1;
					}
			}
		}
	}
	return 0;
}int TSFile::GetNextDTS(char* p, __int64* lpDTS,WORD PID)
{
	if((p)&&(lpDTS))
	{
		if ( (p >= (char *) m_buffer_pointer) && 
			 (p < (char *) m_buffer_pointer + m_file_length) )
		{
			while(p + PACKET_SIZE  < (char *) m_buffer_pointer + m_file_length)
			{
				p += PACKET_SIZE;
				
				if( (PID == 0xFFFF) ||
					(PID == MAKEWORD(p[2], p[1]&0x1F)))
				{
					if(GetDTSInAdaptationField((LPBYTE)p, lpDTS))
						return 1;
					if(GetDTSInPES((LPBYTE)p, lpDTS))
						return 1;
				}
			}
		}
	}
	return 0;
}

int TSFile::GetPID(PACKET* p, WORD& PID)
{
	if(p)
	{
		if ( ((char*)p >= (char *) m_buffer_pointer) && 
			 ((char*)p < (char *) m_buffer_pointer + m_buffer_length) )
		{
			PID = MAKEWORD(p[0][2], p[0][1]&0x1F);
			return 1;
		}
	}
	PID = 0xFFFF;
	return 0;
}
int TSFile::GetPacketInformation(	Parse_Mask Mask,
									PACKET* p,
									string& s)
{
	if(p)
	{
		if ( ((char*)p >= (char *) m_buffer_pointer) && 
			 ((char*)p < (char *) m_buffer_pointer + m_buffer_length) )
		{
			 bool transport_error = ( p[0][1]&0x80? true:false) ;
			 bool payload_unit_start_indicator= ( p[0][1]&0x40? true:false) ;
			 bool transport_priority = ( p[0][1]&0x20? true:false) ; 
			 
			 BYTE scrambling_control= ( p[0][3]&0xC0 )>>6;
			 BYTE adaption_field_control=( p[0][3]&0x30 )>>4;
			 BYTE Continuity_Counter =( p[0][3]&0x0F );
			 ostringstream oss;
			 __int64 l;
			 if( ( (int)(Mask & ts) == (int)ts) ||
				 GetPCRInAdaptationField((LPBYTE)p,&l) || 
				 GetOPCRInAdaptationField((LPBYTE)p,&l) ||
				 GetDTSInAdaptationField((LPBYTE)p,&l) ||
				 GetDTSInPES((LPBYTE)p,&l) ||
				 GetPTSInPES((LPBYTE)p,&l) )
			{

    			 WORD PID = MAKEWORD(p[0][2], p[0][1]&0x1F);
					 oss << endl << "Packet number : " << m_buffer_index+ (((LPBYTE)p-m_buffer_pointer)/PACKET_SIZE)<< endl;
					 oss << "transport_error                 :"<< transport_error << endl;
					 oss << "payload_unit_start_indicator    :"<< payload_unit_start_indicator << endl;
					 oss << "transport_priority              :"<< transport_priority << endl;
					 oss << "PID                             :"<< PID << endl;
					 oss << "scrambling_control              :"<< (int) scrambling_control << endl;
					 oss << "adaption_field_control          :"<< (int) adaption_field_control << endl;
					 oss << "Continuity_Counter              :"<< (int) Continuity_Counter  << endl;
					 {
						 if(GetPCRInAdaptationField((LPBYTE)p,&l))
							 oss << "PCR                             :"<<  l  << endl;
						 if(GetOPCRInAdaptationField((LPBYTE)p,&l))
							 oss << "OPCR                            :"<<  l  << endl;
						 if(GetDTSInAdaptationField((LPBYTE)p,&l))
							 oss << "DTS in Adaptation field         :"<<  l   << endl;
						 if(GetDTSInPES((LPBYTE)p,&l))
							 oss << "DTS in PES                      :"<<  l   << endl;
						 if(GetPTSInPES((LPBYTE)p,&l))
							 oss << "PTS in PES                      :"<<  l   << endl;
					 }						 
					 char bufferDump[PACKET_SIZE*5];
					 int len = sizeof(bufferDump);
					 DumpHexBuffer(bufferDump, len,(LPBYTE)p, PACKET_SIZE);
					 oss  << bufferDump << endl;
					 s = oss.str().c_str();
	  				 return 1;
			}
		}
	}
	return 0;
}
