#include "pch.h"
#include "RoomTypedef.h"

using namespace std;
using namespace Server;

Server::CRoomMembersSet::CRoomMembersSet()
{
    MaxMember = 0;
}

Server::CRoomMembersSet::CRoomMembersSet(const CRoomMembersSet& X)
{
    *this = X;
}

Server::CRoomMembersSet::CRoomMembersSet(CRoomMembersSet&& X) noexcept
{
    *this = move(X);
}

CRoomMembersSet& Server::CRoomMembersSet::operator=(const CRoomMembersSet& X)
{
    MaxMember = X.MaxMember;
    memcpy_s(MembersIDSet, sizeof(MembersIDSet), X.MembersIDSet, sizeof(X.MembersIDSet));

    return *this;
}

CRoomMembersSet& Server::CRoomMembersSet::operator=(CRoomMembersSet&& X) noexcept
{
    MaxMember = X.MaxMember;
    memcpy_s(MembersIDSet, sizeof(MembersIDSet), X.MembersIDSet, sizeof(X.MembersIDSet));

    X.MaxMember = 0;
    memset(X.MembersIDSet, 0, sizeof(X.MembersIDSet));

    return *this;
}

Server::CRoomMemberSetInfo::CRoomMemberSetInfo()
{
    NumMembers = 0;
}

Server::CRoomMemberSetInfo::CRoomMemberSetInfo(const CRoomMembersSet& InMemberSet, int InNumMembers)
{
    MemberSet = InMemberSet;
    NumMembers = InNumMembers;
}

Server::CRoomMemberChangedData::CRoomMemberChangedData()
{
    MemberChanged = EMemberChanged::None;
    ChangedMemberID = PacketConfig::InvalidID;
}

Server::CRoomMemberChangedData::CRoomMemberChangedData(EMemberChanged InMemberChanged, int InChangedMemberID, const CRoomMembersSet& InChangedMembersSet, int InNumMembers)
{
    MemberChanged = InMemberChanged;
    ChangedMemberID = InChangedMemberID;
    MemberSetInfo = CRoomMemberSetInfo(InChangedMembersSet, InNumMembers);
}

Server::CFailedJoinRoomData::CFailedJoinRoomData()
{
    Reason = EReceivedJoinRoomResult::InvalidResult;
}

Server::CFailedJoinRoomData::CFailedJoinRoomData(EReceivedJoinRoomResult InReason)
{
    Reason = InReason;
}

Server::CSucceededJoinRoomData::CSucceededJoinRoomData()
{
    ID = DataTypeConfig::InvalidDataType;
    UDPReceivePort = PortConfig::InvalidUDPPort;
}

Server::CSucceededJoinRoomData::CSucceededJoinRoomData(int InID, unsigned short InUDPReceivePort)
{
    ID = InID;
    UDPReceivePort = InUDPReceivePort;
}

Server::CRoomConstructorDesc::CRoomConstructorDesc()
{
    OnMemberChangedCallback = nullptr;
}

Server::CRoomSocketConstructorDesc::CRoomSocketConstructorDesc()
{
    OnMemberChangedCallback = nullptr;
}

