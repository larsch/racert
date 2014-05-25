#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <cstdint>
#include <process.h>

#include <thread>
#include <mutex>
#include <deque>
#include <condition_variable>

#include "ipaddr.h"
#include "tick_count.h"

template<typename T>
class threadqueue {
public:
	void push(const T& v) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push_back(v);
		m_cvar.notify_one();
	}
	T pop() {
		std::lock_guard<std::mutex> lock(m_mutex);
		while (m_queue.empty()) {
			m_cvar.wait(m_mutex);
		}
		T ret = m_queue.front();
		m_queue.pop_front();
		return ret;
	}
private:
	std::deque<T> m_queue;
	std::mutex m_mutex;
	std::condition_variable_any m_cvar;
};

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

void tracerouteold(ipaddr addr)
{
	HANDLE icmp = IcmpCreateFile();
	for (UCHAR ttl = 0; ttl <= 30; ++ttl) {
      BYTE requestData[32];
      WORD requestSize = 32;
	  ZeroMemory(requestData, requestSize);
	  BYTE replyBuffer[2048];
	  DWORD replySize = 2048;
      DWORD timeout = 1500;
      IP_OPTION_INFORMATION requestOptions;
	  requestOptions.Ttl = ttl;
      requestOptions.Tos = 0;
      requestOptions.Flags = 0;
      requestOptions.OptionsSize = 0;
	  requestOptions.OptionsData = NULL;
	  //tick_count before;
	  //uint64_t tick = xclock();
      DWORD tick = timeGetTime();
	  std::cerr << "send to " << addr << std::endl;
      DWORD result = IcmpSendEcho(icmp, addr.network(), requestData, requestSize,
                                  &requestOptions, replyBuffer, replySize,
                                  timeout);
      DWORD delta = timeGetTime() - tick;
	  //uint64_t delta = xclock() - tick;
	  //tick_count after;
	  std::cout << result << " ";
	  if (result == 0)
      {
		  DWORD err = GetLastError();
		  std::cout << unsigned(ttl) << " ? (" << err << ")" << std::endl;
      }
      else
      {
		//  double delay = (after - before).msecs_double();
		  double delay = delta;
         PICMP_ECHO_REPLY reply = (PICMP_ECHO_REPLY)replyBuffer;
		 ipaddr sourceAddress = ipaddr::from_network(reply->Address);
			 std::cout << unsigned(ttl) << " " << sourceAddress << " " << delay << "ms" << std::endl;
			 if (sourceAddress == addr)
			 {
				 break;
			 }
      }
   }
	IcmpCloseHandle(icmp);
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
		} else {
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

