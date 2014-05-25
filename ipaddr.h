#ifndef _ipaddr_h
#define _ipaddr_h

#include <WinSock2.h>
#include <Windows.h>
#include <cstdint>
#include <iosfwd>

class ipaddr {
public:
	static ipaddr from_network(uint32_t _addr) { return ipaddr(ntohl(_addr)); }
	ipaddr(uint32_t _addr = 0) { m_addr.d = _addr; }
	ipaddr(IN_ADDR _addr) { m_addr.d = ntohl(_addr.s_addr); }
	ipaddr(uint8_t _a, uint8_t _b, uint8_t _c, uint8_t _d) { m_addr.b[0] = _a; m_addr.b[1] = _b; m_addr.b[2] = _c; m_addr.b[3] = _d; }
	uint32_t host() const { return m_addr.d; }
	uint32_t network() const { return htonl(m_addr.d); }
	uint8_t& operator[](size_t _index) { return m_addr.b[_index]; }
	const uint8_t& operator[](size_t _index) const { return m_addr.b[_index]; }
private:
	union {
		uint8_t b[4];
		uint32_t d;
	} m_addr;
};

inline bool operator==(const ipaddr& _lhs, const ipaddr& _rhs) {
	return _lhs.host() == _rhs.host();
}

std::ostream& operator<<(std::ostream& _os, const ipaddr& _ipaddr);

#endif // _ipaddr_h
