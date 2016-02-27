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

#ifndef _threadqueue_h
#define _threadqueue_h

#include <condition_variable>
#include <mutex>
#include <deque>
#include <chrono>

template<typename T>
class threadqueue {
public:
	void push(const T& v) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push_back(v);
		m_cvar.notify_one();
	}
	bool try_pop(T& _result, unsigned int _timeout) {
		std::lock_guard<std::mutex> lock(m_mutex);
		auto abs_time = std::chrono::system_clock::now() + std::chrono::milliseconds(_timeout);
		while (m_queue.empty()) {
			if (m_cvar.wait_until(m_mutex, abs_time) == std::cv_status::timeout) {
				return false;
			}
		}
		_result = m_queue.front();
		m_queue.pop_front();
		return true;
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

#endif // _threadqueue_h
