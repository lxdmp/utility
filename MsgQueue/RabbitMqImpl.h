/*
 * 考虑所有交互在一个connection上,每个交互声明一个独立的通道,
 * 所有操作都在后台辅助线程中进行以达到异步效果.
 */
#ifndef _RABBITMQ_TERMINAL_IMPL_H_
#define _RABBITMQ_TERMINAL_IMPL_H_

#include <boost/static_assert.hpp>
#include <boost/system/error_code.hpp>
#include <amqp_tcp_socket.h>
#include "MsgQueueTerminalUtil.h"

#define RABBIT_MQ_ENABLE_LOG
//#undef RABBIT_MQ_ENABLE_LOG

#if defined(RABBIT_MQ_ENABLE_LOG)
#include <boost/date_time/posix_time/posix_time.hpp>
#define RABBIT_MQ_LOG(...) \
	do{ \
		char buf[1024]; \
		memset(buf, 0, sizeof(buf)); \
		sprintf(buf, __VA_ARGS__); \
		boost::posix_time::ptime actual = boost::posix_time::second_clock::local_time(); \
		std::cout<<std::setfill('0') \
			<<std::setw(4)<<actual.date().year()<<"-" \
			<<std::setw(2)<<actual.date().month().as_number()<<"-" \
			<<std::setw(2)<<actual.date().day()<<" " \
			<<std::setw(2)<<actual.time_of_day().hours()<<":" \
			<<std::setw(2)<<actual.time_of_day().minutes()<<":" \
			<<std::setw(2)<<actual.time_of_day().seconds()<<" - " \
			<<__FUNCTION__<<" - "<<buf<<std::endl; \
	}while(0)
#else
#define RABBIT_MQ_LOG(...)
#endif

/*****************
 * rabbit_mq_impl
 *****************/
static const int rabbit_mq_default_parallel_num = 1;
static const int rabbit_mq_default_port = 5672;
static const int rabbit_mq_default_max_size = 128<<10; // 128kbytes
static const int rabbit_mq_default_connect_timeout_val = 10000; // 10000ms
static const char rabbit_mq_default_vhost[] = "/";
static const char rabbit_mq_default_username[] = "guest";
static const char rabbit_mq_default_password[] = "guest";
static const int rabbit_mq_default_channel_max_num = 8;

class rabbit_mq_impl : public boost::enable_shared_from_this<rabbit_mq_impl>, public boost::noncopyable
{
public:
	struct channel_t
	{
		channel_t(int idx) : 
			_chn_idx(idx), _opened(false)
		{
		}

		bool opened() const{return this->_opened;}
		bool closed() const{return !this->opened();}
		void open(){_opened=true;}
		void close(){_opened=false;}

		const int _chn_idx;
		bool _opened;
	};

public:
	rabbit_mq_impl() : 
		_stat(IDLE), 
		_ev(NULL), 
		_async_proxy_parallel_num(rabbit_mq_default_parallel_num), 
		_connection(NULL), 
		_socket(NULL), 
		_broker_port(rabbit_mq_default_port), 
		_frame_max_size(rabbit_mq_default_max_size), 
		_connect_timeout_val(rabbit_mq_default_connect_timeout_val), 
		_virtual_host(rabbit_mq_default_vhost), 
		_chn_max_num(rabbit_mq_default_channel_max_num), 
		_pended_requests_num(0)
	{
	}

	void bindEventLoop(event_loop *ev, int parallel_num)
	{
		if(this->_stat!=IDLE)
			throw std::runtime_error("event loop can not bind at this time.");
		this->_ev = ev;
		if(parallel_num>1)
			this->_async_proxy_parallel_num = parallel_num;
		else
			this->_async_proxy_parallel_num = rabbit_mq_default_parallel_num;
	}

	void setBrokerHost(const std::string &ip, int port)
	{
		_broker_ip = ip;
		if(port>0)
			_broker_port = port;
		else
			_broker_port = rabbit_mq_default_port;
	}

	void setFrameMaxSize(int max_size)
	{
		if(max_size>0)
			this->_frame_max_size = max_size;
		else
			this->_frame_max_size = rabbit_mq_default_max_size;
	}

