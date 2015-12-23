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
#include "TSStreamer.h"

TSStreamer::TSStreamer(void)
{
	index = 0;
}
TSStreamer::~TSStreamer(void)
{
	Unload();
}
long gindex = 0;
int TSStreamer::SendPacket(char *p)
{
	memcpy(&buffer[index++*PACKET_SIZE],p,PACKET_SIZE);

	if(index == MAX_PACKET)
	{
		index = 0;
		return Send((char *)buffer,MAX_PACKET*PACKET_SIZE);
	}

	return 1;
}
