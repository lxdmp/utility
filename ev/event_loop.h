#ifndef _IO_SERVICE_H_
#define _IO_SERVICE_H_

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>

class event_loop : public boost::noncopyable
{
public:
	event_loop(int thread_num=1);
	~event_loop();
	
	int exec();
	void terminate();
	bool stopped() const;
	
	boost::asio::io_service& get_io_service();
	inline const int get_thread_num() const {return m_thread_num;}

	void enable_auto_balance();
	
private:
	void th_func(int);

	boost::asio::io_service m_io_service;
	boost::thread_group m_thread_grp;
	const int m_thread_num;
	bool m_auto_balance;
};

#endif

