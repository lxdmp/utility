#ifndef _CPPBIND_JSON_ENCODE_IMP__
#define _CPPBIND_JSON_ENCODE_IMP__

#include "binder.hpp"

namespace cppbind{

class EncodeBinder : public BinderBase
{
public:
    EncodeBinder()
	{
		this->_root = cJSON_CreateObject();
	}

	~EncodeBinder()
	{
		if(this->_root)
			cJSON_Delete(this->_root);
	}

public:
	cJSON* getContent() const
	{
        return this->_root;
    }

    void setContent(cJSON *content)
	{
		if(this->_root)
			cJSON_Delete(this->_root);
        this->_root = content;
    }

    template<typename T>
    void bind(const std::string &name, T &v, const char *default_value = NULL)
	{
		cJSON *jv = encode(v);
		cJSON_AddItemToObject(this->_root, name.c_str(), jv);
    }

    template<typename T>
    void bind(const std::string &name, boost::shared_ptr<T> &v, const char *default_value = NULL)
	{
		if(v.get())
		{
			cJSON *jv = encode(*(v.get()));
			cJSON_AddItemToObject(this->_root, name.c_str(), jv);
		}
    }

private:
	// stl container
    template<typename T>
	cJSON* encode(std::vector<T> &e) const
	{
		cJSON* jv = cJSON_CreateArray();
        for(typename std::vector<T>::iterator iter=e.begin(); iter!=e.end(); ++iter)
		{
			cJSON *item = encode(*iter);
			cJSON_AddItemToArray(jv, item);
		}
		return jv;
    }
	
    template<typename T>
	cJSON* encode(std::list<T> &e) const
	{
		cJSON* jv = cJSON_CreateArray();
		for(typename std::list<T>::iterator iter=e.begin(); iter!=e.end(); ++iter)
		{
			cJSON *item = encode(*iter);
			cJSON_AddItemToArray(jv, item);
		}
		return jv;
    }

    template<typename T>
	cJSON* encode(std::map<std::string, T> &e) const
	{
		cJSON *jv = cJSON_CreateObject();
		for(typename std::map<std::string, T>::iterator iter=e.begin(); iter!=e.end(); ++iter)
		{
			cJSON *item = encode(iter->second);
			cJSON_AddItemToObject(jv, iter->first.c_str(), item);
		}
		return jv;
    }

private:
	// custom type
    template<typename T>
	cJSON* encode(T &e) const
	{
		Binder binder;
		e.setBind(&binder);
		return cJSON_Duplicate(binder.getContent(), 1);
    }

private:
	// basic type
    cJSON* encode(bool&) const;
    cJSON* encode(int&) const;
    cJSON* encode(long&) const;
    cJSON* encode(float&) const;
    cJSON* encode(double&) const;
    cJSON* encode(std::string&) const;
	
 private:
	cJSON* _root;
};

}
#endif

