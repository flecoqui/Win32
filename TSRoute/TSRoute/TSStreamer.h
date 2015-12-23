#include "UDPStreamer.h"
#define PACKET_SIZE 188
#define MAX_PACKET 7
class TSStreamer : public UDPMulticastStreamer
{
public:
	TSStreamer(void);
	~TSStreamer(void);
	int SendPacket(char *p);
private:
	char buffer[PACKET_SIZE*MAX_PACKET];
	int index;

};