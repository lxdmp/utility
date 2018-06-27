/*
 * 实现一条"(单工)管道"
 */
#ifndef _PIPE_H_
#define _PIPE_H_

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
class event_loop;

/* 
 * (单工)管道
 *
 * 创建时需指定输入、输出端口所在的事件循环.
 */
class simplex_pipe : public boost::noncopyable
{
public:
	simplex_pipe(event_loop &ev_with_ep_in, event_loop &ev_with_ep_out);
	~simplex_pipe();

	/* 
	 * 从输出端读取 
	 */
	size_t read_from_ep_out(char *buf, int buf_size);
	template<typename ReadHandler>
		void async_read_from_ep_out(char *buf, int buf_size, ReadHandler handler);

	/* 
	 * 从输入端写入 
	 */
	size_t write_from_ep_in(char *buf, int buf_size);
	template<typename WriteHandler>
		void async_write_from_ep_in(char *buf, int buf_size, WriteHandler handler);

private:
	boost::asio::ip::udp::socket m_srv; /* 输出端 */
	boost::asio::ip::udp::socket m_client;  /* 输入端 */
};

/***********
 * 模板实现 
 ***********/
template <typename ReadHandler>
void simplex_pipe::async_read_from_ep_out(char *buf, int buf_size, ReadHandler handler)
{
	m_srv.async_receive(boost::asio::buffer(buf, buf_size), handler);
}

template<typename WriteHandler>
void simplex_pipe::async_write_from_ep_in(char *buf, int buf_size, WriteHandler handler)
{
	m_client.async_send(boost::asio::buffer(buf, buf_size), handler);
}

#endif

