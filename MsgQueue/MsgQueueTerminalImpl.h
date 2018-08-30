#ifndef _MSG_QUEUE_TERMINAL_IMPL_H_
#define _MSG_QUEUE_TERMINAL_IMPL_H_

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

template<typename MsgQueueT>
boost::shared_ptr<msg_queue_terminal<MsgQueueT> > msg_queue_terminal<MsgQueueT>::build()
{
	return boost::make_shared<msg_queue_terminal<MsgQueueT> >();
}

template<typename MsgQueueT>
template<typename MutableBufferSequenceT, typename ReadHandlerT> 
void msg_queue_terminal<MsgQueueT>::async_read_some(const MutableBufferSequenceT &buffer, ReadHandlerT handler)
{
	MsgQueueT::async_read_some_impl(buffer, handler);
}

template<typename MsgQueueT>
template<typename ConstBufferSequenceT, typename WriteHandlerT> 
void msg_queue_terminal<MsgQueueT>::async_write_some(const ConstBufferSequenceT &buffer, WriteHandlerT handler)
{
	MsgQueueT::async_write_some_impl(buffer, handler);
}

#endif

