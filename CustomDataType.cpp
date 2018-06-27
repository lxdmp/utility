#include "CustomDataType.h"
#include "tool.h"

/*
 * JsonPosixTime
 */
JsonPosixTime::JsonPosixTime()
{
	// YYYY-MM-DD hh:mm:ss
}

JsonPosixTime::JsonPosixTime(boost::posix_time::ptime pt)
{
	boost::posix_time::ptime *this_pt = static_cast<boost::posix_time::ptime*>(this);
	*this_pt = pt;
}

int JsonPosixTime::decode(const std::string &str, std::string *errmsg)
{
	std::vector<std::string> date_and_time_strs = StringSplitter(' ')(str);
	if(date_and_time_strs.size()!=2){
		std::ostringstream s;
		s<<"Invlid date-time format : "<<str;
		*errmsg = s.str();
		return -1;
	}
	
	std::vector<std::string> date_str = StringSplitter('-')(date_and_time_strs[0]);
	if(date_str.size()!=3){
		std::ostringstream s;
		s<<"Invalid date format : "<<date_and_time_strs[0];
		*errmsg = s.str();
		return -1;
	}

	std::vector<std::string> time_str = StringSplitter(':')(date_and_time_strs[1]);
	if(time_str.size()!=3){
		std::ostringstream s;
		s<<"Invalid time format : "<<date_and_time_strs[1];
		*errmsg = s.str();
		return -1;
	}

	boost::posix_time::ptime *this_pt = static_cast<boost::posix_time::ptime*>(this);
	*this_pt = boost::posix_time::ptime(
		boost::gregorian::date(::atoi(date_str[0].c_str()), ::atoi(date_str[1].c_str()), ::atoi(date_str[2].c_str())), 
		boost::posix_time::time_duration(::atoi(time_str[0].c_str()), ::atoi(time_str[0].c_str()), ::atoi(time_str[0].c_str()))
	);
	return 0;
}

std::string JsonPosixTime::encode()
{
	std::ostringstream s;
	s<<std::setfill('0')
		<<std::setw(4)<<this->date().year()<<"-"
		<<std::setw(2)<<this->date().month().as_number()<<"-"
		<<std::setw(2)<<this->date().day()<<" "
		<<std::setw(2)<<this->time_of_day().hours()<<":"
		<<std::setw(2)<<this->time_of_day().minutes()<<":"
		<<std::setw(2)<<this->time_of_day().seconds();
	return s.str();
}

/*
 * JsonTimeDuration
 */
JsonTimeDuration::JsonTimeDuration()
{
	// hh:mm:ss
}

JsonTimeDuration::JsonTimeDuration(boost::posix_time::time_duration td)
{
    boost::posix_time::time_duration *this_td = static_cast<boost::posix_time::time_duration*>(this);
	*this_td = td;
}

int JsonTimeDuration::decode(const std::string &str, std::string *errmsg)
{
	std::vector<std::string> time_str = StringSplitter(':')(str);
	if(time_str.size()!=3){
		std::ostringstream s;
		s<<"Invalid time format : "<<str;
		*errmsg = s.str();
		return -1;
	}

    boost::posix_time::time_duration *this_td = static_cast<boost::posix_time::time_duration*>(this);
	*this_td = boost::posix_time::time_duration(
		::atoi(time_str[0].c_str()), ::atoi(time_str[0].c_str()), ::atoi(time_str[0].c_str())
	);
	return 0;
}

std::string JsonTimeDuration::encode()
{
	std::ostringstream s;
	s<<std::setfill('0')
		<<std::setw(2)<<this->hours()<<":"
		<<std::setw(2)<<this->minutes()<<":"
		<<std::setw(2)<<this->seconds();
	return s.str();
}

/*
 * JsonGregorianDate
 */
JsonGregorianDate::JsonGregorianDate()
{
	// YYYY-MM-DD
}

JsonGregorianDate::JsonGregorianDate(boost::gregorian::date gd)
{
    boost::gregorian::date *this_gd = static_cast<boost::gregorian::date*>(this);
	*this_gd = gd;
}

int JsonGregorianDate::decode(const std::string &str, std::string *errmsg)
{
	std::vector<std::string> date_str = StringSplitter('-')(str);
	if(date_str.size()!=3){
		std::ostringstream s;
		s<<"Invalid date format : "<<str;
		*errmsg = s.str();
		return -1;
	}

    boost::gregorian::date *this_gd = static_cast<boost::gregorian::date*>(this);
	*this_gd = boost::gregorian::date(
		::atoi(date_str[0].c_str()), ::atoi(date_str[1].c_str()), ::atoi(date_str[2].c_str())
	);
	return 0;
}

std::string JsonGregorianDate::encode()
{
	std::ostringstream s;
	s<<std::setfill('0')
		<<std::setw(4)<<this->year()<<"-"
		<<std::setw(2)<<this->month().as_number()<<"-"
		<<std::setw(2)<<this->day();
	return s.str();
}