	void setConnectTimeout(int millisec)
	{
		if(millisec>0)
			this->_connect_timeout_val = millisec;
		else
			this->_connect_timeout_val = rabbit_mq_default_connect_timeout_val;
	}
	
	void setVirtualHost(const std::string &vhost)
	{
		if(!vhost.empty())
			this->_virtual_host = vhost;
		else
			this->_virtual_host = rabbit_mq_default_vhost;
	}

	void setUserName(const std::string &username)
	{
		if(!username.empty())
			this->_username = username;
		else
			this->_username = rabbit_mq_default_username;
	}

	void setPassword(const std::string &password)
	{
		if(!password.empty())
			this->_password = password;
		else
			this->_password = rabbit_mq_default_password;
	}
	
	void setChannelMaxNum(int max_num)
	{
		if(max_num>0)
			this->_chn_max_num = max_num;
		else
			this->_chn_max_num = rabbit_mq_default_channel_max_num;
	}

	void withExchange(const std::string &exchange)
	{
		this->_with_exchange_tbl.put(exchange);
	}

	void withRoutingKey(const std::string &routing_key)
	{
		this->_with_routing_key_tbl.put(routing_key);
	}

	void withQueue(const std::string &queue)
	{
		this->_with_queue_tbl.put(queue);
	}
	
	void update()
	{
		this->schedule_once();
	}

public:
	template<typename MutableBufferSequenceT, typename ReadHandlerT> 
	void async_read_some_impl(const MutableBufferSequenceT &buffer, ReadHandlerT handler)
	{
		typedef rabbit_mq_impl::read_context_t<MutableBufferSequenceT, ReadHandlerT> read_context_t;
		bool need_buffer_req = true;

		if(this->_stat==rabbit_mq_impl::LOGGED_IN)
		{
			boost::shared_ptr<channel_t> used_channel = this->apply_for_channel();
			if(used_channel.get())
			{
				boost::shared_ptr<read_context_t> context(new read_context_t(
					shared_from_this(), 
					buffer, handler
				));
				context.installChannel(used_channel);
				this->post_async_req(context);
				need_buffer_req = false;
			}
		}

		if(need_buffer_req)
		{
			boost::shared_ptr<rabbit_mq_impl::request_t> new_request(new read_context_t(
				shared_from_this(), 
				buffer, handler
			));
			this->buffer_req(new_request);
		}

		if(this->_stat==rabbit_mq_impl::IDLE)
			this->schedule_once();
	}

	template<typename ConstBufferSequenceT, typename WriteHandlerT> 
	void async_write_some_impl(const ConstBufferSequenceT &buffer, WriteHandlerT handler)
	{
		typedef rabbit_mq_impl::write_context_t<ConstBufferSequenceT, WriteHandlerT> write_context_t;
		bool need_buffer_req = true;

		if(this->_stat==rabbit_mq_impl::LOGGED_IN)
		{
			boost::shared_ptr<channel_t> used_channel = this->apply_for_channel();
			if(used_channel.get())
			{
				boost::shared_ptr<write_context_t> context(new write_context_t(
					shared_from_this(), 
					buffer, handler
				));
				context->installChannel(used_channel);
				this->post_async_req(context);
				need_buffer_req = false;
			}
		}

		if(need_buffer_req)
		{
			boost::shared_ptr<rabbit_mq_impl::request_t> new_request(new write_context_t(
				shared_from_this(), 
				buffer, handler
			));
			this->buffer_req(new_request);
		}

		if(this->_stat==rabbit_mq_impl::IDLE)
			this->schedule_once();
	}

	template<typename ShutdownHandlerT> 
	void shutdown_later(ShutdownHandlerT complete_handler)
	{
		typedef rabbit_mq_impl::shutdown_context_t<ShutdownHandlerT> shutdown_context_t;

		if(this->will_shutdown())
			return;

		this->go_to_stat(rabbit_mq_impl::SHUTDOWN_LATER);

		this->clear_buffered_reqs(); // 清空还在排队的读写请求
		boost::shared_ptr<rabbit_mq_impl::request_t> new_request(new shutdown_context_t(
			shared_from_this(), complete_handler
		));
		this->buffer_req(new_request, true);
		this->schedule_once();
	}

