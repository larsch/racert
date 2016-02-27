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

#ifndef _racert_ipaddr_h
#define _racert_ipaddr_h

#include <WinSock2.h>
#include <Windows.h>
#include <cstdint>
#include <iosfwd>

class ipaddr
{
public:
   static ipaddr from_network(uint32_t _addr) { return ipaddr(ntohl(_addr)); }
   ipaddr(uint32_t _addr = 0) { m_addr.d = _addr; }
   ipaddr(IN_ADDR _addr) { m_addr.d = ntohl(_addr.s_addr); }
   ipaddr(uint8_t _a, uint8_t _b, uint8_t _c, uint8_t _d) { m_addr.b[0] = _a; m_addr.b[1] = _b; m_addr.b[2] = _c; m_addr.b[3] = _d; }
   operator IN_ADDR() const { IN_ADDR saddr; saddr.s_addr = network(); return saddr;  }
   uint32_t host() const { return m_addr.d; }
   uint32_t network() const { return htonl(m_addr.d); }
   uint8_t& operator[](size_t _index) { return m_addr.b[_index]; }
   const uint8_t& operator[](size_t _index) const { return m_addr.b[_index]; }
private:
   union
   {
      uint8_t b[4];
      uint32_t d;
   } m_addr;
};

inline bool operator==(const ipaddr& _lhs, const ipaddr& _rhs)
{
   return _lhs.host() == _rhs.host();
}

inline bool operator!=(const ipaddr& _lhs, const ipaddr& _rhs)
{
   return _lhs.host() != _rhs.host();
}

std::ostream& operator<<(std::ostream& _os, const ipaddr& _ipaddr);

#endif // _racert_ipaddr_h
