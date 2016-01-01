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
#include "tssvc.h"
#include "tstrace.h"
#include "TSFile.h"  
#include "Timer.h"  
#include "UDPReceiver.h"
#include "TSStreamer.h"
#include <conio.h>
#define ADJUSTMENT_PERIOD 5

#define DEFAULT_TRACE_FILE_NAME		"TSROUTE.LOG"
#define DEFAULT_TRACE_FILE_SIZE		300000


GLOBAL_PARAMETER gGlobalParam;
// Support 100 simultaneous streams
STREAM_PARAMETER gStreamParam[1100];
DWORD gdwParamLen = sizeof(gStreamParam)/sizeof(gStreamParam[0]);

SERVICE_STATUS          ServiceStatus; 
SERVICE_STATUS_HANDLE   ServiceStatusHandle; 
LPTSTR  lpszServiceName;
HANDLE  hStopEvent;

void SetTSRouteServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode,
                         DWORD dwCheckPoint,   DWORD dwWaitHint);
int InitParameters(GLOBAL_PARAMETER* gParam, STREAM_PARAMETER* Param, DWORD dwParamLen)
{
	if((gParam) &&(Param) && (dwParamLen>0))
	{
		gParam->action = NoAction;
		gParam->mode = Command;
		gParam->console_trace_level = Information;
		gParam->trace_file = "";
		gParam->trace_level = Information;
		gParam->trace_max_size = 200000;
		gParam->pLog = NULL;
		gParam->bSyncTransmission = false;


		for(DWORD i = 0; i < dwParamLen; i++)
		{
				Param[i].name = "";
				Param[i].output_file = "";
				Param[i].refresh_period = ADJUSTMENT_PERIOD ;

				Param[i].xml_file = "";
				Param[i].ts_file = "";
				Param[i].input_file = "";
				Param[i].output_188_file = "";
				Param[i].program_number = 1;
				Param[i].udp_ip_address = "";
				Param[i].udp_ip_address_interface = "";
				Param[i].udp_port = -1;
				Param[i].input_rtp = false;
				Param[i].input_udp_ip_address = "";
				Param[i].input_udp_ip_address_interface = "";
				Param[i].input_udp_port = -1;
				Param[i].buffersize = 4096*1024;
				Param[i].forcedbitrate = 0;
				Param[i].ttl = 1;
				Param[i].nloop = 0;
				Param[i].bupdatetimestamps = false;
				Param[i].pid = -1;
				Param[i].packetStart = 0;
				Param[i].packetEnd = -1;
				Param[i].timeStart = 0;
				Param[i].timeEnd = -1;

				Param[i].pip_ts_file = "";
				Param[i].pip_udp_ip_address = "";
				Param[i].pip_udp_ip_address_interface = "";
				Param[i].pip_udp_port = -1;
				Param[i].pip_buffersize = 4096*1024;
				Param[i].pip_forcedbitrate = 0;

		}
	}
	return 1;
}
bool PrepareTSFile(STREAM_PARAMETER* Param, DWORD dwLen, SEND_STR* pc, TSFile* pfile)
{
	if((Param!=NULL) && (pc!=NULL)&&(pfile!=NULL))
	{
		char* p = (char *) pfile->GetFirstPacket();
		DWORD MaxCounter = pc->buffersize/PACKET_SIZE;
		while((p!=NULL) && (pc->bStop == false))
		{
			p = (char*)pfile->GetNextPacket((PACKET*)p);
			if(MaxCounter--==0)
			{
				MaxCounter = pc->buffersize/PACKET_SIZE;
				Sleep(1);
			}
		}
		p = (char *) pfile->GetFirstPacket();
		pc->bReadytoStart = true;

		// Wait for completion
		bool bReady = false;
		while((bReady==false) && (pc->bStop == false))
		{
			for(DWORD i = 0;i < dwLen;i++)
			{
				if((Param[i].ts_file.length()==0)|| ((Param[i].ts_file.length()> 0) && (Param[i].Maindata.bReadytoStart == true)) &&
					((Param[i].pip_ts_file.length()==0)||((Param[i].pip_ts_file.length()> 0) && (Param[i].Maindata.bReadytoStart == true))))
				    bReady = true;
				else
					bReady = false;
				if(bReady == false)
					break;
			}
			Sleep(1);
		}

	}
	return true;
}
DWORD FAR PASCAL SendProc( LPSTR lpData )
{
	HANDLE thisThread = GetCurrentThread();
	SetThreadPriority(thisThread, THREAD_PRIORITY_TIME_CRITICAL);

	SEND_STR* pc = (SEND_STR*) lpData;
	if(pc==NULL)
		return 0;
	pc->bError = false;
	pc->ErrorMessage = "No Error";
	pc->bStart = false;
	pc->bReadytoStart = false;
	pc->loopCounter = 0;
	unsigned long lastloopCounter = 0;
	if((pc->bPIP == true) && (pc->pMainStream)) 
		lastloopCounter = pc->pMainStream->loopCounter;

	// Load TS file
	__int64 dwLen;
	TSFile file;
	WORD PCR_PID = 0xFFFF;
	__int64 TempoDelta = 0;
	if(file.Open(pc->send_ts_file))
	{

		if((dwLen = file.Load(pc->buffersize))!=0)
		{
			TSFile::Parse_Mask Mask = TSFile::none;
			WORD PCR_PID=0xFFFF;

			if(pc)
			{
				pc->Counters.Count = 0;
				pc->Counters.duration = 0;
				pc->Counters.PacketCount = 0;
				pc->Counters.bError = false;


				for(int m = 0; m < MAX_PID; m++)
				{
					pc->Counters.Counters[m].bitrate = 0;
					pc->Counters.Counters[m].duration = 0;
					pc->Counters.Counters[m].PID = 0xFFFF;
					pc->Counters.Counters[m].PCR_PID = 0xFFFF;
					pc->Counters.Counters[m].delta = 0;
					pc->Counters.Counters[m].program_map_pid = 0xFFFF;
					pc->Counters.Counters[m].program_number = 0;
					pc->Counters.Counters[m].TimeStampPacketCount = 0;

					pc->Counters.Counters[m].IndexFirstTimeStampPacket = -1;
					pc->Counters.Counters[m].IndexLastTimeStampPacket = -1;

					pc->Counters.Counters[m].FirstTimeStamp = -1;
					pc->Counters.Counters[m].LastTimeStamp = -1;

					pc->Counters.Counters[m].MinPacketBetweenTimeStamp = -1;
					pc->Counters.Counters[m].MaxPacketBetweenTimeStamp = -1;

					pc->Counters.Counters[m].TimeStampDiscontinuity = 0;
					pc->Counters.Counters[m].DiscontinuityIndicator = 0;

					pc->Counters.Counters[m].MinTimeStampBitrate = -1;
					pc->Counters.Counters[m].MaxTimeStampBitrate = -1;
				}
			}

			if(file.GetProgramMapTableID(&pc->Counters)>0)
			{
				
				pc->Counters.duration = 0;
				pc->Counters.bitrate = 0;
				for(int i =  0; i < pc->Counters.Count; i++)
				{
					double locduration;
					double locbitrate;
					pc->Counters.Counters[i].PCR_PID = file.GetPCR_PID(pc->Counters.Counters[i].program_map_pid);

					if(file.GetBitrate(&pc->Counters.Counters[i], pc->Counters.Counters[i].PCR_PID, locduration, locbitrate,pc->Counters.PacketCount))
					{
						if(locduration > pc->Counters.duration)
							pc->Counters.duration = locduration;
						//	bitrate += locbitrate;
						if(locbitrate>pc->Counters.bitrate)
						{
							pc->Counters.bitrate = locbitrate;
							pc->Counters.Delta = pc->Counters.Counters[i].delta;
							PCR_PID = pc->Counters.Counters[i].PCR_PID;
						}
					}

				}

			}


			if((pc->Counters.duration>0)&& (pc->Counters.bitrate>0))
			{				

				int buffersize = PACKET_SIZE*7;

				if(pc->forcedbitrate)
					pc->Counters.bitrate = pc->forcedbitrate;
				if(pc->timeStart!=0)
				{
					double d = pc->Counters.bitrate*pc->timeStart/1000;
					pc->packetStart = (unsigned long) (d/(8*PACKET_SIZE));
				}
				if(pc->timeEnd!=0xFFFFFFFF)
				{
					double d = pc->Counters.bitrate*pc->timeEnd/1000;
					pc->packetEnd = (unsigned long)  (d/(8*PACKET_SIZE));
				}

				if(pc->bupdatetimestamp)
				{
					if((pc->packetEnd == 0xFFFFFFFF)&& (pc->packetStart == 0))
					{
						__int64 start = -1;
						__int64 end = -1;
						file.GetPacketRange(PCR_PID,start,end);
						if((start != -1) && (end != -1))
						{
							pc->packetEnd = end;
							pc->packetStart = start;
						}
					}
				}
 
				if((pc->packetEnd != 0xFFFFFFFF)|| (pc->packetStart != 0))
				{
					__int64 locduration = 0;
					if(pc->packetEnd != 0xFFFFFFFF)
						locduration = ((pc->packetEnd - pc->packetStart)*PACKET_SIZE*8);
					else
						locduration = ((pc->Counters.PacketCount - pc->packetStart)*PACKET_SIZE*8);

					pc->Counters.expectedduration = (double)(locduration/pc->Counters.bitrate);
				}
				else
					pc->Counters.expectedduration = pc->Counters.duration;

				TSStreamer Streamer;
				if(Streamer.Load(pc->udp_ip_address,pc->udp_port,pc->ttl,buffersize,pc->udp_ip_address_interface))
				{
// PCR Buffer
//					if(file.LoadPCRBuffer(PCR_PID))
//					{
						Timer TTempo;
						Timer TLoop;
						Timer TPCR;
						Timer TMonitor;

						if((TTempo.Load()) &&
							(TLoop.Load()) &&
							(TPCR.Load()) &&
							(TMonitor.Load()))
						{

							double TempoTime = 0;
							__int64 delta = 0;
							//__int64 delta = 1561000000;
							__int64 PCR = -1;
							__int64 LastPCR = -1;
							DWORD PCRCounter = 0;
							DWORD MonitorCounter = 0;
							DWORD PacketCounter = 0;
							double MonitorPeriod = 0;
							DWORD dwCurrentBitrate = (DWORD) pc->Counters.bitrate;
							DWORD Tempo = (DWORD)(PACKET_SIZE*8*7*1000)/dwCurrentBitrate;

							if(Tempo>10)
								Tempo = 10;
							Tempo++;
							bool bDiscontinuity;
							long RestOfByteToTransmit = 0;
							char* p = NULL;
							__int64 Delta = 0;
							long NumberofByteToTransmit = 0;
							long NumberofPacketToTransmit = 0;
							long DeltaNumberofPacketToTransmit = 0;

							pc->maxbitrate = dwCurrentBitrate;
							pc->minbitrate = dwCurrentBitrate;
							
							// Prepare buffer
							// wait for 
							if(gGlobalParam.bSyncTransmission)
								PrepareTSFile(gStreamParam, gdwParamLen, pc, &file);

							TLoop.Begin();
							do 
							{




								PacketCounter = 0;
								p = (char *) file.GetFirstPacket();

								TTempo.Begin();
								TMonitor.Begin();

								while((p!=NULL) && (pc->bStop == false))
								{
									Sleep( Tempo);
									{
										TTempo.End(TempoTime);

										if(dwCurrentBitrate == 0)
											dwCurrentBitrate = (DWORD) pc->Counters.bitrate;

										NumberofByteToTransmit= (DWORD)((dwCurrentBitrate*TempoTime)/8) + RestOfByteToTransmit;
										NumberofPacketToTransmit = (NumberofByteToTransmit/PACKET_SIZE) ;
										RestOfByteToTransmit = NumberofByteToTransmit - NumberofPacketToTransmit*PACKET_SIZE;

										if(NumberofPacketToTransmit!=0)
										{
											//NumberofPacketToTransmit++;
											TTempo.Begin();
										}

										for (long i = 0; i < NumberofPacketToTransmit;i++)
										{
											if(pc->bupdatetimestamp)
											{
												file.UpdateTimeStamps(p,delta);
											}
											file.GetPIDPCRfromPacket(p,PCR_PID, &PCR,&bDiscontinuity);
											if(PCR!=-1)
											{
												if(LastPCR ==-1)
												{
													LastPCR = PCR;
													pc->FirstPCR = PCR;
													pc->LastPCR = PCR;
												}
												else
												{
													Delta = 0;
													double dPCR = 0;
													TPCR.End(dPCR);

/*
													if((PCR-LastPCR)>=0)
														Delta = (PCR-LastPCR);
													else
														Delta = 0x40000000000 - (PCR - LastPCR);
*/
													if (((PCR & 0x20000000000) == 0) && ((LastPCR & 0x20000000000) != 0))
														Delta = (0x257FFFFFFFF - LastPCR) + PCR;
													else
														Delta = PCR - LastPCR;




													DWORD dwDelta = (DWORD) Delta;
													if(dwDelta != 0)
													{
														if(!pc->forcedbitrate)
															dwCurrentBitrate = ((__int64)PCRCounter*PACKET_SIZE*8*27000000)/dwDelta;

														if(dwCurrentBitrate>pc->maxbitrate)
															pc->maxbitrate = dwCurrentBitrate;
														if(dwCurrentBitrate<pc->minbitrate)
															pc->minbitrate = dwCurrentBitrate;
													}
													__int64 Deltamicros = (__int64)(dPCR*1000000) - Delta/27;
													__int64 TimeToTransmitAPacketmicros = PACKET_SIZE*8*1000000/dwCurrentBitrate;
													// Avoid crash div/0
													//
													if(TimeToTransmitAPacketmicros == 0)
														TimeToTransmitAPacketmicros = 1;

													int DeltaPacketNumber = (int) (Deltamicros/TimeToTransmitAPacketmicros);
													

													NumberofPacketToTransmit = NumberofPacketToTransmit + DeltaPacketNumber + DeltaNumberofPacketToTransmit;

													if(NumberofPacketToTransmit<0)
													{
														//DeltaNumberofPacketToTransmit = NumberofPacketToTransmit;
														NumberofPacketToTransmit = 0;
													}												
													
													PCRCounter = 0;
													LastPCR = PCR;
													pc->LastPCR = PCR;
												}
												TPCR.Begin();
											}


											if(Streamer.SendPacket(p)==0)
											{
												ostringstream oss;
												oss << "Error when sending a UDP packet" << endl ;

												pc->bError = true;
												pc->ErrorMessage = oss.str().c_str();
											}
											PacketCounter++;
											MonitorCounter++;
											PCRCounter++;
											if((p = (char*)file.GetNextPacket((PACKET*)p)) == NULL)
												break;
										}
										TMonitor.End(MonitorPeriod);
										if(MonitorPeriod>pc->refresh_period)
										{
											double loopduration;
											double currentbitrate=MonitorCounter*PACKET_SIZE*8/MonitorPeriod;
											TLoop.End(loopduration);
											pc->GlobalCounter += MonitorCounter;
											double averagebitrate=pc->GlobalCounter*PACKET_SIZE*8/loopduration;
											// Check bitrate and adjust interval
											double rate = averagebitrate/pc->Counters.bitrate;
											pc->LocalCounter = MonitorCounter;
											pc->currentbitrate =currentbitrate;
											pc->averagebitrate = averagebitrate;
											pc->expectedbitrate = pc->Counters.bitrate;
											pc->interval = ((double)Tempo)/1000;
											pc->LastPCR = LastPCR;
											pc->duration = loopduration;

											TMonitor.Begin();
											MonitorCounter = 0;
										}
										if((pc->bPIP == true) && (pc->pMainStream)) 
										{
											if(lastloopCounter != pc->pMainStream->loopCounter)
											{
												// the main loop has restarted
												// the PIP should restart 
												lastloopCounter = pc->pMainStream->loopCounter;
												break;
											}
										}
									}

								}
								if(pc->bupdatetimestamp)
								{									
									file.ResetPIDDiscontinuity();

									if(file.IsCached())
										delta = pc->Counters.Delta;
									else
										delta += pc->Counters.Delta;
								}

								if(pc->nloop>0)
									pc->nloop--;
								pc->loopCounter++;
							}
							while((pc->nloop)&& (pc->bStop == false));											
//						}
					}
					Streamer.Unload();
				}
				file.Unload();
			}
			file.Close();
		}
		else
		{
			ostringstream oss;

			
			oss << "Can't load the transport stream file " << pc->send_ts_file << endl;
			pc->bError = true;
			pc->ErrorMessage = oss.str().c_str();


		}

	}
	else
	{
		ostringstream oss;
		oss << "Can't open the transport stream file " << pc->send_ts_file << endl;
		pc->bError = true;
		pc->ErrorMessage = oss.str().c_str();

	}
	pc->bStopped = true;
	return 1;
}
/*
int StartStreamFileThread(STREAM_PARAMETER* Param, DWORD dwParamLen)
{
	for(DWORD i = 0; i < dwParamLen; i++)
	{
		if(Param[i].ts_file.length()> 0)
		{
			Param[i].Maindata.hThread = NULL;
			Param[i].PIPdata.hThread = NULL;
			if (NULL == (Param[i].Maindata.hThread = CreateThread( (LPSECURITY_ATTRIBUTES) NULL,
					0, 
					(LPTHREAD_START_ROUTINE) SendProc,
					(LPVOID) &Param[i].Maindata,
					0, &Param[i].Maindata.dwThreadID )))
					return 0;
			if(Param[i].pip_ts_file.length()> 0)
			{
				if (NULL == (Param[i].PIPdata.hThread =	CreateThread( (LPSECURITY_ATTRIBUTES) NULL,
					0, 
					(LPTHREAD_START_ROUTINE) SendProc,
					(LPVOID) &Param[i].PIPdata,
					0, &Param[i].PIPdata.dwThreadID )))
					return 0;
			}
		}
	}
	return 1;
}
int StopStreamFileThread(STREAM_PARAMETER* Param, DWORD dwParamLen)
{
	for(DWORD i = 0; i < dwParamLen; i++)
	{
		if(Param[i].ts_file.length()> 0)
		{
			if (Param[i].Maindata.hThread)
			{
				Param[i].Maindata.bStop = true;
				Sleep(100);
				if(Param[i].Maindata.bStopped == false)
					TerminateThread(Param[i].Maindata.hThread,1);
				CloseHandle(Param[i].Maindata.hThread);
				Param[i].Maindata.hThread = NULL;
			}
			if(Param[i].pip_ts_file.length()> 0)
			{
				if (Param[i].PIPdata.hThread)
				{
					Param[i].PIPdata.bStop = true;
					Sleep(100);
					if(Param[i].PIPdata.bStopped == false)
						TerminateThread(Param[i].PIPdata.hThread,1);
					CloseHandle(Param[i].PIPdata.hThread);
					Param[i].PIPdata.hThread = NULL;
				}
			}
		}
	}
	return 1;
}
*/
DWORD FAR PASCAL RouteProc( LPSTR lpData )
{
	int buffersize = PACKET_SIZE*7000;
	char buffer[70*PACKET_SIZE];
	SEND_STR* p = (SEND_STR*) lpData;
	if(p==NULL)
		return 0;

	HANDLE thisThread = GetCurrentThread();
	SetThreadPriority(thisThread, THREAD_PRIORITY_TIME_CRITICAL);

	//
	__int64 PCR = -1;
	__int64 FirstPCR = -1;
	p->LocalCounter=0; 
	p->GlobalCounter=0;
	p->LastGlobalCounter=0;

	Timer T;
	Timer TMonitor;
	T.Load();
	T.Begin();
	TMonitor.Load();
	TMonitor.Begin();
	double deltamax = -1;
	double deltamin = -1;
	double MonitorPeriod=0;
	DWORD MonitorCounter = 0;


	// Load TS file
	UDPMulticastStreamer Streamer;
	if(Streamer.Load(p->udp_ip_address,p->udp_port,p->ttl,buffersize,p->udp_ip_address_interface))
	{
		UDPMulticastReceiver Receiver;
		if(Receiver.Load(p->input_udp_ip_address,p->input_udp_port,buffersize,p->input_udp_ip_address_interface))
		{

			int len;  
			p->GlobalCounter=0;
			p->LastGlobalCounter=0;

			len = sizeof(buffer);
//			while( (Receiver.Recv(buffer,&len)) && (p->bStop == false))
			while( (Receiver.RecvNonBlocking(buffer,&len,1000)) && (p->bStop == false))
			{
				if(len != 0)
				{
					Streamer.Send(buffer,len);
					MonitorCounter += len;
				}
				len = sizeof(buffer);
				TMonitor.End(MonitorPeriod);
				if(MonitorPeriod>p->refresh_period)
				{
    				double duration;
					T.End(duration);
					p->duration = duration;
					TMonitor.Begin();
					double currentbitrate=MonitorCounter*8/MonitorPeriod;
					p->GlobalCounter += MonitorCounter;
					double averagebitrate=p->GlobalCounter*8/duration;
					p->LocalCounter = MonitorCounter;
					p->currentbitrate =currentbitrate;
					p->averagebitrate = averagebitrate;
					MonitorCounter=0;
				}
			}
			if(Receiver.GetLastError()!=0)
			{
				p->bError = true;
				ostringstream oss;
				oss <<  "Socket Error:" << Receiver.GetLastError() << endl;
				p->ErrorMessage = oss.str().c_str();
			}

			Receiver.Unload();
		}
		else
		{
			p->bError = true;
			ostringstream oss;
			oss <<  "Socket Error:" << Receiver.GetLastError() << endl;
			p->ErrorMessage = oss.str().c_str();
		}
		Streamer.Unload();
	}
	p->bStopped = true;
	return 1;
}
int StartFileStreamerOrRouterThread(STREAM_PARAMETER* Param, DWORD dwParamLen)
{
	for(DWORD i = 0; i < dwParamLen; i++)
	{
		if(Param[i].ts_file.length()> 0)
		{
			Param[i].Maindata.hThread = NULL;
			Param[i].PIPdata.hThread = NULL;
			// File Streamer threads
			if(Param[i].ts_file.length()>0)
			{
				if (NULL == (Param[i].Maindata.hThread = CreateThread( (LPSECURITY_ATTRIBUTES) NULL,
						0, 
						(LPTHREAD_START_ROUTINE) SendProc,
						(LPVOID) &Param[i].Maindata,
						0, &Param[i].Maindata.dwThreadID )))
						return 0;
				if(Param[i].pip_ts_file.length()> 0)
				{
					if (NULL == (Param[i].PIPdata.hThread =	CreateThread( (LPSECURITY_ATTRIBUTES) NULL,
						0, 
						(LPTHREAD_START_ROUTINE) SendProc,
						(LPVOID) &Param[i].PIPdata,
						0, &Param[i].PIPdata.dwThreadID )))
						return 0;
				}
			}
		}
		// Multicast Router threads
		else if(Param[i].input_udp_ip_address.length()>0)
		{
			if (NULL == (Param[i].Maindata.hThread = CreateThread( (LPSECURITY_ATTRIBUTES) NULL,
					0, 
					(LPTHREAD_START_ROUTINE) RouteProc,
					(LPVOID) &Param[i].Maindata,
					0, &Param[i].Maindata.dwThreadID )))
					return 0;
		}
	}
	return 1;
}
int StopFileStreamerOrRouterThread(STREAM_PARAMETER* Param, DWORD dwParamLen)
{
	for(DWORD i = 0; i < dwParamLen; i++)
	{
		if(Param[i].ts_file.length()> 0)
		{
			if (Param[i].Maindata.hThread)
			{
				Param[i].Maindata.bStop = true;
				Sleep(100);
				if(Param[i].Maindata.bStopped == false)
					TerminateThread(Param[i].Maindata.hThread,1);
				CloseHandle(Param[i].Maindata.hThread);
				Param[i].Maindata.hThread = NULL;
			}
			if(Param[i].pip_ts_file.length()> 0)
			{
				if (Param[i].PIPdata.hThread)
				{
					Param[i].PIPdata.bStop = true;
					Sleep(100);
					if(Param[i].PIPdata.bStopped == false)
						TerminateThread(Param[i].PIPdata.hThread,1);
					CloseHandle(Param[i].PIPdata.hThread);
					Param[i].PIPdata.hThread = NULL;
				}
			}
		}
		// Multicast Router threads
		else if(Param[i].input_udp_ip_address.length()>0)
		{
			if (Param[i].Maindata.hThread)
			{
				Param[i].Maindata.bStop = true;
				Sleep(100);
				if(Param[i].Maindata.bStopped == false)
					TerminateThread(Param[i].Maindata.hThread,1);
				CloseHandle(Param[i].Maindata.hThread);
				Param[i].Maindata.hThread = NULL;
			}
		}
	}
	return 1;
}
int GetStreamLoopInformation(string& msg,const char* header, SEND_STR* pdata,long forcedbitrate,VerboseLevel verboselevel)
{
	if(pdata==NULL)
		return 0;
	ostringstream oss;

	if(header)
	oss << endl << header  <<endl;
	oss << "Status after " << pdata->duration << " seconds:" <<endl;
	oss << "Number of packets transmitted during this period: " << pdata->LocalCounter << endl;
	DWORD dw = (DWORD)pdata->currentbitrate;

	oss << "Current bitrate for this period                 : " << dw << " b/s" << endl;
	oss << "Number of packets transmitted                   : " << pdata->GlobalCounter << endl;


	dw = (DWORD)pdata->averagebitrate;
	oss << "Average bitrate                                 : " << dw << " b/s" << endl;
	dw = (DWORD)pdata->expectedbitrate;
	oss << "Expected bitrate                                : " << dw << " b/s" << endl;
	dw = (DWORD)pdata->maxbitrate;
	oss << "Maximum instant bitrate                         : " << dw << " b/s" << endl;
	dw = (DWORD)pdata->minbitrate;
	oss << "Minimum instant bitrate                         : " << dw << " b/s" << endl;
	oss << "Expected duration                               : " << pdata->Counters.expectedduration << " s" << endl;
//	if(forcedbitrate)
    oss << "Transmit period                                 : " << pdata->interval << " seconds" << endl;
	if(pdata->FirstPCR != -1)
	{
		double t = (double) pdata->FirstPCR/27000000;
		oss << "First PCR: " << pdata->FirstPCR << " - " << t << " seconds" << endl;
	}
	if(pdata->LastPCR != -1)
	{
		double t = (double) pdata->LastPCR/27000000;
		oss << "Last  PCR: " << pdata->LastPCR << " - " << t << " seconds" << endl;
	}
	msg = oss.str().c_str();
	return 1;
}
int GetRouteLoopInformation(string& msg,const char* header, SEND_STR* pdata,long forcedbitrate,VerboseLevel verboselevel)
{
	if(pdata==NULL)
		return 0;
	ostringstream oss;

	if(header)
	oss << endl << header  <<endl;
	oss << "Status after " << pdata->duration << " seconds:" <<endl;
	oss << "Number of bytes transmitted during this period  : " << pdata->LocalCounter << endl;
	DWORD dw = (DWORD)pdata->currentbitrate;

	oss << "Current bitrate for this period                 : " << dw << " b/s" << endl;
	oss << "Number of bytes transmitted                     : " << pdata->GlobalCounter << endl;


	dw = (DWORD)pdata->averagebitrate;
	oss << "Average bitrate                                 : " << dw << " b/s" << endl;
	msg = oss.str().c_str();
	return 1;
}
LPCSTR GetTraceLevelString(VerboseLevel vl)
{
	static LPCSTR sInformation = "information";
	static LPCSTR sTSPID = "pid";
	static LPCSTR sTSTimeStamp = "timestamp";
	static LPCSTR sTSPacket = "ts";
	switch(vl)
	{
	case Information:
		return sInformation;
	case TSPID:
		return sTSPID;
	case TSTimeStamp:
		return sTSTimeStamp;
	case TSPacket:
		return sTSPacket;
	default:
		break;
	}
	return sInformation;
}
bool IsTrace(GLOBAL_PARAMETER* gParam, VerboseLevel vl)
{
	if(gParam)
	{
		if(((gParam->console_trace_level>=vl)&& (gParam->mode == Command))||
			(gParam->trace_level>=vl))
			return true;

	}
	return false;
}
bool TraceOut(GLOBAL_PARAMETER* gParam, VerboseLevel vl, LPCTSTR pTrace)
{
	bool bresult = false;
	if(gParam)
	{
		if((gParam->console_trace_level>=vl) && (gParam->mode == Command))
		{
			cout << pTrace << endl;
			bresult = true;
		}
		if(gParam->trace_level>=vl)
		{
			if(gParam->pLog) 
			{
				gParam->pLog->TraceWrite(pTrace);
				bresult = true;
			}
		}
	}
	return bresult;
}

