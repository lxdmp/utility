#ifndef _OBJ_LOADER_IMPL_H_
#define _OBJ_LOADER_IMPL_H_

/**************
 * ObjFactoryT
 **************/
template <typename ObjDerivedT, typename ObjBaseT>
ObjBaseT* ObjFactoryT<ObjDerivedT, ObjBaseT>::create(cJSON *param)
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

/************
 * ObjLoader
 ************/
template <typename ObjBaseT, typename KeyT, typename KeyComparatorT> 
template<typename ObjDerivedT> 
void ObjLoader<ObjBaseT, KeyT, KeyComparatorT>::reg(KeyT key)
{
	boost::shared_ptr<ObjFactoryDecl<ObjBaseT> > factory = boost::shared_ptr<ObjFactoryDecl<ObjBaseT> >(
		new ObjFactoryT<ObjDerivedT, ObjBaseT>()
	);
	_tbl.insert(std::make_pair(key, factory));
}

template <typename ObjBaseT, typename KeyT, typename KeyComparatorT> 
void ObjLoader<ObjBaseT, KeyT, KeyComparatorT>::load(const char *file_path, std::vector<ObjBaseT*> &out) const
{
	std::ifstream ifs(file_path);
	std::stringstream buffer;  
	buffer<<ifs.rdbuf();
	ifs.close();
	std::string contents(buffer.str());
	this->load(contents, out);
}

template <typename ObjBaseT, typename KeyT, typename KeyComparatorT> 
void ObjLoader<ObjBaseT, KeyT, KeyComparatorT>::load(const std::string &content, std::vector<ObjBaseT*> &out) const
{
	cJSON* root = cJSON_Parse(content.c_str());
	if(root)
		throw std::runtime_error(cJSON_GetErrorPtr());
	this->load(root, out);
}

template <typename ObjBaseT, typename KeyT, typename KeyComparatorT> 
void ObjLoader<ObjBaseT, KeyT, KeyComparatorT>::load(cJSON *root, std::vector<ObjBaseT*> &out) const
{
	for(cJSON *node=root->child; node; node=node->next)
	{
		ObjBaseT *obj = this->load(node);
		if(obj)
			out.push_back(obj);
	}
}

template <typename ObjBaseT, typename KeyT, typename KeyComparatorT> 
ObjBaseT* ObjLoader<ObjBaseT, KeyT, KeyComparatorT>::load(cJSON *node, bool ignoreIllegalKey) const
{
	std::string obj_name(node->string);
	typename std::map<KeyT, boost::shared_ptr<ObjFactoryDecl<ObjBaseT> >, KeyComparatorT>::const_iterator factory_iter = _tbl.find(obj_name);
	if(factory_iter==_tbl.end())
	{
		if(!ignoreIllegalKey)
		{
			std::ostringstream s;
			s<<"key named "<<obj_name<<" without indicated loader!!";
			throw std::runtime_error(s.str().c_str());
		}
		return NULL;
	}
	return factory_iter->second->create(node);
}

template <typename ObjBaseT, typename KeyT, typename KeyComparatorT> 
ObjBaseT* ObjLoader<ObjBaseT, KeyT, KeyComparatorT>::load(KeyT key) const
{
	typename std::map<KeyT, boost::shared_ptr<ObjFactoryDecl<ObjBaseT> >, KeyComparatorT>::const_iterator factory_iter = _tbl.find(key);
	if(factory_iter==_tbl.end())
	{
		std::ostringstream s;
		s<<"key named "<<key<<" without indicated loader!!";
		throw std::runtime_error(s.str().c_str());
	}
	return factory_iter->second->create(NULL);
}

#endif