	void shutdown_later()
	{
		this->shutdown_later(boost::bind(
			&rabbit_mq_impl::shutdown_dummy_handler, shared_from_this()
		));
	}

private:
	void shutdown_dummy_handler()
	{
		std::cout<<"rabbit-mq-terminal shutdown."<<std::endl;
	}

	void try_to_shutdown()
	{
		if(this->_pended_requests_num<0)
			throw std::runtime_error("internal error, pended-requests-num negative.");

		if(this->_pended_requests_num>0)
		{
			int pended_num = this->_pended_requests_num;
			RABBIT_MQ_LOG("%d reqs pended, try to shutdown later.", pended_num);
			return;
		}

		size_t num_of_reqs_posted = this->post_buffered_reqs();
		if(num_of_reqs_posted!=1)
		{
			throw std::runtime_error("internal error, post multi reqs for shutdown.");
		}else{
			this->go_to_stat(rabbit_mq_impl::SHUTDOWN_IN_PROGRESS);
			RABBIT_MQ_LOG("now shutdown req posted.");
		}
	}

private:
	boost::shared_ptr<channel_t> apply_for_channel()
	{
		boost::shared_ptr<channel_t> ret;
		if(!this->_available_channels.empty())
			ret = get_channel(this->_available_channels);
		else if(!this->_not_available_channels.empty())
			ret = this->get_channel(this->_not_available_channels);
		return ret;
	}

	boost::shared_ptr<channel_t> get_channel(
		std::queue<boost::shared_ptr<channel_t> > &queue
	)
	{
		boost::shared_ptr<channel_t> ret;
		if(!queue.empty())
		{
			ret = queue.front();
			queue.pop();
		}
		return ret;
	}

	void push_channel(
		std::queue<boost::shared_ptr<channel_t> > &queue, 
		boost::shared_ptr<channel_t> channel
	)
	{
		queue.push(channel);
	}

private:
	struct request_t
	{
		request_t(boost::shared_ptr<rabbit_mq_impl> parent) : _parent(parent)
		{
		}

		virtual ~request_t()
		{
			boost::shared_ptr<rabbit_mq_impl> parent = _parent.lock();
			if(!parent.get())
				return;
			--parent->_pended_requests_num;
		}

		virtual void work() = 0;
		virtual void ack() = 0;
		
		virtual bool ready_to_be_posted(rabbit_mq_impl &parent){return true;}

		boost::weak_ptr<rabbit_mq_impl> _parent;
	};

	template<typename ContextT> 
	void post_async_req(boost::shared_ptr<ContextT> request)
	{
		++_pended_requests_num;
		this->_async_proxy->perform(std::make_pair(
			boost::bind(&ContextT::work, request), 
			boost::bind(&ContextT::ack, request)
		));

		int pended_num = this->_pended_requests_num;
		RABBIT_MQ_LOG("async req posted, %d reqs pended.", pended_num);
	}

	size_t post_buffered_reqs()
	{
		size_t num_of_reqs_posted = 0;
		std::queue<boost::shared_ptr<rabbit_mq_impl::request_t> > &buffer = this->_buffered_requests;
		while(!buffer.empty())
		{
			boost::shared_ptr<rabbit_mq_impl::request_t> req = buffer.front();
			if(!req->ready_to_be_posted(*this))
				break;
			buffer.pop();
			this->post_async_req(req);
			++num_of_reqs_posted;
		}
		return num_of_reqs_posted;
	}

	void clear_buffered_reqs()
	{
		std::queue<boost::shared_ptr<rabbit_mq_impl::request_t> > &buffer = this->_buffered_requests;
		while(!buffer.empty())
			buffer.pop();
	}

	void buffer_req(boost::shared_ptr<rabbit_mq_impl::request_t> new_req, bool force=false)
	{
		bool decide_to_push = false;
		if(force){
			decide_to_push = true;
		}else{ // 关闭过程中不再缓存请求
			if(!this->will_shutdown())
				decide_to_push = true;
		}
		if(decide_to_push)
		{
			this->_buffered_requests.push(new_req);
			RABBIT_MQ_LOG("new req buffered.");
		}
	}

private:
	void schedule_once()
	{
		// 真正的调度配合定时器进行,防止请求由于声明周期未被释放.
		
	}

