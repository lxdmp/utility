#ifndef _CPPBIND_EXCEPTION_H_
#define _CPPBIND_EXCEPTION_H_

#include <string>
#include <stdexcept>

namespace  cppbind
{

class CppBindException : public std::runtime_error
{
public:
    CppBindException(const std::string &node_name, const std::string &base_msg) :
		runtime_error(node_name +" "+base_msg)
    {
	}
    
    CppBindException(const std::string &base_msg) :
		runtime_error(std::string(" ")+base_msg)
    {
	}
	
    CppBindException(const CppBindException &e, const std::string &node_name) : 
		runtime_error(node_name+e.what())
    {
	}
};

}

#endif

