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
#include "UDPStreamer.h"
// Need to link with Ws2_32.lib
//#pragma comment(lib, "ws2_32.lib")


#define IsMulticastAddress(__MCastIP)		(((__MCastIP & 0xFF) >= 0xE0) && ((__MCastIP & 0xFF) <= 0xEF))

UDPMulticastStreamer::UDPMulticastStreamer(void)
{
	m_sock = NULL;
	m_if = false;
}
UDPMulticastStreamer::~UDPMulticastStreamer(void)
{
	Unload();
}
int UDPMulticastStreamer::Load(const char* ip_address, WORD upd_port, char ttl, int SendBufferSize,const char* ip_address_out_bound)
{
	// Check Network
	WORD wVersionRequested;
	WSADATA wsaData;
	char loop = 0;
	int cr;
	m_if = false;

	wVersionRequested = MAKEWORD( 2, 2 );
	if (WSAStartup(wVersionRequested, &wsaData ))
	{
		return 0;
	}

	m_sock = socket( AF_INET, SOCK_DGRAM, 0);
	if(m_sock ==  0)
	{
		return 0;
	}
	struct in_addr ip_in_addr;
	if((inet_pton(AF_INET, ip_address, &ip_in_addr)==1)&&(IsMulticastAddress(ip_in_addr.S_un.S_addr)))
	//	if (IsMulticastAddress(inet_addr(ip_address)))
		{
		cr = setsockopt(m_sock, IPPROTO_IP, IP_MULTICAST_TTL,(char *)&ttl, sizeof(ttl));
		cr = setsockopt(m_sock, IPPROTO_IP, IP_MULTICAST_LOOP,(char *)&loop, sizeof(loop));
	}
	else
	{
		cr = setsockopt(m_sock, IPPROTO_IP, IP_TTL,(char *)&ttl, sizeof(ttl));
	}
	cr = setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (char *)&SendBufferSize, sizeof(SendBufferSize));

	DWORD reuse = 1;
	cr = setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR,(char *)&reuse, sizeof(reuse));

	if((ip_address_out_bound)&&(strlen(ip_address_out_bound)>0))
	{
		struct in_addr m_if_addr;
		if (inet_pton(AF_INET, ip_address_out_bound, &m_if_addr) == 1)
		{
			//	    m_if_addr.s_addr = inet_addr(ip_address_out_bound);
			cr = setsockopt(m_sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&m_if_addr, sizeof(m_if_addr));
			if (cr == 0)
				m_if = true;
		}
	}

	m_addrDest.sin_family = AF_INET;
	m_addrDest.sin_addr.s_addr = ip_in_addr.S_un.S_addr;
	//m_addrDest.sin_addr.s_addr = inet_addr(ip_address);
	m_addrDest.sin_port = htons(upd_port);
	return 1;
}
int UDPMulticastStreamer::Unload(void)
{
	closesocket(m_sock );
	WSACleanup( );
	return 1;
}
int UDPMulticastStreamer::Send(char *p,int len)
{
	if(m_if == true)
		setsockopt(m_sock, IPPROTO_IP, IP_MULTICAST_IF,(char *) &m_if_addr, sizeof(m_if_addr));
	int status = sendto(m_sock, p, len, 0,(struct sockaddr*)&m_addrDest, sizeof(m_addrDest));
	if (status == SOCKET_ERROR) 
	{
		int ErrorCode = WSAGetLastError();
		return 0;
	}
	if (status != len)
	{
		return 0;
	}
	return 1;
}