int DisplayStreamFileCounters(GLOBAL_PARAMETER* gParam,STREAM_PARAMETER* Param, DWORD dwParamLen)
{
	if((gParam == 0) && (Param == 0)) return 0;
	for(DWORD i = 0; i < dwParamLen; i++)
	{
		if(Param[i].ts_file.length()> 0)
		{
			if((Param[i].Maindata.lastduration != Param[i].Maindata.duration)&& (Param[i].Maindata.duration != 0))
			{
				if(Param[i].output_file.length()>0)
				{
					string error_string;
					UpdateResultXmlFile(&Param[i], error_string);
				}
				Param[i].Maindata.lastduration = Param[i].Maindata.duration;
				if(IsTrace(gParam,Information))
				{							
					string msg = "";
					string s = Param[i].name;
					if(Param[i].ts_file.length()>0)
					{
						s += " Main Stream information: ";
						s += Param[i].ts_file;
						GetStreamLoopInformation(msg,s.c_str(),&Param[i].Maindata,Param[i].forcedbitrate, gParam->trace_level);
						TraceOut(gParam, Information, msg.c_str());
						
						if(Param[i].Maindata.bError==true)
						{
							s = Param[i].name;
							s += " Main Stream error message: ";
							s += Param[i].Maindata.ErrorMessage.c_str();
							TraceOut(gParam, Information, s.c_str());
							Param[i].Maindata.bError=false;
						}
					}

				}

			}
			if(Param[i].pip_ts_file.length()> 0)
			{
				if((Param[i].PIPdata.lastduration != Param[i].PIPdata.duration)&& (Param[i].PIPdata.duration != 0))
				{
					if(IsTrace(gParam,Information))
					{							
						string msg = "";
						string s = Param[i].name;
						s += " PIP Stream information: ";
						s += Param[i].pip_ts_file;
						GetStreamLoopInformation(msg,s.c_str(),&Param[i].PIPdata,Param[i].pip_forcedbitrate, gParam->trace_level);
						TraceOut(gParam, Information, msg.c_str());

						Param[i].PIPdata.lastduration = Param[i].PIPdata.duration;
						if(Param[i].PIPdata.bError==true)
						{
							s = Param[i].name;
							s += " PIP Stream error message: ";
							s += Param[i].PIPdata.ErrorMessage.c_str();
							TraceOut(gParam, Information, s.c_str());
							Param[i].PIPdata.bError=false;
						}
					}
				}
			}
		}
	}
	return 1;
}
int DisplayFileStreamerOrRouterCounters(GLOBAL_PARAMETER* gParam,STREAM_PARAMETER* Param, DWORD dwParamLen)
{
	if((gParam == 0) && (Param == 0)) return 0;
	for(DWORD i = 0; i < dwParamLen; i++)
	{
		if(Param[i].ts_file.length()> 0)
		{
			if((Param[i].Maindata.lastduration != Param[i].Maindata.duration)&& (Param[i].Maindata.duration != 0))
			{
				if(Param[i].output_file.length()>0)
				{
					string error_string;
					UpdateResultXmlFile(&Param[i], error_string);
				}
				Param[i].Maindata.lastduration = Param[i].Maindata.duration;
				if(IsTrace(gParam,Information))
				{							
					string msg = "";
					ostringstream oss;
					oss << Param[i].name;
					oss << " Main Stream information: ";
					oss << Param[i].ts_file.c_str();
					oss << endl;
					oss << "towards: ";
					oss << Param[i].udp_ip_address.c_str();
					oss << ":" ;
					oss << Param[i].udp_port;  
					oss << " Interface: ";
					oss << Param[i].udp_ip_address_interface.c_str();
					string s = oss.str().c_str();					
					GetStreamLoopInformation(msg,s.c_str(),&Param[i].Maindata,Param[i].forcedbitrate, gParam->trace_level);
					TraceOut(gParam, Information, msg.c_str());
					
					if(Param[i].Maindata.bError==true)
					{
						s = Param[i].name;
						s += " Main Stream error message: ";
						s += Param[i].Maindata.ErrorMessage.c_str();
						TraceOut(gParam, Information, s.c_str());
						Param[i].Maindata.bError=false;
					}
				}

			}
			if(Param[i].pip_ts_file.length()> 0)
			{
				if((Param[i].PIPdata.lastduration != Param[i].PIPdata.duration)&& (Param[i].PIPdata.duration != 0))
				{
					if(IsTrace(gParam,Information))
					{							
						string msg = "";
						ostringstream oss;
						oss << Param[i].name;
						oss << " PIP Stream information: ";
						oss << Param[i].pip_ts_file.c_str();
						oss << endl;
						oss << "towards: ";
						oss << Param[i].pip_udp_ip_address.c_str();
						oss << ":" ;
						oss << Param[i].pip_udp_port;  
						oss << " Interface: ";
						oss << Param[i].pip_udp_ip_address_interface.c_str();
						string s = oss.str().c_str();					
						GetStreamLoopInformation(msg,s.c_str(),&Param[i].PIPdata,Param[i].pip_forcedbitrate, gParam->trace_level);
						TraceOut(gParam, Information, msg.c_str());

						Param[i].PIPdata.lastduration = Param[i].PIPdata.duration;
						if(Param[i].PIPdata.bError==true)
						{
							s = Param[i].name;
							s += " PIP Stream error message: ";
							s += Param[i].PIPdata.ErrorMessage.c_str();
							TraceOut(gParam, Information, s.c_str());
							Param[i].PIPdata.bError=false;
						}
					}
				}
			}
		}
		else if(Param[i].input_udp_ip_address.length()> 0)
		{

			if((Param[i].Maindata.lastduration != Param[i].Maindata.duration)&& (Param[i].Maindata.duration != 0))
			{
				if(Param[i].output_file.length()>0)
				{
					string error_string;
					UpdateResultXmlFile(&Param[i], error_string);
				}
				Param[i].Maindata.lastduration = Param[i].Maindata.duration;
				if(IsTrace(gParam,Information))
				{							
					string msg = "";
					ostringstream oss;
					oss << Param[i].name;
					oss << " Main Stream information: ";
					oss << Param[i].input_udp_ip_address.c_str();
					oss << ":" ;
					oss << Param[i].input_udp_port;  
					oss << " Interface: ";
					oss << Param[i].input_udp_ip_address_interface.c_str();
					oss << endl;
					oss << "towards: ";
					oss << Param[i].udp_ip_address.c_str();
					oss << ":" ;
					oss << Param[i].udp_port;  
					oss << " Interface: ";
					oss << Param[i].udp_ip_address_interface.c_str();
					string s = oss.str().c_str();
					GetRouteLoopInformation(msg,s.c_str(),&Param[i].Maindata,Param[i].forcedbitrate, gParam->trace_level);
					TraceOut(gParam, Information, msg.c_str());
					
					if(Param[i].Maindata.bError==true)
					{
						s = Param[i].name;
						s += " Main Stream error message: ";
						s += Param[i].Maindata.ErrorMessage.c_str();
						TraceOut(gParam, Information, s.c_str());
						Param[i].Maindata.bError=false;
					}
				}
			}
		}
	}
	return 1;
}
int CheckStreamFileErrors(GLOBAL_PARAMETER* gParam,STREAM_PARAMETER* Param, DWORD dwParamLen,Timer& T)
{
	for(DWORD i = 0; i < dwParamLen; i++)
	{
		if(Param[i].ts_file.length()> 0)
		{
			if(Param[i].Maindata.bStopped==true)
			{
				if(IsTrace(gParam,Information))
				{
					double duration;
					T.End(duration);

					ostringstream oss;
					if(Param[i].Maindata.bError==true)
						oss << endl <<"Main Streaming thread stopped" << endl <<"Error message: " << Param[i].Maindata.ErrorMessage <<endl;
					else
						oss << "End of broadcast, duration: " << duration << " seconds" << endl;
					TraceOut(gParam, Information, oss.str().c_str());
				}
				return 1;
			}
			if(Param[i].ts_file.length()> 0)
			{
				if(Param[i].PIPdata.bStopped==true)
				{
					if(IsTrace(gParam,Information))
					{
						double duration;
						T.End(duration);

						ostringstream oss;
						if(Param[i].PIPdata.bError==true)
							oss << endl <<"PIP Streaming thread stopped" << endl <<"Error message: " << Param[i].PIPdata.ErrorMessage <<endl;
						else
							oss << "End of broadcast, duration: " << duration << " seconds" << endl;
						TraceOut(gParam, Information, oss.str().c_str());
					}
					return 1;
				}
			}
		}
	}
	return 0;
}
int GetRouteStreamInformation(string& msg,const char* input_udp_ip_address, int input_udp_port, const char *input_udp_ip_address_interface, const char* udp_ip_address, int udp_port,const char *udp_ip_address_interface,  int ttl)
{
	ostringstream oss;
	oss << "Starting to route stream " << input_udp_ip_address << ":" << input_udp_port; 
	if(input_udp_ip_address_interface)
		oss <<" interface: " << input_udp_ip_address_interface ;
			
	oss	<< " towards " << udp_ip_address << ":" << udp_port ;
	if(udp_ip_address_interface)
		oss <<" interface: " << udp_ip_address_interface ;
	oss << " TTL: " << ttl << endl;
	msg = oss.str().c_str();
	return 1;
}
int GetMainStreamInformation(string& msg,const char* send_ts_file, const char* udp_ip_address, int udp_port,const char *udp_ip_address_interface,  int ttl,
								 long forcedbitrate, int nloop, bool bupdatetimestamp, long timeStart,long timeEnd,unsigned long packetStart,unsigned long packetEnd)
{
	ostringstream oss;
	oss << "Starting to stream transport stream file " << send_ts_file << " towards " << udp_ip_address << ":" << udp_port ;
	if(udp_ip_address_interface)
		oss <<" interface: " << udp_ip_address_interface ;
	oss << " TTL: " << ttl << endl;
	oss << "Streaming options:" << endl;
	if(forcedbitrate)
		oss << "  Forced bitrate: " << forcedbitrate << " b/s " ;
	oss << "  loop(s) : " ;
	if(nloop >= 0)
		oss << nloop+1 << endl;;
	if(nloop < 0)
		oss << "infinite" << endl;;

	if(bupdatetimestamp)
		oss << "  Time stamps updated after each loop " << endl;;
	if((timeStart!=0)||(timeEnd!=0xFFFFFFFF))
		oss << "  From time " << timeStart << " ms till time " << timeEnd << " ms " << endl;
	if((packetStart!=0)||(packetEnd!=0xFFFFFFFF))
		if(packetEnd!=0xFFFFFFFF)
			oss << "  From packet " << packetStart << " till packet " << packetEnd << endl;
		else
			oss << "  From packet " << packetStart << " till last packet "  << endl;

	msg = oss.str().c_str();
	return 1;
}
int GetPIPStreamInformation(string& msg,const char* send_ts_file, const char* udp_ip_address, int udp_port,const char *udp_ip_address_interface,  int ttl,
								 long forcedbitrate, int nloop, bool bupdatetimestamp)
{
	ostringstream oss;
	oss << "Starting to stream transport stream PIP file " << send_ts_file << " towards " << udp_ip_address << ":" << udp_port ;
	if(udp_ip_address_interface)
		oss <<" interface: " << udp_ip_address_interface ;
	oss << " TTL: " << ttl << endl;
	oss << "Streaming options:" << endl;
	if(forcedbitrate)
		oss << "  Forced bitrate: " << forcedbitrate << " b/s " ;
	oss << "  loop(s) : " ;
	if(nloop >= 0)
		oss << nloop+1 << endl;;
	if(nloop < 0)
		oss << "infinite" << endl;;

	if(bupdatetimestamp)
		oss << "  Time stamps updated after each loop " << endl;;

	msg = oss.str().c_str();
	return 1;
}

