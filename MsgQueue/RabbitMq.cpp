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

std::string rabbit_mq::getBrokerHostIp() const
{
	return this->_impl->getBrokerHostIp();
}

int rabbit_mq::getBrokerHostPort() const
{
	return this->_impl->getBrokerHostPort();
}

rabbit_mq& rabbit_mq::setBrokerHost(std::string ip, int port)
{
	this->_impl->setBrokerHost(ip, port);
	return *this;
}

int rabbit_mq::getFrameMaxSize() const
{
	return this->_impl->getFrameMaxSize();
}

rabbit_mq& rabbit_mq::setFrameMaxSize(int max_size)
{
	this->_impl->setFrameMaxSize(max_size);
	return *this;
}

int rabbit_mq::getConnectTimeout() const
{
	return this->_impl->getConnectTimeout();
}

rabbit_mq& rabbit_mq::setConnectTimeout(int millisec)
{
	this->_impl->setConnectTimeout(millisec);
	return *this;
}

std::string rabbit_mq::getVirtualHost() const
{
	return this->_impl->getVirtualHost();
}

rabbit_mq& rabbit_mq::setVirtualHost(std::string vhost)
{
	this->_impl->setVirtualHost(vhost);
	return *this;
}

std::string rabbit_mq::getUserName() const
{
	return this->_impl->getUserName();
}

rabbit_mq& rabbit_mq::setUserName(std::string username)
{
	this->_impl->setUserName(username);
	return *this;
}

std::string rabbit_mq::getPassword() const
{
	return this->_impl->getPassword();
}

rabbit_mq& rabbit_mq::setPassword(std::string password)
{
	this->_impl->setPassword(password);
	return *this;
}

int rabbit_mq::getChannelMaxNum() const
{
	return this->_impl->getChannelMaxNum();
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

#if defined(RABBIT_MSG_QUEUE_RECV_TEST) || defined(RABBIT_MSG_QUEUE_SEND_TEST)
#include "MsgQueueTerminal.h"
#include "RabbitMq.h"
#include <boost/system/error_code.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/asio/steady_timer.hpp>
#include "../ev/event_loop.h"

struct custom_logic : public boost::enable_shared_from_this<custom_logic>
{
	struct recv_context
	{
		recv_context(boost::shared_ptr<custom_logic> parent) : _parent(parent){}
		void onReaded(const boost::system::error_code &ec, std::size_t bytes_transferred)
		{
			std::cout<<"custom_logic::send_context::onReaded : "<<pthread_self()<<std::endl;
			if(ec)
				throw boost::system::system_error(ec);

			_buf.commit(bytes_transferred);
			std::string received_s(
				boost::asio::buffers_begin(_buf.data()), 
				boost::asio::buffers_end(_buf.data())
			); 
			_buf.consume(bytes_transferred);
			fprintf(stdout, "%s : %d bytes readed : %s\n", 
				__FUNCTION__, bytes_transferred, received_s.c_str()
			);

			_parent->recv();
		}
		boost::shared_ptr<custom_logic> _parent;
		boost::asio::streambuf _buf;
	};

	struct send_context
	{
		send_context(boost::shared_ptr<custom_logic> parent) : _parent(parent){}
		void onWritten(const boost::system::error_code &ec, std::size_t bytes_transferred)
		{
			std::cout<<"custom_logic::send_context::onWritten : "<<pthread_self()<<std::endl;
			if(ec)
				throw boost::system::system_error(ec);

			_buf.consume(bytes_transferred);
			fprintf(stdout, "%s : %d bytes written\n", __FUNCTION__, bytes_transferred);
			if(!_parent->send())
				_parent->_terminal->shutdown_later(
					boost::bind(&custom_logic::shutdownHandler, _parent)
				);
		}
		boost::shared_ptr<custom_logic> _parent;
		boost::asio::streambuf _buf;
	};

	custom_logic(event_loop *ev, boost::shared_ptr<msg_queue_terminal<rabbit_mq> > terminal) : 
		_ev(ev), _terminal(terminal), 
		_send_limit(50)
	{
	}

	void shutdownHandler()
	{
		// 结束
		std::cout<<"already closed, exit ev."<<std::endl;
		this->_ev->terminate();
	}

	bool send()
	{
		printf("\nsend once!!!\n\n");
		const int last = 0;
		for(size_t i=0; i<10; ++i)
		{
			if(_send_limit--<=last)
				return false;
			bool this_is_last = false;
			if(_send_limit==last)
				this_is_last = true;

			boost::shared_ptr<send_context> context(new send_context(shared_from_this()));
			boost::asio::streambuf &buf = context->_buf;
			std::ostream os(&buf);
			if(!this_is_last)
				os<<_send_limit<<std::endl;
			else
				os<<"exit";
			(*_terminal).withExchange("test.topic")
				.withRoutingKey("test.123");
			_terminal->async_write_some(
				buf.data(), 
				boost::bind(&send_context::onWritten, context, _1, _2)
			);
			//return true;
		}
		if(_send_limit<=last)
			return false;
		else
			return true;
	}

	void recv()
	{
		boost::shared_ptr<recv_context> context(new recv_context(shared_from_this()));
		boost::asio::streambuf &buf = context->_buf;
		(*_terminal).withQueue("test.queue1");
		_terminal->async_read_some(
			buf.prepare(_terminal->getFrameMaxSize()), 
			boost::bind(&recv_context::onReaded, context, _1, _2)
		);
	}

	event_loop *_ev;
	boost::shared_ptr<msg_queue_terminal<rabbit_mq> > _terminal;
	int _send_limit;
};
#endif

#ifdef RABBIT_MSG_QUEUE_RECV_TEST
int main()
{
	event_loop ev;
	boost::shared_ptr<msg_queue_terminal<rabbit_mq> > terminal = msg_queue_terminal<rabbit_mq>::build();
	(*terminal).bindEventLoop(&ev, 10)
		.setBrokerHost("127.0.0.1")
		.setConnectTimeout(1000)
		.setVirtualHost("/")
		.setUserName("guest")
		.setPassword("guest")
		.setChannelMaxNum(10);

	boost::shared_ptr<custom_logic> logic = boost::make_shared<custom_logic>(&ev, terminal);

	boost::shared_ptr<boost::asio::steady_timer> timer(new boost::asio::steady_timer(ev.get_io_service()));
	timer->expires_from_now(boost::chrono::microseconds(0));
	timer->async_wait(boost::bind(&custom_logic::recv, logic));

	std::cout<<"main : "<<pthread_self()<<std::endl;
	return ev.exec();
}
#endif

#ifdef RABBIT_MSG_QUEUE_SEND_TEST
int main()
{
	event_loop ev;
	boost::shared_ptr<msg_queue_terminal<rabbit_mq> > terminal = msg_queue_terminal<rabbit_mq>::build();
	(*terminal).bindEventLoop(&ev, 10)
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
