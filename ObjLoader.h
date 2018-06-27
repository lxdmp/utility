#ifndef _RESOURCE_LOADER_H_
#define _RESOURCE_LOADER_H_

#include <map>
#include "cppbind/cppbind_json.hpp"
#include "3rdParty/cJSON/cJSON.h"

#define DECL_OBJ_WITH_PARAMETER(class_name) \
	class_name(){ \
		std::ostringstream s; \
		s<<"useless constructor of "<<typeid(class_name).name()<<" without Parameter!"; \
		throw std::runtime_error(s.str()); \
	} \

#define DECL_OBJ_WITHOUT_PARAMETER(class_name) \
	class Parameter{ \
	public: \
		void setBind(cppbind::Binder* binder){} \
	};\
	class_name(class_name::Parameter&){ \
		std::ostringstream s; \
		s<<"useless constructor of "<<typeid(class_name).name()<<" with Parameter!"; \
		throw std::runtime_error(s.str()); \
	} \

template <typename ObjBaseT>
class ObjFactoryDecl
{
public:
	virtual ~ObjFactoryDecl(){}
	virtual ObjBaseT* create(cJSON *param) = 0;
};

template <typename ObjDerivedT, typename ObjBaseT>
class ObjFactoryT : public ObjFactoryDecl<ObjBaseT>
{
public:
	virtual ObjBaseT* create(cJSON *param)
	{
		if( param && 
			param->type==cJSON_Object && 
			cJSON_GetArraySize(param)>0 )
		{
			boost::shared_ptr<typename ObjDerivedT::Parameter> obj_param(new typename ObjDerivedT::Parameter);
			cppbind::Binder binder(param);
			obj_param->setBind(&binder);
			{
				cppbind::JsonBind<typename ObjDerivedT::Parameter> jsonBind;
				std::stringstream s;
				jsonBind.encode(*obj_param.get(), &s);
				printf("obj with type \"%s\" will be created with param \"%s\".\n", typeid(ObjDerivedT).name(), s.str().c_str());
			}
			return static_cast<ObjBaseT*>(new ObjDerivedT(*obj_param.get()));
		}else{
			printf("obj with type \"%s\" will be created with no param.\n", typeid(ObjDerivedT).name());
			return static_cast<ObjBaseT*>(new ObjDerivedT());
		}
	}
};

template <typename ObjBaseT> 
class ObjLoader
{
public:
	ObjLoader(){}

	template<typename ObjDerivedT> 
	void reg(std::string key)
	{
		ObjFactoryDecl<ObjBaseT>* factory = new ObjFactoryT<ObjDerivedT, ObjBaseT>();
		_tbl.insert(std::make_pair(key, factory));
	}

	void load(const char *file_path, std::vector<ObjBaseT*> &out) const;
	void load(const std::string &content, std::vector<ObjBaseT*> &out) const;
	void load(cJSON *root, std::vector<ObjBaseT*> &out) const;

private:
	std::map<std::string, ObjFactoryDecl<ObjBaseT>*> _tbl;
};

#include "ObjLoaderImpl.h"

#endif

