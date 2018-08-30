#include "echo_pair.h"
#include "event_loop.h"

#define ENABLE_LOG
#undef ENABLE_LOG

#if defined(ENABLE_LOG)
#define LOG(...) fprintf(stdout, __VA_ARGS__)
#else
#define LOG(...)
#endif

async_context::async_context(boost::shared_ptr<echo_pair> parent, boost::asio::ip::udp::socket &s) : 
	m_parent(parent), 
	m_sock(s)
{
}

/***********************
 * async_write_context
 ***********************/
async_write_context::async_write_context(
	boost::shared_ptr<echo_pair> parent, boost::asio::ip::udp::socket &s, 
	const boost::asio::ip::udp::endpoint &ep) : 
	async_context(parent, s)
{
	this->m_remote_ep = ep;
}

void async_write_context::on_written_cb(const boost::system::error_code &ec, std::size_t bytes_transferred)
{
	if(ec)
		throw boost::system::system_error(ec);

	m_buf.consume(bytes_transferred);
	LOG("%s : %d bytes written\n", __FUNCTION__, bytes_transferred);
	
	boost::shared_ptr<echo_pair> parent = m_parent.lock();
	if(parent.get())
	{
		if(m_buf.size()>0)
		{
			std::string c(boost::asio::buffers_begin(m_buf.data()), boost::asio::buffers_end(m_buf.data())); 
			m_buf.consume(m_buf.size());
			LOG("%s : msg \"%s\" not written, continue to send\n", __FUNCTION__, c.c_str());
			parent->try_write_once(m_sock, c, m_remote_ep);
		}else
		{
			LOG("%s : all msg written, try to read more\n", __FUNCTION__);
			parent->try_read_once(m_sock);
		}
	}
}

/***********************
 * async_read_context
 ***********************/
async_read_context::async_read_context(boost::shared_ptr<echo_pair> parent, boost::asio::ip::udp::socket &s) : 
	async_context(parent, s)
{
}

void async_read_context::on_readed_cb(const boost::system::error_code &ec, std::size_t bytes_transferred)
{
	if(ec)
		throw boost::system::system_error(ec);
	
	m_buf.commit(bytes_transferred);
	std::string received_s(boost::asio::buffers_begin(m_buf.data()), boost::asio::buffers_end(m_buf.data())); 
	m_buf.consume(bytes_transferred);
	LOG("%s : %d bytes readed, msg \"%s\"\n", __FUNCTION__, bytes_transferred, received_s.c_str());

	boost::shared_ptr<echo_pair> parent = m_parent.lock();
	if(parent.get())
	{
		std::string sended_s;
		parent->m_custom_readed_cb(received_s, sended_s);
		if(!sended_s.empty())
		{
			LOG("%s : msg complete, msg \"%s\" replied\n", __FUNCTION__, sended_s.c_str());
			parent->try_write_once(m_sock, sended_s, this->m_remote_ep);
		}
		else
		{
			parent->try_read_once(m_sock);
			LOG("%s : msg not complete, continue to read\n", __FUNCTION__);
		}
	}
}

/************
 * echo_pair
 ***********/
echo_pair::echo_pair(echo_pair::custom_readed_cb_decl custom_readed_cb) : 
	m_custom_readed_cb(custom_readed_cb)
{
}

void echo_pair::try_read_once(boost::asio::ip::udp::socket &s)
{
	boost::shared_ptr<async_read_context> context(new async_read_context(shared_from_this(), s));
	
	s.async_receive_from(
		context->m_buf.prepare(4096), 
		context->m_remote_ep, 
		boost::bind(&async_read_context::on_readed_cb, context, _1, _2)
	);
}

void echo_pair::try_write_once(boost::asio::ip::udp::socket &s, const std::string &c, boost::asio::ip::udp::endpoint &ep)
{
	boost::shared_ptr<async_write_context> context(new async_write_context(shared_from_this(), s, ep));

	std::ostream os(&context->m_buf);
	os<<c;
	s.async_send_to(
		context->m_buf.data(), 
		context->m_remote_ep, 
		boost::bind(&async_write_context::on_written_cb, context, _1, _2)
	);
}

/*****************
 * echo_srv/client
 ****************/
echo_srv::echo_srv(boost::shared_ptr<event_loop> ev, echo_pair::custom_readed_cb_decl custom_readed_cb, int port) : 
	echo_pair(custom_readed_cb), 
	m_sock(ev->get_io_service(), boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::from_string("127.0.0.1"), port))
{
}

void echo_srv::start()
{
	this->try_read_once(m_sock);
}

echo_client::echo_client(boost::shared_ptr<event_loop> ev, echo_pair::custom_readed_cb_decl custom_readed_cb, int port) : 
	echo_pair(custom_readed_cb), 
	m_sock(ev->get_io_service(), boost::asio::ip::udp::v4()), 
	m_srv_ep(boost::asio::ip::address_v4::from_string("127.0.0.1"), port)
{
}


void echo_client::try_write_once(const std::string &c)
{
	echo_pair::try_write_once(m_sock, c, m_srv_ep);
}

#ifdef ECHO_SRV_TEST
#include "echo_pair.h"
#include "event_loop.h"
#include "echo_pair_util.h"
class OnSrvReaded : public on_echo_pair_readed
{
public:
	OnSrvReaded(boost::shared_ptr<event_loop> parent) : on_echo_pair_readed(parent){}

	virtual void frame_received(const std::string &frame, std::ostringstream &s)
	{
		printf("recv frame : %s\n", frame.c_str());
		s<<"ok";
	}
};
int main()
{
	boost::shared_ptr<event_loop> ev(new event_loop());
	boost::shared_ptr<OnSrvReaded> cb(new OnSrvReaded(ev));
	boost::shared_ptr<echo_srv> srv(new echo_srv(ev, cb->create_binded_cb(), 12345));
	srv->start();
	ev->exec();

	return 0;
}
#endif

#ifdef ECHO_CLIENT_TEST
#include "event_loop.h"
#include "echo_pair.h"
#include "echo_pair_util.h"
class OnClientReaded : public on_echo_pair_readed
{
public:
	OnClientReaded(boost::shared_ptr<event_loop> parent):on_echo_pair_readed(parent){}
	
	std::string sendedContent()
	{
		char sep = this->frame_seperator();
		static int idx = 0;
		static int limit = 10;
		std::ostringstream s;
		if(idx++<limit)
			s<<idx;
		return s.str();
	}

	virtual void frame_received(const std::string &frame, std::ostringstream &s)
	{
		printf("recv frame : %s\n", frame.c_str());
		std::string c = sendedContent();
		s<<c;
		
		if(c.empty())
			this->delete_later();
	}
};

int main()
{
	boost::shared_ptr<event_loop> ev(new event_loop());
	boost::shared_ptr<OnClientReaded> cb(new OnClientReaded(ev));
	boost::shared_ptr<echo_client> client(new echo_client(ev, cb->create_binded_cb(), 12345));

	std::ostringstream first_sended;
	first_sended<<cb->sendedContent()<<cb->frame_seperator();
	client->try_write_once(first_sended.str());
	ev->exec();

	return 0;
}
#endif

