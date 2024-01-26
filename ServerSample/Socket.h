#pragma once

namespace Server
{
	class CPacket;
	class ISocketReceiveBuffer;
	class ISocketSendMethod;

	class CSocket
	{
		friend class CTCPSocketReceiveBuffer;
		friend class CUDPSocketReceiveBuffer;
		friend class CTCPSocketSendMethod;
		friend class CUDPSocketSendMethod;

		Socket_t Socket;
		Sockaddr_In_t Address;
		int ListenQueueLength;

		ISocketReceiveBuffer* ReceiveBuffer;
		ISocketSendMethod* SendMethod;

	private:
		/* Private Constructors */
		CSocket(Socket_t InSocket, Sockaddr_In_t InAddress);

	public:
		/* Public Constructors */
		CSocket(ESocketType InSocketType);
		virtual ~CSocket();

	public:
		/* Operators */
		bool operator == (const CSocket& X) const;
		bool operator != (const CSocket& X) const;

		/* Socket Operations */
		bool Bind();
		bool Listen(int InQueueLength);
		CSocket* Accept();
		static CSocket* ConnectIPv4(ESocketType InSocketType, const std::string& InIPv4Address, unsigned short InPort);
		static CSocket* SendOnlyUDPIPv4(const std::string& InIPv4Address, unsigned short InPort);
		static CSocket* ReceiveOnlyUDPIPv4(const std::string& InIPv4Address, unsigned short InPort);
		void HalfClose(EHalfClose InHalfClose);
		void Close();

	public:
		/* Socket Functions */
		bool SendPacket(CPacket* InPacket);
		bool SendPacket(const std::shared_ptr<CPacket>& InPacket);
		bool SendPacket(std::shared_ptr<CPacket>&& InPacket);
		bool Receive();
		std::shared_ptr<CPacket> PopPacket();
		int GetPacketsCount() const;

	private:
		bool Internal_Send(const void* InBytes, int InByteLength);
		bool Internal_Receive(Byte* OutBytes, int InByteLength, int* OutRecvLength = nullptr);
		int Internal_Peek();

	public:
		/* Socket Options */
		void SetTimeWait(bool bActive);
		bool IsTimeWait() const;

		void SetTCPNagleAlgorithm(bool bActive);
		bool IsTCPNagleAlgorithm() const;

		/* Readonly Socket Options */
		ESocketType GetSocketType() const;
		bool IsTCPConnected() const;
		
		/* Address Options */
		unsigned short GetPort() const;
		void SetPort(unsigned short InPort);

		std::string GetIPv4Address() const;
		bool SetIPv4Address(const std::string& InIPv4Address);
		void SetToInAddress();

		bool IsValid() const;
		int GetListenQueueLength() const { return ListenQueueLength; }
	};
}

