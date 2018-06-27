#ifndef _OBJ_LOADER_IMPL_
#define _OBJ_LOADER_IMPL_

template <typename ObjBaseT> 
void ObjLoader<ObjBaseT>::load(const char *file_path, std::vector<ObjBaseT*> &out) const
{
	std::ifstream ifs(file_path);
	std::stringstream buffer;  
	buffer<<ifs.rdbuf();
	ifs.close();
	std::string contents(buffer.str());
	this->load(contents, out);
}

template <typename ObjBaseT> 
void ObjLoader<ObjBaseT>::load(const std::string &content, std::vector<ObjBaseT*> &out) const
{
	cJSON* root = cJSON_Parse(content.c_str());
	if(root)
		throw std::runtime_error(cJSON_GetErrorPtr());
	this->load(root, out);
}

template <typename ObjBaseT> 
void ObjLoader<ObjBaseT>::load(cJSON *root, std::vector<ObjBaseT*> &out) const
{
	for(cJSON *node=root->child; node; node=node->next)
	{
		std::string obj_name(node->string);
		typename std::map<std::string, ObjFactoryDecl<ObjBaseT>*>::const_iterator factory_iter = _tbl.find(obj_name);
		if(factory_iter==_tbl.end())
		{
			std::ostringstream s;
			s<<"key named "<<obj_name<<" without indicated loader!!";
			throw std::runtime_error(s.str().c_str());
		}
		ObjBaseT *obj = factory_iter->second->create(node);
		out.push_back(obj);
	}
}

#endif