	void do_schedule_once()
	{
		RABBIT_MQ_LOG("schedule on stat %s with thread %lu", 
			this->format_stat(this->_stat), pthread_self()
		);

		switch(this->_stat)
		{
		case rabbit_mq_impl::IDLE: 
			/*
			 * 初始状态 : 分配异步代理,投递资源分配请求.
			 */
			assert(!this->_async_proxy.get());
			this->_async_proxy.reset(new async_proxy(*this->_ev, this->_async_proxy_parallel_num));
			this->_async_proxy->start();

			this->post_async_req(boost::make_shared<alloc_context_t>(shared_from_this()));
			this->go_to_stat(rabbit_mq_impl::ALLOC);
			break;
		case rabbit_mq_impl::NOT_CONNECTED: // 未连接
			this->post_async_req(boost::make_shared<connect_context_t>(shared_from_this()));
			this->go_to_stat(rabbit_mq_impl::CONNECTING);
			break;
		case rabbit_mq_impl::CONNECTED: // 已连接
			this->post_async_req(boost::make_shared<login_context_t>(shared_from_this()));
			this->go_to_stat(rabbit_mq_impl::LOGGING_IN);
			break;
		case rabbit_mq_impl::LOGGED_IN: // 已登录
			{
				size_t posted_num = this->post_buffered_reqs();
				RABBIT_MQ_LOG("%d reqs posted.", posted_num);
			}
			break;
		case rabbit_mq_impl::SHUTDOWN_LATER: // 预备关闭
			this->try_to_shutdown();
			break;
		case rabbit_mq_impl::ALLOC: // 资源分配中
		case rabbit_mq_impl::CONNECTING: // 连接中
		case rabbit_mq_impl::LOGGING_IN: // 登录中
		case rabbit_mq_impl::SHUTDOWN_IN_PROGRESS: // 关闭中
		default:
			std::ostringstream error_info;
			error_info<<"internal logic error : invalid internal state "<<this->_stat<<".";
			throw std::runtime_error(error_info.str());
		}
	}

	// alloc_context_t
	struct alloc_context_t : public rabbit_mq_impl::request_t
	{
		alloc_context_t(boost::shared_ptr<rabbit_mq_impl> parent) : 
			rabbit_mq_impl::request_t(parent), 
			_alloc_result(boost::system::errc::success)
		{
		}

		virtual void work()
		{
			boost::shared_ptr<rabbit_mq_impl> parent = _parent.lock();
			if(!parent)
			{
				this->_alloc_result = boost::system::errc::operation_canceled;
				return;
			}

			for(int i=0; i<parent->_chn_max_num; ++i)
			{
				int channel_idx = i+1;
				boost::shared_ptr<channel_t> channel(new channel_t(channel_idx));
				parent->push_channel(parent->_not_available_channels, channel);
			}

			parent->_connection = amqp_new_connection();
			parent->_socket = amqp_tcp_socket_new(parent->_connection);
			if(!parent->_socket)
			{
				this->_alloc_result = boost::system::errc::io_error;
				return;
			}
		}

		virtual void ack()
		{
			boost::shared_ptr<rabbit_mq_impl> parent = _parent.lock();
			if(!parent)
				return;

			if(this->_alloc_result!=boost::system::errc::success)
			{
				parent->go_to_stat(rabbit_mq_impl::IDLE);
				throw boost::system::system_error(boost::system::errc::make_error_code(this->_alloc_result));
			}else{
				parent->go_to_stat(rabbit_mq_impl::NOT_CONNECTED);
				parent->schedule_once();
			}
		}

		boost::system::errc::errc_t _alloc_result;
	};

	// connect_context_t
	struct connect_context_t : public rabbit_mq_impl::request_t
	{
		connect_context_t(boost::shared_ptr<rabbit_mq_impl> parent) : 
			rabbit_mq_impl::request_t(parent), 
			_connect_result(boost::system::errc::success)
		{
		}

