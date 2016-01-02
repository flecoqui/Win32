class UDPMulticastStreamer
{
public:
	UDPMulticastStreamer(void);
	~UDPMulticastStreamer(void);
	int Load(const char* ip_address, WORD upd_port, char ttl, int SendBufferSize,const char* ip_address_out_bound = NULL);
	int Unload(void);
	int Send(char *p,int len);
private:
	SOCKET m_sock;
	SOCKADDR_IN  m_addrDest;
	struct in_addr m_if_addr;
	bool m_if;
	static bool RegisterWinSock();
	static bool UnregisterWinSock();
	static int RegistrationCounter;

};