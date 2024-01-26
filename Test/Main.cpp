#include <ServerSample.h>
#include <iostream>
#include <thread>

#include <stdio.h>
#include <conio.h>

using namespace std;
using namespace Server;

const int MaxMessageLength = 8192;
char SendMessageBuffer[MaxMessageLength + 1] = {};

class COnMemberChangedCallback : public IOnRoomSocketMemberChangedCallback
{
	virtual void OnMemberChanged(const CRoomMemberChangedData& InData) override
	{
		switch (InData.MemberChanged)
		{
			case EMemberChanged::Joined:
				cout << "(RoomSocket)New Member(" << InData.ChangedMemberID << ") Joined" << endl;
				break;

			case EMemberChanged::Left:
				cout << "(RoomSocket)Some Member(" << InData.ChangedMemberID << ") Left" << endl;
				break;
		}
		cout << "(RoomSocket)Members: " << InData.MemberSetInfo.NumMembers << endl;
	}
};

class COnRoomsMemberChangedCallback : public IOnRoomMemberChangedCallback
{
	virtual void OnMemberChanged(const CRoomMemberChangedData& InData) override
	{
	}
};

void RecvJob(CRoomSocket* InRoomSocket)
{
	while (InRoomSocket->IsConnected())
	{
		if (InRoomSocket->BeginPacketUse())
		{
			while (InRoomSocket->GetPacketsCount() > 0)
			{
				shared_ptr<CPacket> Packet = InRoomSocket->PopPacket();
				if (Packet->GetHeader().SenderID != InRoomSocket->GetSocketInfo().ID)
				{
					switch (Packet->GetHeader().DataType)
					{
						case DataTypeConfig::BroadcastDataType:
						{
							string Message = Packet->GetData();
							cout << Packet->GetHeader().SenderID << ": " << Message << endl;
						}
						break;

						case DataTypeConfig::SingleTargetDataType:
						{
							string Message = Packet->GetData();
							cout << Packet->GetHeader().SenderID << " Send to you: " << Message << endl;
						}
						break;
					}
				}
			}
			InRoomSocket->EndPacketUse();
		}
		Sleep(100);
	}
}

void SendJob(CRoomSocket* InRoomSocket)
{
	ESocketType SendSocketType = ESocketType::TCP;
	int PersonalMessageTargetID = -1;

	while (InRoomSocket->IsConnected())
	{
		cin >> SendMessageBuffer;

		string Str = SendMessageBuffer;
		if (Str.length() >= 2 && Str[0] == '/')
		{
			string CommandStr = Str.substr(1);
			for (int i = 0; i < CommandStr.length(); ++i)
				CommandStr[i] = tolower(CommandStr[i]);

			int TempPersonalMessageTargetID = INT_MIN;
			try
			{
				TempPersonalMessageTargetID = stoi(CommandStr);
			}
			catch (std::exception E)
			{
				TempPersonalMessageTargetID = INT_MIN;
			}

			if (CommandStr == "/")
			{
				InRoomSocket->Close();
				return;
			}
			else if (CommandStr == "udp")
			{
				SendSocketType = ESocketType::UDP;
				cout << "Set To UDP" << endl;
			}
			else if (CommandStr == "tcp")
			{
				SendSocketType = ESocketType::TCP;
				cout << "Set To TCP" << endl;
			}
			else if (TempPersonalMessageTargetID >= 0)
			{
				PersonalMessageTargetID = TempPersonalMessageTargetID;
				cout << "Send private message to " << PersonalMessageTargetID << endl;
			}
			else if (TempPersonalMessageTargetID == -1)
			{
				PersonalMessageTargetID = TempPersonalMessageTargetID;
				cout << "Sending a message to everyone" << endl;
			}
			
			continue;
		}

		CPacketHeader Header;
		Header.SenderID = InRoomSocket->GetSocketInfo().ID;
		Header.DataType = PersonalMessageTargetID >= 0 ? DataTypeConfig::SingleTargetDataType : DataTypeConfig::BroadcastDataType;
		Header.DataSize = (int)strnlen_s(SendMessageBuffer, MaxMessageLength) + 1;
		Header.SingleTargetReceiverID = PersonalMessageTargetID >= 0 ? PersonalMessageTargetID : PacketConfig::InvalidSingleTargetReceiverID;
		unique_ptr<CPacket> Packet = make_unique<CPacket>(Header, SendMessageBuffer);
		InRoomSocket->SendPacket(SendSocketType, Packet.get());
	}
}

int main()
{
	DEBUG_MEMORY_LEAK();

	WSADATA wsaData;						
	WSAStartup(MAKEWORD(2, 2), &wsaData);	

	CRoom* Room = nullptr;

	unsigned short Port = 1234;
	//cout << "(Port (1024 ~ 49151)) >>";
	//cin >> Port;

	int bIsRoom;
	cout << "(Join: 0, Room: 1) >>";
	cin >> bIsRoom;

	CRoomConstructorDesc RoomDesc;
	RoomDesc.OnMemberChangedCallback = make_shared<COnRoomsMemberChangedCallback>();
	if (bIsRoom)
	{
		Room = new CRoom(Port, 10, RoomDesc);
		Room->Start();
		cout << "Address: " << Util::GetIPv4() << endl;
	}
	
	string IPv4Address;
	if (bIsRoom)
	{
		IPv4Address = Util::GetIPv4();
	}
	else
	{
		cout << "(IPv4) >>";
		cin >> IPv4Address;
	}

	CRoomSocket* RoomSocket = nullptr;
	CRoomSocketConstructorDesc RoomSocketDesc;
	RoomSocketDesc.OnMemberChangedCallback = make_shared<COnMemberChangedCallback>();
	if (CRoom::JoinRoom(IPv4Address, Port, RoomSocketDesc, &RoomSocket) == EJoinRoomResult::Succeeded)
	{
		cout << "Connected" << endl;
		cout << "Your ID: " << RoomSocket->GetSocketInfo().ID << endl;

		thread SendThread(SendJob, RoomSocket);
		thread RecvThread(RecvJob, RoomSocket);

		SendThread.join();
		RecvThread.join();
	}
	else
	{
		cout << "Failed To Connect" << endl;
	}

	Util::SafeDelete(RoomSocket);
	Util::SafeDelete(Room);

	WSACleanup();
}