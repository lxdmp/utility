#ifndef _ECHO_PAIR_UTIL_H_
#define _ECHO_PAIR_UTIL_H_

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "event_loop.h"
#include "echo_pair.h"

class on_echo_pair_readed : public boost::noncopyable, public boost::enable_shared_from_this<on_echo_pair_readed>
{
public:
	on_echo_pair_readed(boost::shared_ptr<event_loop> parent);
	echo_pair::custom_readed_cb_decl create_binded_cb();
	
protected:
	static void delete_later_cb(const boost::system::error_code &ec, boost::weak_ptr<event_loop> ev);
	void cb(const std::string &received, std::string &replied);

protected:
	virtual void frame_received(const std::string &frame, std::ostringstream &s) = 0;
	virtual double delete_delay_sec() const{return 1.0;}

public:
	virtual char frame_seperator() const{return '\n';}
	void delete_later();

private:
	std::string m_buf;
	boost::weak_ptr<event_loop> m_parent;
};

#endif

