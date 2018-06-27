#include "binder.hpp"

using namespace cppbind;

cJSON* EncodeBinder::encode(bool &e) const
{
	if(e)
		return cJSON_CreateTrue();
	else
		return cJSON_CreateFalse();
}

cJSON* EncodeBinder::encode(int &e) const
{
	return cJSON_CreateNumber(e);
}

cJSON* EncodeBinder::encode(long &e) const
{
	return cJSON_CreateNumber(e);
}

cJSON* EncodeBinder::encode(float &e) const
{
	return cJSON_CreateNumber(e);
}

cJSON* EncodeBinder::encode(double &e) const
{
	return cJSON_CreateNumber(e);
}

cJSON* EncodeBinder::encode(std::string &e) const
{
	return cJSON_CreateString(e.c_str());
}

