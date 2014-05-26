#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <cstdint>
#include <process.h>
#include <string>

#include <thread>
#include <iomanip>
#include <map>
#include "threadqueue.h"
#include "ipaddr.h"
#include "tick_count.h"

struct job {
	ipaddr addr;
	unsigned ttl;
};

struct result {
	unsigned ttl;
	tick_count rtt;
	ipaddr source;
	std::string hostname;
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

result resolve(unsigned ttl, ipaddr addr)
{
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr = addr;
	sin.sin_port = 0;
	char hostname[1024];
	size_t hostname_len = 1024;
	int res = getnameinfo((sockaddr*)&sin, sizeof(sin), hostname, hostname_len, 0, 0, NI_NAMEREQD);
	result r;
	r.ttl = ttl;
	r.source = addr;
	if (res == 0) {
		r.hostname = hostname;
	}
	else {
		r.hostname = "?";
	}
	return r;
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

void traceThread(ipaddr addr, unsigned ttl)
{
	resultQueue.push(ping(addr, ttl));
}

void resolveThread(ipaddr addr, unsigned ttl)
{
	resultQueue.push(resolve(ttl, addr));
}

unsigned int jobCount = 0;
unsigned int nextTtl = 1;
unsigned int maxTtl = 255;

console con;

struct rowinfo {
	unsigned ttl;
	tick_count lowest;
	tick_count average;
	tick_count highest;
	std::deque<tick_count> latest;
	ipaddr address;
	std::string hostname;
};

void printRow(const rowinfo &r)
{
	std::cout << std::setw(3) << r.ttl << "  ";
	bool allTimeout = true;
	for (unsigned int i = 0; i < 3; ++i) {
		if (r.latest.size() > i) {
			tick_count rtt = r.latest[i];
			if (rtt.count() == UINT64_MAX)
				std::cout << "      *    ";
			else {
				std::cout.precision(2);
				std::cout.setf(std::ios::fixed, std::ios::floatfield);
				std::cout << std::setw(7) << rtt.msecs_double() << " ms" << " ";
				allTimeout = false;
			}
		}
		else {
			std::cout << "      ?    ";
			allTimeout = false;
		}
	}
	if (r.address != INADDR_ANY) {
		if (r.hostname.empty() || r.hostname == "?")
			std::cout << r.address;
		else
			std::cout << r.hostname << " [" << r.address << "]";
	}
	else if (allTimeout)
	{
		std::cout << "Request timed out.";
	}
	std::cout << std::endl;
}

std::map<unsigned int, rowinfo> m;

void outputRow(const rowinfo& r)
{
	while (r.ttl > nextTtl)
	{
		rowinfo& row = m[nextTtl];
		row.ttl = nextTtl;
		printRow(row);
		++nextTtl;
	}
	if (r.ttl == nextTtl)
	{
		printRow(r);
		++nextTtl;
	}
	if (r.ttl < nextTtl) {
		position save = con.cursor_position();
		int delta = nextTtl - r.ttl;
		position temp(0, save.y - delta);
		con.set_cursor_position(temp);
		printRow(r);
		con.set_cursor_position(save);
	}
}

void handleResult(ipaddr addr, const result& r)
{
	if (r.ttl > maxTtl)
		return;
	rowinfo& row = m[r.ttl];
	if (r.hostname.empty()) {
		row.ttl = r.ttl;
		row.latest.push_back(r.rtt);
		row.address = r.source;
		if (row.latest.size() < 3) {
			new std::thread(traceThread, addr, r.ttl);
			++jobCount;
		}
		if (row.hostname.empty() && row.address != INADDR_ANY) {
			new std::thread(resolveThread, r.source, r.ttl);
			++jobCount;
		}
		if (r.source == addr && maxTtl > r.ttl)
			maxTtl = r.ttl;
	}
	else {
		row.hostname = r.hostname;
	}
	outputRow(row);
}

void waitForResults(ipaddr addr, unsigned int timeout)
{
	result r;
	if (resultQueue.try_pop(r, timeout)) {
		handleResult(addr, r);
		--jobCount;
	}
}

void traceroute(ipaddr addr, unsigned int maxHops)
{
	for (unsigned int ttl = 1; ttl <= maxHops; ++ttl) {
		new std::thread(traceThread, addr, ttl);
		++jobCount;
		waitForResults(addr, 500);
	}
	while (jobCount > 0)
		waitForResults(addr, 100);
}

//void tracerouteold(ipaddr addr)
//{
//	for (unsigned ttl = 0; ttl <= 30; ++ttl) {
//		result r = ping(addr, ttl);
//		std::cout << r.ttl << " ";
//		if (r.timeout)
//			std::cout << "?";
//		else
//			std::cout << r.rtt.msecs_double() << "ms " << r.source;
//		std::cout << std::endl;
//		if (r.source == addr)
//			break;
//	}
//}

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
	throw std::runtime_error(std::string("Unable to resolve target system name ") + host + ".");
}

void printUsage() {
	std::cout << std::endl <<
		"Usage: racert [-h maximum_hops] target_name" << std::endl <<
		std::endl <<
		"Options:" << std::endl <<
		"    -h maximum_hops   Maximum number of hops to search for target." << std::endl;
}

int main_safe(int argc, char** argv) {
	const char* host = 0;
	unsigned int maxHops = 30;

	for (int argi = 1; argi < argc; ++argi)
	{
		char* str = argv[argi];
		if (str[0] == '-' || str[0] == '/') {
			switch (str[1]) {
			case 'h':
				maxHops = atoi(argv[++argi]); break;
			default:
				throw std::runtime_error(std::string(str) + " is not a valid command option.");
			}
		}
		else if (host) {
			printUsage();
			return 1;
		} else {
			host = str;
		}
	}

	if (!host) {
		printUsage();
		return 1;
	}

	ipaddr addr(getaddr(host));
	std::cout << std::endl << "Tracing route to " << host << " [" << addr << "]" << std::endl;
	std::cout << "over a maximum of " << maxHops << " hops:" << std::endl << std::endl;
	traceroute(addr, maxHops);
	std::cout << std::endl << "Trace complete." << std::endl;
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

