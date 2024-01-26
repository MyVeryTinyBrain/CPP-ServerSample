#include "pch.h"
#include "Room.h"
#include "Socket.h"
#include "Util.h"
#include "Packet.h"
#include "RoomSocket.h"

using namespace std;
using namespace chrono;
using namespace Server;

Server::CMemberData::CMemberData(CSocket* InTCPSendReceiveSocket, CSocket* InUDPSendSocket, int InID)
{
    TCPSendReceiveSocket = InTCPSendReceiveSocket;
    UDPSendSocket = InUDPSendSocket;
    ID = InID;
}

Server::CMemberData::~CMemberData()
{
    TCPReceiveThread.detach();
    if (TCPReceiveThread.joinable())
    {
        TCPReceiveThread.join();
    }

    Util::SafeDelete(TCPSendReceiveSocket);
    Util::SafeDelete(UDPSendSocket);
}

bool Server::CMemberData::operator==(const CMemberData& X) const
{
    return
        (TCPSendReceiveSocket == X.TCPSendReceiveSocket) &&
        (ID == X.ID);
}

bool Server::CMemberData::operator!=(const CMemberData& X) const
{
    return !Server::CMemberData::operator==(X);
}

bool Server::CMemberData::IsValid() const
{
    return 
        (TCPSendReceiveSocket != 0) &&
        (UDPSendSocket != 0) &&
        (ID >= 0);
}

Server::CRoom::CRoom(unsigned short InRoomPort, int InMaxMember, const CRoomConstructorDesc& InDesc)
{
    TCPAcceptSocket = new CSocket(ESocketType::TCP);
    TCPAcceptSocket->SetTimeWait(false);
    TCPAcceptSocket->SetToInAddress();
    TCPAcceptSocket->SetPort(InRoomPort);
    TCPAcceptSocket->Bind();

    UDPReceiveSocket = CSocket::ReceiveOnlyUDPIPv4(IPConfig::InaddrAny(), InRoomPort);

    TerminateThreads = false;

    FixedUpdateInterval = 1.0 / 60.0;

    MembersSet.MaxMember = max(1, InMaxMember);

    OnMemberChangedCallback = InDesc.OnMemberChangedCallback;
}

Server::CRoom::~CRoom()
{
    vector<CMemberData*> CopiedMembers = Members;
    for_each(CopiedMembers.begin(), CopiedMembers.end(), [](CMemberData* Member)
    {
        Member->TCPSendReceiveSocket->Close();
    });

    TCPAcceptSocket->Close();
    UDPReceiveSocket->Close();
    TerminateThreads = true;

    if (AcceptThread.joinable())
    {
        AcceptThread.join();
    }
    if (UDPReceiveThread.joinable())
    {
        UDPReceiveThread.join();
    }
    if (FixedUpdateThread.joinable())
    {
        FixedUpdateThread.join();
    }

    for_each(Members.begin(), Members.end(), [](CMemberData* Member)
    {
        Util::SafeDelete(Member);
    });
	Members.clear();
    MembersMap.clear();

    Util::SafeDelete(TCPAcceptSocket);
    Util::SafeDelete(UDPReceiveSocket);
}

void Server::CRoom::Start()
{
    AcceptThread = thread(AcceptJob, this);
    FixedUpdateThread = thread(FixedUpdateJob, this);
    UDPReceiveThread = thread(UDPReceiveJob, this);
}

void Server::CRoom::Broadcast(ESocketType InSendSocketType, const std::shared_ptr<CPacket>& InPacket)
{
	switch (InSendSocketType)
	{
		case Server::ESocketType::TCP:
		{
			lock_guard<mutex> BroadcastTCPPacketsLockGuard(BroadcastTCPPacketsLock);
			BroadcastTCPPackets.push_back(InPacket);
		}
		break;

		case Server::ESocketType::UDP:
		{
			lock_guard<mutex> BroadcastUDPPacketsLockGuard(BroadcastUDPPacketsLock);
			BroadcastUDPPackets.push_back(InPacket);
		}
		break;
	}
}

