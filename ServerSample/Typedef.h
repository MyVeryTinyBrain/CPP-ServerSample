#pragma once

namespace Server
{
	typedef char Byte;
	typedef SOCKET Socket_t;
	typedef SOCKADDR_IN Sockaddr_In_t;

	enum class ESocketType
	{
		Invalid,
		TCP,
		UDP,
	};

	enum class EHalfClose
	{
		Read,
		Write,
		ReadWrite,
	};

	enum DataTypeConfig
	{
		InvalidDataType = INT_MIN,

		/* Contains the data you receive when you successfully enter the room */
		SucceededJoinRoomDataType,
		
		/* Contains data received on unsuccessful room entry */
		FailedRoomJoinDataType,

		MemberJoinedDataType,
		MemberLeftDataType,
		
		/* Use this type to broadcast data to other members */
		BroadcastDataType,

		/* This packet is sent to a single destination. */
		SingleTargetDataType,
	};

	enum PortConfig
	{
		InvalidUDPPort = USHRT_MAX,
	};

	struct IPConfig
	{
		static std::string InaddrAny() { return "0.0.0.0"; }
	};

	struct PacketConfig
	{
		static const int MaxPacketSize = 51000;
		static const int ServerID = INT_MAX;
		static const int InvalidID = INT_MIN;
		static const int InvalidSingleTargetReceiverID = -1;
		static const int InvalidDataType = DataTypeConfig::InvalidDataType;
	};

	enum PacketFlags
	{
		None = 0x0,

		/* A flag indicating that this is an information packet sent from the server. */
		InformationPacket = 0x1,
	};
}