		virtual void work()
		{
			boost::shared_ptr<rabbit_mq_impl> parent = _parent.lock();
			if(!parent)
			{
				this->_connect_result = boost::system::errc::operation_canceled;
				return;
			}

			struct timeval connect_timeout;
			connect_timeout.tv_sec = parent->_connect_timeout_val/1000;
			connect_timeout.tv_usec = parent->_connect_timeout_val%1000*1000;
			if(amqp_socket_open_noblock(parent->_socket, 
				parent->_broker_ip.c_str(), parent->_broker_port, 
				&connect_timeout)!=AMQP_STATUS_OK)
			{
				this->_connect_result = boost::system::errc::host_unreachable;
				return;
			}
		}

		virtual void ack()
		{
			boost::shared_ptr<rabbit_mq_impl> parent = _parent.lock();
			if(!parent)
				return;

			if(this->_connect_result!=boost::system::errc::success)
			{
				parent->go_to_stat(rabbit_mq_impl::NOT_CONNECTED);
				throw boost::system::system_error(boost::system::errc::make_error_code(this->_connect_result));
			}else{
				parent->go_to_stat(rabbit_mq_impl::CONNECTED);
				parent->schedule_once();
			}
		}

		boost::system::errc::errc_t _connect_result;
	};

	// login_context_t
	struct login_context_t : public rabbit_mq_impl::request_t
	{
		login_context_t(boost::shared_ptr<rabbit_mq_impl> parent) : 
			rabbit_mq_impl::request_t(parent), 
			_login_result(boost::system::errc::success)
		{
		}

		virtual void work()
		{
			boost::shared_ptr<rabbit_mq_impl> parent = _parent.lock();
			if(!parent)
			{
				this->_login_result = boost::system::errc::operation_canceled;
				return;
			}

			if(amqp_login(parent->_connection, parent->_virtual_host.c_str(), 
				parent->_chn_max_num, parent->_frame_max_size, 
				0, AMQP_SASL_METHOD_PLAIN, 
				parent->_username.c_str(), parent->_password.c_str()).reply_type!=AMQP_RESPONSE_NORMAL)
			{
				this->_login_result = boost::system::errc::connection_refused;
				return;
			}
		}

		virtual void ack()
		{
			boost::shared_ptr<rabbit_mq_impl> parent = _parent.lock();
			if(!parent)
				return;

			if(this->_login_result!=boost::system::errc::success)
			{
				parent->go_to_stat(rabbit_mq_impl::CONNECTED);
				throw boost::system::system_error(boost::system::errc::make_error_code(this->_login_result));
			}else{
				parent->go_to_stat(rabbit_mq_impl::LOGGED_IN);
				parent->schedule_once();
			}
		}

		boost::system::errc::errc_t _login_result;
	};

	// rw_context_t
	struct rw_context_t : public rabbit_mq_impl::request_t
	{
		rw_context_t(boost::shared_ptr<rabbit_mq_impl> parent) : 
			rabbit_mq_impl::request_t(parent)
		{
		}

		virtual bool ready_to_be_posted(rabbit_mq_impl &parent)
		{
			boost::shared_ptr<channel_t> used_channel = parent.apply_for_channel();
			if(!used_channel.get())
				return false;
			this->installChannel(used_channel);
			return true;
		}

		void installChannel(boost::shared_ptr<channel_t> used_channel)
		{
			assert(used_channel.get() && !this->_used_channel.get());
			this->_used_channel = used_channel;
			RABBIT_MQ_LOG("channel %d(%s) installed.", 
				used_channel->_chn_idx, (used_channel->opened()?"opened":"closed")
			);
		}

		boost::shared_ptr<channel_t> _used_channel;
	};

	// read_context_t
	template<typename MutableBufferSequenceT, typename ReadHandlerT> 
	struct read_context_t : public rabbit_mq_impl::rw_context_t
	{
		read_context_t(
			boost::shared_ptr<rabbit_mq_impl> parent, 
			const MutableBufferSequenceT &buffer, ReadHandlerT handler
		) : rabbit_mq_impl::rw_context_t(parent), 
			_buffer(buffer), _handler(handler), 
			_read_result(boost::system::errc::success), 
			_bytes_readed(0)
		{
			// queue
			parent->_with_queue_tbl.get(this->_with_queue);
			if(this->_with_queue.empty())
				throw std::runtime_error("no queue indicated for reading");
		}

