#include "echo_pair_util.h"
#include <boost/asio/steady_timer.hpp>

#define ENABLE_LOG
#undef ENABLE_LOG

#if defined(ENABLE_LOG)
#define LOG(format, ...) fprintf(stdout, format, __VA_ARGS__)
#else
#define LOG(format, ...)
#endif

struct echo_pair_util_string_splitter
{
	echo_pair_util_string_splitter(char ch):_ch(ch){}
	std::vector<std::string> operator()(const std::string &s)
	{
		std::vector<std::string> res;
		size_t start = 0, end  = 0;

		end = s.find_first_of(_ch, start);
		while(end!=std::string::npos){
			if(end>=start)
				res.push_back(s.substr(start, end-start));
			start = end+1;
			end = s.find_first_of(_ch, start);
		}
		res.push_back(s.substr(start));
		return res;
	}
	char _ch;
};

on_echo_pair_readed::on_echo_pair_readed(boost::shared_ptr<event_loop> parent) : 
	m_parent(parent)
{
}

echo_pair::custom_readed_cb_decl on_echo_pair_readed::create_binded_cb()
{
	return boost::bind(&on_echo_pair_readed::cb, shared_from_this(), _1, _2);
}

void on_echo_pair_readed::delete_later_cb(const boost::system::error_code &ec, boost::weak_ptr<event_loop> ev)
{
	LOG("%s : cb called\n", __FUNCTION__);
	boost::shared_ptr<event_loop> real_ev = ev.lock();
	if(real_ev.get()){
		real_ev->terminate();
		LOG("%s : terminate event loop\n", __FUNCTION__);
	}
}

void on_echo_pair_readed::delete_later()
{
	double sec = this->delete_delay_sec();
	LOG("%s : delete %.3f sec later\n", __FUNCTION__, sec);
	boost::shared_ptr<event_loop> parent = m_parent.lock();
	if(parent.get())
	{
		boost::shared_ptr<boost::asio::steady_timer> del_custom_timer(new boost::asio::steady_timer(parent->get_io_service()));
		del_custom_timer->expires_from_now(boost::chrono::microseconds(int(sec*1000000)));
		del_custom_timer->async_wait(boost::bind(on_echo_pair_readed::delete_later_cb, _1, m_parent));
	}
}

void on_echo_pair_readed::cb(const std::string &received, std::string &replied)
{
	if(received.empty())
		return;

	LOG("%s : msg \"%s\" added into buf\n", __FUNCTION__, received.c_str());
	m_buf += received;
	echo_pair_util_string_splitter splitter(this->frame_seperator());
	std::vector<std::string> l = splitter(m_buf);
	std::ostringstream ret;
	for(size_t i=0; i<l.size(); ++i)
	{
		std::string &s = l[i];
		if(i+1==l.size()){
			if(s.empty()){
				m_buf.clear();
				LOG("%s : buf cleared\n", __FUNCTION__);
			}else{
				m_buf = l.back();
				LOG("%s : msg \"%s\" contained in buf\n", __FUNCTION__, m_buf.c_str());
			}
		}else{
			if(s.empty())
			{
				this->delete_later();
				break;
			}else{
				std::ostringstream replied;
				LOG("%s : 1 frame proced\n", __FUNCTION__);
				this->frame_received(s, replied);
				replied<<frame_seperator();
				ret<<replied.str();
			}
		}
	}

	if(!ret.str().empty())
		replied = ret.str();
}

