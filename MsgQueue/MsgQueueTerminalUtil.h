#ifndef _MSG_QUEUE_TERMINAL_UTIL_H_
#define _MSG_QUEUE_TERMINAL_UTIL_H_

#include <map>
#include <boost/thread/mutex.hpp>

template<typename ValueT> 
class chained_call_context_tbl
{
public:
	void put(const ValueT &value);
	void get(ValueT &value) const;

private:
	static unsigned long current_thread_id();

	boost::shared_mutex _access_lock;
	std::map<unsigned long, ValueT> _tbl;
};

#include "MsgQueueTerminalUtilImpl.h"

#endif