		virtual void work()
		{
			boost::shared_ptr<rabbit_mq_impl> parent = _parent.lock();
			if(!parent.get())
			{
				this->_read_result = boost::system::errc::operation_canceled;
				return;
			}

			if(!_used_channel.get())
			{
				this->_read_result = boost::system::errc::operation_canceled;
				return;
			}

			if(_used_channel->closed())
			{
				amqp_channel_open(parent->_connection, _used_channel->_chn_idx);
				if(amqp_get_rpc_reply(parent->_connection).reply_type!=AMQP_RESPONSE_NORMAL)
				{
					this->_read_result = boost::system::errc::io_error;
					return;
				}
				_used_channel->open();
			}

			if(amqp_basic_get(parent->_connection, _used_channel->_chn_idx,
				amqp_cstring_bytes(this->_with_queue), true).reply_type!=AMQP_RESPONSE_NORMAL)
			{
				this->_read_result = boost::system::errc::no_message_available;
				return;
			}

			amqp_message_t message;
			if(amqp_read_message(parent->_connection, _used_channel->_chn_idx, 
				&message, 0).reply_type!=AMQP_RESPONSE_NORMAL)
			{
				amqp_destroy_message(&message);
				this->_read_result = boost::system::errc::no_message;
				return;
			}

			void *data = message.body.bytes;
			this->_bytes_readed = message.body.len;
			boost::asio::buffer_copy(this->_buffer, boost::asio::buffer(data, this->_bytes_readed));

			amqp_destroy_message(&message);
		}

		virtual void ack()
		{
			boost::shared_ptr<rabbit_mq_impl> parent = _parent.lock();
			if(!parent)
				return;

			if(_used_channel.get())
			{
				if(_used_channel->opened())
					parent->push_channel(parent->_available_channels, _used_channel);
				else
					parent->push_channel(parent->_not_available_channels, _used_channel);
				this->_used_channel.reset();
			}

			boost::system::error_code ec = boost::system::errc::make_error_code(this->_read_result);
			this->_handler(ec, this->_bytes_readed);
			parent->schedule_once();
		}

		const MutableBufferSequenceT &_buffer;
		ReadHandlerT _handler;
		std::string _with_queue;
		boost::system::errc::errc_t _read_result;
		size_t _bytes_readed;
	};

	// write_context_t
	template<typename ConstBufferSequenceT, typename WriteHandlerT> 
	struct write_context_t : public rabbit_mq_impl::rw_context_t
	{
		write_context_t(
			boost::shared_ptr<rabbit_mq_impl> parent, 
			const ConstBufferSequenceT &buffer, WriteHandlerT handler
		) : rabbit_mq_impl::rw_context_t(parent), 
			_buffer(buffer), _handler(handler), 
			_write_result(boost::system::errc::success), 
			_bytes_written(0)
		{
			// exchange
			parent->_with_exchange_tbl.get(this->_with_exchange);
			if(this->_with_exchange.empty())
				throw std::runtime_error("no exchange indicated for writing.");

			// routing_key
			parent->_with_routing_key_tbl.get(this->_with_routing_key);
			if(this->_with_routing_key.empty())
				throw std::runtime_error("no routing key indicated for writing.");
		}

		virtual void work()
		{
			boost::shared_ptr<rabbit_mq_impl> parent = _parent.lock();
			if(!parent.get())
			{
				this->_write_result = boost::system::errc::operation_canceled;
				return;
			}

			if(!_used_channel.get())
			{
				this->_write_result = boost::system::errc::operation_canceled; 
				return;
			}

			if(_used_channel->closed())
			{
				amqp_channel_open(parent->_connection, _used_channel->_chn_idx);
				if(amqp_get_rpc_reply(parent->_connection).reply_type!=AMQP_RESPONSE_NORMAL)
				{
					this->_write_result = boost::system::errc::io_error;
					return;
				}
				_used_channel->open();
			}

			amqp_basic_properties_t props;
			props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
			props.content_type = amqp_cstring_bytes("text/plain");
			props.delivery_mode = 1;
			std::string s(boost::asio::buffers_begin(this->_buffer), boost::asio::buffers_end(this->_buffer));
			RABBIT_MQ_LOG("msg \"%s\" will be written, %d bytes.", s.c_str(), s.length());
			if(amqp_basic_publish(parent->_connection, _used_channel->_chn_idx, 
				amqp_cstring_bytes(this->_with_exchange.c_str()), 
				amqp_cstring_bytes(this->_with_routing_key.c_str()), 
				0, 0, 
				&props, amqp_cstring_bytes(s.c_str()))!=AMQP_STATUS_OK)
			{
				this->_write_result = boost::system::errc::io_error;
				return;
			}
			this->_bytes_written = s.length();
			RABBIT_MQ_LOG("msg written with success.");
		}

