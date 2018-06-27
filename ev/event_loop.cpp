#include <iostream>
#include <sstream>
#include "event_loop.h"
#include "define.h"

#define ENABLE_LOG
#undef ENABLE_LOG

#if defined(ENABLE_LOG)
#define LOG(format, ...) fprintf(stdout, format, __VA_ARGS__)
#else
#define LOG(format, ...)
#endif

event_loop::event_loop(int thread_num) : 
	m_thread_num(thread_num), 
	m_auto_balance(false)
{
	if(m_thread_num<=0){
		std::stringstream stream;
		stream<<__FUNCTION__<<" : "<<"negative thread num."<<std::endl;
		throw std::runtime_error(stream.str());
	}

	LOG("event_loop init\n");
}

event_loop::~event_loop()
{
	if(!this->stopped())
		this->terminate();
	
	LOG("event_loop deinit\n");
}

void event_loop::enable_auto_balance()
{
	m_auto_balance = true;
}

int event_loop::exec()
{
	try{
		for(int i=0; i<m_thread_num; ++i)
			m_thread_grp.create_thread(
				boost::bind(&event_loop::th_func, this, i)
			);
		m_thread_grp.join_all();
	}
	catch(std::exception &e){
		std::cout<<e.what()<<std::endl;
		return -1;
	}

	return 0;
}

static void enable_auto_balance_wrapper(int thread_idx);
void event_loop::th_func(int thread_idx)
{
	if(m_auto_balance)
		enable_auto_balance_wrapper(thread_idx);
	
	m_io_service.run();
}

boost::asio::io_service& event_loop::get_io_service()
{
	return m_io_service;
}

void event_loop::terminate()
{
	m_io_service.stop();
}

bool event_loop::stopped() const
{
	return m_io_service.stopped();
}

/* 工作线程平均分布实现 */
static int core_idx(int thread_idx, int core_num)
{
	return thread_idx%core_num;
}

#if defined(_WIN32) || defined(_WIN64)
#include <WinBase.h>
static void enable_auto_balance_on_windows(int thread_idx)
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	int cpu_core_idx = core_idx(thread_idx, info.dwNumberOfProcessors);
	SetThreadAffinityMask(GetCurrentThread(), 1<<cpu_core_idx);
}
#endif

#ifdef __linux__
#include <unistd.h>
#include <pthread.h>
#include <sys/sysinfo.h>
static void enable_auto_balance_on_linux(int thread_idx)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(core_idx(thread_idx, get_nprocs()), &mask);
	pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);
}
#endif

static void enable_auto_balance_wrapper(int thread_idx)
{
#if defined(_WIN32) || defined(_WIN64)
	enable_auto_balance_on_windows(thread_idx);
#endif

#ifdef __linux__
	enable_auto_balance_on_linux(thread_idx);
#endif
}


#ifdef EVENT_LOOP_UNIT_TEST
#include <boost/asio/steady_timer.hpp>
#include <boost/chrono.hpp>

static void start_timer(boost::asio::io_service &serivce);

static void timer_cb(
	const boost::system::error_code &ec, 
	boost::shared_ptr<boost::asio::steady_timer> t)
{
	static int count = 5;
	if(ec)
		throw boost::system::system_error(ec);
	
	if(count--){
		printf("timer timeout\n");
		start_timer(t->get_io_service());
	}
}

static void start_timer(boost::asio::io_service &service)
{
	boost::shared_ptr<boost::asio::steady_timer> timer(new boost::asio::steady_timer(service));
	timer->expires_from_now(boost::chrono::seconds(1));
	timer->async_wait(boost::bind(timer_cb, _1, timer));
}

int main()
{
	event_loop ev;
	start_timer(ev.get_io_service());
	return ev.exec();
}
#endif