int InitFileStreamerOrRouterData(GLOBAL_PARAMETER* gParam,STREAM_PARAMETER* Param, DWORD dwParamLen)
{
	for(DWORD i = 0; i < dwParamLen; i++)
	{
		if(Param[i].ts_file.length()> 0)
		{
			//mainstream
			Param[i].Maindata.bPIP = false;
			Param[i].Maindata.loopCounter = 0;
			Param[i].Maindata.pMainStream = NULL;
			Param[i].Maindata.GlobalCounter = 0;
			Param[i].Maindata.LastGlobalCounter = 0;
			Param[i].Maindata.LastPCR = 0;
			Param[i].Maindata.FirstPCR = 0;
			Param[i].Maindata.bStop = false;
			Param[i].Maindata.bStopped = false;
			Param[i].Maindata.bStart = false;
			Param[i].Maindata.bReadytoStart = false;
			Param[i].Maindata.bError = false;
			Param[i].Maindata.ErrorMessage = "";
			Param[i].Maindata.send_ts_file = Param[i].ts_file.c_str();
			Param[i].Maindata.udp_ip_address = Param[i].udp_ip_address.c_str();
			Param[i].Maindata.udp_ip_address_interface = Param[i].udp_ip_address_interface.c_str();
			Param[i].Maindata.udp_port = Param[i].udp_port;
			Param[i].Maindata.ttl = Param[i].ttl;
			Param[i].Maindata.forcedbitrate= Param[i].forcedbitrate;
			Param[i].Maindata.bupdatetimestamp = Param[i].bupdatetimestamps;
			Param[i].Maindata.packetStart = Param[i].packetStart ;
			Param[i].Maindata.packetEnd = Param[i].packetEnd;
			Param[i].Maindata.timeStart = Param[i].timeStart ;
			Param[i].Maindata.timeEnd = Param[i].timeEnd ;
			Param[i].Maindata.nloop = Param[i].nloop;
			Param[i].Maindata.duration = 0;
			Param[i].Maindata.buffersize = Param[i].buffersize;
			Param[i].Maindata.lastduration = 0;
			Param[i].Maindata.hThread = 0;
			Param[i].Maindata.dwThreadID = 0;
			Param[i].Maindata.refresh_period = Param[i].refresh_period;


			//pipstream
			Param[i].PIPdata.bPIP = true;
			Param[i].PIPdata.loopCounter = 0;
			Param[i].PIPdata.pMainStream = &Param[i].Maindata;
			Param[i].PIPdata.GlobalCounter = 0;
			Param[i].PIPdata.LastGlobalCounter = 0;
			Param[i].PIPdata.LastPCR = 0;
			Param[i].PIPdata.FirstPCR = 0;
			Param[i].PIPdata.bStop = false;
			Param[i].PIPdata.bStopped = false;
			Param[i].PIPdata.bStart = false;
			Param[i].PIPdata.bReadytoStart = false;
			Param[i].PIPdata.bError = false;
			Param[i].PIPdata.ErrorMessage = "";
			Param[i].PIPdata.send_ts_file = Param[i].pip_ts_file.c_str();
			Param[i].PIPdata.udp_ip_address = Param[i].pip_udp_ip_address.c_str();
			Param[i].PIPdata.udp_ip_address_interface = Param[i].udp_ip_address_interface.c_str();
			Param[i].PIPdata.udp_port = Param[i].pip_udp_port;
			Param[i].PIPdata.ttl = Param[i].ttl;
			Param[i].PIPdata.forcedbitrate= Param[i].pip_forcedbitrate;
			Param[i].PIPdata.bupdatetimestamp = Param[i].bupdatetimestamps;
			Param[i].PIPdata.packetStart = 0 ;
			Param[i].PIPdata.packetEnd = 0xFFFFFFFF;
			Param[i].PIPdata.timeStart = 0 ;
			Param[i].PIPdata.timeEnd = 0xFFFFFFFF;
			Param[i].PIPdata.nloop = Param[i].nloop;
			Param[i].PIPdata.duration = 0;
			Param[i].PIPdata.buffersize = Param[i].pip_buffersize;
			Param[i].PIPdata.lastduration = 0;
			Param[i].PIPdata.hThread = 0;
			Param[i].PIPdata.dwThreadID = 0;
			Param[i].PIPdata.refresh_period = Param[i].refresh_period;

			if(IsTrace(gParam,Information))
			{
				string msg = "";
				GetMainStreamInformation(msg, 
					Param[i].ts_file.c_str(),
					Param[i].udp_ip_address.c_str(),
					Param[i].udp_port,
					Param[i].udp_ip_address_interface.c_str(),
					Param[i].ttl,
					Param[i].forcedbitrate,
					Param[i].nloop,
					Param[i].bupdatetimestamps,
					Param[i].timeStart,
					Param[i].timeEnd,
					Param[i].packetStart, 
					Param[i].packetEnd);
				TraceOut(gParam, Information, msg.c_str());

				if(Param[i].pip_ts_file.length()>0)
				{
					GetPIPStreamInformation(msg, 
						Param[i].pip_ts_file.c_str(),
						Param[i].pip_udp_ip_address.c_str(),
						Param[i].pip_udp_port,
						Param[i].udp_ip_address_interface.c_str(),
						Param[i].ttl,
						Param[i].pip_forcedbitrate,
						Param[i].nloop,
						Param[i].bupdatetimestamps);
					TraceOut(gParam, Information, msg.c_str());
				}
			}
		}
		else if(Param[i].input_udp_ip_address.length()> 0)
		{
			//mainstream
			Param[i].Maindata.bPIP = false;
			Param[i].Maindata.loopCounter = 0;
			Param[i].Maindata.pMainStream = NULL;
			Param[i].Maindata.GlobalCounter = 0;
			Param[i].Maindata.LastGlobalCounter = 0;
			Param[i].Maindata.LastPCR = 0;
			Param[i].Maindata.FirstPCR = 0;
			Param[i].Maindata.bStop = false;
			Param[i].Maindata.bStopped = false;
			Param[i].Maindata.bStart = false;
			Param[i].Maindata.bReadytoStart = false;
			Param[i].Maindata.bError = false;
			Param[i].Maindata.ErrorMessage = "";
			Param[i].Maindata.input_rtp = Param[i].input_rtp;
			Param[i].Maindata.input_udp_ip_address = Param[i].input_udp_ip_address.c_str();
			Param[i].Maindata.input_udp_ip_address_interface= Param[i].input_udp_ip_address_interface.c_str();
			Param[i].Maindata.input_udp_port= Param[i].input_udp_port;
			Param[i].Maindata.udp_ip_address = Param[i].udp_ip_address.c_str();
			Param[i].Maindata.udp_ip_address_interface = Param[i].udp_ip_address_interface.c_str();
			Param[i].Maindata.udp_port = Param[i].udp_port;
			Param[i].Maindata.ttl = Param[i].ttl;
			Param[i].Maindata.forcedbitrate= Param[i].forcedbitrate;
			Param[i].Maindata.bupdatetimestamp = Param[i].bupdatetimestamps;
			Param[i].Maindata.packetStart = Param[i].packetStart ;
			Param[i].Maindata.packetEnd = Param[i].packetEnd;
			Param[i].Maindata.timeStart = Param[i].timeStart ;
			Param[i].Maindata.timeEnd = Param[i].timeEnd ;
			Param[i].Maindata.nloop = Param[i].nloop;
			Param[i].Maindata.duration = 0;
			Param[i].Maindata.buffersize = Param[i].buffersize;
			Param[i].Maindata.lastduration = 0;
			Param[i].Maindata.hThread = 0;
			Param[i].Maindata.dwThreadID = 0;
			Param[i].Maindata.refresh_period = Param[i].refresh_period;


			//pipstream
			Param[i].PIPdata.bPIP = true;
			Param[i].PIPdata.loopCounter = 0;
			Param[i].PIPdata.pMainStream = &Param[i].Maindata;
			Param[i].PIPdata.GlobalCounter = 0;
			Param[i].PIPdata.LastGlobalCounter = 0;
			Param[i].PIPdata.LastPCR = 0;
			Param[i].PIPdata.FirstPCR = 0;
			Param[i].PIPdata.bStop = false;
			Param[i].PIPdata.bStopped = false;
			Param[i].PIPdata.bStart = false;
			Param[i].PIPdata.bReadytoStart = false;
			Param[i].PIPdata.bError = false;
			Param[i].PIPdata.ErrorMessage = "";
			Param[i].PIPdata.send_ts_file = "";
			Param[i].PIPdata.udp_ip_address = Param[i].pip_udp_ip_address.c_str();
			Param[i].PIPdata.udp_ip_address_interface = Param[i].udp_ip_address_interface.c_str();
			Param[i].PIPdata.udp_port = Param[i].pip_udp_port;
			Param[i].PIPdata.ttl = Param[i].ttl;
			Param[i].PIPdata.forcedbitrate= Param[i].pip_forcedbitrate;
			Param[i].PIPdata.bupdatetimestamp = Param[i].bupdatetimestamps;
			Param[i].PIPdata.packetStart = 0 ;
			Param[i].PIPdata.packetEnd = 0xFFFFFFFF;
			Param[i].PIPdata.timeStart = 0 ;
			Param[i].PIPdata.timeEnd = 0xFFFFFFFF;
			Param[i].PIPdata.nloop = Param[i].nloop;
			Param[i].PIPdata.duration = 0;
			Param[i].PIPdata.buffersize = Param[i].pip_buffersize;
			Param[i].PIPdata.lastduration = 0;
			Param[i].PIPdata.hThread = 0;
			Param[i].PIPdata.dwThreadID = 0;
			Param[i].PIPdata.refresh_period = Param[i].refresh_period;

			if(IsTrace(gParam,Information))
			{
				string msg = "";
				GetRouteStreamInformation(msg, 
					Param[i].input_udp_ip_address.c_str(),
					Param[i].input_udp_port,
					Param[i].input_udp_ip_address_interface.c_str(),
					Param[i].udp_ip_address.c_str(),
					Param[i].udp_port,
					Param[i].udp_ip_address_interface.c_str(),
					Param[i].ttl);
				TraceOut(gParam, Information, msg.c_str());

			}
		}
		else
			break;
	}
	return 1;
}
int InitStreamFileData(GLOBAL_PARAMETER* gParam,STREAM_PARAMETER* Param, DWORD dwParamLen)
{
	for(DWORD i = 0; i < dwParamLen; i++)
	{
		if(Param[i].ts_file.length()> 0)
		{
			//mainstream
			Param[i].Maindata.bPIP = false;
			Param[i].Maindata.loopCounter = 0;
			Param[i].Maindata.pMainStream = NULL;
			Param[i].Maindata.GlobalCounter = 0;
			Param[i].Maindata.LastGlobalCounter = 0;
			Param[i].Maindata.LastPCR = 0;
			Param[i].Maindata.FirstPCR = 0;
			Param[i].Maindata.bStop = false;
			Param[i].Maindata.bStopped = false;
			Param[i].Maindata.bStart = false;
			Param[i].Maindata.bReadytoStart = false;
			Param[i].Maindata.bError = false;
			Param[i].Maindata.ErrorMessage = "";
			Param[i].Maindata.send_ts_file = Param[i].ts_file.c_str();
			Param[i].Maindata.udp_ip_address = Param[i].udp_ip_address.c_str();
			Param[i].Maindata.udp_ip_address_interface = Param[i].udp_ip_address_interface.c_str();
			Param[i].Maindata.udp_port = Param[i].udp_port;
			Param[i].Maindata.ttl = Param[i].ttl;

			Param[i].Maindata.input_udp_ip_address = Param[i].input_udp_ip_address.c_str();
			Param[i].Maindata.input_udp_ip_address_interface = Param[i].input_udp_ip_address_interface.c_str();
			Param[i].Maindata.input_udp_port = Param[i].input_udp_port;
			Param[i].Maindata.input_rtp = Param[i].input_rtp;

			Param[i].Maindata.forcedbitrate= Param[i].forcedbitrate;
			Param[i].Maindata.bupdatetimestamp = Param[i].bupdatetimestamps;
			Param[i].Maindata.packetStart = Param[i].packetStart ;
			Param[i].Maindata.packetEnd = Param[i].packetEnd;
			Param[i].Maindata.timeStart = Param[i].timeStart ;
			Param[i].Maindata.timeEnd = Param[i].timeEnd ;
			Param[i].Maindata.nloop = Param[i].nloop;
			Param[i].Maindata.duration = 0;
			Param[i].Maindata.buffersize = Param[i].buffersize;
			Param[i].Maindata.lastduration = 0;
			Param[i].Maindata.hThread = 0;
			Param[i].Maindata.dwThreadID = 0;
			Param[i].Maindata.refresh_period = Param[i].refresh_period;


			//pipstream
			Param[i].PIPdata.bPIP = true;
			Param[i].PIPdata.loopCounter = 0;
			Param[i].PIPdata.pMainStream = &Param[i].Maindata;
			Param[i].PIPdata.GlobalCounter = 0;
			Param[i].PIPdata.LastGlobalCounter = 0;
			Param[i].PIPdata.LastPCR = 0;
			Param[i].PIPdata.FirstPCR = 0;
			Param[i].PIPdata.bStop = false;
			Param[i].PIPdata.bStopped = false;
			Param[i].PIPdata.bStart = false;
			Param[i].PIPdata.bReadytoStart = false;
			Param[i].PIPdata.bError = false;
			Param[i].PIPdata.ErrorMessage = "";
			Param[i].PIPdata.send_ts_file = Param[i].pip_ts_file.c_str();
			Param[i].PIPdata.udp_ip_address = Param[i].pip_udp_ip_address.c_str();
			Param[i].PIPdata.udp_ip_address_interface = Param[i].udp_ip_address_interface.c_str();
			Param[i].PIPdata.udp_port = Param[i].pip_udp_port;
			Param[i].PIPdata.ttl = Param[i].ttl;
			Param[i].PIPdata.forcedbitrate= Param[i].pip_forcedbitrate;
			Param[i].PIPdata.bupdatetimestamp = Param[i].bupdatetimestamps;
			Param[i].PIPdata.packetStart = 0 ;
			Param[i].PIPdata.packetEnd = 0xFFFFFFFF;
			Param[i].PIPdata.timeStart = 0 ;
			Param[i].PIPdata.timeEnd = 0xFFFFFFFF;
			Param[i].PIPdata.nloop = Param[i].nloop;
			Param[i].PIPdata.duration = 0;
			Param[i].PIPdata.buffersize = Param[i].pip_buffersize;
			Param[i].PIPdata.lastduration = 0;
			Param[i].PIPdata.hThread = 0;
			Param[i].PIPdata.dwThreadID = 0;
			Param[i].PIPdata.refresh_period = Param[i].refresh_period;

			if(IsTrace(gParam,Information))
			{
				string msg = "";
				GetMainStreamInformation(msg, 
					Param[i].ts_file.c_str(),
					Param[i].udp_ip_address.c_str(),
					Param[i].udp_port,
					Param[i].udp_ip_address_interface.c_str(),
					Param[i].ttl,
					Param[i].forcedbitrate,
					Param[i].nloop,
					Param[i].bupdatetimestamps,
					Param[i].timeStart,
					Param[i].timeEnd,
					Param[i].packetStart, 
					Param[i].packetEnd);
				TraceOut(gParam, Information, msg.c_str());

				if(Param[i].pip_ts_file.length()>0)
				{
					GetPIPStreamInformation(msg, 
						Param[i].pip_ts_file.c_str(),
						Param[i].pip_udp_ip_address.c_str(),
						Param[i].pip_udp_port,
						Param[i].udp_ip_address_interface.c_str(),
						Param[i].ttl,
						Param[i].pip_forcedbitrate,
						Param[i].nloop,
						Param[i].bupdatetimestamps);
					TraceOut(gParam, Information, msg.c_str());
				}
			}
		}
		else
			break;
	}
	return 1;
}

