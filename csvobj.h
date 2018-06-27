#ifndef _CSV_OBJ_H_
#define _CSV_OBJ_H_
#include <typeinfo>

#include "csvloader.h"
#include "log.h"

inline void getRowsFromCSV(const char *filename, std::list<CSVRow> &rows)
{
	CSVLoader csv(filename);
	rows = csv.getRows();
}

template<typename T>
void getFromCSV(const char *filename, std::list<T*> *list)
{
	std::list<CSVRow> rows;
	getRowsFromCSV(filename, rows);
	//LOG_INFO("%lu lines loaded from file %s", rows.size(), filename);

	for(std::list<CSVRow>::iterator it  = rows.begin(); it != rows.end(); it++)
	{
		std::vector<std::string> cells = it->getCells();
		T* e = T::createObj(cells);
		assert(e != NULL);
		list->push_back(e);    
	}
	//LOG_INFO("%lu %s loaded from file %s", list->size(), typeid(T).name(), filename);
}

template<typename T> 
class CSVRowMapper
{
public:
	std::list<T*> result;

	void operator()(const char *filename)
	{
		std::list<CSVRow> rows;
		getRowsFromCSV(filename, rows);
		for(std::list<CSVRow>::iterator iter  = rows.begin(); iter != rows.end(); ++iter)
		{
			std::vector<std::string> cells = iter->getCells();
			this->rowAdded(cells);
		}
	}

	void rowAdded(const std::vector<std::string> &cells)
	{
		T* e = T::createObj(cells);
		assert(e);
		result.push_back(e);
	}
};

#endif
