#pragma once

namespace Server
{
	class CPacket;
	class CSocket;

	class ISocketSendMethod
	{
	public:
		virtual ~ISocketSendMethod() {}
		virtual bool Send(CSocket* InSocket, CPacket* InPacket) = 0;
	};

	class CTCPSocketSendMethod : public ISocketSendMethod
	{
	public:
		virtual bool Send(CSocket* InSocket, CPacket* InPacket) override;
	};

	class CUDPSocketSendMethod : public ISocketSendMethod
	{
		Byte* UDPSendBuffer;
		int UDPSendBufferSize;

	public:
		CUDPSocketSendMethod();
		virtual ~CUDPSocketSendMethod();
		virtual bool Send(CSocket* InSocket, CPacket* InPacket) override;
	};
}

