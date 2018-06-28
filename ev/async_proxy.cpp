#include <boost/thread.hpp>
#include "event_loop.h"
#include "async_proxy.h"

#define ENABLE_LOG
#undef ENABLE_LOG

#if defined(ENABLE_LOG)
#define LOG(format, ...) fprintf(stdout, format, __VA_ARGS__)
#else
#define LOG(format, ...)
#endif

#define CLASS_NAME async_proxy

#define MEMBER_FUNC_WITH_NO_ARG(func) \
	boost::bind(&CLASS_NAME::func, this)
#define MEMBER_FUNC_WITH_ONE_ARG(func, arg1) \
	boost::bind(&CLASS_NAME::func, this, arg1)
#define MEMBER_FUNC_WITH_TWO_ARG(func, arg1, arg2) \
	boost::bind(&CLASS_NAME::func, this, arg1, arg2)
#define MEMBER_FUNC_WITH_THREE_ARG(func, arg1, arg2, arg3) \
	boost::bind(&CLASS_NAME::func, this, arg1, arg2, arg3)

async_proxy::async_proxy(event_loop &ev, int thread_num):
	m_external_ev(ev), 
	m_internal_ev(thread_num), 
	req_pipe(ev, m_internal_ev), m_reqs_pended(0), 
	ack_pipe(m_internal_ev, ev), m_acks_pended(0)
{
	LOG("proxy init\n");
}

async_proxy::~async_proxy()
{
	LOG("proxy deinit\n");
	// 停止内部事件循环
	if(!this->stopped())
		this->stop();
	
	// 清空缓存
	{
		boost::unique_lock<boost::mutex> lock(m_reqs_buf_mutex);
		while(!m_reqs_buf.empty())
			m_reqs_buf.pop();
	}
	{
		boost::unique_lock<boost::mutex> lock(m_acks_buf_mutex);
		while(!m_acks_buf.empty())
			m_acks_buf.pop();
	}
}

// 启动该异步代理
void async_proxy::start()
{
	// 启动内部事件循环
	req_pipe.async_read_from_ep_out(m_dummy_char, 1, MEMBER_FUNC_WITH_TWO_ARG(req_pipe_readed_cb, _1, _2));
	boost::shared_ptr<boost::thread> th(new boost::thread(boost::bind(&event_loop::exec, &m_internal_ev)));
	m_internal_th = th;
		
	// 启动外部事件循环
	ack_pipe.async_read_from_ep_out(m_dummy_char, 1, MEMBER_FUNC_WITH_TWO_ARG(ack_pipe_readed_cb, _1, _2));
}

void async_proxy::enable_auto_balance()
{
	m_internal_ev.enable_auto_balance();
}

// 停止该异步代理
void async_proxy::stop()
{
	m_internal_ev.terminate();
	if(m_internal_th.get())
	{
		if(m_internal_th->joinable())
			m_internal_th->join();
		m_internal_th.reset();
	}
}

bool async_proxy::stopped() const
{
	return m_internal_ev.stopped();
}

// 提交请求
void async_proxy::perform(async_work work)
{
	boost::shared_ptr<async_work> work_copy(new async_work(work));
	this->perform(work_copy);
}

void async_proxy::perform(async_work &work)
{
	boost::shared_ptr<async_work> work_copy(new async_work(work));
	this->perform(work_copy);
}

void async_proxy::perform(boost::shared_ptr<async_work> work)
{
	req_pipe.async_write_from_ep_in(m_dummy_char, 1, MEMBER_FUNC_WITH_THREE_ARG(req_pipe_written_cb, _1, _2, work));
}

/* 
 * 请求/应答管道的读写回调
 */
void async_proxy::req_pipe_written_cb(const boost::system::error_code &ec, std::size_t transfered, 
	boost::shared_ptr<async_work> work)
{
	if(ec){
		fprintf(stderr, "%s : %s\n", __FUNCTION__, ec.message().c_str());
		throw boost::system::system_error(ec);
	}
	LOG("%s : %d bytes written.\n", __FUNCTION__, transfered);

	bool need_notify = false;
	{
		boost::unique_lock<boost::mutex> lock(m_reqs_buf_mutex);
		if(m_reqs_pended>0){
			--m_reqs_pended;
			need_notify = true;
		}
		m_reqs_buf.push(work);
	}
	if(need_notify)
		m_reqs_buf_wc.notify_one();
}

