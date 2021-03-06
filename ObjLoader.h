﻿// 具有公共接口对象的动态加载实现.
#ifndef _OBJ_LOADER_H_
#define _OBJ_LOADER_H_

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
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
	virtual ObjBaseT* create(cJSON *param);
};

template <typename ObjBaseT, 
	typename KeyT=std::string, 
	typename KeyComparatorT=std::less<KeyT> > 
class ObjLoader
{
public:
	ObjLoader(){}

	template<typename ObjDerivedT> 
	void reg(KeyT key);

	// 加载一组对象
	void load(const char *file_path, std::vector<ObjBaseT*> &out) const; // out中分配的对象需由用户程序维护
	void load(const std::string &content, std::vector<ObjBaseT*> &out) const;
	void load(cJSON *root, std::vector<ObjBaseT*> &out) const;

	// 加载一个对象
	ObjBaseT* load(cJSON *node, bool ignoreIllegalKey=false) const;
	ObjBaseT* load(KeyT key) const;

private:
	std::map<KeyT, boost::shared_ptr<ObjFactoryDecl<ObjBaseT> >, KeyComparatorT > _tbl;
};

#include "ObjLoaderImpl.h"

#endif

