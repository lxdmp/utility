#include "TypeConverter.h"

/*
 * bool
 */
template<>
bool TypedConver<bool>(const std::string& str, const bool &dummy)
{
	std::string copy(str);
	std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
	if(copy.compare("true")==0)
		return true;
	return false;
}

template<>
void TypedConver<bool>(const std::string& str,  bool *out)
{
	std::string copy(str);
	std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
	if(copy.compare("true")==0)
		*out = true;
	else
		*out = false;
}

/*
 * boost::posix_time::ptime
 */
template<>
boost::posix_time::ptime TypedConver<boost::posix_time::ptime>(const std::string& str, const boost::posix_time::ptime &dummy)
{
	boost::posix_time::ptime out;
	std::istringstream is(str);
    std::locale locale_input = std::locale(std::locale::classic(),new boost::posix_time::time_input_facet("%Y-%m-%d %H:%M:%S"));
    is.imbue(locale_input);
    is>>out;
	return out;
}

template<>
void TypedConver<boost::posix_time::ptime>(const std::string& str,  boost::posix_time::ptime *out)
{
    std::istringstream is(str);
    std::locale locale_input = std::locale(std::locale::classic(),new boost::posix_time::time_input_facet("%Y-%m-%d %H:%M:%S"));
    is.imbue(locale_input);
    is>>*out;
}
