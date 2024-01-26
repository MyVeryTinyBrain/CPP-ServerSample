#pragma once

#include "RoomTypedef.h"

namespace Server
{
	class CSocket;
	class CPacket;
	class CRoomSocket;

	class CMemberData
	{
	public:
		CSocket* TCPSendReceiveSocket;
		CSocket* UDPSendSocket;
		int ID;
		std::thread TCPReceiveThread;

		/* Single Target TCP Packets */
		std::mutex SingleTargetTCPPacketsLock;
		std::vector<std::shared_ptr<CPacket>> SingleTargetTCPPackets;
		/* ------------------------- */
		
		/* Single Target UDP Packets */
		std::mutex SingleTargetUDPPacketsLock;
		std::vector<std::shared_ptr<CPacket>> SingleTargetUDPPackets;
		/* ------------------------- */

		/* Constructors */
		CMemberData(CSocket* InTCPSendReceiveSocket, CSocket* InUDPSendSocket, int InID);
		virtual ~CMemberData();

		/* Operators */
		bool operator == (const CMemberData& X) const;
		bool operator != (const CMemberData& X) const;

		bool IsValid() const;
	};

	class CRoom
	{
		CSocket* TCPAcceptSocket;
		CSocket* UDPReceiveSocket;

		bool TerminateThreads;

		std::thread AcceptThread;

		/* Broadcast TCP Packets */
		std::mutex BroadcastTCPPacketsLock;
		std::vector<std::shared_ptr<CPacket>> BroadcastTCPPackets;
		/* ---------------- */

		std::thread UDPReceiveThread;
		/* Broadcast UDP Packets */
		std::mutex BroadcastUDPPacketsLock;
		std::vector<std::shared_ptr<CPacket>> BroadcastUDPPackets;
		/* ---------------- */

		std::thread FixedUpdateThread;
		double FixedUpdateInterval;

		/* Members */
		std::mutex MembersLock;
		CRoomMembersSet MembersSet;
		std::vector<CMemberData*> Members;
		std::unordered_map<int, CMemberData*> MembersMap;
		/* ------ */

		std::shared_ptr<IOnRoomMemberChangedCallback> OnMemberChangedCallback;

	public:
		/* Construcotrs */
		CRoom(unsigned short InRoomPort, int InMaxMember, const CRoomConstructorDesc& InDesc);
		virtual ~CRoom();

		/* Functions */
		void Start();
		void Broadcast(ESocketType InSendSocketType, const std::shared_ptr<CPacket>& InPacket);
		void Broadcast(ESocketType InSendSocketType, std::shared_ptr<CPacket>&& InPacket);

		/* Options */
		double GetFixedUpdateInterval() const { return FixedUpdateInterval; }
		void SetFixedUpdateInterval(double InFixedUpdateInterval) { FixedUpdateInterval = InFixedUpdateInterval; }
		int GetMaxMember() const { return MembersSet.MaxMember; }
		int GetCurrentMembersCount() const { return (int)Members.size(); }

		bool IsValid() const;

		/* Connect To Room */
		static EJoinRoomResult JoinRoom(
			const std::string& InIPv4Address,
			unsigned short InPort,
			CRoomSocketConstructorDesc InDesc,
			CRoomSocket** OutRoomSocket);

	private:
		/* Private Methods */
		EReceivedJoinRoomResult JoinSocket(CSocket* InTCPSocket);
		void LeaveSocket(CSocket* InTCPSocket);
		static unsigned short GetMemberUDPReceivePort(unsigned short InMemberID);
		
		/* Events */
		EReceivedJoinRoomResult HandlingJoinFailedSocket(CSocket* InTargetTCPSocket, EReceivedJoinRoomResult Reason);
		void HandlingJoinedMember(CMemberData* InJoinedMemberData, const CRoomMemberChangedData& InMemberChangedData);
		void HandlingLeftMember(CMemberData* InLeftMemberData, const CRoomMemberChangedData& InMemberChangedData);

	private:
		/* Thread Jobs */
		static void AcceptJob(CRoom* InRoom);
		static void TCPReceiveMemberJob(CRoom* InRoom, CMemberData* Member);
		static void UDPReceiveJob(CRoom* InRoom);
		static void FixedUpdateJob(CRoom* InRoom);

		/* Functions of Thread */
		static void HandlingPacket(CRoom* InRoom, std::shared_ptr<CPacket>&& InPacket, ESocketType InReceivedSocketType);
	};
}

