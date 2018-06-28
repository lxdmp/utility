#ifndef _CPPBIND_JSONBIND_IMP_
#define _CPPBIND_JSONBIND_IMP_

#include <assert.h>
#include <string>
#include <sstream>
#include <fstream>
#include <list>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include "cppbind_exception.h"
#include "../3rdParty/cJSON/cJSON.h"

namespace cppbind
{

class BinderBase
{
public:
	virtual ~BinderBase(){}
    virtual cJSON* getContent() const = 0;
	virtual void setContent(cJSON *content) = 0;
};

class Binder
{
public:
	Binder(); // encoder
    Binder(cJSON *root); // decoder

    template<typename T>
    void bind(const std::string &name, T &v, const char *default_value = NULL);
    cJSON* getContent() const;
	void setContent(cJSON *content);
    bool isEncode();

public:
    boost::shared_ptr<BinderBase> binder_imp;
};

}

#include "./decode_binder.hpp"
#include "./encode_binder.hpp"

namespace cppbind
{
template<typename T>
void Binder::bind(const std::string &name, T &v, const char *default_value)
{
	EncodeBinder *encoder = NULL;
	DecodeBinder *decoder = NULL;
	if((decoder=dynamic_cast<DecodeBinder*>(this->binder_imp.get())))
        decoder->bind(name, v, default_value);
	else if((encoder=dynamic_cast<EncodeBinder*>(this->binder_imp.get())))
        encoder->bind(name, v, default_value);
	else
		throw CppBindException("internal implementation error");
}

}

#endif

