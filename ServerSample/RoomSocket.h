#pragma once

#include "RoomTypedef.h"

namespace Server
{
	class CSocket;
	class CPacket;

	class CRoomSocket
	{
		CSucceededJoinRoomData SucceededJoinRoomData;
		CFailedJoinRoomData FailedJoinRoomData;

		CSocket* TCPSocket = nullptr;
		CSocket* UDPSendSocket = nullptr;
		CSocket* UDPReceiveSocket = nullptr;

		std::thread TCPReceiveThread;
		std::thread UDPReceiveThread;
		
		/* Received TCP Packets */
		std::mutex ReceivedQueueLock;
		int PacketMaxReceivedQueueSize;
		std::queue<std::shared_ptr<CPacket>> ReceivedQueue;
		/* -------------------- */

		bool bIsUsingPacket;
		bool bIsConnected;

		std::shared_ptr<IOnRoomSocketMemberChangedCallback> OnMemberChangedCallback;

	public:
		/* Constructors */
		CRoomSocket(
			const CSucceededJoinRoomData& InSucceededJoinRoomData,
			const CFailedJoinRoomData& InFailedJoinRoomData,
			CSocket* InTCPSocket, 
			CSocket* InUDPSendSocket, 
			CSocket* InUDPReceiveSocket,
			const CRoomSocketConstructorDesc& InDesc);
		virtual ~CRoomSocket();

		/* Packet Queue Functions */
		/* To use the packet functions of this class, BeginPacketUse() must be called before use, and EndPacketUse() must be called after use. */
		/* If you call a related function without calling BeginPacketUse(), that function will not work. */
		/* After calling the Close() function, related functions are not available */
		bool BeginPacketUse();
		std::shared_ptr<CPacket> PopPacket();
		int GetPacketsCount();
		int GetPacketMaxReceivedQueueSize() const;
		void SetPacketMaxReceivedQueueSize(int InPacketMaxReceivedQueueSize);
		void EndPacketUse();

		/* Socket Functions */
		bool SendPacket(ESocketType InSendSocketType, CPacket* InPacket);
		bool SendPacket(ESocketType InSendSocketType, const std::shared_ptr<CPacket>& InPacket);
		bool SendPacket(ESocketType InSendSocketType, std::shared_ptr<CPacket>&& InPacket);
		void Close();
		bool IsConnected() const { return bIsConnected; }

		/* Socket Options */
		void SetTCPNagleAlgorithm(bool bActive);
		bool IsTCPNagleAlgorithm() const;

		/* Address Options */
		unsigned short GetTCPPort() const;
		unsigned short GetUDPSendPort() const;
		unsigned short GetUDPReceivePort() const;
		std::string GetIPv4Address() const;

		/* Options */
		bool IsValid() const;
		const CSucceededJoinRoomData& GetSocketInfo() const { return SucceededJoinRoomData; }

	private:
		/* Thread Jobs */
		static void TCPReceiveJob(CRoomSocket* InRoomSocket);
		static void UDPReceiveJob(CRoomSocket* InRoomSocket);

		/* Functions of Thread */
		static void InformationPacketHandling(CRoomSocket* InRoomSocket, ESocketType InSocketType, const std::shared_ptr<CPacket>& InPacket);
	};
}