void Server::CRoom::Broadcast(ESocketType InSendSocketType, std::shared_ptr<CPacket>&& InPacket)
{
	switch (InSendSocketType)
	{
		case Server::ESocketType::TCP:
		{
			lock_guard<mutex> BroadcastTCPPacketsLockGuard(BroadcastTCPPacketsLock);
			BroadcastTCPPackets.push_back(move(InPacket));
		}
		break;

		case Server::ESocketType::UDP:
		{
			lock_guard<mutex> BroadcastUDPPacketsLockGuard(BroadcastUDPPacketsLock);
			BroadcastUDPPackets.push_back(move(InPacket));
		}
		break;
	}
}

bool Server::CRoom::IsValid() const
{
    return
        (TCPAcceptSocket->IsValid()) && 
        (UDPReceiveSocket->IsValid());
}

EJoinRoomResult Server::CRoom::JoinRoom(
    const std::string& InIPv4Address, 
    unsigned short InPort, 
    CRoomSocketConstructorDesc InDesc,
    CRoomSocket** OutRoomSocket)
{
    CSocket* TCPSocket = CSocket::ConnectIPv4(ESocketType::TCP, InIPv4Address, InPort);
    if (TCPSocket == nullptr)
    {
        return EJoinRoomResult::FailedConnectWithTCPSocket;
    }

    // Receive Join Room Result Data Packet --------------------------------------
    // Blocking ------------------------------------------------------------------
    while (TCPSocket->GetPacketsCount() == 0)
    {
        if (false == TCPSocket->Receive())
        {
            Util::SafeDelete(TCPSocket);
            return EJoinRoomResult::FailedReceiveJoinRoomResultData;
        }
    }

    shared_ptr<CPacket> Packet = move(TCPSocket->PopPacket());
    CSucceededJoinRoomData SucceededJoinRoomData;
    CFailedJoinRoomData FailedJoinRoomData;
    if (Packet->GetHeader().DataType == DataTypeConfig::SucceededJoinRoomDataType)
    {
        memcpy_s(&SucceededJoinRoomData, sizeof(SucceededJoinRoomData), Packet->GetData(), sizeof(SucceededJoinRoomData));
    }
    else if (Packet->GetHeader().DataType == DataTypeConfig::FailedRoomJoinDataType)
    {
        memcpy_s(&FailedJoinRoomData, sizeof(FailedJoinRoomData), Packet->GetData(), sizeof(FailedJoinRoomData));
    }
    // ---------------------------------------------------------------------------

    CSocket* UDPReceiveSocket = CSocket::ReceiveOnlyUDPIPv4(InIPv4Address, SucceededJoinRoomData.UDPReceivePort);
    if (UDPReceiveSocket == nullptr)
    {
        Util::SafeDelete(TCPSocket);
        return EJoinRoomResult::FailedCreateUDPReceiveSocket;
    }

    CSocket* UDPSendSocket = CSocket::SendOnlyUDPIPv4(InIPv4Address, InPort);
    if (UDPSendSocket == nullptr)
    {
        Util::SafeDelete(TCPSocket);
        Util::SafeDelete(UDPReceiveSocket);
        return EJoinRoomResult::FailedCreateUDPSendSocket;
    }

    if (Packet->GetHeader().DataType == DataTypeConfig::SucceededJoinRoomDataType)
    {
        *OutRoomSocket = new CRoomSocket(SucceededJoinRoomData, FailedJoinRoomData, TCPSocket, UDPSendSocket, UDPReceiveSocket, InDesc);
        return EJoinRoomResult::Succeeded;
    }
    else
    {
        return EJoinRoomResult::Failed;
    }
}

