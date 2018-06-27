#ifndef _DEFINE_H_
#define _DEFINE_H_

#ifdef __GNU__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif

// 读写锁定义
#include <boost/thread/mutex.hpp>
typedef boost::shared_lock<boost::shared_mutex> read_lock;
typedef boost::unique_lock<boost::shared_mutex> write_lock;
typedef boost::shared_mutex rw_mutex;

#endif

