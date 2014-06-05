// Copyright (C) 2014 Lars Christensen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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

