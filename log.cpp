#include <stdio.h>
#include <boost/filesystem.hpp>
#include "log.h"
#include "tool.h"

void initRootLogger(const char *exe_path)
{
	const char config_file_name[] = "urconfig.properties";
	log4cplus::Logger root = log4cplus::Logger::getRoot();
	std::string path = exe_path;

	path = path.substr(0, path.find_last_of(pathSeperator())) + pathSeperator() + config_file_name;

	const std::string possible_path[] = {
		config_file_name, 
		path
	};

	for(size_t i=0; i<sizeof(possible_path)/sizeof(possible_path[0]); ++i)
	{
		std::string path_str = possible_path[i];
		boost::filesystem::path conf_file_path(path_str);
		if(boost::filesystem::exists(conf_file_path))
		{
			log4cplus::PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT(path_str.c_str()));
			break;
		}
	}
}