void ErrorStopService(LPTSTR lpszAPI)
{
   TCHAR   buffer[256]  = TEXT("");
   TCHAR   error[1024]  = TEXT("");
   LPVOID lpvMessageBuffer;

   ostringstream oss;
   oss << "API = " << lpszAPI << " error code = " << GetLastError() << endl;
   
   // Obtain the error string.
   FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&lpvMessageBuffer, 0, NULL);
   oss << "Message = " << (TCHAR *)lpvMessageBuffer << endl;

   TraceOut(&gGlobalParam, Information, oss.str().c_str());
   // Free the buffer allocated by the system.
   LocalFree(lpvMessageBuffer);


   // If you have threads running, tell them to stop. Something went
   // wrong, and you need to stop them so you can inform the SCM.
   SetEvent(hStopEvent);

   // Wait for the threads to stop.
   // Stop the service.
   SetTSRouteServiceStatus(SERVICE_STOPPED, GetLastError(), 0, 0);
}
void SetTSRouteServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode,
                         DWORD dwCheckPoint,   DWORD dwWaitHint)
{
   SERVICE_STATUS ss;  // Current status of the service.

   // Disable control requests until the service is started.
   if (dwCurrentState == SERVICE_START_PENDING)
      ss.dwControlsAccepted = 0;
   else
      ss.dwControlsAccepted =
         SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN;
         // Other flags include SERVICE_ACCEPT_PAUSE_CONTINUE
         // and SERVICE_ACCEPT_SHUTDOWN.

   // Initialize ss structure.
   ss.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
   ss.dwServiceSpecificExitCode = 0;
   ss.dwCurrentState            = dwCurrentState;
   ss.dwWin32ExitCode           = dwWin32ExitCode;
   ss.dwCheckPoint              = dwCheckPoint;
   ss.dwWaitHint                = dwWaitHint;

   // Send status of the service to the Service Controller.
   SetServiceStatus(ServiceStatusHandle, &ss);
}
VOID WINAPI ServiceCtrlHandler (DWORD dwCtrlCode) 
{ 
   DWORD dwState = SERVICE_RUNNING;

   switch(dwCtrlCode)
   {
      case SERVICE_CONTROL_STOP:
         dwState = SERVICE_STOP_PENDING;
         break;

      case SERVICE_CONTROL_SHUTDOWN:
         dwState = SERVICE_STOP_PENDING;
         break;

      case SERVICE_CONTROL_INTERROGATE:
         break;

      default:
         break;
   }

   // Set the status of the service.
   SetTSRouteServiceStatus(dwState, NO_ERROR, 0, 0);

   // Tell service_main thread to stop.
   if ((dwCtrlCode == SERVICE_CONTROL_STOP) ||
       (dwCtrlCode == SERVICE_CONTROL_SHUTDOWN))
   {
      if (!SetEvent(hStopEvent))
         ErrorStopService(TEXT("SetEvent"));
   }
}
void WINAPI ServiceStart (DWORD argc, LPTSTR *argv) 
{ 
	string s;
 
   // Obtain the name of the service.
   lpszServiceName = argv[0];

   // Register the service ctrl handler.
   ServiceStatusHandle = RegisterServiceCtrlHandler(lpszServiceName,
           (LPHANDLER_FUNCTION)ServiceCtrlHandler);
    if (ServiceStatusHandle == (SERVICE_STATUS_HANDLE)0) 
    { 
		ostringstream oss;
		oss << "[TSROUTE] RegisterServiceCtrlHandler failed " << GetLastError() << endl;
		TraceOut(&gGlobalParam, Information, oss.str().c_str());
        return; 
    } 

	// initialise the data
	{
		ostringstream oss;
		oss << "Initialising TSRoute Streamer" << endl;
		TraceOut(&gGlobalParam, Information, oss.str().c_str());
	}
	InitFileStreamerOrRouterData(&gGlobalParam,gStreamParam,gdwParamLen);

	Timer T;
	if(!T.Load())
    { 
		ostringstream oss;
		oss << "[TSROUTE] Timer failed " << GetLastError() << endl;
		TraceOut(&gGlobalParam, Information, oss.str().c_str());
        return; 
    } 
	T.Begin();

	// start thread
	{
		ostringstream oss;
		oss << "Starting TSRoute threads" << endl;
		TraceOut(&gGlobalParam, Information, oss.str().c_str());
	}
	if(!StartFileStreamerOrRouterThread(gStreamParam,gdwParamLen))
	{
		ostringstream oss;
		oss << "[TSROUTE] Starting TSTOOL threads failed " << GetLastError() << endl;
		TraceOut(&gGlobalParam, Information, oss.str().c_str());
        return; 
	}
	{
		ostringstream oss;
		oss << "TSRoute threads started" << endl;
		TraceOut(&gGlobalParam, Information, oss.str().c_str());
	}


    // Handle error condition 
    hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hStopEvent == NULL)
      ErrorStopService(TEXT("CreateEvent"));

   SetTSRouteServiceStatus(SERVICE_RUNNING, 0, 0, 0);

   s = lpszServiceName;
   s += " Service started";
   TraceOut(&gGlobalParam, Information, s.c_str());

   //
   // Main loop for the service.
   // 

   while(WaitForSingleObject(hStopEvent, 1000) != WAIT_OBJECT_0)
   {

/***************************************************************/ 
   // Main loop for service.
/***************************************************************/ 
		Sleep(1);
		DisplayFileStreamerOrRouterCounters(&gGlobalParam,gStreamParam,gdwParamLen);
		if(CheckStreamFileErrors(&gGlobalParam,gStreamParam,gdwParamLen,T))
			break;
   }
   // Wait for threads to exit.
	// stop thread
	{
		ostringstream oss;
		oss << "Stopping TSRoute threads" << endl;
		TraceOut(&gGlobalParam, Information, oss.str().c_str());
	}
	StopFileStreamerOrRouterThread(gStreamParam,gdwParamLen);
	// stop thread
	{
		ostringstream oss;
		oss << "TSRoute threads stopped" << endl;
		TraceOut(&gGlobalParam, Information, oss.str().c_str());
	}
	T.Unload();
   // Close the event handle and the thread handle.
   if (!CloseHandle(hStopEvent))
      ErrorStopService(TEXT("CloseHandle"));
   // Stop the service.
   s = lpszServiceName;
   s += " Service stopped";
   TraceOut(&gGlobalParam, Information, s.c_str());
   SetTSRouteServiceStatus(SERVICE_STOPPED, NO_ERROR, 0, 0);
 
} 


int DisplayCounters(const char* c_titre, PARSE_COUNTERS* pCounters,const char* parse_ts_file,bool bmessagebox)
{
	DWORD dwbr;
	ostringstream oss;
	oss << "File " << parse_ts_file << " analysis: " << endl<< endl;
	oss << " " << pCounters->PacketCount << " Packets detected" << endl<< endl; 

	for(int k = 0; k < (sizeof(pCounters->Counters)/sizeof(pCounters->Counters[0])); k++)
	{
		if(pCounters->Counters[k].PID!=0xFFFF)
		{

			oss << "PID detected   : " << pCounters->Counters[k].PID << endl;
			oss << "program_map_pid: " << pCounters->Counters[k].program_map_pid << endl;
			oss << "Program number : " << pCounters->Counters[k].program_number << endl;
			oss << "PCR_PID        : " << pCounters->Counters[k].PCR_PID << endl;
			oss << endl << endl;

			oss << endl <<  "Parsing Transport Stream PID - " << pCounters->Counters[k].PCR_PID << " - Number of packets detected: "<< pCounters->PacketCount << endl;
			string ts;
			ts = "PCR ";
			oss << endl;
			//oss << " " << pCounters->Counters[k].TimeStampPacketCount << " packets with "<< ts.c_str() <<" field detected" << endl; 

			if(pCounters->Counters[k].TimeStampPacketCount>0)
			{
				oss << "   First packet with "<< ts.c_str() <<" field:" <<endl;
				oss << "     Position               : " << pCounters->Counters[k].IndexFirstTimeStampPacket << endl;
				oss << "     "<< ts.c_str() <<"                   : " << pCounters->Counters[k].FirstTimeStamp << endl; 
				oss << "   Last packet with "<< ts.c_str() <<" field:" <<endl;
				oss << "     Position               : " << pCounters->Counters[k].IndexLastTimeStampPacket << endl;
				oss << "     "<< ts.c_str() <<"                   : " << pCounters->Counters[k].LastTimeStamp << endl; 
			//	oss << "   Max packets between "<< ts.c_str() <<" : " << pCounters->Counters[k].MaxPacketBetweenTimeStamp << endl; 
			//	oss << "   Min packets between "<< ts.c_str() <<" : " << pCounters->Counters[k].MinPacketBetweenTimeStamp << endl; 
			//	oss << "   "<< ts.c_str() <<" discontinuity       : " << pCounters->Counters[k].TimeStampDiscontinuity << endl;
			//	oss << "   "<< ts.c_str() <<" discontinuity Indic.: " << pCounters->Counters[k].DiscontinuityIndicator << endl;
			//	oss << "   Min "<< ts.c_str() <<" bitrate         : " <<pCounters->Counters[k].MinTimeStampBitrate << " b/s" << endl;
			//	oss << "   Max "<< ts.c_str() <<" bitrate         : " <<pCounters->Counters[k].MaxTimeStampBitrate << " b/s" << endl;
				dwbr = (DWORD) pCounters->Counters[k].bitrate ;
				oss << "   "<< ts.c_str() <<" bitrate             : " <<dwbr << " b/s" << endl;
				oss << "   "<< ts.c_str() <<" duration            : " <<pCounters->Counters[k].duration << " s" << endl;
			}
		}
	}			
			oss << endl;
		
	
	dwbr = (DWORD) pCounters->bitrate;
	oss << " Expected bitrate for the complete transport stream file  : " << dwbr << " b/s" << endl;
	oss << " Expected duration for the complete transport stream file : " << pCounters->duration << " seconds" << endl;
	oss << " Expected duration for the loop                           : " << pCounters->expectedduration << " seconds" << endl;
	
	oss << endl;

	if(bmessagebox)
		MessageBox(NULL, oss.str().c_str(), c_titre, MB_OK); 
	else
		cout << oss.str().c_str();
	return 1;
}