EReceivedJoinRoomResult Server::CRoom::JoinSocket(CSocket* InTCPSocket)
{
    lock_guard<mutex> MemberLockGuard(MembersLock);

    if (Members.size() >= MembersSet.MaxMember)
    {
        return HandlingJoinFailedSocket(InTCPSocket, EReceivedJoinRoomResult::RoomIsFull);
    }

    auto Iterator = find_if(Members.begin(), Members.end(), [=](CMemberData* Member) {return InTCPSocket == Member->TCPSendReceiveSocket; });
    if (Iterator == Members.end())
    {
        // Alloc ID
        int ID = PacketConfig::InvalidID;
        for (int i = 0; i < MembersSet.MaxMember; ++i)
        {
            if (MembersSet.MembersIDSet[i] == false)
            {
                MembersSet.MembersIDSet[i] = true;
                ID = i;
                break;
            }
        }

        if (ID == PacketConfig::InvalidID)
        {
            return HandlingJoinFailedSocket(InTCPSocket, EReceivedJoinRoomResult::IDAllocError);
        }

        CSocket* UDPSendSocket = CSocket::ConnectIPv4(ESocketType::UDP, InTCPSocket->GetIPv4Address(), GetMemberUDPReceivePort(ID));
        if (UDPSendSocket == nullptr)
        {
            MembersSet.MembersIDSet[ID] = false;
            return HandlingJoinFailedSocket(InTCPSocket, EReceivedJoinRoomResult::UDPSendSocketBindError);
        }

        CMemberData* NewMember = new CMemberData(InTCPSocket, UDPSendSocket, ID);
        Members.push_back(NewMember);
        MembersMap.insert(make_pair(ID, NewMember));
        NewMember->TCPReceiveThread = thread(TCPReceiveMemberJob, this, NewMember);
        
        CRoomMemberChangedData MemberChangedData(EMemberChanged::Joined, ID, MembersSet, (int)Members.size());
        HandlingJoinedMember(NewMember, MemberChangedData);
        return EReceivedJoinRoomResult::Succeeded;
    }
    else
    {
        return HandlingJoinFailedSocket(InTCPSocket, EReceivedJoinRoomResult::AlreadyJoined);
    }

    return EReceivedJoinRoomResult::Succeeded;
}

void Server::CRoom::LeaveSocket(CSocket* InTCPSocket)
{
    lock_guard<mutex> MemberLockGuard(MembersLock);

    auto Iterator = find_if(Members.begin(), Members.end(), [=](CMemberData* Member) {return InTCPSocket == Member->TCPSendReceiveSocket; });
    if (Iterator != Members.end())
    {
        CMemberData* Member = *Iterator;

        CRoomMemberChangedData MemberChangedData(EMemberChanged::Left, Member->ID, MembersSet, (int)Members.size());
        MemberChangedData.MemberSetInfo.MemberSet.MembersIDSet[Member->ID] = false;
        MemberChangedData.MemberSetInfo.NumMembers -= 1;
        HandlingLeftMember(Member, MemberChangedData);
        
        // Release ID
        MembersSet.MembersIDSet[Member->ID] = false;
      
        Members.erase(Iterator);
        MembersMap.erase(Member->ID);

        Util::SafeDelete(Member);
    }
}

unsigned short Server::CRoom::GetMemberUDPReceivePort(unsigned short InMemberID)
{
    const unsigned short BasePort = 6700;
    return InMemberID = BasePort + InMemberID;
}

EReceivedJoinRoomResult Server::CRoom::HandlingJoinFailedSocket(CSocket* InTargetTCPSocket, EReceivedJoinRoomResult Reason)
{
    CFailedJoinRoomData FailedJoinRoomData(Reason);

    CPacketHeader Header;
    Header.SenderID = PacketConfig::ServerID;
    Header.DataType = DataTypeConfig::FailedRoomJoinDataType;
    Header.DataSize = (int)sizeof(FailedJoinRoomData);
    Header.SetFlag(PacketFlags::InformationPacket, true);

    unique_ptr<CPacket> Packet = make_unique<CPacket>(Header, &FailedJoinRoomData);
    InTargetTCPSocket->SendPacket(Packet.get());

    return Reason;
}

