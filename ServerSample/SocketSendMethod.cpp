#include "pch.h"
#include "SocketSendMethod.h"
#include "Socket.h"
#include "Packet.h"
#include "Util.h"

using namespace std;
using namespace Server;

bool Server::CTCPSocketSendMethod::Send(CSocket* InSocket, CPacket* InPacket)
{
	if (InSocket->Internal_Send(&InPacket->GetHeader(), sizeof(InPacket->GetHeader())))
	{
		if (InSocket->Internal_Send(InPacket->GetData(), InPacket->GetHeader().DataSize))
		{
			return true;
		}
	}
	return false;
}

Server::CUDPSocketSendMethod::CUDPSocketSendMethod()
{
	UDPSendBufferSize = 1024;
	UDPSendBuffer = new Byte[UDPSendBufferSize]{};
}

Server::CUDPSocketSendMethod::~CUDPSocketSendMethod()
{
	Util::SafeDeleteArray(UDPSendBuffer);
}

bool Server::CUDPSocketSendMethod::Send(CSocket* InSocket, CPacket* InPacket)
{
	int PacketSize = InPacket->GetPacketSize();
	int HeaderSize = sizeof(InPacket->GetHeader());
	int DataSize = InPacket->GetHeader().DataSize;

	// Resize Send Buffer
	if (UDPSendBufferSize < PacketSize)
	{
		Util::SafeDeleteArray(UDPSendBuffer);
		UDPSendBuffer = new Byte[PacketSize]{};
		UDPSendBufferSize = PacketSize;
	}

	memcpy_s(UDPSendBuffer, UDPSendBufferSize, &InPacket->GetHeader(), HeaderSize);
	memcpy_s(UDPSendBuffer + HeaderSize, UDPSendBufferSize - HeaderSize, InPacket->GetData(), DataSize);
	return InSocket->Internal_Send(UDPSendBuffer, PacketSize);
}