DWORD FAR PASCAL RecvProc( LPSTR lpData )
{
	int buffersize = PACKET_SIZE*7000;
	char buffer[70*PACKET_SIZE];
	RECV_STR* p = (RECV_STR*) lpData;
	if(p==NULL)
		return 0;

	HANDLE thisThread = GetCurrentThread();
	SetThreadPriority(thisThread, THREAD_PRIORITY_TIME_CRITICAL);

	//
	__int64 PCR = -1;
	__int64 FirstPCR = -1;

//	double d;
//	WORD PID;
	Timer T;
	T.Load();
	T.Begin();
	double deltamax = -1;
	double deltamin = -1;

	// Load TS file
	TSFile file;
	if(file.Create(p->recv_ts_file))
	{
		UDPMulticastReceiver Receiver;
		if(Receiver.Load(p->udp_ip_address,p->udp_port,buffersize,p->udp_ip_address_interface))
		{

			int len; 
			p->GlobalCounter=0;

			len = sizeof(buffer);
			while( (Receiver.Recv(buffer,&len)) && (p->bStop == false))
			{
				if(len != 0)
				{
					file.Write(buffer,len);
//
/*					BYTE* packet = (LPBYTE)buffer;
					int l   = len;

					while((packet != NULL) && (l >= PACKET_SIZE))
					{
						if((packet[0]==0x47)&& ((packet[1]&0x80)==0))
						{
							if( ((packet[3]&0x20)!=0) && (packet[4]!=0))
							 {
								 if(packet[5]&0x10)
								 {
									PID= MAKEWORD(packet[2], packet[1]&0x1F);
									__int64 t = (((packet[6] << 24) | (packet[7] << 16) | (packet[8] << 8) | (packet[9])) & 0xFFFFFFFF);
									t = t << 1;

									if((packet[10] & 0x80)== 0x80)
										PCR = (t) | 0x000000001;
									else
										PCR = (t) & 0xFFFFFFFFE;
									T.End(d);

									ostringstream oss;
									double pcr = (double) PCR/90000;
									double delta = pcr - d;
									
									if((delta > deltamax) || (deltamax == -1))
										deltamax = delta;
									if((delta < deltamin) || (deltamin == -1))
										deltamin = delta;

									oss << "PCR received at " << d << " s, PCR:"<< PCR << " - " << pcr << " s, delta " << deltamin << " < " << delta << " < " << deltamax << " , PID: " << PID << endl;
									TraceOut(&gGlobalParam, Information, oss.str().c_str());

								 }
							 }
						}
						l -= PACKET_SIZE;
						packet += PACKET_SIZE;
					}
*/
					p->GlobalCounter += len;
				}
				len = sizeof(buffer);
			}
			if(Receiver.GetLastError()!=0)
			{
				p->bError = true;
				ostringstream oss;
				oss <<  "Socket Error:" << Receiver.GetLastError() << endl;
				p->ErrorMessage = oss.str().c_str();
			}

			Receiver.Unload();
		}
		else
		{
			p->bError = true;
			ostringstream oss;
			oss <<  "Socket Error:" << Receiver.GetLastError() << endl;
			p->ErrorMessage = oss.str().c_str();
		}
		file.Close();
	}
	p->bStopped = true;
	return 1;
}
DWORD FAR PASCAL RecvStream( LPSTR lpData )
{
	int buffersize = PACKET_SIZE*7000;
	char buffer[7*PACKET_SIZE];
	CHECK_STR* p = (CHECK_STR*) lpData;
	if(p==NULL)
		return 0;

	HANDLE thisThread = GetCurrentThread();
	SetThreadPriority(thisThread, THREAD_PRIORITY_TIME_CRITICAL);

	//
	__int64 PCR = -1;
	__int64 FirstPCR = -1;

//	double d;
//	WORD PID;
	Timer T;
	T.Load();
	T.Begin();
	double deltamax = -1;
	double deltamin = -1;

	// Load TS file
	TSFile file;
//	if(file.Create(p->recv_ts_file))
//	{
		p->ContinuityErrorCounter = 0;
		p->PCRAccuracyErrorCounter = 0;
		p->packetCounter = 0;
		p->bitrate = 0;
		file.InitPIDInformation();

		UDPMulticastReceiver Receiver;
		if(Receiver.Load(p->udp_ip_address,p->udp_port,buffersize,p->udp_ip_address_interface))
		{

			int len; 
			p->GlobalCounter=0;
			__int64 packetCount = 0;
			__int64 delta = 0;
			__int64 PCR = -1;
			__int64 LastPCR = -1;
			__int64 FirstPCR = -1;
			__int64 PCRPacketCounter = 0;
			__int64 PCRCounter = 0;
			p->bitrate = 0; 
			bool AccuracyError = false;
			len = sizeof(buffer);
			Timer TTempo;
			if(TTempo.Load()==0)
				return 0;

			while( (Receiver.Recv(buffer,&len)) && (p->bStop == false))
			{
				if(len != 0)
				{
					file.Write(buffer,len);

					BYTE* packet = (LPBYTE)buffer;
					int l   = len;

					while((packet != NULL) && (l >= PACKET_SIZE))
					{
						WORD PID = 0xFFFF;
						PID = MAKEWORD(packet[2], packet[1]&0x1F);
						{
							string s;
							if(file.CheckContinuityCounter((LPBYTE)packet,packetCount,s))
							{
								p->ContinuityErrorCounter++;
								TraceOut(&gGlobalParam,Information,s.c_str());							
							}


							// Check PCR Accuracy
							if(p->bitrate>0)
							{
								__int64 Accuracy;

								if(file.GetPCRInAdaptationField((LPBYTE)packet,&PCR))
		//						file.GetPIDPCRfromPacket((char *)p,PCR_PID, &PCR,&bDiscontinuity);
		//						if(PCR!=-1)
								{
																							
									WORD PID = MAKEWORD(packet[2],packet[1]&0x1F);

									if(file.GetLastPCRCounter(PID,LastPCR,PCRPacketCounter)==false)
										file.SetLastPCRCounter(PID, PCR,packetCount);
									else
									{
										delta = 0;

/*										if((PCR-LastPCR)>=0)
											delta = (PCR-LastPCR);
										else
											delta = 0x40000000000 - (PCR - LastPCR);
											*/

										if (((PCR & 0x20000000000) == 0) && ((LastPCR & 0x20000000000) != 0))
											delta = (0x257FFFFFFFF - LastPCR) + PCR;
										else
											delta = PCR - LastPCR;

										Accuracy = ((__int64)(packetCount-PCRPacketCounter)*PACKET_SIZE*8*1000000000)/p->bitrate;
										Accuracy -= delta*1000/27 ;

										if (Accuracy<0)
											Accuracy = -Accuracy;
										if(Accuracy > 500)
										{
											if(AccuracyError == false)
											{
												AccuracyError = true;
												p->PCRAccuracyErrorCounter++;
 												ostringstream oss;
												oss << "PCR Accuracy Error starts with packet " << packetCount << " PID: "  << PID << " Inaccuracy: " << Accuracy << " ns " <<  endl;
												s = oss.str().c_str();
												TraceOut(&gGlobalParam,Information,s.c_str());							
											}
										}
										else
										{
											if(AccuracyError == true)
											{
												AccuracyError = false;
 												ostringstream oss;
												oss << "PCR Accuracy Error ends with packet " << packetCount << " PID: "  << PID << " Inaccuracy: " << Accuracy << " ns " << endl;
												s = oss.str().c_str();
												TraceOut(&gGlobalParam,Information,s.c_str());							
											}
										}



									}
									file.SetLastPCRCounter(PID, PCR,packetCount);
								}		
							}
							else
							{
								// We need to discover the bitrate
								if(file.GetPCRInAdaptationField((LPBYTE)packet,&PCR))
								{
									if(file.GetFirstPCRCounter(PID,LastPCR,PCRPacketCounter)==false)
									{
										file.SetFirstPCRCounter(PID, PCR,packetCount);
										TTempo.Begin();
									}
									else
									{
										double duration;
										TTempo.End(duration);
										file.SetLastPCRCounter(PID, PCR,packetCount);
										// more than 10 seconds of packet reception
										// sufficient to evaluate the bitrate
										if(duration > 10)
										{
											if(file.GetFirstPCRCounter(PID,FirstPCR,PCRPacketCounter)==true)
											{
												/*
												if((PCR-FirstPCR)>=0)
													delta = (PCR-FirstPCR);
												else
													delta = 0x40000000000 - (PCR - FirstPCR);
													*/

												if (((PCR & 0x20000000000) == 0) && ((FirstPCR & 0x20000000000) != 0))
													delta = (0x257FFFFFFFF - FirstPCR) + PCR;
												else
													delta = PCR - FirstPCR;
												

												__int64 locbitrate = (packetCount-PCRPacketCounter)*PACKET_SIZE*8;
												if(delta>0)
												{
													p->bitrate =  (locbitrate*27000000) / delta;
													
												}
											}

										}

									}
								}
							}
							if( packetCount%8000 == 0) 
								Sleep(1);
						}
						packet = (BYTE*) file.GetNextPacket((PACKET *)packet);
						packetCount++;

						p->packetCounter = packetCount;


						l -= PACKET_SIZE;
						packet += PACKET_SIZE;
					}

					p->GlobalCounter += len;
				}
				len = sizeof(buffer);
			}
			Receiver.Unload();
		}
//		file.Close();
//	}
	p->bStopped = true;
	return 1;
}
int RecvFile(GLOBAL_PARAMETER* gParam, const char* recv_ts_file, const char* udp_ip_address, int udp_port, const char* udp_ip_address_interface,unsigned long refresh_period)
{
    string titre = "Information about Transport Stream Tool Receiver"; 
    const char *c_titre = titre.c_str ( ); 	

	RECV_STR data;
	data.GlobalCounter = 0;
	data.bStop = false;
	data.bStopped = false;
	data.bError = false;
	data.ErrorMessage = "";
	data.recv_ts_file = recv_ts_file;
	data.udp_ip_address = udp_ip_address;
	data.udp_ip_address_interface = udp_ip_address_interface;
	data.udp_port = udp_port;
	data.refresh_period = refresh_period;
	Timer T;
	if(T.Load())
	{
		__int64 GlobalCounter=0;
		__int64 LocalCounter=0;
		Timer LocalTimer;
		LocalTimer.Load();

		if(IsTrace(gParam,Information))
		{
			ostringstream oss;

			oss << "Starting to receive transport stream file " << recv_ts_file << " from " << udp_ip_address << ":" << udp_port << endl;
			oss << "Press any key to stop the capture " << endl;
			TraceOut(gParam, Information, oss.str().c_str());
		}
		T.Begin();
		LocalTimer.Begin();
		HANDLE hThread;
		DWORD dwThreadID;
		if (NULL != (hThread  =
						CreateThread( (LPSECURITY_ATTRIBUTES) NULL,
										0, 
										(LPTHREAD_START_ROUTINE) RecvProc,
										(LPVOID) &data,
										0, &dwThreadID )))
		{

			while(!_kbhit())
			{
				Sleep(1);

				double period;
				LocalTimer.End(period);
				if(period>data.refresh_period)
				{
					if(IsTrace(gParam,Information))
					{

						double duration;
						T.End(duration);
						ostringstream oss;
						oss << endl <<"Status after " << duration << " seconds:" <<endl;

						if(GlobalCounter<=data.GlobalCounter)
							LocalCounter = data.GlobalCounter-GlobalCounter;
						else
							LocalCounter = 0xFFFFFFFFFFFFFFFF - (GlobalCounter - data.GlobalCounter);

						GlobalCounter = data.GlobalCounter;
						oss << "Number of bytes received for this period: " << LocalCounter << endl;
						__int64 currentbitrate = LocalCounter*8;
						__int64 t = (__int64) period*1000;
						currentbitrate = (currentbitrate*1000)/t;
						oss << "Current bitrate for this period         : " << currentbitrate << " b/s" << endl;
						oss << "Number of bytes received                : " << GlobalCounter << endl;
						__int64 averagebitrate = GlobalCounter*8;
						t = (__int64) duration*1000;
						averagebitrate = (averagebitrate*1000)/t;

						oss << "Average bitrate                         : " << averagebitrate << " b/s"<< endl;
						
						TraceOut(gParam, Information, oss.str().c_str());
					}
					LocalTimer.Begin();
				}
				if(data.bStopped==true)
				{
					if(IsTrace(gParam,Information))
					{
						ostringstream oss;
						oss << endl <<"Reception thread stopped, not enough memory or network configuration error" <<endl;
						if(data.ErrorMessage.length()>0)
							oss << data.ErrorMessage.c_str()<<endl;
						TraceOut(gParam, Information, oss.str().c_str());
					}
					break;
				}

			}
			data.bStop = true;
			Sleep(1000);
			if(data.bStopped == false)
				TerminateThread(hThread,1);
			CloseHandle(hThread);

		}
		if(IsTrace(gParam,Information))
		{
			double duration;
			T.End(duration);
			ostringstream oss;
			oss << "End of reception, duration: " << duration << " seconds" << endl;
			TraceOut(gParam, Information, oss.str().c_str());

		}
		T.Unload();

	}
	return 1;
}
int MRouteStream(GLOBAL_PARAMETER* gParam, 
				 const char* udp_ip_address, 
				 int udp_port, 
				 const char* udp_ip_address_interface,
				 int ttl,
				 const char* input_udp_ip_address, 
				 int input_udp_port, 
				 const char* input_udp_ip_address_interface,
				 bool input_rtp, 
				 unsigned long refresh_period)
{
    string titre = "Information about Transport Stream Multicast Router"; 
    const char *c_titre = titre.c_str ( ); 	

	ROUTE_STR data;
	data.GlobalCounter = 0;
	data.bStop = false;
	data.bStopped = false;
	data.udp_ip_address = udp_ip_address;
	data.udp_ip_address_interface = udp_ip_address_interface;
	data.udp_port = udp_port;
	data.ttl = ttl;
	data.input_udp_ip_address = input_udp_ip_address;
	data.input_udp_ip_address_interface = input_udp_ip_address_interface;
	data.input_udp_port = input_udp_port;
	data.input_rtp = input_rtp;
	data.refresh_period = refresh_period;
	Timer T;
	if(T.Load())
	{
		__int64 GlobalCounter=0;
		__int64 LocalCounter=0;
		Timer LocalTimer;
		LocalTimer.Load();

		if(IsTrace(gParam,Information))
		{
			ostringstream oss;

			oss << "Starting to route multicast stream " << endl;
			oss << "from Interface: " << (input_udp_ip_address_interface==NULL || strlen(input_udp_ip_address_interface)==0?" default " : input_udp_ip_address_interface) << (input_rtp==true?" RTP stream: ":" stream: ") << input_udp_ip_address << ":" << input_udp_port <<   endl;
			oss << "to Interface: " << (udp_ip_address_interface==NULL || strlen(udp_ip_address_interface)==0?" default " :udp_ip_address_interface) << udp_ip_address << ":" << udp_port << endl;
			oss << "Press any key to stop the router " << endl;
			TraceOut(gParam, Information, oss.str().c_str());
		}
		T.Begin();
		LocalTimer.Begin();
		HANDLE hThread;
		DWORD dwThreadID;
		if (NULL != (hThread  =
						CreateThread( (LPSECURITY_ATTRIBUTES) NULL,
										0, 
										(LPTHREAD_START_ROUTINE) RouteProc,
										(LPVOID) &data,
										0, &dwThreadID )))
		{

			while(!_kbhit())
			{
				Sleep(1);

				double period;
				LocalTimer.End(period);
				if(period>data.refresh_period)
				{
					if(IsTrace(gParam,Information))
					{

						double duration;
						T.End(duration);
						ostringstream oss;
						oss << endl <<"Status after " << duration << " seconds:" <<endl;

						if(GlobalCounter<=data.GlobalCounter)
							LocalCounter = data.GlobalCounter-GlobalCounter;
						else
							LocalCounter = 0xFFFFFFFFFFFFFFFF - (GlobalCounter - data.GlobalCounter);

						GlobalCounter = data.GlobalCounter;
						oss << "Number of bytes received for this period: " << LocalCounter << endl;
						__int64 currentbitrate = LocalCounter*8;
						__int64 t = (__int64) period*1000;
						currentbitrate = (currentbitrate*1000)/t;
						oss << "Current bitrate for this period         : " << currentbitrate << " b/s" << endl;
						oss << "Number of bytes received                : " << GlobalCounter << endl;
						__int64 averagebitrate = GlobalCounter*8;
						t = (__int64) duration*1000;
						averagebitrate = (averagebitrate*1000)/t;

						oss << "Average bitrate                         : " << averagebitrate << " b/s"<< endl;
						
						TraceOut(gParam, Information, oss.str().c_str());
					}
					LocalTimer.Begin();
				}
				if(data.bStopped==true)
				{
					if(IsTrace(gParam,Information))
					{
						ostringstream oss;
						oss << endl <<"Router thread stopped, not enough memory or network configuration error" <<endl;
						TraceOut(gParam, Information, oss.str().c_str());
					}
					break;
				}

			}
			data.bStop = true;
			Sleep(1000);
			if(data.bStopped == false)
				TerminateThread(hThread,1);
			CloseHandle(hThread);

		}
		if(IsTrace(gParam,Information))
		{
			double duration;
			T.End(duration);
			ostringstream oss;
			oss << "End of router, duration: " << duration << " seconds" << endl;
			TraceOut(gParam, Information, oss.str().c_str());

		}
		T.Unload();

	}
	return 1;
}
int CheckStream(GLOBAL_PARAMETER* gParam, const char* udp_ip_address, int udp_port, const char* udp_ip_address_interface,unsigned long refresh_period)
{
    string titre = "Information about Transport Stream Tool Receiver"; 
    const char *c_titre = titre.c_str ( ); 	

	CHECK_STR data;
	data.GlobalCounter = 0;
	data.bStop = false;
	data.bStopped = false;
	data.recv_ts_file = "";
	data.udp_ip_address = udp_ip_address;
	data.udp_ip_address_interface = udp_ip_address_interface;
	data.udp_port = udp_port;
	data.refresh_period = refresh_period;
	Timer T;
	data.UnderrunErrorCounter = 0;
	bool bUnderrunErrorReady = false;
	bool bUnderrunErrorDetected = false;

	if(T.Load())
	{
		__int64 GlobalCounter=0;
		__int64 LocalCounter=0;
		Timer LocalTimer;
		LocalTimer.Load();

		if(IsTrace(gParam,Information))
		{
			ostringstream oss;

			oss << "Starting to check transport stream from " << udp_ip_address << ":" << udp_port << endl;
			oss << "Press any key to stop the capture " << endl;
			TraceOut(gParam, Information, oss.str().c_str());
		}
		T.Begin();
		LocalTimer.Begin();
		HANDLE hThread;
		DWORD dwThreadID;
		if (NULL != (hThread  =
						CreateThread( (LPSECURITY_ATTRIBUTES) NULL,
										0, 
										(LPTHREAD_START_ROUTINE) RecvStream,
										(LPVOID) &data,
										0, &dwThreadID )))
		{

			while(!_kbhit())
			{
				Sleep(1);

				double period;
				LocalTimer.End(period);
				if(period>data.refresh_period)
				{
					if(IsTrace(gParam,Information))
					{

						double duration;
						T.End(duration);
						ostringstream oss;
						oss << endl <<"Status after " << duration << " seconds:" <<endl;

						if(GlobalCounter<=data.GlobalCounter)
							LocalCounter = data.GlobalCounter-GlobalCounter;
						else
							LocalCounter = 0xFFFFFFFFFFFFFFFF - (GlobalCounter - data.GlobalCounter);

						GlobalCounter = data.GlobalCounter;
						oss << "Number of bytes received for this period: " << LocalCounter << endl;
						__int64 currentbitrate = LocalCounter*8*1000;
						__int64 t = (__int64) period*1000;
						currentbitrate = currentbitrate/t;
						oss << "Current bitrate for this period         : " << currentbitrate << " b/s" << endl;
						oss << "Number of bytes received                : " << GlobalCounter << endl;
						__int64 averagebitrate = data.packetCounter*8*PACKET_SIZE;
						t = (__int64)duration*1000;
						averagebitrate = (averagebitrate*1000)/t;


						if(bUnderrunErrorReady==false)
						{
							if((data.bitrate>0) && (currentbitrate > data.bitrate*0.90))
								bUnderrunErrorReady=true;				
						}
						else
						{
							if(bUnderrunErrorDetected == false)
							{
								if((data.bitrate>0) && (currentbitrate < data.bitrate*0.90))
								{
									data.UnderrunErrorCounter++;
									bUnderrunErrorDetected = true;
								}
							}
							else
							{
								if((data.bitrate>0) && (currentbitrate > data.bitrate*0.90))
									bUnderrunErrorDetected = false;
							}
						}

						oss << "Average bitrate                         : " << averagebitrate << " b/s"<< endl;
						oss << "Continuity Errors detected              : " << data.ContinuityErrorCounter << endl;
						oss << "PCR Accuracy Errors detected            : " << data.PCRAccuracyErrorCounter << endl;
						oss << "Underrun Errors detected                : " << data.UnderrunErrorCounter << endl;
						oss << "Packet received                         : " << data.packetCounter << endl;
						oss << "Bitrate detected                        : " << data.bitrate << " b/s"<< endl;
						
						TraceOut(gParam, Information, oss.str().c_str());
					}
					LocalTimer.Begin();
				}
				if(data.bStopped==true)
				{
					if(IsTrace(gParam,Information))
					{
						ostringstream oss;
						oss << endl <<"Reception thread stopped, not enough memory or network configuration error" <<endl;
						TraceOut(gParam, Information, oss.str().c_str());
					}
				break;
				}

			}
			data.bStop = true;
			Sleep(1000);
			if(data.bStopped == false)
				TerminateThread(hThread,1);
			CloseHandle(hThread);

		}
		if(IsTrace(gParam,Information))
		{
			double duration;
			T.End(duration);
			ostringstream oss;
			oss << "End of reception, duration: " << duration << " seconds" << endl;
			TraceOut(gParam, Information, oss.str().c_str());

		}
		T.Unload();

	}
	return 1;
}
/*
int StreamFile(GLOBAL_PARAMETER* gParam, STREAM_PARAMETER* Param, DWORD dwParamLen)
{
	if(Param == NULL)
		return 0;
	// innitialise the data
	{
		ostringstream oss;
		oss << "Initialising TSRoute Streamer" << endl;
		TraceOut(gParam, Information, oss.str().c_str());
	}
	InitStreamFileData(gParam,Param,dwParamLen);

	Timer T;
	if(T.Load())
	{		
		T.Begin();
		// start thread
		{
			ostringstream oss;
			oss << "Starting TSRoute threads" << endl;
			TraceOut(gParam, Information, oss.str().c_str());
		}
		if(StartStreamFileThread(Param,dwParamLen))
		{			
			// start thread
			{
				ostringstream oss;
				oss << "TSRoute threads started" << endl;
				TraceOut(gParam, Information, oss.str().c_str());
			}
			if(gParam->bSyncTransmission)
			{
				ostringstream oss;
				oss << "As Sync Transmission is activated, wait until complete parsing of TS files..." << endl;
				TraceOut(gParam, Information, oss.str().c_str());
			}
			while(!_kbhit())
			{
				Sleep(1);
				DisplayFileStreamerOrRouterCounters(gParam,Param,dwParamLen);
				if(CheckStreamFileErrors(gParam,Param,dwParamLen,T))
					break;
			}
			// stop thread
			{
				ostringstream oss;
				oss << "Stopping TSRoute threads" << endl;
				TraceOut(gParam, Information, oss.str().c_str());
			}
			StopStreamFileThread(Param,dwParamLen);
			// stop thread
			{
				ostringstream oss;
				oss << "TSRoute threads stopped" << endl;
				TraceOut(gParam, Information, oss.str().c_str());
			}
		}
		T.Unload();
	}
	return 1;

}
*/
int StreamFileOrRoute(GLOBAL_PARAMETER* gParam, STREAM_PARAMETER* Param, DWORD dwParamLen)
{
	if(Param == NULL)
		return 0;
	// innitialise the data
	{
		ostringstream oss;
		oss << "Initialising TSRoute Streamer" << endl;
		TraceOut(gParam, Information, oss.str().c_str());
	}
	InitFileStreamerOrRouterData(gParam,Param,dwParamLen);

	Timer T;
	if(T.Load())
	{		
		T.Begin();
		// start thread
		{
			ostringstream oss;
			oss << "Starting TSRoute threads" << endl;
			TraceOut(gParam, Information, oss.str().c_str());
		}
		if(StartFileStreamerOrRouterThread(Param,dwParamLen))
		{			
			// start thread
			{
				ostringstream oss;
				oss << "TSRoute threads started" << endl;
				TraceOut(gParam, Information, oss.str().c_str());
			}
			if(gParam->bSyncTransmission)
			{
				ostringstream oss;
				oss << "As Sync Transmission is activated, wait until complete parsing of TS files..." << endl;
				TraceOut(gParam, Information, oss.str().c_str());
			}
			while(!_kbhit())
			{
				Sleep(1);
				DisplayFileStreamerOrRouterCounters(gParam,Param,dwParamLen);
				if(CheckStreamFileErrors(gParam,Param,dwParamLen,T))
					break;
			}
			// stop thread
			{
				ostringstream oss;
				oss << "Stopping TSRoute threads" << endl;
				TraceOut(gParam, Information, oss.str().c_str());
			}
			StopFileStreamerOrRouterThread(Param,dwParamLen);
			// stop thread
			{
				ostringstream oss;
				oss << "TSRoute threads stopped" << endl;
				TraceOut(gParam, Information, oss.str().c_str());
			}
		}
		T.Unload();
	}
	return 1;

}
int ConvertFile(GLOBAL_PARAMETER* pG, const char* input_file,const char* output_file, DWORD& count, string& error_string)
{
	TSFile input;
	TSFile output;
	__int64 dwLen = 0;

	count = 0;
	error_string = "";
	if( (input_file!=NULL) && (output_file!=NULL))
	{
		if(input.Open(input_file)!=0)
		{
			if((dwLen = input.Load(4096*1024))!=0)
			{
				int pl = input.GetPacketLength();
				if(pl == 204)
				{
					if(output.Create(output_file)!=0)
					{
						BYTE Buffer[204];
						input.ResetRead();
						while(input.Read204(Buffer))
						{
							count++;
							if(output.Write((char*)Buffer,188)==0)
								return 0;
						}
						return 1;
					}
					else
						error_string = "Can't create output file";
				}
				else
					error_string = "The input file doesn't contain 204 bytes packets";

			}
				else
					error_string = "Can't create output file";
		}
		else
			error_string = "Can't open input file";
	}
	else
		error_string = "Incorrect parameters";
	return 0;
}
int FilterFile(GLOBAL_PARAMETER* pG, const char* input_file,const char* output_file, WORD Program, DWORD& count, string& error_string)
{
	TSFile input;
	TSFile output;
	__int64 dwLen = 0;

	count = 0;
	error_string = "";
	if( (input_file!=NULL) && (output_file!=NULL))
	{
		if(input.Open(input_file)!=0)
		{
			if((dwLen = input.Load(4096*1024))!=0)
			{
				int pl = input.GetPacketLength();
				if(pl == 188)
				{
					PARSE_COUNTERS Counters;
// June 10
//					input.SetStep(1);
					PID_INFO PIDArray[64];
					WORD PIDBlackList[64];
					// NULL Packet
					PIDBlackList[0] = 0x1FFF;
					for(int  i = 1; i < sizeof(PIDBlackList)/sizeof(WORD); i++)
						PIDBlackList[i] = 0xFFFF;
					for(int  i = 0; i < sizeof(PIDArray)/sizeof(PID_INFO); i++)
					{
						PIDArray[i].PID = 0xFFFF;
						PIDArray[i].Type = 0xFF;
					}
					Counters.Count = 0;
					Counters.duration = 0;
					Counters.PacketCount = 0;
					Counters.bError = false;


					for(int m = 0; m < MAX_PID; m++)
					{
						Counters.Counters[m].bitrate = 0;
						Counters.Counters[m].duration = 0;
						Counters.Counters[m].PID = 0xFFFF;
						Counters.Counters[m].PCR_PID = 0xFFFF;
						Counters.Counters[m].program_map_pid = 0xFFFF;
						Counters.Counters[m].program_number = 0;
						Counters.Counters[m].TimeStampPacketCount = 0;

						Counters.Counters[m].IndexFirstTimeStampPacket = -1;
						Counters.Counters[m].IndexLastTimeStampPacket = -1;

						Counters.Counters[m].FirstTimeStamp = -1;
						Counters.Counters[m].LastTimeStamp = -1;

						Counters.Counters[m].MinPacketBetweenTimeStamp = -1;
						Counters.Counters[m].MaxPacketBetweenTimeStamp = -1;

						Counters.Counters[m].TimeStampDiscontinuity = 0;
						Counters.Counters[m].DiscontinuityIndicator = 0;

						Counters.Counters[m].MinTimeStampBitrate = -1;
						Counters.Counters[m].MaxTimeStampBitrate = -1;
					}

					if(input.GetProgramMapTableID(&Counters)>0)
					{
						for(int i =  0; i < Counters.Count; i++)
						{														
							DWORD dw = sizeof(PIDArray)/sizeof(PID_INFO);
							WORD PCR_PID = Counters.Counters[i].PCR_PID = input.GetPCR_PID(Counters.Counters[i].program_map_pid,PIDArray,dw);

							
							if(Counters.Counters[i].program_number != Program)
							{
								for(int  k = 0; k < sizeof(PIDBlackList)/sizeof(WORD); k++)
								{
									if(PIDBlackList[k] == Counters.Counters[i].program_map_pid)
										break;
									if(PIDBlackList[k] == 0xFFFF)
									{
										PIDBlackList[k] = Counters.Counters[i].program_map_pid;
										break;
									}
								}
								for(int  j = 0; j < sizeof(PIDArray)/sizeof(PID_INFO); j++)
								{
									if(PIDArray[j].PID != 0xFFFF)
									{
										for(int  k = 0; k < sizeof(PIDBlackList)/sizeof(WORD); k++)
										{
											if(PIDBlackList[k] == PIDArray[j].PID)
												break;
											if(PIDBlackList[k] == 0xFFFF)
											{
												PIDBlackList[k] = PIDArray[j].PID;
												break;
											}
										}
									}
									else
										break;
								}
							}
						}
					}
					if(output.Create(output_file)!=0)
					{
						BYTE Buffer[204];
						input.ResetRead();
						while(input.Read188(Buffer))
						{
						
							WORD locPID = MAKEWORD(Buffer[2], Buffer[1]&0x1F);
							BOOL bfound = FALSE;
							for(int  k = 0; k < sizeof(PIDBlackList)/sizeof(WORD); k++)
							{
								if(PIDBlackList[k] == locPID)
								{
									bfound = TRUE;
									break;
								}
								if(PIDBlackList[k] == 0xFFFF)
									break;
							}
							if(bfound == FALSE)
							{
								count++;
								if(output.Write((char*)Buffer,188)==0)
									return 0;
							}
						}
						return 1;
					}
					else
						error_string = "Can't create output file";
				}
				else
					error_string = "The input file doesn't contain 204 bytes packets";

			}
				else
					error_string = "Can't create output file";
		}
		else
			error_string = "Can't open input file";
	}
	else
		error_string = "Incorrect parameters";
	return 0;
}

