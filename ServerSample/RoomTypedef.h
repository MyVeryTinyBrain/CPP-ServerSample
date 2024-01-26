#pragma once

namespace Server
{
	struct RoomConfig
	{
		const static int MaxMemeberLimit = 1000;
	};

	enum class EReceivedJoinRoomResult
	{
		InvalidResult,
		Succeeded,
		IDAllocError,
		UDPSendSocketBindError,
		AlreadyJoined,
		RoomIsFull,
	};

	enum class EJoinRoomResult
	{
		Succeeded,

		/* CFailedJoinRoomData contains the detailed failure reason */
		Failed,

		FailedConnectWithTCPSocket,
		FailedReceiveJoinRoomResultData,
		FailedCreateUDPReceiveSocket,
		FailedCreateUDPSendSocket,
	};

	enum class EMemberChanged
	{
		None,
		Joined,
		Left,
	};

	class CRoomMembersSet
	{
	public:
		bool MembersIDSet[RoomConfig::MaxMemeberLimit] = {};
		int MaxMember;

		CRoomMembersSet();
		CRoomMembersSet(const CRoomMembersSet& X);
		CRoomMembersSet(CRoomMembersSet&& X) noexcept;

		CRoomMembersSet& operator = (const CRoomMembersSet& X);
		CRoomMembersSet& operator = (CRoomMembersSet&& X) noexcept;
	};

	class CRoomMemberSetInfo
	{
	public:
		CRoomMembersSet MemberSet;
		int NumMembers;

		CRoomMemberSetInfo();
		CRoomMemberSetInfo(const CRoomMembersSet& InMemberSet, int InNumMembers);
	};

	class CRoomMemberChangedData
	{
	public:
		EMemberChanged MemberChanged;
		int ChangedMemberID;
		CRoomMemberSetInfo MemberSetInfo;

		CRoomMemberChangedData();
		CRoomMemberChangedData(EMemberChanged InMemberChanged, int InChangedMemberID, const CRoomMembersSet& InChangedMembersSet, int InNumMembers);
	};

	class CFailedJoinRoomData
	{
	public:
		EReceivedJoinRoomResult Reason;

	public:
		/* Constructors */
		CFailedJoinRoomData();
		CFailedJoinRoomData(EReceivedJoinRoomResult InReason);
	};

	class CSucceededJoinRoomData
	{
	public:
		int ID;
		unsigned short UDPReceivePort;
	public:
		CSucceededJoinRoomData();
		CSucceededJoinRoomData(int InID, unsigned short InUDPReceivePort);
	};

	class IOnRoomMemberChangedCallback
	{
	public:
		/* Called during threaded behavior. */
		virtual void OnMemberChanged(const CRoomMemberChangedData& InData) = 0;
	};

	class CRoomConstructorDesc
	{
	public:
		std::shared_ptr<IOnRoomMemberChangedCallback> OnMemberChangedCallback;

		CRoomConstructorDesc();
	};

	class IOnRoomSocketMemberChangedCallback
	{
	public:
		/* Called during threaded behavior. */
		virtual void OnMemberChanged(const CRoomMemberChangedData& InData) = 0;
	};

	class CRoomSocketConstructorDesc
	{
	public:
		std::shared_ptr<IOnRoomSocketMemberChangedCallback> OnMemberChangedCallback;

		CRoomSocketConstructorDesc();
	};
}

