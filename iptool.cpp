#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <cstdint>
#include <process.h>

#include <thread>
#include "threadqueue.h"
#include "ipaddr.h"
#include "tick_count.h"

struct job {
	ipaddr addr;
	unsigned ttl;
};

struct result {
	unsigned ttl;
	bool timeout;
	tick_count rtt;
	ipaddr source;
};

threadqueue<result*> resultQueue;

void handleResults()
{
	for (;;) {
		result* r = resultQueue.pop();
		std::cout << r->ttl << "  ";
		if (r->timeout)
			std::cout << "?";
		else
			std::cout << r->source << " " << r->rtt.msecs_double() << "ms";
		std::cout << std::endl;
		delete r;
	}
}

void postResult(unsigned ttl, bool timeout, tick_count rtt, ipaddr source = ipaddr())
{
	result* r = new result;
	r->ttl = ttl;
	r->timeout = timeout;
	r->rtt = rtt;
	r->source = source;
	resultQueue.push(r);
}

void traceThread(ipaddr addr, unsigned ttl)
{
	std::cout << "pinging " << addr << " with ttl " << ttl << std::endl;

	HANDLE icmp = IcmpCreateFile();
	BYTE requestData[32];
	WORD requestSize = 32;
	BYTE replyBuffer[1024];
	DWORD replySize = 1024;
	DWORD timeout = 5000;
	IP_OPTION_INFORMATION requestOptions;
	requestOptions.Ttl = UCHAR(ttl);
	requestOptions.Tos = 0;
	requestOptions.Flags = 0;
	requestOptions.OptionsSize = 0;
	tick_count before;
	//DWORD tick = timeGetTime();
	DWORD result = IcmpSendEcho(icmp, addr.network(), requestData, requestSize,
		&requestOptions, replyBuffer, replySize,
		timeout);
	//DWORD delta = timeGetTime() - tick;
	tick_count after;
	if (result == 0)
	{
		std::cerr << ">" << GetLastError() << "<" << std::endl;
		postResult(ttl, true, 0);
	}
	else
	{
		PICMP_ECHO_REPLY reply = (PICMP_ECHO_REPLY)replyBuffer;
		ipaddr sourceAddress = ipaddr::from_network(reply->Address);
		postResult(ttl, false, after - before, sourceAddress);
	}
	IcmpCloseHandle(icmp);
}

//void startTrace(ipaddr addr, unsigned ttl)
//{
//	job* j = new job;
//	j->ttl = ttl;
//	j->addr = addr;
//	_beginthread(traceThread, 0, j);
//}

void traceroute(ipaddr addr)
{
	for (unsigned int ttl = 0; ttl <= 30; ++ttl)
		//new std::thread(traceThread, addr, ttl);
		traceThread(addr, ttl);
	//startTrace(addr, ttl);
	handleResults();
}

result ping(ipaddr addr, unsigned ttl)
{
	HANDLE icmp = IcmpCreateFile();
	BYTE requestData[32];
	WORD requestSize = 32;
	ZeroMemory(requestData, requestSize);
	BYTE replyBuffer[2048];
	DWORD replySize = 2048;
	DWORD timeout = 1500;
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
	IcmpCloseHandle(icmp);

	if (result == 0)
	{
		DWORD err = GetLastError();
		std::cout << unsigned(ttl) << " ? (" << err << ")" << std::endl;

		r.rtt = 0;
		r.source = ipaddr();
		r.timeout = true;
		return r;
	}
	else
	{
		PICMP_ECHO_REPLY reply = (PICMP_ECHO_REPLY)replyBuffer;
		r.rtt = after - before;
		r.source = ipaddr::from_network(reply->Address);
		r.timeout = false;
		return r;
	}
}

void tracerouteold(ipaddr addr)
{
	for (unsigned ttl = 0; ttl <= 30; ++ttl) {
		result r = ping(addr, ttl);
		std::cout << r.ttl << " ";
		if (r.timeout)
			std::cout << "?";
		else
			std::cout << r.rtt.msecs_double() << "ms " << r.source << std::endl;
		if (r.source == addr)
			break;
	}
}

void getinfo(const char* str)
{
	addrinfo* info;

	int res = getaddrinfo(str, 0, 0, &info);
	if (res == 0)
	{
		addrinfo* cur = info;
		while (cur) {
			if (cur->ai_addr->sa_family == AF_INET)
			{
				struct sockaddr_in* sin = (sockaddr_in*)cur->ai_addr;
				ipaddr addr = ipaddr(sin->sin_addr);
				std::cout << addr;
			}
			if (cur->ai_canonname)
			{
				std::cout << " " << cur->ai_canonname;
			}
			std::cout << std::endl;
			cur = cur->ai_next;
		}
	}
	else
	{
		std::cerr << "Failed to getaddrinfo() for " << str << "(" << res << ")" << std::endl;
	}
}

ipaddr getaddr(const char* host) {
	addrinfo* info;
	int res = getaddrinfo(host, 0, 0, &info);
	if (res == 0)
	{
		addrinfo* cur = info;
		while (cur) {
			if (cur->ai_addr->sa_family == AF_INET)
			{
				struct sockaddr_in* sin = (sockaddr_in*)cur->ai_addr;
				ipaddr addr = ipaddr(sin->sin_addr);
				freeaddrinfo(info);
				return addr;
			}
			cur = cur->ai_next;
		}
		freeaddrinfo(info);
	}
	throw std::runtime_error("Failed to get address for host");
}

int main_safe(int argc, char** argv) {
	const char* host = 0;
	for (int argi = 1; argi < argc; ++argi)
	{
		char* str = argv[argi];
		if (str[0] == '-' || str[1] == '/' || host) {
			throw std::runtime_error("Bad parameter");
		}
		else {
			host = str;
		}
	}
	ipaddr addr(getaddr(host));
	std::cout << "Tracing route to " << host << "[" << addr << "]" << std::endl;
	tracerouteold(addr);
	return 0;
}

int main(int argc, char** argv)
{
	WSAData data;
	WSAStartup(MAKEWORD(1, 0), &data);
	timeBeginPeriod(1);

	//HANDLE icmp = IcmpCreateFile();
	//BYTE requestData[32];
	//WORD requestSize = 32;
	//BYTE replyBuffer[2048];
	//DWORD replySize = 2048;
	//DWORD timeout = 5000;
	//IP_OPTION_INFORMATION requestOptions;
	//requestOptions.Ttl = 4;
	//requestOptions.Tos = 0;
	//requestOptions.Flags = 0;
	//requestOptions.OptionsSize = 0;
	////tick_count before;
	////uint64_t tick = xclock();
	//DWORD tick = timeGetTime();
	//DWORD result = IcmpSendEcho(icmp, 0x08080808, requestData, requestSize,
	//	&requestOptions, replyBuffer, replySize,
	//	timeout);
	//std::cout << result << std::endl;

	//tracerouteold(ipaddr(8, 8, 8, 8));

	//return 0;

	// timeEndPeriod(1);
	try {
		return main_safe(argc, argv);
	}
	catch (const std::exception& _e) {
		std::cerr << _e.what() << std::endl;
		return 1;
	}
}