int ParseFile(GLOBAL_PARAMETER* pG, const char* parse_ts_file,long packetStart,long packetEnd, string& error_string ,int pid = -1,unsigned long buffersize = 0)
{
    string titre = "Information about Transport Stream Tool Parser"; 
    const char *c_titre = titre.c_str ( ); 	

	HANDLE thisThread = GetCurrentThread();
	SetThreadPriority(thisThread, THREAD_PRIORITY_TIME_CRITICAL);

	// Load TS file
	__int64 dwLen;
	TSFile file;
	if(file.Open(parse_ts_file))
	{
		if((dwLen = file.Load(buffersize))!=0)
		{
			PARSE_COUNTERS Counters;
			TSFile::Parse_Mask Mask = TSFile::none;

			Mask = TSFile::none;
			if(IsTrace(pG,TSPID))
				Mask = TSFile::pid;
			if(IsTrace(pG,TSTimeStamp))
				Mask = TSFile::timestamp;
			if(IsTrace(pG,TSPacket))
				Mask = TSFile::ts;

			{
				ostringstream oss;
				oss << "Starting to parse transport stream file " << parse_ts_file << endl;
				
				if ( (IsTrace(pG,Information)) ||
					(pid != -1) ||
					((packetStart!=0)||(packetEnd!=0xFFFFFFFF)))
					oss << "Parsing options:" << endl;
				if(packetEnd!=0xFFFFFFFF)
					oss << "  Parse from packet " << packetStart << " till packet " << packetEnd << endl;
				else
					oss << "  Parse from packet " << packetStart << " till packet " << (dwLen/PACKET_SIZE)-1 << endl;
				if((pG->console_trace_level == Information) || (pG->trace_level == Information))
					oss << "  Display information about PID and PCR_PID" << endl;
				if((pG->console_trace_level == TSTimeStamp) || (pG->trace_level == TSTimeStamp))
					oss << "  Filter: Parse packets with Time Stamp (PCR, OPCR, DTS, PTS" << endl;
				if((pG->console_trace_level == TSPacket) || (pG->trace_level == TSPacket))
					oss << "  Filter: Parse all the TS packets" << endl;
				if(pid != -1)
					oss << "  Filter: Parse packets with PID = " << pid << " only " << endl;
				TraceOut(pG,Information,oss.str().c_str());
			}
			file.SetLimit(packetStart,packetEnd);
// June 10
// 			file.SetStep(1);
			if(IsTrace(pG,Information))
			{
				PID_INFO PIDArray[64];
				for(int  i = 0; i < sizeof(PIDArray)/sizeof(PID_INFO); i++)
				{
					PIDArray[i].PID = 0xFFFF;
					PIDArray[i].Type = 0xFF;
				}


				Counters.Count = 0;
				Counters.duration = 0;
				Counters.PacketCount = 0;
				Counters.bError = false;


				for(int m = 0; m < MAX_PID; m++)
				{
					Counters.Counters[m].bitrate = 0;
					Counters.Counters[m].duration = 0;
					Counters.Counters[m].PID = 0xFFFF;
					Counters.Counters[m].PCR_PID = 0xFFFF;
					Counters.Counters[m].program_map_pid = 0xFFFF;
					Counters.Counters[m].program_number = 0;
					Counters.Counters[m].TimeStampPacketCount = 0;

					Counters.Counters[m].IndexFirstTimeStampPacket = -1;
					Counters.Counters[m].IndexLastTimeStampPacket = -1;

					Counters.Counters[m].FirstTimeStamp = -1;
					Counters.Counters[m].LastTimeStamp = -1;

					Counters.Counters[m].MinPacketBetweenTimeStamp = -1;
					Counters.Counters[m].MaxPacketBetweenTimeStamp = -1;

					Counters.Counters[m].TimeStampDiscontinuity = 0;
					Counters.Counters[m].DiscontinuityIndicator = 0;

					Counters.Counters[m].MinTimeStampBitrate = -1;
					Counters.Counters[m].MaxTimeStampBitrate = -1;
				}

				/*
				PACKET* p = file.GetFirstPacket();
				while(p)
				{
					WORD PID = 0xFFFF;
					file.GetPID(p,PID);
					if(PID!=0xFFFF) 
					{
						for(int  i = 0; i < sizeof(PIDArray)/sizeof(WORD); i++)
						{
							if(PIDArray[i].PID== PID)
								break;
							if(PIDArray[i].PID == 0xFFFF)
							{
								PIDArray[i].PID = PID;
								break;
							}
						}
					}
					p = file.GetNextPacket(p);
				}
				
				{
					ostringstream oss;
					oss << "List of PID in the TS file " ;
					if(packetEnd!=0xFFFFFFFF)
						oss << "from packet " << packetStart << " till packet: " << packetEnd << endl;
					else
						oss << "from packet " << packetStart << " till last packet: " << endl;
					int c = 0;
					for(int  i = 0; i < sizeof(PIDArray)/sizeof(PID_INFO); i++)
					{
						if(PIDArray[i].PID != 0xFFFF)
						{
							oss << " " << PIDArray[i].PID << endl;
							c++;
						}
						else
							break;
					}
					if(c==0)
							oss << " None" << endl;

					oss << endl;
					TraceOut(pG,Information,oss.str().c_str());
				}*/
				{
					ostringstream oss;
					oss << "List of PCR_PID in the TS file ";
					if((packetStart!=0)||(packetEnd!=0xFFFFFFFF))
						if(packetEnd!=0xFFFFFFFF)
							oss << "from packet " << packetStart << " till packet " << packetEnd << endl;
						else
							oss << "from packet " << packetStart << " till last packet " << endl;
					else
						oss << endl;

					int c = 0;
					if(file.GetProgramMapTableID(&Counters)>0)
					{

						for(int i =  0; i < Counters.Count; i++)
						{
							double locduration;
							double locbitrate;
							DWORD dw = sizeof(PIDArray)/sizeof(PID_INFO);
							WORD PCR_PID = Counters.Counters[i].PCR_PID = file.GetPCR_PID(Counters.Counters[i].program_map_pid,PIDArray,dw);
							oss << endl;
							oss << " PCR_PID (" << i+1 <<  ")"  << endl;
							oss << " PCR_PID         : " << PCR_PID << endl;
							oss << " program_map_pid : " << Counters.Counters[i].program_map_pid << endl;
							oss << " program_number  : "  << Counters.Counters[i].program_number << endl;
							for(int  j = 0; j < sizeof(PIDArray)/sizeof(PID_INFO); j++)
							{
								if(PIDArray[j].PID != 0xFFFF)
								{
							oss << "  stream (" << j+1 <<") "<< endl;
							oss << "  elementary_pid  : " << PIDArray[j].PID << endl;
							oss << "  stream_type     : " << (int)PIDArray[j].Type << endl;
							oss << "                    " << 	file.GetStreamType(PIDArray[j].Type) << endl;
								}
							}
							c++;
							if(file.GetBitrate(&Counters.Counters[i], Counters.Counters[i].PCR_PID, locduration, locbitrate,Counters.PacketCount))
							{
								oss << " Expected bitrate: " << locbitrate/1000000 <<  " Mbit/s - Expected duration: " << locduration << " seconds  "<< endl;
							}
						}
					}
					if(c==0)
							oss << " None" << endl;
					TraceOut(pG,Information,oss.str().c_str());
				}
			}

			if(IsTrace(pG,TSTimeStamp))
			{
				PACKET* p = file.GetFirstPacket();
				__int64 packetCount = 0;
				while(p)
				{
					WORD PID = 0xFFFF;
					file.GetPID(p,PID);
					if((pid == -1) || (PID == pid))
					{
						string s;
						if(file.GetPacketInformation(Mask,p,s))
						{
							TraceOut(pG,Information,s.c_str());
							Sleep(1);
						}
					}
					p = file.GetNextPacket(p);
				}
			}
			file.Unload();
			file.Close();
		}
		else
		{
			error_string = "Can't load file ";
			error_string += parse_ts_file;
			return 0;
		}
	}
	else
	{
		error_string = "Can't open file " ;
		error_string += parse_ts_file;
		return 0;
	}
	return 1;

}
int CheckFile(GLOBAL_PARAMETER* pG, const char* parse_ts_file,long packetStart,long packetEnd, string& error_string , long& ContinuityErrorCounter,long& PCRAccuracyErrorCounter, int pid = -1,unsigned long buffersize = 0)
{
    string titre = "Information about Transport Stream Tool Checker"; 
    const char *c_titre = titre.c_str ( ); 	

	HANDLE thisThread = GetCurrentThread();
	SetThreadPriority(thisThread, THREAD_PRIORITY_TIME_CRITICAL);

	// Load TS file
	__int64 dwLen;
	TSFile file;
	if(file.Open(parse_ts_file))
	{
		if((dwLen = file.Load(buffersize))!=0)
		{
			PARSE_COUNTERS Counters;
			TSFile::Parse_Mask Mask = TSFile::none;
			WORD PCR_PID= 0xFFFF;

			Mask = TSFile::none;
			if(IsTrace(pG,TSPID))
				Mask = TSFile::pid;
			if(IsTrace(pG,TSTimeStamp))
				Mask = TSFile::timestamp;
			if(IsTrace(pG,TSPacket))
				Mask = TSFile::ts;

			{
				ostringstream oss;
				oss << "Starting to check transport stream file " << parse_ts_file << endl;
				
				if ( (IsTrace(pG,Information)) ||
					(pid != -1) ||
					((packetStart!=0)||(packetEnd!=0xFFFFFFFF)))
					oss << "Checking options:" << endl;
				if(packetEnd!=0xFFFFFFFF)
					oss << "  Check from packet " << packetStart << " till packet " << packetEnd << endl;
				else
					oss << "  Check from packet " << packetStart << " till packet " << (dwLen/PACKET_SIZE)-1 << endl;
				if(pid != -1)
					oss << "  Filter: Check packets with PID = " << pid << " only " << endl;
				TraceOut(pG,Information,oss.str().c_str());
			}
			file.SetLimit(packetStart,packetEnd);

			{
				PID_INFO PIDArray[64];
				for(int  i = 0; i < sizeof(PIDArray)/sizeof(PID_INFO); i++)
				{
					PIDArray[i].PID = 0xFFFF;
					PIDArray[i].Type = 0xFF;
				}


				Counters.Count = 0;
				Counters.duration = 0;
				Counters.PacketCount = 0;
				Counters.bError = false;


				for(int m = 0; m < MAX_PID; m++)
				{
					Counters.Counters[m].bitrate = 0;
					Counters.Counters[m].duration = 0;
					Counters.Counters[m].PID = 0xFFFF;
					Counters.Counters[m].PCR_PID = 0xFFFF;
					Counters.Counters[m].program_map_pid = 0xFFFF;
					Counters.Counters[m].program_number = 0;
					Counters.Counters[m].TimeStampPacketCount = 0;

					Counters.Counters[m].IndexFirstTimeStampPacket = -1;
					Counters.Counters[m].IndexLastTimeStampPacket = -1;

					Counters.Counters[m].FirstTimeStamp = -1;
					Counters.Counters[m].LastTimeStamp = -1;

					Counters.Counters[m].MinPacketBetweenTimeStamp = -1;
					Counters.Counters[m].MaxPacketBetweenTimeStamp = -1;

					Counters.Counters[m].TimeStampDiscontinuity = 0;
					Counters.Counters[m].DiscontinuityIndicator = 0;

					Counters.Counters[m].MinTimeStampBitrate = -1;
					Counters.Counters[m].MaxTimeStampBitrate = -1;
				}

				{
					ostringstream oss;
					oss << "List of PCR_PID in the TS file ";
					if((packetStart!=0)||(packetEnd!=0xFFFFFFFF))
						if(packetEnd!=0xFFFFFFFF)
							oss << "from packet " << packetStart << " till packet " << packetEnd << endl;
						else
							oss << "from packet " << packetStart << " till last packet " << endl;
					else
						oss << endl;

					int c = 0;
					if(file.GetProgramMapTableID(&Counters)>0)
					{

						for(int i =  0; i < Counters.Count; i++)
						{
							double locduration;
							double locbitrate;
							DWORD dw = sizeof(PIDArray)/sizeof(PID_INFO);
							PCR_PID = Counters.Counters[i].PCR_PID = file.GetPCR_PID(Counters.Counters[i].program_map_pid,PIDArray,dw);
							oss << endl;
							oss << " PCR_PID (" << i+1 <<  ")"  << endl;
							oss << " PCR_PID         : " << PCR_PID << endl;
							oss << " program_map_pid : " << Counters.Counters[i].program_map_pid << endl;
							oss << " program_number  : "  << Counters.Counters[i].program_number << endl;
							for(int  j = 0; j < sizeof(PIDArray)/sizeof(PID_INFO); j++)
							{
								if(PIDArray[j].PID != 0xFFFF)
								{
							oss << "  stream (" << j+1 <<") "<< endl;
							oss << "  elementary_pid  : " << PIDArray[j].PID << endl;
							oss << "  stream_type     : " << (int)PIDArray[j].Type << endl;
							oss << "                    " << 	file.GetStreamType(PIDArray[j].Type) << endl;
								}
							}
							c++;
							if(file.GetBitrate(&Counters.Counters[i], Counters.Counters[i].PCR_PID, locduration, locbitrate,Counters.PacketCount))
							{
								oss << " Expected bitrate: " << locbitrate/1000000 <<  " Mbit/s - Expected duration: " << locduration << " seconds  "<< endl;
								if(locduration > Counters.duration)
									Counters.duration = locduration;
								//	bitrate += locbitrate;
								if(locbitrate>Counters.bitrate)
								{
									Counters.bitrate = locbitrate;
									Counters.Delta = Counters.Counters[i].delta;
									PCR_PID = Counters.Counters[i].PCR_PID;
								}
							}
						}
					}
					if(c==0)
							oss << " None" << endl;
					TraceOut(pG,Information,oss.str().c_str());
				}
			}

			ContinuityErrorCounter = 0;
			PCRAccuracyErrorCounter = 0;
			file.InitPIDInformation();
			{
				__int64 delta = 0;
				__int64 PCR = -1;
				__int64 LastPCR = -1;
				__int64 FirstPCR = -1;
				bool bDiscontinuity = false;
				__int64 PCRPacketCounter = 0;
				DWORD dwCurrentBitrate = (DWORD) Counters.bitrate;


				PACKET* p = file.GetFirstPacket();
				__int64 packetCount = 0;
				bool AccuracyError = false;
				while(p)
				{
					WORD PID = 0xFFFF;
					file.GetPID(p,PID);
					if((pid == -1) || (PID == pid))
					{
						string s;
						if(file.CheckContinuityCounter((LPBYTE)p,packetCount,s))
						{
							ContinuityErrorCounter++;
							TraceOut(pG,Information,s.c_str());							
						}


						// Check PCR Accuracy

						__int64 Accuracy;

						if(file.GetPCRInAdaptationField((LPBYTE)p,&PCR))
//						file.GetPIDPCRfromPacket((char *)p,PCR_PID, &PCR,&bDiscontinuity);
//						if(PCR!=-1)
						{
																					
							WORD PID = MAKEWORD(p[0][2], p[0][1]&0x1F);

							if(file.GetLastPCRCounter(PID,LastPCR,PCRPacketCounter)==false)
								file.SetLastPCRCounter(PID, PCR,packetCount);
							else
							{
								delta = 0;

/*								if((PCR-LastPCR)>=0)
									delta = (PCR-LastPCR);
								else
									delta = 0x40000000000 - (PCR - LastPCR);
*/
								if (((PCR & 0x20000000000) == 0) && ((LastPCR & 0x20000000000) != 0))
									delta = (0x257FFFFFFFF - LastPCR) + PCR;
								else
									delta = PCR - LastPCR;

								Accuracy = ((__int64)(packetCount-PCRPacketCounter)*PACKET_SIZE*8*1000000000)/dwCurrentBitrate;
								Accuracy -= delta*1000/27 ;

								if (Accuracy<0)
									Accuracy = -Accuracy;
								if(Accuracy > 500)
								{
									if(AccuracyError == false)
									{
										AccuracyError = true;
										PCRAccuracyErrorCounter++;
 										ostringstream oss;
										oss << "PCR Accuracy Error starts with packet " << packetCount << " PID: "  << PID << " Inaccuracy: " << Accuracy << " ns " <<  endl;
										s = oss.str().c_str();
										TraceOut(pG,Information,s.c_str());							
									}
								}
								else
								{
									if(AccuracyError == true)
									{
										AccuracyError = false;
 										ostringstream oss;
										oss << "PCR Accuracy Error ends with packet " << packetCount << " PID: "  << PID << " Inaccuracy: " << Accuracy << " ns " << endl;
										s = oss.str().c_str();
										TraceOut(pG,Information,s.c_str());							
									}
								}

							}
							file.SetLastPCRCounter(PID, PCR,packetCount);
						}		
						if( packetCount%8000 == 0) 
							Sleep(1);
					}
					p = file.GetNextPacket(p);
					packetCount++;
					

				}
			}
			file.Unload();
			file.Close();
		}
		else
		{
			error_string = "Can't load file ";
			error_string += parse_ts_file;
			return 0;
		}
	}
	else
	{
		error_string = "Can't open file " ;
		error_string += parse_ts_file;
		return 0;
	}
	return 1;

}
int GetFullPath(string& s,LPCTSTR path)
{
	s = "";
	if((path)&& (strlen(path)>2))
	{
		if((path[1] != ':') &&(path[0] != '\\')&&(path[0] != '.'))
		{
 			char strDir[1024]; 

			// in Service mode trace file mandatory
			GetProcessDirectory(sizeof(strDir),strDir);
			
			s = strDir;
			s += "\\";
			s += path;

		}
		else
			s = path;
	}
	return 0;
}
bool GetTraceLevel(LPCTSTR lp, VerboseLevel& vb) 
{
	vb = None;
	if(lp)
	{
		string s = lp;
		if(s == "none")
		{
			vb = None;
			return true;
		}
		else if(s == "information")
		{
			vb = Information;
			return true;
		}
		else if(s == "pid")
		{
			vb = TSPID;
			return true;
		}
		else if(s == "timestamp")
		{
			vb = TSTimeStamp;
			return true;
		}
		else if(s == "ts")
		{
			vb = TSPacket;
			return true;
		}
	}
	return false;
}

