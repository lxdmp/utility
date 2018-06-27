#ifndef _CSV_LOADER_H_
#define _CSV_LOADER_H_

#include <list>
#include <string>
#include <assert.h>
#include <fstream>
#include "log.h"

class CSVRow
{
private:
   std::string line;
public:
   CSVRow(const char* line)
   {
       this->line = line;
   }
   CSVRow(){
   }
   
   std::vector<std::string>  getCells(const char *delimeters=",")
   {
	   std::vector<std::string>  cells;
       size_t current;
       size_t next = -1;
       do{
           current = next + 1;
           next = this->line.find_first_of(delimeters, current);
           std::string content = this->line.substr(current, next - current);
           size_t first = content.find_first_not_of(" \"");
           size_t last = content.find_last_not_of(" \"");
		   if(first != std::string::npos && last != std::string::npos) {
               content = content.substr(first, last-first+ 1);
		   }else {
			   content = "";
		   }
           cells.push_back(content);
       }while (next !=  std::string::npos);
       return cells;
   }
};


class CSVLoader
{
private:
	std::string file;
public:
	CSVLoader(const char *file)
	{
		this->file  = file;
	}
public:
	std::list<CSVRow> getRows()
	{  
		std::list<CSVRow> rowList;
		std::ifstream  input(this->file.c_str());
		if(!input.good())
		{
			std::string info = "Can not open file: " +  this->file;
			throw std::runtime_error(info);
		}
		while (input.good())
		{
			std::string line;
			std::getline(input,line);
			if(line.size() > 0 && line.find("//") != 0)
				rowList.push_back(CSVRow(line.c_str()));
		}
		input.close();
		return rowList;
	}
};

#endif
