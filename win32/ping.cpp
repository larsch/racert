#include "ping.h"

#include <iostream>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>

result ping(ipaddr addr, unsigned ttl, unsigned timeout)
{
	HANDLE icmp = IcmpCreateFile();
	BYTE requestData[32];
	WORD requestSize = 32;
	ZeroMemory(requestData, requestSize);
	BYTE replyBuffer[2048];
	DWORD replySize = 2048;
	IP_OPTION_INFORMATION requestOptions;
	requestOptions.Ttl = UCHAR(ttl);
	requestOptions.Tos = 0;
	requestOptions.Flags = 0;
	requestOptions.OptionsSize = 0;
	requestOptions.OptionsData = NULL;
	tick_count before;
	DWORD result = IcmpSendEcho(icmp, addr.network(), requestData, requestSize,
		&requestOptions, replyBuffer, replySize,
		timeout);
	tick_count after;

	struct result r;
	r.ttl = ttl;
	r.timeout = timeout;
	IcmpCloseHandle(icmp);

	if (result == 0)
	{
		// DWORD err = GetLastError();
		// std::cout << unsigned(ttl) << " ? (" << err << ")" << std::endl;

		r.rtt = UINT64_MAX;
		r.source = ipaddr();
		return r;
	}
	else
	{
		PICMP_ECHO_REPLY reply = (PICMP_ECHO_REPLY)replyBuffer;
		r.rtt = after - before;
		r.source = ipaddr::from_network(reply->Address);
		return r;
	}
}

