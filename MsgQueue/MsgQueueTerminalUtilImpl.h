#ifndef _MSG_QUEUE_TERMINAL_UTIL_IMPL_H_
#define _MSG_QUEUE_TERMINAL_UTIL_IMPL_H_

#include <stdio.h>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

/************************
 * chained_call_context_tbl
 ************************/
template<typename ValueT> 
void chained_call_context_tbl<ValueT>::put(const ValueT &value)
{
	unsigned long thread_id = chained_call_context_tbl<ValueT>::current_thread_id();
	typename std::map<unsigned long, ValueT>::const_iterator iter = this->_tbl.find(thread_id);
	{
		boost::unique_lock<boost::shared_mutex> lock(this->_access_lock);
		if(iter!=this->_tbl.end())
			this->_tbl.erase(thread_id);
		this->_tbl.insert(std::make_pair(thread_id, value));
	}
}

template<typename ValueT> 
void chained_call_context_tbl<ValueT>::get(ValueT &value) const
{
	unsigned long thread_id = chained_call_context_tbl<ValueT>::current_thread_id();
	typename std::map<unsigned long, ValueT>::const_iterator iter = this->_tbl.find(thread_id);
	{
		boost::shared_lock<boost::shared_mutex> lock(this->_access_lock);
		if(iter!=this->_tbl.end())
			value = iter->second;
	}
}

template<typename ValueT> 
unsigned long chained_call_context_tbl<ValueT>::current_thread_id()
{
    std::string thread_id = boost::lexical_cast<std::string>(boost::this_thread::get_id());
    unsigned long thread_number = 0;
    sscanf(thread_id.c_str(), "%lx", &thread_number);
    return thread_number;
}

#endif

