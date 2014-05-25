

#include "ipaddr.h"
#include <ostream>

std::ostream& operator<<(std::ostream& _os, const ipaddr& _ipaddr) {
	return _os << unsigned(_ipaddr[3]) << "." << unsigned(_ipaddr[2]) << "." << unsigned(_ipaddr[1]) << "." << unsigned(_ipaddr[0]);
}

