// Copyright (C) 2014-2016 Lars Christensen
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

#include <cstdint>
#include <process.h>
#include <string>

#include <WinSock2.h>
#include <ws2tcpip.h>

#include <thread>
#include <iomanip>
#include <map>
#include <iostream>

#include "threadqueue.h"
#include "ipaddr.h"
#include "tick_count.h"
#include "ping.h"

static volatile bool interrupted = false;

struct position {
   position(int x = 0, int y = 0) : x(x), y(y) {}
   int x;
   int y;
};

class console {
public:
   console() : output(GetStdHandle(STD_OUTPUT_HANDLE)) {}
   position cursor_position() const
   {
      CONSOLE_SCREEN_BUFFER_INFO info;
      GetConsoleScreenBufferInfo(output, &info);
      return position(info.dwCursorPosition.X, info.dwCursorPosition.Y);
   }
   void set_cursor_position(const position& pos)
   {
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
   } else {
      r.hostname = "";
   }
   return r;
}

void traceThread(ipaddr addr, unsigned ttl, unsigned timeout)
{
   resultQueue.push(ping(addr, ttl, timeout));
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

void printRow(const rowinfo& r)
{
   std::cout << std::setw(3) << r.ttl << "  ";
   bool allTimeout = true;
   for (unsigned int i = 0; i < 3; ++i) {
      if (r.latest.size() > i) {
         tick_count rtt = r.latest[i];
         if (rtt.count() == UINT64_MAX) {
            std::cout << "      *    ";
         } else {
            std::cout.precision(2);
            std::cout.setf(std::ios::fixed, std::ios::floatfield);
            std::cout << std::setw(7) << rtt.msecs_double() << " ms" << " ";
            allTimeout = false;
         }
      } else {
         std::cout << "      ?    ";
         allTimeout = false;
      }
   }
   if (r.address != ipaddr::any) {
      if (r.hostname == "?") {
         std::cout << r.address;
      } else if (r.hostname.empty()) {
         std::cout << "? [" << r.address << "]";
      } else {
         std::cout << r.hostname << " [" << r.address << "]";
      }
   } else if (allTimeout) {
      std::cout << "Request timed out.";
   }
   std::cout << std::endl;
}

std::map<unsigned int, rowinfo> m;

void outputRow(const rowinfo& r)
{
   // print rows up to the row being output
   while (r.ttl > nextTtl) {
      rowinfo& row = m[nextTtl];
      row.ttl = nextTtl;
      printRow(row);
      ++nextTtl;
   }
   // print the row if not already printed
   if (r.ttl == nextTtl) {
      printRow(r);
      ++nextTtl;
   }
   // else skip back and update row
   else if (r.ttl < nextTtl) {
      position save = con.cursor_position();
      int delta = nextTtl - r.ttl;
      position temp(0, save.y - delta);
      con.set_cursor_position(temp);
      printRow(r);
      con.set_cursor_position(save);
   }
}

#include <vector>
std::vector<std::thread*> threads;

template<class _Fn, class... _Args>
void launchJob(_Fn&& _Fx, _Args&& ... _Ax)
{
   std::thread* t = new std::thread(_Fx, _Ax...);
   threads.push_back(t);
   ++jobCount;
}

void handleResult(ipaddr addr, const result& r)
{
   if (r.ttl > maxTtl) {
      return;
   }
   rowinfo& row = m[r.ttl];
   if (r.hostname.empty()) {
      row.ttl = r.ttl;
      row.latest.push_back(r.rtt);
      row.address = r.source;
      if (row.latest.size() < 3) {
         launchJob(traceThread, addr, r.ttl, r.timeout);
         // new std::thread(traceThread, addr, r.ttl, r.timeout);
         //++jobCount;
      }
      if (row.hostname.empty() && row.address != ipaddr::any) {
         new std::thread(resolveThread, r.source, r.ttl);
         ++jobCount;
      }
      if (r.source == addr && maxTtl > r.ttl) {
         maxTtl = r.ttl;
      }
   } else {
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

void traceroute(ipaddr addr, unsigned int maxHops, unsigned int timeout)
{
   for (unsigned int ttl = 1; ttl <= maxHops && !interrupted; ++ttl) {
      new std::thread(traceThread, addr, ttl, timeout);
      ++jobCount;
      waitForResults(addr, 500);
   }
   while (jobCount > 0 && !interrupted) {
      waitForResults(addr, 100);
   }
   if (interrupted) {
      throw std::runtime_error("Interrupted");
   }
}

ipaddr getaddr(const char* host)
{
   addrinfo* info;
   int res = getaddrinfo(host, 0, 0, &info);
   if (res == 0) {
      addrinfo* cur = info;
      while (cur) {
         if (cur->ai_addr->sa_family == AF_INET) {
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

void printUsage()
{
   std::cout << std::endl <<
      "Usage: racert [-h maximum_hops] target_name" << std::endl <<
      std::endl <<
      "Options:" << std::endl <<
      "    -h maximum_hops    Maximum number of hops to search for target." << std::endl <<
      "    -w timeout         Wait timeout milliseconds for each reply." << std::endl;
}

int main_safe(int argc, char** argv)
{
   const char* host = 0;
   unsigned int maxHops = 30;
   unsigned int timeout = 5000;

   for (int argi = 1; argi < argc; ++argi) {
      char* str = argv[argi];
      if (str[0] == '-' || str[0] == '/') {
         switch (str[1]) {
         case 'h':
            maxHops = atoi(argv[++argi]);
            break;
         case 'w':
            timeout = atoi(argv[++argi]);
            break;
         default:
            throw std::runtime_error(std::string(str) + " is not a valid command option.");
         }
      } else if (host) {
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
   traceroute(addr, maxHops, timeout);
   std::cout << std::endl << "Trace complete." << std::endl;
   return 0;
}

BOOL WINAPI ConsoleCtrlHandler(DWORD CtrlType)
{
   (void)CtrlType;
   interrupted = true;
   return TRUE;
}

int main(int argc, char** argv)
{
   SetConsoleCtrlHandler(&ConsoleCtrlHandler, TRUE);
   WSAData data;
   WSAStartup(MAKEWORD(1, 0), &data);
   try {
      return main_safe(argc, argv);
   } catch (const std::exception& _e) {
      std::cerr << _e.what() << std::endl;
      return 1;
   }
}
