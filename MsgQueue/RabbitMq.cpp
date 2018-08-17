#include <stdexcept>
#include "RabbitMq.h"

/************
 * rabbit_mq
 ************/
rabbit_mq::rabbit_mq() : 
	_impl(new rabbit_mq_impl())
{
}

rabbit_mq& rabbit_mq::bindEventLoop(event_loop *ev, int parallel_num)
{
	this->_impl->bindEventLoop(ev, parallel_num);
	return *this;
}

rabbit_mq& rabbit_mq::setBrokerHost(std::string ip, int port)
{
	this->_impl->setBrokerHost(ip, port);
	return *this;
}

rabbit_mq& rabbit_mq::setFrameMaxSize(int max_size)
{
	this->_impl->setFrameMaxSize(max_size);
	return *this;
}

rabbit_mq& rabbit_mq::setConnectTimeout(int millisec)
{
	this->_impl->setConnectTimeout(millisec);
	return *this;
}

rabbit_mq& rabbit_mq::setVirtualHost(std::string vhost)
{
	this->_impl->setVirtualHost(vhost);
	return *this;
}

rabbit_mq& rabbit_mq::setUserName(std::string username)
{
	this->_impl->setUserName(username);
	return *this;
}

rabbit_mq& rabbit_mq::setPassword(std::string password)
{
	this->_impl->setPassword(password);
	return *this;
}

rabbit_mq& rabbit_mq::setChannelMaxNum(int max_num)
{
	this->_impl->setChannelMaxNum(max_num);
	return *this;
}

rabbit_mq& rabbit_mq::withExchange(std::string exchange)
{
	this->_impl->withExchange(exchange);
	return *this;
}

rabbit_mq& rabbit_mq::withRoutingKey(std::string routing_key)
{
	this->_impl->withRoutingKey(routing_key);
	return *this;
}

rabbit_mq& rabbit_mq::withQueue(std::string queue)
{
	this->_impl->withQueue(queue);
	return *this;
}

rabbit_mq& rabbit_mq::update()
{
	this->_impl->update();
	return *this;
}

void rabbit_mq::shutdown_later()
{
	this->_impl->shutdown_later();
}


#ifdef RABBIT_MSG_QUEUE_RECV_TEST

#endif

#ifdef RABBIT_MSG_QUEUE_SEND_TEST
#include "MsgQueueTerminal.h"
#include "RabbitMq.h"
#include <boost/system/error_code.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/asio/steady_timer.hpp>
#include "../ev/event_loop.h"

struct custom_logic : public boost::enable_shared_from_this<custom_logic>
{
	custom_logic(event_loop *ev, boost::shared_ptr<msg_queue_terminal<rabbit_mq> > terminal) : 
		_ev(ev), _terminal(terminal), 
		_limit(10)
	{
	}

	void writeHandler(const boost::system::error_code &ec, std::size_t bytes_transferred)
	{
		std::cout<<"custom_logic writeHandler : "<<pthread_self()<<std::endl;
		if(ec)
			throw boost::system::system_error(ec);

		_buf.consume(bytes_transferred);
		fprintf(stdout, "%s : %d bytes written\n", __FUNCTION__, bytes_transferred);
		if(!this->send())
			_terminal->shutdown_later(boost::bind(&custom_logic::shutdownHandler, shared_from_this()));
	}

	void shutdownHandler()
	{
		// 结束
		std::cout<<"already closed, exit ev."<<std::endl;
		this->_ev->terminate();
	}

	bool send()
	{
		const int last = 0;
		if(_limit--<=last)
			return false;
		bool this_is_last = false;
		if(_limit==last)
			this_is_last = true;

		boost::asio::streambuf &buf = this->_buf;
		std::ostream os(&buf);
		if(!this_is_last)
			os<<_limit<<std::endl;
		else
			os<<"exit";
		(*_terminal).withExchange("test.topic")
			.withRoutingKey("test.123");
		_terminal->async_write_some(
			buf.data(), 
			boost::bind(&custom_logic::writeHandler, shared_from_this(), _1, _2)
		);
		return true;
	}

	event_loop *_ev;
	boost::shared_ptr<msg_queue_terminal<rabbit_mq> > _terminal;
	boost::asio::streambuf _buf;
	int _limit;
};

int main()
{
	event_loop ev;
	boost::shared_ptr<msg_queue_terminal<rabbit_mq> > terminal = msg_queue_terminal<rabbit_mq>::build();
	(*terminal).bindEventLoop(&ev, 1)
		.setBrokerHost("127.0.0.1")
		.setConnectTimeout(1000)
		.setVirtualHost("/")
		.setUserName("guest")
		.setPassword("guest")
		.setChannelMaxNum(10);

	boost::shared_ptr<custom_logic> logic = boost::make_shared<custom_logic>(&ev, terminal);

	boost::shared_ptr<boost::asio::steady_timer> timer(new boost::asio::steady_timer(ev.get_io_service()));
	timer->expires_from_now(boost::chrono::microseconds(0));
	timer->async_wait(boost::bind(&custom_logic::send, logic));

	std::cout<<"main : "<<pthread_self()<<std::endl;
	return ev.exec();
}
#endif
