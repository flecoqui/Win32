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
#include "Timer.h"

Timer::Timer(void)
{
	m_begin.QuadPart = 0;
	m_frequency = 0;
}
int Timer::Load(void)
{
    LARGE_INTEGER Frequency; 
    if (!QueryPerformanceFrequency(&Frequency)) 
		return 0;
    m_frequency = Frequency.QuadPart; 
	return 1;
}
int Timer::Unload(void)
{
	m_begin.QuadPart = 0;
	m_frequency = 0;
	return 1;
}
int Timer::Begin(void)
{
	QueryPerformanceCounter(&m_begin);
	return 1;
}
int Timer::End(double& duration)
{
	LARGE_INTEGER end;
	QueryPerformanceCounter(&end);
	duration = (double)(end.QuadPart-m_begin.QuadPart)/(double)m_frequency;
	return 1;
}
//double DeltaMax = 0;

int Timer::Wait(double tempo)
{
	LARGE_INTEGER begin;
	LARGE_INTEGER end;
	double Delta;
	QueryPerformanceCounter(&begin);
	
	while(QueryPerformanceCounter(&end))
	{
		DWORD period;
		/*
		if((tempo> 0.001) && ((double)(end.QuadPart-begin.QuadPart)/(double)m_frequency < (tempo-0.001)))
			period = 1;
		else if(  (double)(end.QuadPart-begin.QuadPart)/(double)m_frequency < (tempo))
			period = 0;*/
		if(  (double)(end.QuadPart-begin.QuadPart)/(double)m_frequency > (tempo))
		{
			Delta = ((double)(end.QuadPart-begin.QuadPart)/(double)m_frequency) - tempo;
//			if(Delta>DeltaMax)
//				DeltaMax = Delta;
			break;
		}
		else if ((tempo - 0.001)< 0)
			period = 0;
		else if(  (double)(end.QuadPart-begin.QuadPart)/(double)m_frequency < (tempo - 0.001))
			period = 1;
		else
			period = 0;
		Sleep(period);
		/*
		MSG win32msg;
		int retval;
		while ((retval = ::PeekMessage(&win32msg, NULL, 0, 0,0)) != 0)
				{
					if (retval == -1)
						break;
					::TranslateMessage(&win32msg);
					::DispatchMessage(&win32msg);
				}
				*/
	}
	/*
	while(QueryPerformanceCounter(&end))
	{
		if(  (double)(end.QuadPart-begin.QuadPart)/(double)m_frequency > (tempo))
		{
			Delta = ((double)(end.QuadPart-begin.QuadPart)/(double)m_frequency) - tempo;
			if(Delta>DeltaMax)
				DeltaMax = Delta;
			break;
		}
		//Sleep(0);
	}*/
	return 1;
}
