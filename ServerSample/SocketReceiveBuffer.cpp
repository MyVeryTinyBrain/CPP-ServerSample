#include "pch.h"
#include "SocketReceiveBuffer.h"
#include "Util.h"
#include "Socket.h"
#include "Packet.h"

using namespace std;
using namespace Server;

Server::CTCPSocketReceiveBuffer::CTCPSocketReceiveBuffer(int InBufferCapacity)
{
	BufferCapacity = InBufferCapacity;
	Buffer = new Byte[InBufferCapacity]{};
	BufferUsingSize = 0;

	bShouldRecvHeader = true;
	NextRecvByteSize = GetHeaderByteSize();

	DataSize = 0;
	PacketSize = 0;
}

Server::CTCPSocketReceiveBuffer::~CTCPSocketReceiveBuffer()
{
	Util::SafeDeleteArray(Buffer);
}

bool Server::CTCPSocketReceiveBuffer::RecvToBuffer(CSocket* InSocket)
{
	int BufferRemainingLength = BufferCapacity - BufferUsingSize;
	if (BufferRemainingLength < NextRecvByteSize)
	{
		/* Increase Buffer Size */
		int NewCapacity = BufferCapacity + NextRecvByteSize;
		Byte* NewBuffer = new Byte[NewCapacity]{};
		memcpy_s(NewBuffer, NewCapacity, Buffer, BufferCapacity);

		Util::SafeDeleteArray(Buffer);
		Buffer = NewBuffer;
		BufferCapacity = NewCapacity;
		BufferRemainingLength = BufferCapacity - BufferUsingSize;
	}

	int RecvBytesLength = 0;
	if (false == InSocket->Internal_Receive(Buffer + BufferUsingSize, BufferRemainingLength, &RecvBytesLength))
	{
		return false;
	}
	BufferUsingSize += RecvBytesLength;

	int BufferCursor = 0;
	// BufferUsingSize - BufferCursor: Remaining Buffer Using Size
	while (BufferUsingSize - BufferCursor >= NextRecvByteSize)
	{
		int RemainingBufferUsingSize = BufferUsingSize - BufferCursor;
		Byte* CursoredBuffer = Buffer + BufferCursor;
		int ReadByteSize = NextRecvByteSize;

		if (bShouldRecvHeader)
		{
			memcpy_s(&Header, sizeof(Header), CursoredBuffer, sizeof(Header));

			/* Should (Extract Or Recv) Data */
			NextRecvByteSize = GetDataByteSize();
		}
		else
		{
			shared_ptr<CPacket> Packet = make_shared<CPacket>(Header, CursoredBuffer);
			DataSize += Packet->GetHeader().DataSize;
			PacketSize += Packet->GetPacketSize();
			Packets.push(move(Packet));

			/* Should (Extract Or Recv) Header */
			NextRecvByteSize = GetHeaderByteSize();;
		}

		BufferCursor += ReadByteSize;
		bShouldRecvHeader = !bShouldRecvHeader;
	}
	BufferUsingSize -= BufferCursor;

	return true;
}

std::shared_ptr<CPacket> Server::CTCPSocketReceiveBuffer::PopPacket()
{
	shared_ptr<CPacket> Packet = move(Packets.front());
	Packets.pop();
	DataSize -= Packet->GetHeader().DataSize;
	PacketSize -= Packet->GetPacketSize();
	return Packet;
}

Server::CUDPSocketReceiveBuffer::CUDPSocketReceiveBuffer(int InBufferCapacity)
{
	InBufferCapacity = max(sizeof(CPacketHeader), InBufferCapacity);

	BufferCapacity = InBufferCapacity;
	Buffer = new Byte[InBufferCapacity]{};

	DataSize = 0;
	PacketSize = 0;
}

Server::CUDPSocketReceiveBuffer::~CUDPSocketReceiveBuffer()
{
	Util::SafeDeleteArray(Buffer);
}

bool Server::CUDPSocketReceiveBuffer::RecvToBuffer(CSocket* InSocket)
{
	int Length = InSocket->Internal_Peek();
	if (Length > BufferCapacity)
	{
		SetBufferCacpacity(Length);
	}

	int RecvBytesLength = 0;
	if (false == InSocket->Internal_Receive(Buffer, BufferCapacity, &RecvBytesLength))
	{
		if (Length <= 0)
		{
			return false;
		}
	}

	CPacketHeader Header;
	int HeaderSize = sizeof(Header);
	if (RecvBytesLength < HeaderSize)
	{
		// It's Invalid Data
		return true;
	}
	memcpy_s(&Header, HeaderSize, Buffer, HeaderSize);
	
	int ReceivedDataSize = RecvBytesLength - HeaderSize;
	if (ReceivedDataSize < Header.DataSize)
	{
		// It's Invalid Data
		return true;
	}

	Byte* DataCursoredBuffer = Buffer + HeaderSize;
	shared_ptr<CPacket> Packet = make_shared<CPacket>(Header, DataCursoredBuffer);
	DataSize += Packet->GetHeader().DataSize;
	PacketSize += Packet->GetPacketSize();
	Packets.push(move(Packet));

	return true;
}

void Server::CUDPSocketReceiveBuffer::SetBufferCacpacity(int InBufferCapacity)
{
	if (InBufferCapacity == BufferCapacity)
	{
		return;
	}

	Util::SafeDeleteArray(Buffer);
	BufferCapacity = InBufferCapacity;
	Buffer = new Byte[InBufferCapacity]{};
}

std::shared_ptr<CPacket> Server::CUDPSocketReceiveBuffer::PopPacket()
{
	shared_ptr<CPacket> Packet = move(Packets.front());
	Packets.pop();
	DataSize -= Packet->GetHeader().DataSize;
	PacketSize -= Packet->GetPacketSize();
	return Packet;
}
