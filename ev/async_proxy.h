/*
 * 一套"同步模拟异步"的实现(针对于例如db访问等原生阻塞的接口).
 * 将其与某事件循环(请求服务的事件循环)绑定,操作的前后部均在该事件循环中进行,阻塞操作(同步)部分在工作线程(由内部的事件循环提供)中进行.
 */
#ifndef _ASYNC_PROXY_H_
#define _ASYNC_PROXY_H_

#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/atomic.hpp>
#include <queue>
#include "define.h"
#include "pipe.h"

class event_loop;

/* 异步代理 */
class async_proxy : public boost::noncopyable
{
public:
	// 异步操作声明
	typedef boost::function<void(void)> async_cb_decl;
	typedef std::pair<async_cb_decl, async_cb_decl> async_work;
	
public:
	async_proxy(event_loop &ev, int thread_num=1);
	~async_proxy();
	
	void start(); // 启动该异步代理
	void stop(); // 停止该异步代理
	
	bool stopped() const;
	
	/* 提交异步请求 */
	void perform(async_work work);
	void perform(async_work &work);
	void perform(boost::shared_ptr<async_work> work);

	void enable_auto_balance();
	
private:
	boost::shared_ptr<boost::thread> m_internal_th;

	// 请求/应答管道的读写回调
	void req_pipe_written_cb(const boost::system::error_code &ec, std::size_t transfered, boost::shared_ptr<async_work> work);
	void req_pipe_readed_cb(const boost::system::error_code &ec, std::size_t transfered);
	void ack_pipe_written_cb(const boost::system::error_code &ec, std::size_t transfered, boost::shared_ptr<async_work> work);
	void ack_pipe_readed_cb(const boost::system::error_code &ec, std::size_t transfered);
	
private:
	event_loop &m_external_ev;
	event_loop m_internal_ev;

	// 请求管道
	simplex_pipe req_pipe;
	std::queue< boost::shared_ptr<async_work> > m_reqs_buf;
	boost::atomic<int> m_reqs_pended;
	boost::mutex m_reqs_buf_mutex;
	boost::condition_variable m_reqs_buf_wc; // 读回调可能先于写回调触发
	
	// 回复管道
	simplex_pipe ack_pipe;
	std::queue< boost::shared_ptr<async_work> > m_acks_buf;
	boost::atomic<int> m_acks_pended;
	boost::mutex m_acks_buf_mutex;
	boost::condition_variable m_acks_buf_wc;

	char m_dummy_char[1]; // 管道只为与事件循环组合而作异步通知用,读取缓存只需保证生命周期,内容不重要.
};

#endif