void Server::CRoom::HandlingJoinedMember(CMemberData* InJoinedMemberData, const CRoomMemberChangedData& InMemberChangedData)
{
    int JoinedMemeberID = InJoinedMemberData->ID;

    {   // Send SucceededJoinRoomData To Joined Member
        CSucceededJoinRoomData SucceededJoinRoomData(JoinedMemeberID, GetMemberUDPReceivePort(JoinedMemeberID));

        CPacketHeader Header;
        Header.SenderID = PacketConfig::ServerID;
        Header.DataType = DataTypeConfig::SucceededJoinRoomDataType;
        Header.DataSize = (int)sizeof(SucceededJoinRoomData);
        Header.SetFlag(PacketFlags::InformationPacket, true);

        unique_ptr<CPacket> Packet = make_unique<CPacket>(Header, &SucceededJoinRoomData);
        InJoinedMemberData->TCPSendReceiveSocket->SendPacket(Packet.get());
    }

    if (OnMemberChangedCallback)
    {
        OnMemberChangedCallback->OnMemberChanged(InMemberChangedData);
    }

    {   // Broadcast New Memeber Joined
        CPacketHeader Header;
        Header.SenderID = PacketConfig::ServerID;
        Header.DataType = DataTypeConfig::MemberJoinedDataType;
        Header.DataSize = sizeof(InMemberChangedData);
        Header.SetFlag(PacketFlags::InformationPacket, true);

        shared_ptr<CPacket> Packet = make_shared<CPacket>(Header, &InMemberChangedData);
        Broadcast(ESocketType::TCP, Packet);
    }
}

void Server::CRoom::HandlingLeftMember(CMemberData* InLeftMemberData, const CRoomMemberChangedData& InMemberChangedData)
{
    int ID = InLeftMemberData->ID;

    if (OnMemberChangedCallback)
    {
        OnMemberChangedCallback->OnMemberChanged(InMemberChangedData);
    }

    {   // Broadcast Some Member Left
        CPacketHeader Header;
        Header.SenderID = PacketConfig::ServerID;
        Header.DataType = DataTypeConfig::MemberLeftDataType;
        Header.DataSize = sizeof(InMemberChangedData);
        Header.SetFlag(PacketFlags::InformationPacket, true);

        shared_ptr<CPacket> Packet = make_shared<CPacket>(Header, &InMemberChangedData);
        Broadcast(ESocketType::TCP, Packet);
    }
}

void Server::CRoom::AcceptJob(CRoom* InRoom)
{
    InRoom->TCPAcceptSocket->Listen(InRoom->MembersSet.MaxMember);
    while (false == InRoom->TerminateThreads)
    {
        CSocket* ClientSocket = InRoom->TCPAcceptSocket->Accept();
        if (ClientSocket)
        {
            InRoom->JoinSocket(ClientSocket);
        }
    }
}

void Server::CRoom::TCPReceiveMemberJob(CRoom* InRoom, CMemberData* Member)
{
    while (1)
    {
        if (false == Member->TCPSendReceiveSocket->Receive())
        {
            InRoom->LeaveSocket(Member->TCPSendReceiveSocket);
            return;
        }
        else
        {
            while (Member->TCPSendReceiveSocket->GetPacketsCount() > 0)
            {
                shared_ptr<CPacket> Packet = move(Member->TCPSendReceiveSocket->PopPacket());
                HandlingPacket(InRoom, move(Packet), ESocketType::TCP);
            }
        }
    }
}

void Server::CRoom::UDPReceiveJob(CRoom* InRoom)
{
    while (1)
    {
        if (false == InRoom->UDPReceiveSocket->Receive())
        {
            return;
        }
        else
        {
            while (InRoom->UDPReceiveSocket->GetPacketsCount() > 0)
            {
                shared_ptr<CPacket> Packet = move(InRoom->UDPReceiveSocket->PopPacket());
                HandlingPacket(InRoom, move(Packet), ESocketType::UDP);
            }
        }
    }
}

