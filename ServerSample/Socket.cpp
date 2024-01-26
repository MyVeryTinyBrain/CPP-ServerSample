#include "pch.h"
#include "Socket.h"
#include "Util.h"
#include "SocketReceiveBuffer.h"
#include "Packet.h"
#include "SocketSendMethod.h"

using namespace std;
using namespace Server;

Server::CSocket::CSocket(Socket_t InSocket, Sockaddr_In_t InAddress)
{
	Socket = InSocket;

	Address = InAddress;

	ListenQueueLength = 0;

	switch (GetSocketType())
	{
		case ESocketType::TCP:
			ReceiveBuffer = new CTCPSocketReceiveBuffer(1024);
			SendMethod = new CTCPSocketSendMethod();
			break;
		case ESocketType::UDP:
			ReceiveBuffer = new CUDPSocketReceiveBuffer(2048);
			SendMethod = new CUDPSocketSendMethod();
			break;
		default:
			ReceiveBuffer = nullptr;
			SendMethod = nullptr;
			break;
	}
}

Server::CSocket::CSocket(ESocketType InSocketType)
{
	Socket = socket(PF_INET, Util::ToNativeSocketType(InSocketType), 0);

	memset(&Address, 0, sizeof(Address));
	Address.sin_family = AF_INET;

	ListenQueueLength = 0;

	switch (GetSocketType())
	{
		case ESocketType::TCP:
			ReceiveBuffer = new CTCPSocketReceiveBuffer(1024);
			SendMethod = new CTCPSocketSendMethod();
			break;
		case ESocketType::UDP:
			ReceiveBuffer = new CUDPSocketReceiveBuffer(1024);
			SendMethod = new CUDPSocketSendMethod();
			break;
		default:
			ReceiveBuffer = nullptr;
			SendMethod = nullptr;
			break;
	}
}

Server::CSocket::~CSocket()
{
	Close();

	Util::SafeDelete(ReceiveBuffer);
	Util::SafeDelete(SendMethod);
}

bool Server::CSocket::operator==(const CSocket& X) const
{
	return 
		(Socket == X.Socket) && 
		(Address.sin_family == X.Address.sin_family) &&
		(Address.sin_addr.S_un.S_addr == X.Address.sin_addr.S_un.S_addr) &&
		(Address.sin_port == X.Address.sin_port) &&
		(Address.sin_zero == X.Address.sin_zero) &&
		(ListenQueueLength == X.ListenQueueLength);
}

bool Server::CSocket::operator!=(const CSocket& X) const
{
	return !Server::CSocket::operator==(X);
}

bool Server::CSocket::Bind()
{
	return SOCK_SUCCEEDED(bind(Socket, (sockaddr*)&Address, sizeof(Address)));
}

bool Server::CSocket::Listen(int InQueueLength)
{
	if(SOCK_SUCCEEDED(listen(Socket, InQueueLength)))
	{
		ListenQueueLength = InQueueLength;
		return true;
	}

	ListenQueueLength = 0;
	return false;
}

Server::CSocket* Server::CSocket::Accept()
{
	Sockaddr_In_t ClientAddress = {};
	int ClientAddressSize = sizeof(ClientAddress);
	Socket_t ClientSocket = accept(Socket, (sockaddr*)&ClientAddress, &ClientAddressSize);

	if (ClientSocket != INVALID_SOCKET)
	{
		CSocket* NewSocket = new CSocket(ClientSocket, ClientAddress);
		return NewSocket;
	}

	return nullptr;
}

Server::CSocket* Server::CSocket::ConnectIPv4(ESocketType InSocketType, const string& InIPv4Address, unsigned short InPort)
{
	Socket_t ServerSocket = socket(PF_INET, Util::ToNativeSocketType(InSocketType), 0);
	Sockaddr_In_t ServerAddress = {};

	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port = htons(InPort);

	unsigned long UL = inet_addr(InIPv4Address.c_str());
	if (UL == INADDR_NONE)
	{
		return nullptr;
	}
	ServerAddress.sin_addr.s_addr = UL;

	if (SOCK_SUCCEEDED(connect(ServerSocket, (sockaddr*)&ServerAddress, sizeof(ServerAddress))))
	{
		CSocket* NewSocket = new CSocket(ServerSocket, ServerAddress);
		return NewSocket;
	}

	return nullptr;
}

CSocket* Server::CSocket::SendOnlyUDPIPv4(const std::string& InIPv4Address, unsigned short InPort)
{
	CSocket* UDPSendSocket = CSocket::ConnectIPv4(ESocketType::UDP, InIPv4Address, InPort);
	return UDPSendSocket;
}

CSocket* Server::CSocket::ReceiveOnlyUDPIPv4(const std::string& InIPv4Address, unsigned short InPort)
{
	CSocket* UDPReceiveSocket = new CSocket(ESocketType::UDP);
	UDPReceiveSocket->SetIPv4Address(InIPv4Address);
	UDPReceiveSocket->SetPort(InPort);
	UDPReceiveSocket->Bind();
	return UDPReceiveSocket;
}

