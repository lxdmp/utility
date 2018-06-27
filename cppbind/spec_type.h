#ifndef _CPPBIND_SPEC_TYPE_BASE_H_
#define _CPPBIND_SPEC_TYPE_BASE_H_

#include "binder.hpp"

namespace cppbind
{

class SpecTypeBase
{
public:
    virtual ~SpecTypeBase(){};
    virtual int decode(const std::string &, std::string *err_msg)=0;
    virtual std::string encode()=0;
public:
	void setBind(cppbind::Binder *binder)
	{
		if(binder->isEncode()){
			std::string value = this->encode();
			cJSON *jv = cJSON_CreateString(value.c_str());
			binder->setContent(jv);
        }else{
			cJSON *jv = binder->getContent();
			if(jv->type!=cJSON_String)
				throw CppBindException("should be a string");
            std::string err_msg;
            int ret = this->decode(jv->valuestring, &err_msg);
			if(ret)
				throw CppBindException(err_msg);
        }
    }
};

}

#endif

