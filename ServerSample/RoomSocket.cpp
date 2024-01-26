#include "pch.h"
#include "RoomSocket.h"
#include "Socket.h"
#include "Packet.h"
#include "Util.h"

using namespace std;
using namespace Server;

Server::CRoomSocket::CRoomSocket(
	const CSucceededJoinRoomData& InSucceededJoinRoomData, 
	const CFailedJoinRoomData& InFailedJoinRoomData,
	CSocket* InTCPSocket, 
	CSocket* InUDPSendSocket, 
	CSocket* InUDPReceiveSocket,
	const CRoomSocketConstructorDesc& InDesc)
{
	SucceededJoinRoomData = InSucceededJoinRoomData;
	FailedJoinRoomData = InFailedJoinRoomData;
	TCPSocket = InTCPSocket;
	UDPSendSocket = InUDPSendSocket;
	UDPReceiveSocket = InUDPReceiveSocket;

	PacketMaxReceivedQueueSize = 8192;
	bIsUsingPacket = false;
	bIsConnected = true;

	TCPReceiveThread = thread(TCPReceiveJob, this);
	UDPReceiveThread = thread(UDPReceiveJob, this);

	OnMemberChangedCallback = InDesc.OnMemberChangedCallback;
}

Server::CRoomSocket::~CRoomSocket()
{
	Close();

	if (TCPReceiveThread.joinable())
	{
		TCPReceiveThread.join();
	}
	if (UDPReceiveThread.joinable())
	{
		UDPReceiveThread.join();
	}

	Util::SafeDelete(TCPSocket);
	Util::SafeDelete(UDPSendSocket);
	Util::SafeDelete(UDPReceiveSocket);
}

bool Server::CRoomSocket::BeginPacketUse()
{
	if (false == bIsConnected)
	{
		return false;
	}

	if (bIsUsingPacket)
	{
		return false;
	}

	ReceivedQueueLock.lock();
	bIsUsingPacket = true;
	return true;
}

std::shared_ptr<CPacket> Server::CRoomSocket::PopPacket()
{
	if (false == bIsUsingPacket)
	{
		return nullptr;
	}

	shared_ptr<CPacket> Packet = move(ReceivedQueue.front());
	ReceivedQueue.pop();
	return Packet;
}

int Server::CRoomSocket::GetPacketsCount()
{
	if (false == bIsUsingPacket)
	{
		return 0;
	}

	return (int)ReceivedQueue.size();
}

int Server::CRoomSocket::GetPacketMaxReceivedQueueSize() const
{
	if (false == bIsUsingPacket)
	{
		return 0;
	}

	return PacketMaxReceivedQueueSize;
}

void Server::CRoomSocket::SetPacketMaxReceivedQueueSize(int InPacketMaxReceivedQueueSize)
{
	if (false == bIsUsingPacket)
	{
		return;
	}

	PacketMaxReceivedQueueSize = InPacketMaxReceivedQueueSize;
}

void Server::CRoomSocket::EndPacketUse()
{
	if (false == bIsUsingPacket)
	{
		return;
	}

	ReceivedQueueLock.unlock();
	bIsUsingPacket = false;
}

bool Server::CRoomSocket::SendPacket(ESocketType InSendSocketType, CPacket* InPacket)
{
	switch (InSendSocketType)
	{
		case Server::ESocketType::TCP:
			return TCPSocket->SendPacket(InPacket);
		case Server::ESocketType::UDP:
			return UDPSendSocket->SendPacket(InPacket);
	}

	return false;
}

bool Server::CRoomSocket::SendPacket(ESocketType InSendSocketType, const std::shared_ptr<CPacket>& InPacket)
{
	return SendPacket(InSendSocketType, InPacket.get());
}

bool Server::CRoomSocket::SendPacket(ESocketType InSendSocketType, std::shared_ptr<CPacket>&& InPacket)
{
	return SendPacket(InSendSocketType, InPacket.get());
}

void Server::CRoomSocket::Close()
{
	EndPacketUse();

	TCPSocket->Close();
	UDPSendSocket->Close();
	UDPReceiveSocket->Close();
	bIsConnected = false;
}

void Server::CRoomSocket::SetTCPNagleAlgorithm(bool bActive)
{
	return TCPSocket->SetTCPNagleAlgorithm(bActive);
}

