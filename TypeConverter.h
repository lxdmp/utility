#ifndef TYPE_CONVERTER_H_
#define TYPE_CONVERTER_H_

#include <iostream>
#include <sstream>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>

template<typename T>
T TypedConver(const std::string& str, const T &dummy)
{
	T result;
	std::istringstream input(str);
	input >> result;
	return result;
}

template<typename T>
void  TypedConver(const std::string& str, T *out)
{
	std::istringstream input(str);
	input >> (*out);
}

template<>
bool TypedConver<bool>(const std::string& str, const bool &dummy);

template<>
void TypedConver<bool>(const std::string& str,  bool *out);

template<>
boost::posix_time::ptime TypedConver<boost::posix_time::ptime>(const std::string& str, const boost::posix_time::ptime &dummy);

template<>
void TypedConver<boost::posix_time::ptime>(const std::string& str,  boost::posix_time::ptime *out);

#endif

