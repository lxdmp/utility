#include "binder.hpp"

using namespace cppbind;

Binder::Binder()
{
    this->binder_imp = boost::shared_ptr<BinderBase>(new EncodeBinder());
}

Binder::Binder(cJSON *root)
{
    this->binder_imp =  boost::shared_ptr<BinderBase>(new DecodeBinder(root));
}

cJSON* Binder::getContent() const
{
     return binder_imp->getContent();
}

void Binder::setContent(cJSON *jv)
{
     binder_imp->setContent(jv);
}

bool Binder::isEncode()
{
   if(dynamic_cast<EncodeBinder*>(this->binder_imp.get()))
	   return true;
   else
	   return false;
}

