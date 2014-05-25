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

struct position {
	position(int x = 0, int y = 0) : x(x), y(y) {}
	int x;
	int y;
};

class console {
public:
	console() : output(GetStdHandle(STD_OUTPUT_HANDLE)) {}
	position cursor_position() const {
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(output, &info);
		return position(info.dwCursorPosition.X, info.dwCursorPosition.Y);
	}
	void set_cursor_position(const position& pos) {
		COORD cursorPosition = { short(pos.x), short(pos.y) };
		SetConsoleCursorPosition(output, cursorPosition);
	}
private:
	HANDLE output;
};

threadqueue<result> resultQueue;

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
		// DWORD err = GetLastError();
		// std::cout << unsigned(ttl) << " ? (" << err << ")" << std::endl;

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

void traceThread(ipaddr addr, unsigned ttl)
{
	// std::cout << "pinging " << addr << " with ttl " << ttl << std::endl;
	resultQueue.push(ping(addr, ttl));
}

unsigned int jobCount = 0;

unsigned int nextTtl = 0;

console con;

void printResultLine(const result &r)
{
	std::cout << r.ttl << "  ";
	if (r.timeout)
		std::cout << "?";
	else
		std::cout << r.source << " " << r.rtt.msecs_double() << "ms";
	std::cout << std::endl;
}

void outputResult(const result& r)
{
	while (r.ttl > nextTtl)
	{
		std::cout << "skip " << r.ttl << " " << nextTtl << std::endl;
		++nextTtl;
	}
	if (r.ttl == nextTtl)
	{
		printResultLine(r);
		++nextTtl;
	}
	if (r.ttl < nextTtl) {
		position save = con.cursor_position();
		int delta = nextTtl - r.ttl;
		position temp(0, save.y - delta);
		con.set_cursor_position(temp);
		printResultLine(r);
		con.set_cursor_position(save);
	}

}

void waitForResults(unsigned int timeout)
{
	result r;
	if (resultQueue.try_pop(r, timeout)) {
		outputResult(r);
		--jobCount;
	}
}

void traceroute(ipaddr addr)
{
	for (unsigned int ttl = 0; ttl <= 30; ++ttl) {
		new std::thread(traceThread, addr, ttl);
		++jobCount;
		waitForResults(500);
	}
	while (jobCount > 0)
		waitForResults(100);
}

void tracerouteold(ipaddr addr)
{
	for (unsigned ttl = 0; ttl <= 30; ++ttl) {
		result r = ping(addr, ttl);
		std::cout << r.ttl << " ";
		if (r.timeout)
			std::cout << "?";
		else
			std::cout << r.rtt.msecs_double() << "ms " << r.source;
		std::cout << std::endl;
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
	traceroute(addr);
	return 0;
}

int main(int argc, char** argv)
{
	WSAData data;
	WSAStartup(MAKEWORD(1, 0), &data);
	try {
		return main_safe(argc, argv);
	}
	catch (const std::exception& _e) {
		std::cerr << _e.what() << std::endl;
		return 1;
	}
}

