
class UDPMulticastReceiver
{
public:
	UDPMulticastReceiver(void);
	~UDPMulticastReceiver(void);
	int Load(const char* ip_address, WORD upd_port, int RecvBufferSize,const char* ip_address_out_bound = NULL);
	int Unload(void);
	int Recv(char *p,int* lplen);
	int RecvNonBlocking(char *p,int* lplen,DWORD TimeOut);
	int GetLastError();
private:
	SOCKET m_sock;
	SOCKADDR_IN  m_addrRecv;
	int ErrorCode;
	WSAOVERLAPPED Overlapped;
	unsigned long m_address;
	struct ip_mreq m_imr;
};