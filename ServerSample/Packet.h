#pragma once

namespace Server
{
	class CPacketHeader
	{
	public:
		/* Required input parameters */
		int SenderID;
		int DataType;	
		int DataSize;
		unsigned int Flags;

		/* Optional entry parameters */
		int SingleTargetReceiverID;

		/* Custom parameters */
		int UserDataType;
		int UserFlags;

		/* Constructors */
		CPacketHeader();

		/* Operations */
		bool IsActivatedFlag(unsigned int InFlag) const { return (Flags & InFlag) > 0; }
		void SetFlag(unsigned int InFlag, bool bInActive);

		bool IsActivatedUserFlag(unsigned int InUserFlag) const { return (UserFlags & InUserFlag) > 0; }
		void SetUserFlag(unsigned int InUserFlag, bool bInActive);

		/* Options */
		bool IsValid() const;
	};

	class CPacket
	{
		CPacketHeader Header;
		Byte* Data;

	public:
		/* Constructors */
		CPacket();
		CPacket(const CPacketHeader& InHeader, const void* InData);
		CPacket(const CPacket& X);
		CPacket(CPacket&& X) noexcept;
		virtual ~CPacket();

		/* Operators */
		CPacket& operator = (const CPacket& X);
		CPacket& operator = (CPacket&& X) noexcept;

		/* Operations */
		bool SetData(int InDataType, int InDataSize, const void* InData);

		/* Options */
		const Byte* GetData() const { return Data; }
		const CPacketHeader& GetHeader() const { return Header; }
		bool IsValid() const;
		int GetPacketSize() const { return sizeof(Header) + Header.DataSize; }
	};
}