		virtual void ack()
		{
			boost::shared_ptr<rabbit_mq_impl> parent = _parent.lock();
			if(!parent)
				return;

			if(_used_channel.get())
			{
				if(_used_channel->opened())
					parent->push_channel(parent->_available_channels, _used_channel);
				else
					parent->push_channel(parent->_not_available_channels, _used_channel);
				_used_channel.reset();
			}

			boost::system::error_code ec = boost::system::errc::make_error_code(this->_write_result);
			this->_handler(ec, this->_bytes_written);
			parent->schedule_once();
		}

		const ConstBufferSequenceT &_buffer;
		WriteHandlerT _handler;
		std::string _with_exchange, _with_routing_key;
		boost::system::errc::errc_t _write_result;
		size_t _bytes_written;
	};

	// shutdown_context_t
	template<typename ShutdownHandlerT> 
	struct shutdown_context_t : public rabbit_mq_impl::request_t
	{
		shutdown_context_t(
			boost::shared_ptr<rabbit_mq_impl> parent, ShutdownHandlerT handler
		) : rabbit_mq_impl::request_t(parent), 
			_handler(handler), 
			_shutdown_result(boost::system::errc::success)
		{
		}

		virtual void work()
		{
			boost::shared_ptr<rabbit_mq_impl> parent = _parent.lock();
			if(!parent.get())
			{
				this->_shutdown_result = boost::system::errc::operation_canceled;
				return;
			}

			// 将available_channel都协商关闭
			typedef boost::shared_ptr<rabbit_mq_impl::channel_t> channel_ptr_t;
			channel_ptr_t channel = parent->get_channel(parent->_available_channels);
			while(channel.get())
			{
				if(amqp_channel_close(
					parent->_connection, 
					channel->_chn_idx, 
					AMQP_REPLY_SUCCESS).reply_type!=AMQP_RESPONSE_NORMAL)
				{
					RABBIT_MQ_LOG("channel with idx %d closed failed.", channel->_chn_idx);
				}else{
					channel->close();
					parent->push_channel(parent->_not_available_channels, channel);
				}
				channel = parent->get_channel(parent->_available_channels);
			}

			// 将连接关闭
			if(amqp_connection_close(
				parent->_connection, AMQP_REPLY_SUCCESS).reply_type!=AMQP_RESPONSE_NORMAL)
			{
				RABBIT_MQ_LOG("close connection failed.");
				this->_shutdown_result = boost::system::errc::io_error;
				return;
			}

			if(amqp_destroy_connection(parent->_connection)!=AMQP_STATUS_OK)
			{
				RABBIT_MQ_LOG("destroy connection failed.");
				this->_shutdown_result = boost::system::errc::io_error;
				return;
			}
		}

		virtual void ack()
		{
			boost::shared_ptr<rabbit_mq_impl> parent = _parent.lock();
			if(!parent)
				return;

			typedef boost::shared_ptr<rabbit_mq_impl::channel_t> channel_ptr_t;
			channel_ptr_t channel = parent->get_channel(parent->_not_available_channels);
			while(channel.get())
			{
				if(!channel->closed())
				{
					std::ostringstream s;
					s<<"internal error, channel with idx "<<channel->_chn_idx
						<<" not closed while cleaning resource.";
					throw std::runtime_error(s.str());
				}
				channel.reset();
				channel = parent->get_channel(parent->_not_available_channels);
			}

			if(this->_shutdown_result!=boost::system::errc::success)
			{
				RABBIT_MQ_LOG("%s", boost::system::system_error(
					boost::system::errc::make_error_code(this->_shutdown_result)
				).what());
			}
			this->_handler();

			printf("123\n");
			parent->go_to_stat(rabbit_mq_impl::IDLE);
		}

