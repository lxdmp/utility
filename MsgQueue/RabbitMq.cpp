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

#ifdef RABBIT_MSG_QUEUE_TEST
#include "MsgQueueTerminal.h"
#include "RabbitMq.h"
#include <boost/system/error_code.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include "../ev/event_loop.h"

struct custom_logic
{
	custom_logic(event_loop *ev) : _ev(ev){}

	void connectHandler(const boost::system::error_code &ec)
	{
		// 投递消息
	}

	void writeHandler()
	{
	}

	void shutdownHandler()
	{
		// 结束
	}

	event_loop *_ev;
};

int main()
{
	event_loop ev;
	boost::shared_ptr<msg_queue_terminal<rabbit_mq> > terminal = msg_queue_terminal<rabbit_mq>::build();
	(*terminal).bindEventLoop(&ev)
		.test();
	boost::shared_ptr<custom_logic> logic = boost::make_shared<custom_logic>(&ev);

	// 连接
	//terminal->async_connect("127.0.0.1", 5672, 100, boost::bind(&custom_logic::connectHandler, logic, _1));

	return ev.exec();
}
#endif
