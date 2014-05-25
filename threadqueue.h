

#ifndef _threadqueue_h
#define _threadqueue_h

#include <condition_variable>
#include <mutex>
#include <deque>

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

#endif // _threadqueue_h
