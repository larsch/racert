// Copyright (C) 2014 Lars Christensen
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
