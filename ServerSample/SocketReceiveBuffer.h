#pragma once

#include "Packet.h"

namespace Server
{
	class CSocket;

	class ISocketReceiveBuffer
	{
	public:
		virtual ~ISocketReceiveBuffer() {}

		/* Buffer Operations */
		virtual bool RecvToBuffer(CSocket* InSocket) = 0;
		virtual int GetBufferCapacity() const = 0;

		/* Queue Operations */
		virtual std::shared_ptr<CPacket> PopPacket() = 0;
		virtual int GetPacketsCount() const = 0;
		virtual int GetPacketsSize() const = 0;
	};

	class CTCPSocketReceiveBuffer : public ISocketReceiveBuffer
	{
		Byte* Buffer;
		int BufferCapacity;
		int BufferUsingSize;

		bool bShouldRecvHeader;
		int NextRecvByteSize;
		CPacketHeader Header;

		std::queue<std::shared_ptr<CPacket>> Packets;
		int DataSize;
		int PacketSize;

	public:
		/* Constructors */
		CTCPSocketReceiveBuffer(int InBufferCapacity);
		virtual ~CTCPSocketReceiveBuffer();

		/* Buffer Operations */
		virtual bool RecvToBuffer(CSocket* InSocket) override;
		virtual int GetBufferCapacity() const override { return BufferCapacity; }

		/* Queue Operations */
		virtual std::shared_ptr<CPacket> PopPacket() override;
		virtual int GetPacketsCount() const override { return (int)Packets.size(); }
		virtual int GetPacketsSize() const override { return DataSize; }

	private:
		int GetHeaderByteSize() const { return sizeof(Header); }
		int GetDataByteSize() const { return Header.DataSize; }
	};

	class CUDPSocketReceiveBuffer : public ISocketReceiveBuffer
	{
		Byte* Buffer;
		int BufferCapacity;

		std::queue<std::shared_ptr<CPacket>> Packets;
		int DataSize;
		int PacketSize;

	public:
		/* Constructors */
		CUDPSocketReceiveBuffer(int InBufferCapacity);
		virtual ~CUDPSocketReceiveBuffer();

		/* Buffer Operations */
		virtual bool RecvToBuffer(CSocket* InSocket) override;
		virtual int GetBufferCapacity() const override { return BufferCapacity; }
		void SetBufferCacpacity(int InBufferCapacity);

		/* Queue Operations */
		virtual std::shared_ptr<CPacket> PopPacket() override;
		virtual int GetPacketsCount() const override { return (int)Packets.size(); }
		virtual int GetPacketsSize() const override { return DataSize; }
	};
}

