#ifndef _MSQ_QUEUE_TERMINAL_
#define _MSQ_QUQUE_TERMINAL_

#include <boost/function.hpp>

template<typename MsgQueueT>
class msg_queue_terminal : public MsgQueueT
{
public:
	static boost::shared_ptr<msg_queue_terminal<MsgQueueT> > build();

public:
	template<typename MutableBufferSequenceT, typename ReadHandlerT> 
	void async_read_some(const MutableBufferSequenceT &buffer, ReadHandlerT handler);

	template<typename ConstBufferSequenceT, typename WriteHandlerT> 
	void async_write_some(const ConstBufferSequenceT &buffer, WriteHandlerT handler);
};

#include "MsgQueueTerminalImpl.h"

#endif