void async_proxy::req_pipe_readed_cb(const boost::system::error_code &ec, std::size_t transfered)
{
	if(ec){
		fprintf(stderr, "%s : %s\n", __FUNCTION__, ec.message().c_str());
		if(m_internal_ev.stopped())
			return;
		throw boost::system::system_error(ec);
	}
	LOG("%s : %d bytes readed.\n", __FUNCTION__, transfered);

	// 启动新一次的请求读取
	req_pipe.async_read_from_ep_out(m_dummy_char,1,MEMBER_FUNC_WITH_TWO_ARG(req_pipe_readed_cb, _1, _2));
	
	// 获取请求并执行
	boost::shared_ptr<async_work> work;
	{
		boost::unique_lock<boost::mutex> lock(m_reqs_buf_mutex);
		while(m_reqs_buf.empty()){
			++m_reqs_pended;
			m_reqs_buf_wc.wait(lock);
		}
		work = m_reqs_buf.front();
		m_reqs_buf.pop();	
	}
	if(work.get())
		work->first();
	ack_pipe.async_write_from_ep_in(m_dummy_char, 1, MEMBER_FUNC_WITH_THREE_ARG(ack_pipe_written_cb, _1, _2, work));
}

void async_proxy::ack_pipe_written_cb(
	const boost::system::error_code &ec, std::size_t transfered, 
	boost::shared_ptr<async_work> work)
{
	if(ec){
		fprintf(stderr, "%s : %s\n", __FUNCTION__, ec.message().c_str());
		if(m_internal_ev.stopped())
			return;
		throw boost::system::system_error(ec);
	}
	LOG("%s : %d bytes written.\n", __FUNCTION__, transfered);
	
	bool need_notify = false;
	{
		boost::unique_lock<boost::mutex> lock(m_acks_buf_mutex);
		if(m_acks_pended>0){
			--m_acks_pended;
			need_notify = true;
		}
		m_acks_buf.push(work);
	}
	if(need_notify)
		m_acks_buf_wc.notify_one();
}

void async_proxy::ack_pipe_readed_cb(
	const boost::system::error_code &ec, std::size_t transfered)
{
	if(ec){
		fprintf(stderr, "%s : %s\n", __FUNCTION__, ec.message().c_str());
		throw boost::system::system_error(ec);
	}
	LOG("%s : %d bytes readed.\n", __FUNCTION__, transfered);

	// 启动新一次的反馈读取
	if(!m_internal_ev.stopped())
		ack_pipe.async_read_from_ep_out(m_dummy_char,1,MEMBER_FUNC_WITH_TWO_ARG(ack_pipe_readed_cb, _1, _2));
	
	// 获取反馈并执行
	boost::shared_ptr<async_work> work;
	{
		boost::unique_lock<boost::mutex> lock(m_acks_buf_mutex);
		while(m_acks_buf.empty()){
			++m_acks_pended;
			m_acks_buf_wc.wait(lock);
		}
		work = m_acks_buf.front();
		m_acks_buf.pop();
	}
	if(work.get())
		work->second();
}

#ifdef ASYNC_PROXY_UNIT_TEST
#include <boost/asio/steady_timer.hpp>
#include <boost/chrono.hpp>

static void start_timer(boost::asio::io_service &service, boost::shared_ptr<async_proxy> proxy);

class work : public boost::enable_shared_from_this<work>
{
public:
	work(int n):_n(n){
		printf("%s : %d\n", __FUNCTION__, _n);
	}
	~work(){
		printf("%s : %d\n", __FUNCTION__, _n);
	}
	void do_work(){
		printf("%s : here is work begin, %ud, %d\n", __FUNCTION__, pthread_self(), _n);
		usleep(rand()%10+1);
		printf("%s : here is work end, %ud, %d\n", __FUNCTION__, pthread_self(), _n);
	}
	void do_reply(){
		printf("%s : here is reply, %ud, %d\n", __FUNCTION__, pthread_self(), _n);
	}
private:
	const int _n;
};

static void timer_cb(
	const boost::system::error_code &ec, 
	boost::shared_ptr<boost::asio::steady_timer> t, 
	boost::shared_ptr<async_proxy> proxy)
{
	static int count = 50;
	LOG("enter\n");
	if(ec)
		throw boost::system::system_error(ec);
	
	if(count--){
		//printf("timer timeout\n");
		start_timer(t->get_io_service(), proxy);
		
		int limit = rand()%300+1;
		LOG("%d with %d\n", limit, count+1);
		for(int i=0; i<limit; ++i)
		{
			boost::shared_ptr<work> new_work(new work(count+1));
			proxy->perform(async_proxy::async_work(boost::bind(&work::do_work, new_work), boost::bind(&work::do_reply, new_work)));
		}
	}else{
		proxy->stop();
		t->get_io_service().stop();
	}
}

static void start_timer(boost::asio::io_service &service, boost::shared_ptr<async_proxy> proxy)
{
	boost::shared_ptr<boost::asio::steady_timer> timer(new boost::asio::steady_timer(service));
	timer->expires_from_now(boost::chrono::microseconds(200000));
	timer->async_wait(boost::bind(timer_cb, _1, timer, proxy));
}

#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
int main()
{
	event_loop ev;
	boost::shared_ptr<async_proxy> proxy(new async_proxy(ev, 100));
	proxy->enable_auto_balance();
	proxy->start();

	start_timer(ev.get_io_service(), proxy);
	printf("%s : tid %ud\n", __FUNCTION__, pthread_self());

	return ev.exec();
}
#endif

