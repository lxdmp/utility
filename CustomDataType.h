#ifndef _CUSTOM_DATA_TYPE_H_
#define _CUSTOM_DATA_TYPE_H_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include "cppbind/cppbind_json.hpp"
#include "cppbind/spec_type.h"

// 日期时间
class JsonPosixTime : public boost::posix_time::ptime, public cppbind::SpecTypeBase 
{
public:
	JsonPosixTime();
	JsonPosixTime(boost::posix_time::ptime pt);
private:
	int decode(const std::string &str, std::string *errmsg);
    std::string encode();
};

// 持续时间
class JsonTimeDuration : public boost::posix_time::time_duration, public cppbind::SpecTypeBase 
{
public:
	JsonTimeDuration();
	JsonTimeDuration(boost::posix_time::time_duration td);
private:
    int decode(const std::string &str, std::string *errmsg);
    std::string encode();
};

// 日期
class JsonGregorianDate : public boost::gregorian::date, public cppbind::SpecTypeBase
{
public:
	JsonGregorianDate();
	JsonGregorianDate(boost::gregorian::date gd);
private:
	int decode(const std::string &str, std::string *errmsg);
	std::string encode();
};

#endif