int GetIpAddressFromName(string name, PSTR str, size_t len)
{
	struct addrinfo *result = NULL;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;
	struct sockaddr_in  *sockaddr_ipv4;
	WSADATA wsaData;
	int iResult;
	string s;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		return  1;
	}

	DWORD dwRetval = getaddrinfo(name.c_str(), NULL, &hints, &result);
	if (dwRetval == 0)
	{
		for (ptr = result; ptr != NULL;ptr = ptr->ai_next) {

			switch (ptr->ai_family) {
			case AF_UNSPEC:
				break;
			case AF_INET:
				sockaddr_ipv4 = (struct sockaddr_in *) ptr->ai_addr;
				if (inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), str, len) != NULL)
				{
					WSACleanup();
					return 0;
				}
			case AF_INET6:
				break;
			case AF_NETBIOS:
				break;
			default:
				break;
			}
		}
	}
	WSACleanup();
	return 1;
}

int ParseCommandLine(int argc, _TCHAR* argv[], GLOBAL_PARAMETER* pGParam, STREAM_PARAMETER* pParam, LPDWORD lpdwParamLen, string& error_string)
{
	bool error = false; 
	error_string = "";
	string s;

	if((pParam)&& (lpdwParamLen))
	{
		DWORD i = 0;
		//while ((i < *lpdwParamLen) && (error == false))
		//{

			while ((--argc!=0) && (error == false))
			{ 
				error = true;
				string option = *++argv;
				if(option[0]== '-')
				{
					if(option == "-parse")
					{
						pGParam->action = Parse;
						error = false;
					}
					else if(option == "-help")
					{
						pGParam->action = NoAction;
						error = false;
					}
					else if(option == "-convert")
					{
						pGParam->action = Convert;
						if(--argc!=0)
						{
							GetFullPath(pParam[i].input_file, *++argv);
							if(--argc!=0)
							{
								GetFullPath(pParam[i].output_188_file, *++argv);
								*lpdwParamLen = 1;
								error = false;
							}
						}
						if(error==true)
						{
							error_string = "Conversion input or output file not specified";
							break;
						}
					}
					else if(option == "-filter")
					{
						pGParam->action = Filter;
						if(--argc!=0)
						{
							GetFullPath(pParam[i].input_file, *++argv);
							if(--argc!=0)
							{
								GetFullPath(pParam[i].output_188_file, *++argv);
								if(--argc!=0)
								{
									pParam[i].program_number = atoi(*++argv);
									*lpdwParamLen = 1;
									error = false;
								}
							}
						}
						if(error==true)
						{
							error_string = "Filtering input file, output file or program number not specified";
							break;
						}
					}
					else if(option == "-stream")
					{
						pGParam->action = Stream;
						error = false;
					}
					else if(option == "-mroute")
					{
						pGParam->action = MRoute;
						error = false;
					}
					else if(option == "-receive")
					{
						pGParam->action = Receive;
						error = false;
					}
					else if(option == "-check")
					{
						pGParam->action = Check;
						error = false;
					}
					else if(option == "-install")
					{
						pGParam->action = Install;
						error = false;
					}
					else if(option == "-uninstall")
					{
						pGParam->action = Uninstall;
						error = false;
					}
					else if(option == "-start")
					{
						pGParam->action = Start;
						error = false;
					}
					else if(option == "-stop")
					{
						pGParam->action = Stop;
						error = false;
					}
					else if(option == "-service")
					{
						pGParam->mode = Service;
						error = false;
					}
					else if(option == "-file")
					{
						if(--argc!=0)
						{
							GetFullPath(pParam[i].ts_file, *++argv);
							*lpdwParamLen = 1;
							error = false;
						}
						if(error==true)
						{
							error_string = "transport stream file not specified";
							break;
						}

					}
					else if(option == "-xmlfile")
					{
						if(--argc!=0)
						{
							if(pGParam->action == Install)
							{
								GetFullPath(pParam[i].xml_file,*++argv);
								error = false;
							}
							else
							{
								string xml_file = "";
								GetFullPath(xml_file,*++argv);
								return ParseCommandLinesInXmlFile(xml_file.c_str(),pGParam,pParam, lpdwParamLen, error_string);
							}
						}
						if(error==true)
						{
							error_string = "xml file not specified";
							break;
						}

					}
					else if(option == "-pipfile")
					{
						if(--argc!=0)
						{
							GetFullPath(pParam[i].pip_ts_file, *++argv);
							error = false;
						}
						if(error==true)
						{
							error_string = "transport stream pip file not specified";
							break;
						}

					}
					else if(option == "-tracefile")
					{
						if(--argc!=0)
						{
							GetFullPath(pGParam->trace_file ,*++argv);
							error = false;
						}
						if(error==true)
						{
							error_string = "trace file not specified";
							break;
						}
					}
					else if(option == "-tracesize")
					{
						if(--argc!=0)
						{
							pGParam->trace_max_size = atol(*++argv);
							error = false;
						}
						if(error==true)
						{
							error_string = "trace file maximum size not specified";
							break;
						}

					}
					else if(option == "-tracelevel")
					{
						if(--argc!=0)
						{
							if(GetTraceLevel(*++argv,pGParam->trace_level))
								error = false;
							else
							{
								error_string = "Invalid verbose level";
								error = true;
							}
						}
						else
						{
							pGParam->trace_level = Information;
							error = false;
						}
					}
					else if(option == "-consolelevel")
					{
						if(--argc!=0)
						{
							if(GetTraceLevel(*++argv,pGParam->console_trace_level))
								error = false;
							else
							{
								error_string = "Invalid verbose level";
								error = true;
							}
						}
						else
						{
							pGParam->console_trace_level = Information;
							error = false;
						}
					}
					else if(option == "-address")
					{
						if(--argc!=0)
						{
							string s = *++argv;
							if(s.length()>1)
							{
								size_t pos = s.find(':');
								if((pos>0) && (pos < (s.length()-1)))
								{
									char str[INET_ADDRSTRLEN];
									string s_ip_address = s.substr(0,pos);
									if ((s_ip_address.length() > 0) && (isalpha(s_ip_address[0])))
									{
										if(GetIpAddressFromName(s_ip_address,str, INET_ADDRSTRLEN)==0)
											pParam[i].udp_ip_address = str;
									}
									if(pParam[i].udp_ip_address.length()==0)
										pParam[i].udp_ip_address = s_ip_address.c_str();
									pParam[i].udp_port = atoi(s.substr(pos+1,s.length()-1).c_str());
									error = false;
								}
							}
						}
						if(error == true)
						{
							error_string = "udp address not defined";
							break;
						}

					}
					else if(option == "-inputaddress")
					{
						if(--argc!=0)
						{
							string s = *++argv;
							if(s.length()>1)
							{
								size_t pos = s.find(':');
								if((pos>0) && (pos < (s.length()-1)))
								{
									string s_ip_input_address = s.substr(0,pos);
									pParam[i].input_udp_ip_address = s_ip_input_address.c_str();
									pParam[i].input_udp_port = atoi(s.substr(pos+1,s.length()-1).c_str());
									*lpdwParamLen = 1;
									error = false;
								}
							}
						}
						if(error == true)
						{
							error_string = "udp input address not defined";
							break;
						}

					}
					else if(option == "-interfaceaddress")
					{
						if(--argc!=0)
						{
							pParam[i].udp_ip_address_interface = *++argv;
							error = false;
						}
						if(error==true)
						{
							error_string = "IP address associated with the muticast interface not specified";
							break;
						}
					}
					else if(option == "-inputinterfaceaddress")
					{
						if(--argc!=0)
						{
							pParam[i].input_udp_ip_address_interface = *++argv;
							error = false;
						}
						if(error==true)
						{
							error_string = "IP address associated with the input muticast interface not specified";
							break;
						}
					}
					else if(option == "-pipaddress")
					{
						if(--argc!=0)
						{
							string s = *++argv;
							if(s.length()>1)
							{
								size_t pos = s.find(':');
								if((pos>0) && (pos < (s.length()-1)))
								{
									string s_ip_address = s.substr(0,pos);
									pParam[i].pip_udp_ip_address = s_ip_address.c_str();
									pParam[i].pip_udp_port = atoi(s.substr(pos+1,s.length()-1).c_str());
									error = false;
								}
							}
						}
						if(error == true)
						{
							error_string = "udp address not defined";
							break;
						}

					}
					else if(option == "-buffersize")
					{
						if(--argc!=0)
						{
							pParam[i].buffersize = atol(*++argv);
							error = false;
						}
						if(error==true)
						{
							error_string = "buffersize not defined";
							break;
						}
					}
					else if(option == "-pipbuffersize")
					{
						if(--argc!=0)
						{
							pParam[i].pip_buffersize = atol(*++argv);
							error = false;
						}
						if(error==true)
						{
							error_string = "pipbuffersize not defined";
							break;
						}
					}
					else if(option == "-ttl")
					{
						if(--argc!=0)
						{
							pParam[i].ttl = atoi(*++argv);
							error = false;
						}
						if(error==true)
						{
							error_string = "ttl not defined";
							break;
						}
					}
					else if(option == "-packet")
					{
						if(((argc-1)!=0)&&(*(argv+1)[0] != '-'))
						{
							--argc;
							pParam[i].packetStart = atol(*++argv);
							if(((argc-1)!=0)&&(*(argv+1)[0] != '-'))
							{
								--argc;
								pParam[i].packetEnd = atol(*++argv);
							}
							if(pParam[i].packetEnd!=0xFFFFFFFF)
								if(pParam[i].packetEnd < pParam[i].packetStart)
								{
									error_string = "-packet option incorrect: last packet < first packet";
									break;
								}
						}
						error = false;
					}
					else if(option == "-time")
					{
						if(((argc-1)!=0)&&(*(argv+1)[0] != '-'))
						{
							--argc;
							pParam[i].timeStart = atol(*++argv);
							if(((argc-1)!=0)&&(*(argv+1)[0] != '-'))
							{
								--argc;
								pParam[i].timeEnd = atol(*++argv);
							}
						}
						error = false;
					}
					else if(option == "-loop")
					{
						pParam[i].nloop = -1;
						if(((argc-1)!=0)&&(*(argv+1)[0] != '-'))
						{
							--argc;
							pParam[i].nloop = atoi(*++argv);
						}
						error = false;
					}
					else if(option == "-pid")
					{
						if(--argc!=0)
						{
							pParam[i].pid = atoi(*++argv);
							error = false;
						}
						if(error==true)
						{
							error_string = "PID not defined";
							break;
						}
					}
					else if(option == "-forcedbitrate")
					{
						if(--argc!=0)
						{
							pParam[i].forcedbitrate = atoi(*++argv);
							error = false;
						}
						if(error==true)
						{
							error_string = "forcedbitrate not defined";
							break;
						}
					}
					else if(option == "-pipforcedbitrate")
					{
						if(--argc!=0)
						{
							pParam[i].pip_forcedbitrate = atoi(*++argv);
							error = false;
						}
						if(error==true)
						{
							error_string = "forced bitrate for PIP stream not defined";
							break;
						}
					}
					else if(option == "-updatetimestamps")
					{
						pParam[i].bupdatetimestamps = true;
						error = false;
					}
					else if(option == "-rtp")
					{
						pParam[i].input_rtp = true;
						error = false;
					}
					else
					{
						error_string = "Invalid option";
						break;

					}
				}			
			} 
//		i++;
//		}
	}
	if(error == false)
	{
		for(DWORD dw = 0; dw < *lpdwParamLen;dw++)
		{
			if((pParam[dw].ts_file.length()==0) && (pParam[dw].xml_file.length()))
			{
				*lpdwParamLen = dw;
				return 1;
			}
			else
			{
				if(pParam[dw].name.length()==0)
				{
					ostringstream oss;
					oss << "Stream" << dw+1 ;
					pParam[dw].name = oss.str().c_str();
				}
			}
		}

		return 1;
	}
	return 0;
}

