#include "binder.hpp"

using namespace cppbind;

void DecodeBinder::decode(cJSON *content, bool *e) const
{
	if( content->type!=cJSON_True && 
		content->type!=cJSON_False )
	{
		std::ostringstream s;
		s<<content->string<<" not with bool val";
		throw CppBindException(s.str());
	}
	(content->type==cJSON_False)?(*e=false):(*e=true);
}

void DecodeBinder::decode(cJSON *content, int *e) const
{
	if(content->type!=cJSON_Number)
	{
		std::ostringstream s;
		s<<content->string<<" not with int val";
	}
	*e = content->valueint;
}

void DecodeBinder::decode(cJSON *content, long *e) const
{
	if(content->type!=cJSON_Number)
	{
		std::ostringstream s;
		s<<content->string<<" not with long val";
	}
	*e = content->valueint;
}

void DecodeBinder::decode(cJSON *content, float *e) const
{
	if(content->type!=cJSON_Number)
	{
		std::ostringstream s;
		s<<content->string<<" not with float val";
	}
	*e = content->valuedouble;
}

void DecodeBinder::decode(cJSON *content, double *e) const
{
	if(content->type!=cJSON_Number)
	{
		std::ostringstream s;
		s<<content->string<<" not with double val";
	}
	*e = content->valuedouble;
}

void DecodeBinder::decode(cJSON *content, std::string *e) const
{
	if(content->type!=cJSON_String)
	{
		std::ostringstream s;
		s<<content->string<<" not with string val";
	}
	e->assign(content->valuestring);
}

