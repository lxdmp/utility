#ifndef _DEFINE_H_
#define _DEFINE_H_

#ifdef __GNU__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif

#define BOOST_ASIO_DISABLE_STD_CHRONO

#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
typedef boost::shared_mutex rw_mutex;
typedef boost::shared_lock<rw_mutex> read_lock;
typedef boost::unique_lock<rw_mutex> write_lock;

#endif