		ShutdownHandlerT _handler;
		boost::system::errc::errc_t _shutdown_result;
	};

private:
	typedef enum{
		IDLE = 0, // 初始状态
		ALLOC, // 资源分配中
		NOT_CONNECTED, // 未连接
		CONNECTING, // 连接中
		CONNECTED, // 已连接
		LOGGING_IN, // 登录中
		LOGGED_IN, // 已登录
		SHUTDOWN_LATER, // 预备关闭
		SHUTDOWN_IN_PROGRESS // 正在关闭
	}rabbit_mq_terminal_stat_t;

	const char* format_stat(rabbit_mq_impl::rabbit_mq_terminal_stat_t stat)
	{
		static const char* strs[] = {
			"idle", 
			"alloc", 
			"not-connected", 
			"connecting", 
			"connected", 
			"logging-in", 
			"logged-in", 
			"shutdown-later", 
			"shutdown-in-progress"
		};
		return strs[(int)(stat)];
	}
	
	bool is_shutdown_stat(rabbit_mq_impl::rabbit_mq_terminal_stat_t stat) const
	{
		return (stat==rabbit_mq_impl::SHUTDOWN_LATER || 
			stat==rabbit_mq_impl::SHUTDOWN_IN_PROGRESS);
	}

	bool will_shutdown() const
	{
		return this->is_shutdown_stat(this->_stat);
	}

	void go_to_stat(rabbit_mq_impl::rabbit_mq_terminal_stat_t new_stat)
	{
		bool decide_to_change_stat = false;
		bool updated = false;

		if(this->is_shutdown_stat(new_stat))
		{
			decide_to_change_stat = true;
		}else{
			if(!will_shutdown())
			{
				decide_to_change_stat = true;
			}else{
				if( this->_stat==rabbit_mq_impl::SHUTDOWN_IN_PROGRESS && 
					new_stat==rabbit_mq_impl::IDLE )
					decide_to_change_stat = true;
			}
		}

		if(decide_to_change_stat)
		{
			this->_stat = new_stat;
			RABBIT_MQ_LOG("go to stat %s.", this->format_stat(this->_stat));
		}
	}

private:
	rabbit_mq_terminal_stat_t _stat;
	event_loop *_ev;
	boost::shared_ptr<async_proxy> _async_proxy;
	int _async_proxy_parallel_num;
	amqp_connection_state_t _connection;
	amqp_socket_t *_socket;
	std::queue<boost::shared_ptr<channel_t> > _not_available_channels;
	std::queue<boost::shared_ptr<channel_t> > _available_channels;

	std::string _broker_ip;
	int _broker_port;
	int _frame_max_size;
	int _connect_timeout_val;
	std::string _virtual_host;
	std::string _username, _password;
	int _chn_max_num;
	chained_call_context_tbl<std::string> _with_exchange_tbl, _with_routing_key_tbl, _with_queue_tbl;

	std::queue<boost::shared_ptr<rabbit_mq_impl::request_t> > _buffered_requests;
	boost::atomic<int> _pended_requests_num; // 所有在进行的请求数目(包括读写及各种处理)
};

/************
 * rabbit_mq
 ************/
template<typename MutableBufferSequenceT, typename ReadHandlerT> 
void rabbit_mq::async_read_some_impl(const MutableBufferSequenceT &buffer, ReadHandlerT handler)
{
	this->_impl->async_read_some_impl(buffer, handler);
}

template<typename ConstBufferSequenceT, typename WriteHandlerT> 
void rabbit_mq::async_write_some_impl(const ConstBufferSequenceT &buffer, WriteHandlerT handler)
{
	this->_impl->async_write_some_impl(buffer, handler);
}

template<typename ShutdownHandlerT> 
void rabbit_mq::shutdown_later(ShutdownHandlerT complete_handler)
{
	this->_impl->shutdown_later(complete_handler);
}

#endif

