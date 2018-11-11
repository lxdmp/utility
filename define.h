#ifndef _DEFINE_H_
#define _DEFINE_H_

#ifdef __GNU__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif

#include <boost/thread/shared_mutex.hpp>
typedef boost::shared_mutex rw_mutex_t;
typedef boost::shared_ptr<rw_mutex_t> read_lock_t;
typedef boost::shared_ptr<rw_mutex_t> write_lock_t;

#endif