bool DumpParameters(ostringstream& oss, GLOBAL_PARAMETER* pG, STREAM_PARAMETER* p,DWORD dwParamLen)
{
	if((p)&&(pG))
	{
		oss << endl << "Microsoft Transport Stream Route Version 1.0.0.0:" << endl; 
		if(pG->mode == Service)
			oss << " Mode               : Service" << endl ;
		else
	        oss << " Mode               : Command" << endl ;
		if(pG->trace_file.length()>0) 
		{
			oss << " Trace file         : " << pG->trace_file << endl;
		    oss << " Trace size: " << pG->trace_max_size << endl;
			oss << " Trace level: " << GetTraceLevelString(pG->trace_level) <<endl;
		}
		oss << " Console trace level: " << GetTraceLevelString(pG->console_trace_level) <<endl;

			switch(pG->action)
			{
			case Stream:
				for(DWORD dw = 0; dw < dwParamLen;dw++)
				{
					if(p->ts_file.length()>0)
					{
						oss << endl << " Action: Stream" << endl;
						if(p->name.length()>0) oss << " Name: " << p->name << endl;
						oss << " File: " << p->ts_file << endl;
						oss << " IP Address: " << p->udp_ip_address << endl;
						oss << " UDP Port: " << p->udp_port << endl;
						if(p->udp_ip_address_interface.length()>0)
						oss << " IP Interface: " << p->udp_ip_address_interface << endl;
						if(p->forcedbitrate>0)
						oss << " Forced bitrate: " << p->forcedbitrate << " b/s" << endl;
						if(p->buffersize>0)
						oss << " Buffer size: " << p->buffersize << " bytes" << endl;
						if(p->packetStart!=0)
							oss << " Packet start: " << p->packetStart<< endl;
						if(p->packetEnd!=-1)
							oss << " Packet end: " << p->packetEnd<< endl;
						if(p->timeStart!=0)
							oss << " Time start: " << p->timeStart<< endl;
						if(p->timeEnd!=-1)
							oss << " Time end: " << p->timeEnd<< endl;
						oss << " Loop(s) : " ;
						if(p->nloop >= 0)
							oss << p->nloop+1 << endl;;
						if(p->nloop < 0)
							oss << "infinite" << endl;;

						if(p->pip_ts_file.length()>0)
						{
						oss << " PIP File: " << p->pip_ts_file << endl;
						oss << " PIP IP Address: " << p->pip_udp_ip_address << endl;
						oss << " PIP UDP Port: " << p->pip_udp_port << endl;
						if(p->pip_udp_ip_address_interface.length()>0)
						oss << " PIP IP Interface: " << p->pip_udp_ip_address_interface << endl;
						if(p->pip_forcedbitrate>0)
						oss << " PIP Forced bitrate: " << p->pip_forcedbitrate << " b/s" << endl;
						if(p->pip_buffersize>0)
						oss << " PIP buffer size: " << p->pip_buffersize << " bytes" << endl;
						oss << endl;
						if(p->bupdatetimestamps == true)
						oss << " Update time stamps: ON" << endl;
						else
						oss << " Update time stamps: OFF" << endl;
						if(p->nloop>0)
						oss << " Number of loops: " << p->nloop << endl;
						else
						oss << " Number of loops: Infinite" << endl;
						}
						if(p->output_file.length()>0) 
						{
							oss << " Result file: " << p->output_file << " period: " << p->refresh_period << endl;
						}
					}
					if(p->input_udp_ip_address.length()>0)
					{
						oss << endl << " Action: Route" << endl;
						if(p->name.length()>0) oss << " Name: " << p->name << endl;
						oss << " Input IP Address: " << p->input_udp_ip_address << endl;
						oss << " Input UDP Port: " << p->input_udp_port << endl;
						if(p->input_udp_ip_address_interface.length()>0)
						oss << " Input IP Interface: " << p->input_udp_ip_address_interface << endl;
						oss << " Output IP Address: " << p->udp_ip_address << endl;
						oss << " Output UDP Port: " << p->udp_port << endl;
						if(p->udp_ip_address_interface.length()>0)
						oss << " Output IP Interface: " << p->udp_ip_address_interface << endl;
						if(p->input_rtp)
						oss << " Input RTP streams expected and Output MPEG2-TS streams" << endl;
						if(p->output_file.length()>0) 
						{
							oss << " Result file: " << p->output_file << " period: " << p->refresh_period << endl;
						}
					}

					p++;
				}

				break;
			case Parse:
				oss << endl << " Action: Parse" << endl;
				oss << " File: " << p->ts_file << endl;
				if(p->pid!=-1)
				oss << " PID: " << p->pid << endl;
				if(p->packetStart!=0)
					oss << " Packet start: " << p->packetStart<< endl;
				if(p->packetEnd!=-1)
					oss << " Packet end: " << p->packetEnd<< endl;
				if(IsTrace(pG,TSTimeStamp))
				oss << " Parse only Time stamp packets" << endl;
				if(IsTrace(pG,TSPacket))
				oss << " Parse all TS packets" << endl;
				break;
			case Receive:
				oss << endl << " Action: Receive" << endl;
				oss << " File: " << p->ts_file << endl;
				oss << " IP Address: " << p->udp_ip_address << endl;
				oss << " UDP Port: " << p->udp_port << endl;
				if(p->udp_ip_address_interface.length()>0)
				oss << " IP Interface: " << p->udp_ip_address_interface << endl;
				break;
			case Check:
				oss << endl << " Action: Check" << endl;
				if(p->ts_file.length()>0)
					oss << " File: " << p->ts_file << endl;
				else if(p->udp_ip_address.length()>0)
				{
					oss << " IP Address: " << p->udp_ip_address << endl;
					oss << " UDP Port: " << p->udp_port << endl;
					if(p->udp_ip_address_interface.length()>0)
						oss << " IP Interface: " << p->udp_ip_address_interface << endl;
				}
				break;
			case Install:
				oss << endl << " Action: Install" << endl;
				if(p->xml_file.length()>0)
				oss << " Xml file: " << p->xml_file << endl;
				break;
			case Uninstall:
				oss << endl << " Action: Uninstall" << endl;
				break;
			case Start:
				oss << endl << " Action: Start" << endl;
				break;
			case Stop:
				oss << endl << " Action: Stop" << endl;
				break;
			case Convert:
				oss << endl << " Action: Convert" << endl;
				oss << " Input file: " << p->input_file << endl;
				oss << " Output file: " << p->output_188_file << endl;
				break;
			case Filter:
				oss << endl << " Action: Filter" << endl;
				oss << " Input file: " << p->input_file << endl;
				oss << " Output file: " << p->output_188_file << endl;
				oss << " Program Number: " << p->program_number << endl;
				break;
			default:
				break;
			}
		return true;
	}
	return false;
}
int GetProcessDirectory(int Size, LPTSTR Path)
{
	if((Path) && (Size>0))
	{
		
		if( GetModuleFileName((HMODULE)GetModuleHandle(NULL),(LPTSTR)Path,Size )>0)
		{
			size_t Len = strlen(Path);
			while(Len>0)
			{
				if(Path[Len--] == '\\')
				{
					Path[Len+1]='\0';
					return (int)Len+1;
				}
			}
		}
	}
	return 0;
}
int _tmain(int argc, _TCHAR* argv[])
{
	ostringstream oss;
	string error_string = "";
	// service table
	lpszServiceName = SZSERVICENAME;
    SERVICE_TABLE_ENTRY   DispatchTable[] = 
    { 
	  { SZSERVICENAME, ServiceStart      }, 
	  { NULL,              NULL          } 
    }; 
	
	InitParameters(&gGlobalParam,gStreamParam, gdwParamLen);


	gGlobalParam.mode = Command;
	gGlobalParam.action = NoAction;
	if( (ParseCommandLine(argc, argv, &gGlobalParam, gStreamParam, &gdwParamLen, error_string)==0) ||
		(gGlobalParam.action == NoAction))
	{
		if(error_string.length()>0)oss << "Error message: "<< error_string.c_str() << endl;
		oss << "Microsoft Transport Stream Route Version 1.0.0.0:" << endl; 
		oss << "Usage:" << endl; 
		oss << "TSRoute [-parse  -file <file>  [-pid <PID>][-packet <packetStart> <packetEnd>]" << endl;
		oss << "                              [-buffersize <size>]] "<< endl;
		oss << "       [-stream -file <file>  [-address <IP Address>:<UDP port>] "<< endl;
		oss << "                              [-ttl <time to live>]"<< endl; 
		oss << "                              [-loop [n]] " << endl; 
		oss << "                              [-forcedbitrate  <bitrate bit/s> ]" << endl;
		oss << "                              [-packet <packetStart> <packetEnd>]" << endl;
		oss << "                              [-time <timeStart ms> <timeEnd ms>]" << endl;
		oss << "                              [-buffersize <size>]" << endl; 
		oss << "                              [-interfaceaddress <IP Address>]]" << endl; 
		oss << "                              [-pipfile  <file> ]" << endl;
		oss << "                              [-pipaddress <IP Address>:<UDP port>] "<< endl;
		oss << "                              [-pipbuffersize <size>]" << endl; 
		oss << "                              [-pipforcedbitrate  <bitrate bit/s> ]" << endl;
		oss << "       [-stream -inputaddress <IP Address>:<UDP port> "<< endl;
		oss << "                              [-inputinterfaceaddress <IP Address>]]" << endl; 
		oss << "                              [-rtp]"<< endl; 
		oss << "                              [-address <IP Address>:<UDP port>] "<< endl;
		oss << "                              [-interfaceaddress <IP Address>]]" << endl; 
		oss << "                              [-ttl <time to live>]"<< endl; 
		oss << "       [-stream -xmlfile <file> ]" << endl;
		oss << "       [-receive -file <file> -address <IP Address>:<UDP port>" << endl; 
		oss << "                              [-interfaceaddress <IP Address>]]" << endl; 
		oss << "       [-check -file <file> " << endl; 
		oss << "       [-check -address <IP Address>:<UDP port>" << endl; 
		oss << "                              [-interfaceaddress <IP Address>]" << endl; 
		oss << "       [-install [-xmlfile <file>]]" << endl;
		oss << "       [-uninstall]" << endl;
		oss << "       [-start]" << endl;
		oss << "       [-stop]" << endl;
		oss << "       [-convert <file204> <file188>]" << endl;
		oss << "       [-filter <ifile> <ofile> <program_number>]" << endl;
		oss << "       [-consolelevel [none|information|pid|timestamp|ts]]" << endl; 
		oss << "       [-tracefile <tracefile>[-tracesize <maxtracesize>]" << endl;
		oss << "                  [-tracelevel [none|information|pid|timestamp|ts]]]" << endl; 
		oss << "Arguments:" << endl;  
		oss << "-file <TS file>             : Transport stream file to parse, stream or receive" << endl; 
		oss << "-parse                      : Parse the transport stream file" << endl; 
		oss << "-stream                     : Stream the transport stream file over UDP" << endl; 
		oss << "                            : or route Multicast stream " << endl; 
		oss << "-receive                    : Capture a transport stream over UDP into a file" << endl; 
		oss << "-check                      : Check for continuity counter errors and " << endl; 
		oss << "                              and PCR Accuracy errors in a transport stream " << endl; 
		oss << "                              file or a transport stream on the network" << endl; 
		oss << "-install                    : Install the Win32 service" << endl; 
		oss << "-uninstall                  : Uninstall the Win32 service" << endl; 
		oss << "-start                      : Start the Win32 service" << endl; 
		oss << "-stop                       : Stop the Win32 service" << endl; 
		oss << "-convert <file204> <file188>: Convert TS stream 204 bytes packets into" << endl;
		oss << "                              TS stream 188 bytes packets" << endl;
		oss << "-filter <ifile> <ofile> <pn>: Filter a TS file <ifile> and keep the packets " << endl;
		oss << "                              associated with program number <pn> and store the" << endl;
		oss << "                              packets into <ofile> file." << endl;
		oss << "-address <IPAddr>:<UDPport> : Multicast IP address and UDP port to stream or " << endl; 
		oss << "                              receive the TS file" << endl; 
		oss << "-interfaceaddress <IPAddr>  : IP address associated with the Interface to " << endl; 
		oss << "                              stream or receive the TS file" << endl; 
		oss << "-inputrtp                   : input RTP streams expected " << endl; 
		oss << "                              if this option is set the application will " << endl; 
		oss << "                              remove the RTP header before streaming" << endl; 
		oss << "-inputaddress <IPAddr>:<UDPport> : Input Multicast IP address and input UDP " << endl; 
		oss << "                              port " << endl; 
		oss << "-inputinterfaceaddress <IPAddr>  : IP address associated with the input Interface" << endl; 
		oss << "-ttl <time to live>         : TTL for multicast UDP packet" << endl; 
		oss << "-forcedbitrate <bitrate>    : Force the bitrate, in that case, it won't use the"<< endl; 
		oss << "                              PCR information. bitrate in bit/second" << endl; 
		oss << "-loop [n]                   : define the number of loops when streaming the "<< endl;
		oss << "                              transport stream file" << endl; 
		oss << "-updatetimestamps           : update the time stamps (PCR, DTS, PTS) in the "<< endl; 
		oss << "                              transport stream file for each streaming loop" << endl; 
		oss << "-packet <packetStart> <packetEnd>"<<endl;
		oss << "                            : Stream the file from packet <packetStart> till " << endl; 
		oss << "                              packet <packetEnd>" << endl;
		oss << "-time <timeStart ms> <timeEnd ms>"<<endl;
		oss << "                            : Stream the file from <timeStart> till <timeEnd>"  << endl;
		oss << "                              time in ms" << endl;
		oss << "-pid <PID>                  : filter only the packets with PID = <PID> during " << endl;
		oss << "                              the parsing process." << endl;
		oss << "-buffersize <size>          : size of the buffer in byte "<<endl;
		oss << "-tracefile <tracefile>      : Trace file" << endl; 
		oss << "-tracesize <size>           : Maximum trace file size in byte" << endl; 
		oss << "-tracelevel <level>         : Trace level" << endl; 
		oss << "                              none       :  no trace " << endl;
		oss << "                              information: information and error messages only " << endl;
		oss << "                              pid        : display PID in TS packet" << endl;
		oss << "                              timestamp  : display TS packet with PCR, PTS, DTS" << endl;
		oss << "                              ts         : display all TS packets" << endl;
		oss << "-consolelevel <level>       : Console trace level" << endl; 
		oss << "                              none       : no trace " << endl;
		oss << "                              information: information and error messages only " << endl;
		oss << "                              pid        : display PID in TS packet" << endl;
		oss << "                              timestamp  : display TS packet with PCR, PTS, DTS" << endl;
		oss << "                              ts         : display all TS packets" << endl;
		oss << "-pipfile <TS file>          : PIP Transport stream file to stream " << endl; 
		oss << "-pipaddress <IPAd>:<UDPp>   : Multicast IP address and UDP port to stream the" << endl; 
		oss << "                              PIP transport stream file" << endl; 
		oss << "-pipforcedbitrate <bitrate> : Force the bitrate for PIP stream"<< endl; 
		oss << "-pipbuffersize <size>       : size of the buffer in byte for PIP stream"<<endl;
		oss << "-xmlfile <xmlfile>          : Path to the XML file which contains the " << endl; 
		oss << "                              input parameters for streaming" << endl; 
		cout << oss.str().c_str();
		return -1;
	}
	if(gGlobalParam.trace_file.length()>0)
		gGlobalParam.pLog = new TSTrace(gGlobalParam.trace_file.c_str(),gGlobalParam.trace_max_size);

	if(gGlobalParam.mode == Service)
	{		 
		string s;

 		char strDir[1024]; 

		// in Service mode trace file mandatory
		GetProcessDirectory(sizeof(strDir),strDir);
		
		if(gGlobalParam.pLog==NULL)
		{
			s = strDir;
			s += "\\";
			s += DEFAULT_TRACE_FILE_NAME;

			gGlobalParam.console_trace_level = None;
			gGlobalParam.pLog = new TSTrace(s.c_str(),DEFAULT_TRACE_FILE_SIZE);
		}


		s = lpszServiceName;
		s += " Service Arguments: " ;
		LPTSTR* p = argv;
		for(int i = 0; i < argc; i++)
		{
			 s += " ";
			 s += *p++;
		}
		TraceOut(&gGlobalParam, Information, s.c_str());
		TraceOut(&gGlobalParam, Information, "Service Mode: Call StartServiceCtrlDispatcher");
	   if (StartServiceCtrlDispatcher( DispatchTable)!=0)
		   return 0;
	   oss << "StartServiceCtrlDispatcher failed - Error: " << GetLastError() << endl;
	   TraceOut(&gGlobalParam, Information, oss.str().c_str());
	}
	else
	{

		if(gGlobalParam.action != NoAction)
		{
				
			{
				ostringstream oss;
				DumpParameters(oss,&gGlobalParam,&gStreamParam[0],gdwParamLen);
				TraceOut(&gGlobalParam, Information, oss.str().c_str());
			}
			if(gGlobalParam.action == Install)
			{
				ostringstream oss;
				if(InstallService(gStreamParam[0].xml_file.c_str())==true)
					oss << " Result: MPEG2-TS Streamer service installed" ;
				else
					oss << " Result: Error when installing MPEG2-TS Streamer service, are you running TSRoute.exe as Administrator? It's required for the installation";
				if(gStreamParam[0].xml_file.length()>0)
					oss << " Xml file (" << gStreamParam[0].xml_file.c_str() << " )";
				TraceOut(&gGlobalParam,Information,oss.str().c_str());
			}
			else if(gGlobalParam.action == Uninstall)
			{
				ostringstream oss;

				StopService();
				if(UninstallService()==true)
					oss << " Result: MPEG2-TS Streamer service uninstalled";
				else
					oss << " Result: Error when uninstalling MPEG2-TS Streamer service";
				
				TraceOut(&gGlobalParam,Information,oss.str().c_str());
			}
			else if(gGlobalParam.action == Start)
			{
				ostringstream oss;
				if(StartService()==true)
					oss << " Result: MPEG2-TS Streamer service started";
				else
					oss << " Result: Error when starting MPEG2-TS Streamer service";
				
				TraceOut(&gGlobalParam,Information,oss.str().c_str());
			}
			else if(gGlobalParam.action == Stop)
			{
				ostringstream oss;
				if(StopService()==true)
					oss << " Result: MPEG2-TS Streamer service stopped";
				else
					oss << " Result: Error when stopping MPEG2-TS Streamer service";
				TraceOut(&gGlobalParam,Information,oss.str().c_str());
			}
			if(gGlobalParam.action == Stream)
			{
				StreamFileOrRoute(&gGlobalParam,gStreamParam, gdwParamLen);
				ostringstream oss;
				oss << " Result: MPEG2-TS Streamer stopped";
				TraceOut(&gGlobalParam,Information,oss.str().c_str());
			}
			else if(gGlobalParam.action == Receive)
			{
				
				RecvFile(&gGlobalParam, gStreamParam[0].ts_file.c_str(), gStreamParam[0].udp_ip_address.c_str(), gStreamParam[0].udp_port, (gStreamParam[0].udp_ip_address_interface.length()>0?gStreamParam[0].udp_ip_address_interface.c_str():NULL),gStreamParam[0].refresh_period);
				if(IsTrace(&gGlobalParam,Information))
					ParseFile(&gGlobalParam,gStreamParam[0].ts_file.c_str(), gStreamParam[0].packetStart,gStreamParam[0].packetEnd,error_string, -1,gStreamParam[0].buffersize);
				ostringstream oss;
				oss << " Result: MPEG2-TS Receiver stopped";
				TraceOut(&gGlobalParam,Information,oss.str().c_str());
					
			}
			else if(gGlobalParam.action == MRoute)
			{
				
				MRouteStream(&gGlobalParam, 
					gStreamParam[0].udp_ip_address.c_str(), 
					gStreamParam[0].udp_port, 
					(gStreamParam[0].udp_ip_address_interface.length()>0?gStreamParam[0].udp_ip_address_interface.c_str():NULL),
					gStreamParam[0].ttl, 
					gStreamParam[0].input_udp_ip_address.c_str(), 
					gStreamParam[0].input_udp_port, 
					(gStreamParam[0].input_udp_ip_address_interface.length()>0?gStreamParam[0].input_udp_ip_address_interface.c_str():NULL),
					gStreamParam[0].input_rtp, 
 					gStreamParam[0].refresh_period);
				if(IsTrace(&gGlobalParam,Information))
					ParseFile(&gGlobalParam,gStreamParam[0].ts_file.c_str(), gStreamParam[0].packetStart,gStreamParam[0].packetEnd,error_string, -1,gStreamParam[0].buffersize);
				ostringstream oss;
				oss << " Result: MPEG2-TS Receiver stopped";
				TraceOut(&gGlobalParam,Information,oss.str().c_str());
					
			}
			else if(gGlobalParam.action == Check)
			{
				if(gStreamParam[0].ts_file.length()>0)
				{
					error_string = "";
					long ContinuityErrorCounter = 0;
					long PCRAccuracyErrorCounter = 0;

					if(CheckFile(&gGlobalParam, gStreamParam[0].ts_file.c_str(),gStreamParam[0].packetStart,gStreamParam[0].packetEnd,error_string,ContinuityErrorCounter,	PCRAccuracyErrorCounter, gStreamParam[0].pid,gStreamParam[0].buffersize)==0)
						oss << " Result: Error Message - " << error_string.c_str() << endl;
					else
					{
						oss << " Result: File " << gStreamParam[0].ts_file.c_str() << " successfully checked" << endl;
						oss << " Continuity Errors detected  : " << ContinuityErrorCounter << endl;
						oss << " PCR Accuracy Errors detected: " << PCRAccuracyErrorCounter << endl;
					}
					TraceOut(&gGlobalParam,Information,oss.str().c_str());
				}
				else if(gStreamParam[0].udp_ip_address.length()>0)
				{
					CheckStream(&gGlobalParam, gStreamParam[0].udp_ip_address.c_str(), gStreamParam[0].udp_port, (gStreamParam[0].udp_ip_address_interface.length()>0?gStreamParam[0].udp_ip_address_interface.c_str():NULL),gStreamParam[0].refresh_period);
					ostringstream oss;
					oss << " Result: MPEG2-TS Checker stopped";
					TraceOut(&gGlobalParam,Information,oss.str().c_str());
				}					
			}
			else if(gGlobalParam.action == Convert)
			{
				
				ostringstream oss;				

				error_string = "";
				DWORD count = 0;
				oss << endl;
				if(ConvertFile(&gGlobalParam, gStreamParam[0].input_file.c_str(),gStreamParam[0].output_188_file.c_str(),count, error_string)==0)
					oss << " Result: Error Message - " << error_string.c_str() << endl;
				else
					oss << " Result: Input file " << gStreamParam[0].input_file.c_str() << " successfully converted! " << endl << "         Output file: " << gStreamParam[0].output_188_file.c_str() << endl << "         Number of packets: " << count << endl;
				TraceOut(&gGlobalParam,Information,oss.str().c_str());
					
			}
			else if(gGlobalParam.action == Filter)
			{
				
				ostringstream oss;				

				error_string = "";
				DWORD count = 0;
				oss << endl;
				if(FilterFile(&gGlobalParam, gStreamParam[0].input_file.c_str(),gStreamParam[0].output_188_file.c_str(),gStreamParam[0].program_number,count, error_string)==0)
					oss << " Result: Error Message - " << error_string.c_str() << endl;
				else
					oss << " Result: Input file " << gStreamParam[0].input_file.c_str() << " successfully converted! " << endl << "         Output file: " << gStreamParam[0].output_188_file.c_str() << endl << "         Number of packets: " << count << endl;
				TraceOut(&gGlobalParam,Information,oss.str().c_str());
					
			}
			
			else if(gGlobalParam.action == Parse)
			{
				ostringstream oss;				

				error_string = "";
				oss << endl;
				if(ParseFile(&gGlobalParam,gStreamParam[0].ts_file.c_str(),gStreamParam[0].packetStart,gStreamParam[0].packetEnd,error_string, gStreamParam[0].pid,gStreamParam[0].buffersize)==0)
					oss << " Result: Error Message - " << error_string.c_str() << endl;
				else
					oss << " Result: File " << gStreamParam[0].ts_file.c_str() << " successfully parsed" << endl;
				TraceOut(&gGlobalParam,Information,oss.str().c_str());
			}
				
		}
	}
	return 0;
}


