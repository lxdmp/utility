/*
 * a adapter to adapt rabbitmq-c to ev (wrapper of boost.asio).
 */
#ifndef _RABBITMQ_TERMINAL_H_
#define _RABBITMQ_TERMINAL_H_

#include <iostream>
#include <boost/shared_ptr.hpp>
#include <amqp.h>
#include "../ev/event_loop.h"
#include "../ev/async_proxy.h"

class rabbit_mq_terminal_channel;
class rabbit_mq_impl;

class rabbit_mq
{
public:
	rabbit_mq();
	rabbit_mq& bindEventLoop(event_loop *ev, int parallel_num=0);

	std::string getBrokerHostIp() const;
	int getBrokerHostPort() const;
	int getFrameMaxSize() const;
	rabbit_mq& setBrokerHost(std::string ip, int port=0);
	rabbit_mq& setFrameMaxSize(int max_size);
	int getConnectTimeout() const;
	rabbit_mq& setConnectTimeout(int millisec);
	std::string getVirtualHost() const;
	rabbit_mq& setVirtualHost(std::string vhost);
	std::string getUserName() const;
	rabbit_mq& setUserName(std::string username);
	std::string getPassword() const;
	rabbit_mq& setPassword(std::string password);
	int getChannelMaxNum() const;
	rabbit_mq& setChannelMaxNum(int max_num);

	rabbit_mq& withExchange(std::string exchange);
	rabbit_mq& withRoutingKey(std::string routing_key);
	rabbit_mq& withQueue(std::string queue);

	rabbit_mq& update();

public:
	template<typename MutableBufferSequenceT, typename ReadHandlerT> 
	void async_read_some_impl(const MutableBufferSequenceT &buffer, ReadHandlerT handler);

	template<typename ConstBufferSequenceT, typename WriteHandlerT> 
	void async_write_some_impl(const ConstBufferSequenceT &buffer, WriteHandlerT handler);

	template<typename ShutdownHandlerT> 
	void shutdown_later(ShutdownHandlerT complete_handler);

	void shutdown_later();

private:
	boost::shared_ptr<rabbit_mq_impl> _impl;
};

#include "RabbitMqImpl.h"

#endif