bool Server::CSocket::IsValid() const
{
	return 
		(Socket != INVALID_SOCKET) &&
		(GetSocketType() == ESocketType::TCP || GetSocketType() == ESocketType::UDP) &&
		(SendMethod != nullptr) &&
		(ReceiveBuffer != nullptr);
}

void Server::CSocket::SetTimeWait(bool bActive)
{
	int Active = bActive;
	int OptLen = sizeof(Active);
	setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, (char*)&Active, OptLen);
}

bool Server::CSocket::IsTimeWait() const
{
	int Opt;
	int OptLen = sizeof(Opt);
	if (SOCK_FAILED(getsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, (char*)&Opt, &OptLen)))
	{
		return false;
	}
	return Opt;
}

void Server::CSocket::SetTCPNagleAlgorithm(bool bActive)
{
	int Active = bActive;
	int OptLen = sizeof(Active);
	setsockopt(Socket, IPPROTO_TCP, TCP_NODELAY, (char*)&Active, OptLen);
}

bool Server::CSocket::IsTCPNagleAlgorithm() const
{
	int Opt;
	int OptLen = sizeof(Opt);
	if (SOCK_FAILED(getsockopt(Socket, IPPROTO_TCP, TCP_NODELAY, (char*)&Opt, &OptLen)))
	{
		return false;
	}
	return Opt;
}

ESocketType Server::CSocket::GetSocketType() const
{
	int Opt;
	int OptLen = sizeof(Opt);
	if (SOCK_FAILED(getsockopt(Socket, SOL_SOCKET, SO_TYPE, (char*)&Opt, &OptLen)))
	{
		return ESocketType::Invalid;
	}
	return Util::FromNativeSocketType(Opt);
}

bool Server::CSocket::IsTCPConnected() const
{
	int error_code;
	int error_code_size = sizeof(error_code);
	return SOCK_SUCCEEDED(getsockopt(Socket, SOL_SOCKET, SO_ERROR, (char*)&error_code, &error_code_size)) && SOCK_SUCCEEDED(error_code);
}

void Server::CSocket::Close()
{
	closesocket(Socket);
}

bool Server::CSocket::SendPacket(CPacket* InPacket)
{
	if (false == InPacket->IsValid())
	{
		// Can't Send Invalid Packet
		return true;
	}

	return SendMethod->Send(this, InPacket);
}

bool Server::CSocket::SendPacket(const shared_ptr<CPacket>& InPacket)
{
	return SendPacket(InPacket.get());
}

bool Server::CSocket::SendPacket(shared_ptr<CPacket>&& InPacket)
{
	return SendPacket(InPacket.get());
}

bool Server::CSocket::Receive()
{
	return ReceiveBuffer->RecvToBuffer(this);
}

std::shared_ptr<CPacket> Server::CSocket::PopPacket()
{
	return move(ReceiveBuffer->PopPacket());
}

int Server::CSocket::GetPacketsCount() const
{
	return ReceiveBuffer->GetPacketsCount();
}

bool Server::CSocket::Internal_Send(const void* InBytes, int InByteLength)
{
	return SOCK_SUCCEEDED(send(Socket, (const Byte*)InBytes, InByteLength, 0));
}

bool Server::CSocket::Internal_Receive(Byte* OutBytes, int InByteLength, int* OutRecvLength)
{
	int Length = recv(Socket, OutBytes, InByteLength, 0);
	if (OutRecvLength)
	{
		*OutRecvLength = Length;
	}
	return SOCK_SUCCEEDED(Length);
}

int Server::CSocket::Internal_Peek()
{
	static const int TempSize = PacketConfig::MaxPacketSize;
	static Byte Temp[TempSize]{};
	int Length = recv(Socket, Temp, TempSize, MSG_PEEK);
	return Length;
}

void Server::CSocket::HalfClose(EHalfClose InHalfClose)
{
	int How = SD_SEND;
	switch (InHalfClose)
	{
		case EHalfClose::Read:
			How = SD_RECEIVE;
			break;
		case EHalfClose::Write:
			How = SD_SEND;
			break;
		case EHalfClose::ReadWrite:
			How = SD_BOTH;
			break;
	}
	shutdown(Socket, How);
}

unsigned short Server::CSocket::GetPort() const
{
	return ntohs(Address.sin_port);
}

void Server::CSocket::SetPort(unsigned short InPort)
{
	Address.sin_port = htons(InPort);
}

string Server::CSocket::GetIPv4Address() const
{
	return inet_ntoa(Address.sin_addr);
}

bool Server::CSocket::SetIPv4Address(const string& InIPv4Address)
{
	unsigned long UL = inet_addr(InIPv4Address.c_str());
	if (UL == INADDR_NONE)
		return false;

	Address.sin_addr.s_addr = UL;
	return true;
}

void Server::CSocket::SetToInAddress()
{
	Address.sin_addr.s_addr = htonl(INADDR_ANY);
}