bool Server::CRoomSocket::IsTCPNagleAlgorithm() const
{
	return TCPSocket->IsTCPNagleAlgorithm();
}

unsigned short Server::CRoomSocket::GetTCPPort() const
{
	return TCPSocket->GetPort();
}

unsigned short Server::CRoomSocket::GetUDPSendPort() const
{
	return UDPSendSocket->GetPort();
}

unsigned short Server::CRoomSocket::GetUDPReceivePort() const
{
	return UDPReceiveSocket->GetPort();
}

std::string Server::CRoomSocket::GetIPv4Address() const
{
	return TCPSocket->GetIPv4Address();
}

bool Server::CRoomSocket::IsValid() const
{
	return
		(TCPSocket->IsValid()) &&
		(UDPSendSocket->IsValid()) &&
		(UDPReceiveSocket->IsValid());
}

void Server::CRoomSocket::TCPReceiveJob(CRoomSocket* InRoomSocket)
{
	while (InRoomSocket->bIsConnected)
	{
		if (false == InRoomSocket->TCPSocket->Receive())
		{
			InRoomSocket->bIsConnected = false;
			return;
		}
		else if (InRoomSocket->TCPSocket->GetPacketsCount() > 0)
		{
			lock_guard<mutex> ReceivedQueueLockGuard(InRoomSocket->ReceivedQueueLock);

			while (InRoomSocket->TCPSocket->GetPacketsCount() > 0)
			{
				shared_ptr<CPacket> Packet = move(InRoomSocket->TCPSocket->PopPacket());

				while (InRoomSocket->ReceivedQueue.size() > InRoomSocket->PacketMaxReceivedQueueSize)
				{
					InRoomSocket->ReceivedQueue.pop();
				}

				if (Packet->GetHeader().IsActivatedFlag(PacketFlags::InformationPacket))
				{
					InformationPacketHandling(InRoomSocket, ESocketType::TCP, Packet);
				}
				else
				{
					if (Packet->GetHeader().SenderID != InRoomSocket->SucceededJoinRoomData.ID)
					{
						InRoomSocket->ReceivedQueue.push(move(Packet));
					}
				}
			}
		}
	}
}

void Server::CRoomSocket::UDPReceiveJob(CRoomSocket* InRoomSocket)
{
	while (InRoomSocket->bIsConnected)
	{
		if (false == InRoomSocket->UDPReceiveSocket->Receive())
		{
			InRoomSocket->bIsConnected = false;
			return;
		}
		else if (InRoomSocket->UDPReceiveSocket->GetPacketsCount() > 0)
		{
			lock_guard<mutex> ReceivedQueueLockGuard(InRoomSocket->ReceivedQueueLock);

			while (InRoomSocket->UDPReceiveSocket->GetPacketsCount() > 0)
			{
				shared_ptr<CPacket> Packet = move(InRoomSocket->UDPReceiveSocket->PopPacket());

				while (InRoomSocket->ReceivedQueue.size() > InRoomSocket->PacketMaxReceivedQueueSize)
				{
					InRoomSocket->ReceivedQueue.pop();
				}

				if (Packet->GetHeader().IsActivatedFlag(PacketFlags::InformationPacket))
				{
					InformationPacketHandling(InRoomSocket, ESocketType::TCP, Packet);
				}
				else
				{
					if (Packet->GetHeader().SenderID != InRoomSocket->SucceededJoinRoomData.ID)
					{
						InRoomSocket->ReceivedQueue.push(move(Packet));
					}
				}
			}
		}
	}
}

void Server::CRoomSocket::InformationPacketHandling(CRoomSocket* InRoomSocket, ESocketType InSocketType, const std::shared_ptr<CPacket>& InPacket)
{
	switch (InPacket->GetHeader().DataType)
	{
		case DataTypeConfig::MemberLeftDataType:
		case DataTypeConfig::MemberJoinedDataType:
		{
			CRoomMemberChangedData MemberChangedData;
			memcpy_s(&MemberChangedData, sizeof(MemberChangedData), InPacket->GetData(), sizeof(MemberChangedData));

			if (MemberChangedData.ChangedMemberID != InRoomSocket->GetSocketInfo().ID && InRoomSocket->OnMemberChangedCallback)
			{
				InRoomSocket->OnMemberChangedCallback->OnMemberChanged(MemberChangedData);
			}
		}
		break;
	}
}
