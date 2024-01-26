#pragma once

namespace Server
{
	struct Util
	{
		template <class T>
		static bool SafeDelete(T& InPointer)
		{
			if (InPointer)
			{
				delete InPointer;
				InPointer = nullptr;
				return true;
			}
			return false;
		}

		template <class T>
		static bool SafeDeleteArray(T& InArray)
		{
			if (InArray)
			{
				delete[] InArray;
				InArray = nullptr;
				return true;
			}
			return false;
		}

		static int ToNativeSocketType(ESocketType InSocketType)
		{
			switch (InSocketType)
			{
				case ESocketType::TCP:
					return SOCK_STREAM;
				case ESocketType::UDP:
					return SOCK_DGRAM;
			}
			return -1;
		}

		static ESocketType FromNativeSocketType(int InNativeSocketType)
		{
			switch (InNativeSocketType)
			{
				case (int)ESocketType::TCP:
					return ESocketType::TCP;

				case (int)ESocketType::UDP:
					return ESocketType::UDP;
			}
			return ESocketType::Invalid;
		}

		static std::string GetIPv4()
		{
			char HostName[255];
			PHOSTENT HostInfo;

			if (gethostname(HostName, sizeof(HostName)) == 0)
			{
				if ((HostInfo = gethostbyname(HostName)) != NULL)
				{
					return inet_ntoa(*(struct in_addr*)*HostInfo->h_addr_list);
				}
			}
			return "";
		}
	};
}
