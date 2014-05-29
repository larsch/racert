#ifndef _ping_h
#define _ping_h

#include "tick_count.h"
#include "ipaddr.h"
#include <string>

struct result {
   unsigned ttl;
   tick_count rtt;
   ipaddr source;
   std::string hostname;
   unsigned timeout;
};

result ping(ipaddr addr, unsigned ttl, unsigned timeout);

#endif // _ping_h
