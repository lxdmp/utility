#ifndef _ECHO_SRV_H_
#define _ECHO_SRV_H_

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

class event_loop;
class async_write_context;
class async_read_context;

class echo_pair : public boost::noncopyable, public boost::enable_shared_from_this<echo_pair>
{
public:
	typedef boost::function<void(const std::string&, std::string&)> custom_readed_cb_decl;

protected:
	echo_pair(echo_pair::custom_readed_cb_decl custom_readed_cb);
	virtual ~echo_pair(){}

	void try_read_once(boost::asio::ip::udp::socket &s);
	void try_write_once(boost::asio::ip::udp::socket &s, const std::string &c, boost::asio::ip::udp::endpoint &ep);

protected:
	echo_pair::custom_readed_cb_decl m_custom_readed_cb;
	
	friend class async_read_context;
	friend class async_write_context;
};

class echo_srv :  public echo_pair
{
public:
	echo_srv(boost::shared_ptr<event_loop> ev, echo_pair::custom_readed_cb_decl custom_readed_cb, int port=0);
	boost::asio::ip::udp::socket m_sock;

	void start();
};

class echo_client : public echo_pair
{
public:
	echo_client(boost::shared_ptr<event_loop> ev, echo_pair::custom_readed_cb_decl custom_readed_cb, int port);
	boost::asio::ip::udp::socket m_sock;
	boost::asio::ip::udp::endpoint m_srv_ep;

	void try_write_once(const std::string &c);
};

/***************************
 * async_read/write_context
 ***************************/
class async_context
{
protected:
	async_context(boost::shared_ptr<echo_pair> parent, boost::asio::ip::udp::socket &s);
	virtual ~async_context(){}

public:
	boost::weak_ptr<echo_pair> m_parent;
	boost::asio::ip::udp::socket &m_sock;
	boost::asio::ip::udp::endpoint m_remote_ep;
	boost::asio::streambuf m_buf;
};

class async_write_context : public async_context
{
public:
	async_write_context(boost::shared_ptr<echo_pair> parent, boost::asio::ip::udp::socket &s, const boost::asio::ip::udp::endpoint &ep);
	void on_written_cb(const boost::system::error_code &ec, std::size_t bytes_transferred);
};

class async_read_context : public async_context
{
public:
	async_read_context(boost::shared_ptr<echo_pair> parent, boost::asio::ip::udp::socket &s);
	void on_readed_cb(const boost::system::error_code &ec, std::size_t bytes_transferred);
};

#endif

