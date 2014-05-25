#ifndef _tick_count_h
#define _tick_count_h

#include <cstdint>

class tick_count {
public:
	tick_count() : m_count(counter()) {}
	tick_count(uint64_t v) : m_count(v) {}
	static tick_count now() { return tick_count(counter()); }
	uint64_t count() const { return m_count; }
	double secs_double() const { return m_count / double(freq()); }
	double msecs_double() const { return m_count / (double(freq()) * 0.001); }
private:
	uint64_t m_count;
	static uint64_t counter();
	static uint64_t freq();
};

inline tick_count operator-(const tick_count& _lhs, const tick_count& _rhs)
{
	return tick_count(_lhs.count() - _rhs.count());
}

#endif // _tick_count_h
