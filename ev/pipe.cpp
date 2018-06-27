#include "pipe.h"
#include "event_loop.h"

simplex_pipe::simplex_pipe(event_loop &ev_with_ep_in, event_loop &ev_with_ep_out) : 
	m_srv(ev_with_ep_out.get_io_service(), boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::from_string("127.0.0.1"), 0)), 
	m_client(ev_with_ep_in.get_io_service(), boost::asio::ip::udp::v4()) 
{
	/*
	 * 本地的udp对模拟管道,为保证语义的简单,
	 * 初始化不使用异步方式.
	 */
	boost::asio::ip::udp::endpoint ep = m_srv.local_endpoint();
	m_client.connect(ep);
}

simplex_pipe::~simplex_pipe()
{
	if(m_client.is_open())
		m_client.close();
	if(m_srv.is_open())
		m_srv.close();
}

/* 从输出端读取 */
size_t simplex_pipe::read_from_ep_out(char *buf, int buf_size)
{
	return m_srv.receive(boost::asio::buffer(buf, buf_size));
}

/* 从输入端写入 */
size_t simplex_pipe::write_from_ep_in(char *buf, int buf_size)
{
	return m_client.send(boost::asio::buffer(buf, buf_size));
}

