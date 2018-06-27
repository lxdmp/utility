#ifndef _CPPBIND_JSON_DECODE_IMP_
#define _CPPBIND_JSON_DECODE_IMP_

#include "binder.hpp"

namespace cppbind
{
class DecodeBinder : public BinderBase
{
public:
	DecodeBinder(cJSON *content)
	{
        this->_content = cJSON_Duplicate(content, 1);
	}

	~DecodeBinder()
	{
		if(this->_content)
			cJSON_Delete(this->_content);
	}

public:
    cJSON* getContent() const
	{
        return this->_content;
    }

    void setContent(cJSON *content)
	{
        this->_content = content;
    }

    template<typename T>
    void bind(const std::string& name, T& v, const char *default_value=NULL) const
	{
		cJSON *content = cJSON_GetObjectItem(this->_content, name.c_str());
		if(content)
		{
			try{
				decode(content, &v);
            }catch (CppBindException &e){
				throw CppBindException(e, name);
            }
        }else{
			throw CppBindException(name, "not found");
        }
    }

    template<typename T>
    void bind(const std::string& name, boost::shared_ptr<T> &v, const char *default_value=NULL) const
	{
		if(cJSON_GetObjectItem(this->_content, name.c_str()))
		{
			T e;
            bind(name, e, default_value);
            v = boost::shared_ptr<T>(new T(e));
        }
    }

private:
	// stl container
	template<typename T>
	void decode(cJSON *content, std::vector<T> *e) const
	{
		std::list<T> v;
		decode(content, &v);
		e->insert(e->end(), v.begin(), v.end());
	}

    template<typename T>
    void decode(cJSON *content, std::list<T> *e) const
	{
		if(content->type!=cJSON_Array)
			throw CppBindException("not an array");

		int array_size = cJSON_GetArraySize(content);
		for(int idx=0; idx<array_size; ++idx)
		{
			cJSON *array_item = cJSON_GetArrayItem(content, idx);
			try{
				T item;
				decode(array_item, &item);
				e->push_back(item);
			}catch(CppBindException &e)
			{
				std::ostringstream s;
				s<<"["<<idx<<"]";
				throw CppBindException(e, s.str().c_str());
			}
		}
    }

    template<typename T>
	void decode(cJSON *content, std::map<std::string, T> *e) const
	{
		if(content->type!=cJSON_Object)
			throw CppBindException("not an object");
		
		for(cJSON *node=content->child; node; node=node->next)
		{
			try{
				T item;
				decode(node, &item);
				e->insert(std::make_pair(node->string, item));
			}catch(CppBindException &e){
				throw CppBindException(e, std::string(".")+node->string);
			}
		}
    }

private:
	// custom type
    template<typename T>
	void decode(cJSON *content, T *e) const
	{
		Binder binder(content);
		e->setBind(&binder);
    }

private:
	// basic type
    void decode(cJSON *content, bool*) const;
    void decode(cJSON *content, int*) const;
    void decode(cJSON *content, long*) const;
    void decode(cJSON *content, float*) const;
    void decode(cJSON *content, double*) const;
    void decode(cJSON *content, std::string*) const;

private:
	cJSON *_content;
};

}
#endif

