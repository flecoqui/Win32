#include "stdafx.h"  
#include <windows.h> 
  
#include <iostream> 
#include <string> 
#include <sstream> 
#include <mmsystem.h> 
#include <tchar.h>  
#include <conio.h>  
#include "UDPReceiver.h"
// Need to link with Ws2_32.lib
//#pragma comment(lib, "ws2_32.lib")

 


#define IsMulticastAddress(__MCastIP)		(((__MCastIP & 0xFF) >= 0xE0) && ((__MCastIP & 0xFF) <= 0xEF))


UDPMulticastReceiver::UDPMulticastReceiver(void)
{
	m_sock = NULL;
	ErrorCode = 0;
}
UDPMulticastReceiver::~UDPMulticastReceiver(void)
{
	Unload();
}
int UDPMulticastReceiver::Load(const char* ip_address, WORD upd_port, int RecvBufferSize,const char* ip_address_out_bound)
{
	ErrorCode = 0;
	// Check Network
	WSADATA wsaData;

	int cr;
	
	if (WSAStartup(MAKEWORD(2,2), &wsaData ))
	{
		return 0;
	}


	m_sock = socket( AF_INET, SOCK_DGRAM, 0);
	if(m_sock ==  0)
	{
		ErrorCode = WSAGetLastError();
		WSACleanup( );
		return 0;
	}
	/* bind the socket to the internet address */
	memset(&m_addrRecv, 0, sizeof(struct sockaddr_in));


	m_addrRecv.sin_family = AF_INET;
	m_addrRecv.sin_port = htons(upd_port);
	char reuse = 1;
	cr = setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR,(char *)&reuse, sizeof(reuse));

	if (bind(m_sock, (struct sockaddr *)&m_addrRecv, sizeof(m_addrRecv)) == SOCKET_ERROR) 
	{
		ErrorCode = WSAGetLastError();
		closesocket(m_sock);
		WSACleanup( );
		return 0;
	}
	struct in_addr addr;
	if((ip_address_out_bound)&&(strlen(ip_address_out_bound)>0))
	{
		if (inet_pton(AF_INET, ip_address_out_bound, &addr) == 1)
		{
			//			addr.s_addr = inet_addr(ip_address_out_bound);
			cr = setsockopt(m_sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&addr, sizeof(addr));
		}
	}
    /*
     * initialisation de la structure imr
     */

	//m_address = inet_addr(ip_address);
	if (inet_pton(AF_INET, ip_address, &m_address) == 1)
	{
		if (IsMulticastAddress(m_address))
		{
			struct ip_mreq m_imr;
			m_imr.imr_multiaddr.s_addr = m_address;
			if ((ip_address_out_bound) && (strlen(ip_address_out_bound) > 0))
			{
				if (inet_pton(AF_INET, ip_address_out_bound, &m_imr.imr_interface) != 1)
				{
					ErrorCode = WSAGetLastError();
					closesocket(m_sock);
					WSACleanup();
					return 0;
				}
				//m_imr.imr_interface.s_addr = inet_addr(ip_address_out_bound);
			}
			else
				m_imr.imr_interface.s_addr = htonl(INADDR_ANY);

			if ((cr = setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&m_imr, sizeof(m_imr))) == -1)
			{
				ErrorCode = WSAGetLastError();
				closesocket(m_sock);
				WSACleanup();
				return 0;
			}
		}
	}

	if((cr = setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (char *)&RecvBufferSize, sizeof(RecvBufferSize)))<0)
	{
		ErrorCode = WSAGetLastError();
		closesocket(m_sock);
		WSACleanup( );
		return 0;
	}
	return 1;
}
int UDPMulticastReceiver::Unload(void)
{
	int cr;
	if(IsMulticastAddress(m_address))
	{
		cr = setsockopt(m_sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,(char *)&m_imr, sizeof(m_imr));
	}
	closesocket(m_sock );
	WSACleanup( );
	return 1;
}
int UDPMulticastReceiver::Recv(char *p,int* lplen)
{
	int l = sizeof(m_addrRecv);
	int status = recvfrom(m_sock, p, *lplen, 0,(struct sockaddr*)&m_addrRecv, &l);
	if (status == SOCKET_ERROR) 
	{
		ErrorCode = WSAGetLastError();
		return 0;
	}
	*lplen = status;
	return 1;
}


int UDPMulticastReceiver::RecvNonBlocking(char *p,int* lplen,DWORD TimeOut)
{
	WSABUF DataBuf;
    DWORD BytesRecv = 0;
    DWORD Flags = 0;
	int l = sizeof(m_addrRecv);

	SecureZeroMemory((PVOID) &Overlapped, sizeof(WSAOVERLAPPED) );
	// Create an event handle and setup the overlapped structure.
    Overlapped.hEvent = WSACreateEvent();
    if (Overlapped.hEvent == NULL) {
		ErrorCode = WSAGetLastError();
        WSACleanup();
        return 0;
    }


	if((p==NULL) || (lplen == NULL))
		return 0;
    DataBuf.len = (ULONG) *lplen;
    DataBuf.buf = (CHAR*) p;
    int status = WSARecvFrom(m_sock,
                      &DataBuf,
                      1,
                      &BytesRecv,
                      &Flags,
                      (struct sockaddr*)&m_addrRecv, &l, &Overlapped, NULL);
    if (status == 0) {
		*lplen = BytesRecv;
        WSACloseEvent(Overlapped.hEvent);
		return 1;
	}
    else if (status != 0) {
        ErrorCode = WSAGetLastError();
        if (ErrorCode != WSA_IO_PENDING) {
            WSACloseEvent(Overlapped.hEvent);
            return 0;
        }
        else {
            status = WSAWaitForMultipleEvents(1, &Overlapped.hEvent, TRUE, TimeOut, TRUE);
            if (status == WSA_WAIT_FAILED) {
                ErrorCode = WSAGetLastError();
	            WSACloseEvent(Overlapped.hEvent);
                return 0;
            }
			else if(status == WSA_WAIT_TIMEOUT){
	            WSACloseEvent(Overlapped.hEvent);
				*lplen = 0;
                return 1;
            }

            status = WSAGetOverlappedResult(m_sock, &Overlapped, &BytesRecv,
                                FALSE, &Flags);
            if (status == FALSE) {
                ErrorCode = WSAGetLastError();
	            WSACloseEvent(Overlapped.hEvent);
                return 0;
            }
			else
			{
				*lplen = BytesRecv;
	            WSACloseEvent(Overlapped.hEvent);
				return 1;
			}
        }
        
    }

	*lplen = 0;
    WSACloseEvent(Overlapped.hEvent);
	return 0;
}
	
int UDPMulticastReceiver::GetLastError()
{
	return ErrorCode;
}