void Server::CRoom::FixedUpdateJob(CRoom* InRoom)
{
    double Accumulated = 0;
    system_clock::time_point PrevTime = system_clock::now();
    
    while (false == InRoom->TerminateThreads)
    {
        system_clock::time_point CurrentTime = system_clock::now();
        nanoseconds NsDeltaTick = CurrentTime - PrevTime;
        PrevTime = CurrentTime;
        std::chrono::duration<double> ScDeltaTick = NsDeltaTick;
        Accumulated += ScDeltaTick.count();

        double FixedUpdateInterval = InRoom->FixedUpdateInterval;
        if (Accumulated > FixedUpdateInterval)
        {
            double DeltaSeconds = Accumulated;
            if (FixedUpdateInterval > 0)
            {
                Accumulated -= FixedUpdateInterval;
                Accumulated = fmod(Accumulated, FixedUpdateInterval);
            }
            else
            {
                Accumulated = 0;
            }

            // Send Broadcast Packets
            if (InRoom->BroadcastTCPPackets.empty() == false || InRoom->BroadcastUDPPackets.empty() == false)
            {
                lock_guard<mutex> MemberLockGuard(InRoom->MembersLock);
                lock_guard<mutex> ReceivedTCPPacketsLockGuard(InRoom->BroadcastTCPPacketsLock);
                lock_guard<mutex> ReceivedUDPPacketsLockGuard(InRoom->BroadcastUDPPacketsLock);

                for (CMemberData* Member : InRoom->Members)
                {
                    for (const auto& Packet : InRoom->BroadcastTCPPackets)
                    {
                        Member->TCPSendReceiveSocket->SendPacket(Packet.get());
                    }

                    for (const auto& Packet : InRoom->BroadcastUDPPackets)
                    {
                        Member->UDPSendSocket->SendPacket(Packet.get());
                    }
                }

                InRoom->BroadcastTCPPackets.clear();
                InRoom->BroadcastUDPPackets.clear();
            }

            {   // Send Single Target Pakcets
                lock_guard<mutex> MemberLockGuard(InRoom->MembersLock);

                for (CMemberData* Member : InRoom->Members)
                {
                    if (Member->SingleTargetTCPPackets.empty() == false)
                    {
                        lock_guard<mutex> SingleTargetTCPPacketsLockGuard(Member->SingleTargetTCPPacketsLock);

                        for (const auto& SingleTargetPacket : Member->SingleTargetTCPPackets)
                        {
                            Member->TCPSendReceiveSocket->SendPacket(SingleTargetPacket.get());
                        }

                        Member->SingleTargetTCPPackets.clear();
                    }

                    if (Member->SingleTargetUDPPackets.empty() == false)
                    {
                        lock_guard<mutex> SingleTargetUDPPacketsLockGuard(Member->SingleTargetUDPPacketsLock);

                        for (const auto& SingleTargetPacket : Member->SingleTargetUDPPackets)
                        {
                            Member->UDPSendSocket->SendPacket(SingleTargetPacket.get());
                        }

                        Member->SingleTargetUDPPackets.clear();
                    }
                }
            }
        }
    }
}

void Server::CRoom::HandlingPacket(CRoom* InRoom, shared_ptr<CPacket>&& InPacket, ESocketType InReceivedSocketType)
{
    switch (InPacket->GetHeader().DataType)
    {
        case DataTypeConfig::BroadcastDataType:
        {
            switch (InReceivedSocketType)
            {
                case ESocketType::TCP:
                {
                    lock_guard<mutex> BroadcastTCPPacketsLockGuard(InRoom->BroadcastTCPPacketsLock);
                    InRoom->BroadcastTCPPackets.push_back(move(InPacket));
                }
                break;

                case ESocketType::UDP:
                {
                    lock_guard<mutex> BroadcastUDPPacketsLockGuard(InRoom->BroadcastUDPPacketsLock);
                    InRoom->BroadcastUDPPackets.push_back(move(InPacket));
                }
                break;
            }
        }
        break;

        case DataTypeConfig::SingleTargetDataType:
        {
            lock_guard<mutex> MemberLockGuard(InRoom->MembersLock);

            CMemberData* Member = nullptr;
            auto Iterator = InRoom->MembersMap.find(InPacket->GetHeader().SingleTargetReceiverID);
            if (Iterator != InRoom->MembersMap.end())
            {
                Member = Iterator->second;
            }
            else
            {
                return;
            }

            switch (InReceivedSocketType)
            {
                case ESocketType::TCP:
                {
                    lock_guard<mutex> SingleTargetTCPPacketsLockGuard(Member->SingleTargetTCPPacketsLock);
                    Member->SingleTargetTCPPackets.push_back(move(InPacket));
                }
                break;

                case ESocketType::UDP:
                {
                    lock_guard<mutex> SingleTargetTCPPacketsLockGuard(Member->SingleTargetUDPPacketsLock);
                    Member->SingleTargetUDPPackets.push_back(move(InPacket));
                }
                break;
            }
        }
        break;
    }
}
