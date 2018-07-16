#ifndef _CPPBIND_JSONBIND_
#define _CPPBIND_JSONBIND_

#include <assert.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <list>
#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include "binder.hpp"
#include "../3rdParty/cJSON/cJSON.h"

namespace  cppbind
{

template<typename T>
class JsonBind
{
public:
    boost::shared_ptr<T> decode(std::istream &is) const
	{
		std::string s((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
		return this->decode(s);
    }

	boost::shared_ptr<T> decode(const std::string &s) const
	{
		cJSON *root = NULL;
		root = cJSON_Parse(s.c_str());
		if(!root){
			std::string err_msg(cJSON_GetErrorPtr());
			throw  CppBindException(err_msg);
		}
   
		T* e = new T;
        Binder binder(root);
		cJSON_Delete(root);
        e->setBind(&binder);
        return boost::shared_ptr<T>(e);
	}
     
    void encode(T &e, std::ostream *out) const
	{
		Binder binder;
        e.setBind(&binder);
		cJSON *content = binder.getContent();
		if(content)
		{
			char *format_content = cJSON_Print(content);
			if(format_content)
			{
				*out<<format_content;
				free(format_content);
			}else{
				throw CppBindException("cppbind encode serilization failed");
			}
		}else{
			throw CppBindException("cppbind encode failed");
		}
    }
};

}

#endif

