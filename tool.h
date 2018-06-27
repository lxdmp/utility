#ifndef _TOOL_H_
#define _TOOL_H_

#include <string>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>

struct StringSplitter
{
	StringSplitter(char ch);
	std::vector<std::string> operator()(const std::string &s);
	char _ch;
};

struct StringJoiner
{
	StringJoiner(char ch);
	std::string operator()(const std::vector<std::string> &l);
	char _ch;
};

struct StringStripper
{
	StringStripper(char ch);
	std::string operator()(const std::string &s);
	char _ch;
};

struct ChDir
{
	ChDir(std::string target);
	~ChDir();
	std::string _backup;
};

struct RealSplitter
{
	void operator()(double val);
	int hour_part, min_part;
};

// 区间[start1, end1)与区间[start2, end2)是否相交
bool timeRangeCrossed(boost::posix_time::time_duration start1, boost::posix_time::time_duration end1, 
	boost::posix_time::time_duration start2, boost::posix_time::time_duration end2);

// 区间[start1, end1)是否覆盖区间[start2, end2)
bool timeRangeCovered(boost::posix_time::time_duration start1, boost::posix_time::time_duration end1, 
	boost::posix_time::time_duration start2, boost::posix_time::time_duration end2);

template<typename T> 
std::string ClassName()
{
	std::string s = typeid(T).name();
	StringSplitter splitter(' ');
	return splitter(s).back();
};

template<typename ClassT>
struct ChClassDir : public ChDir
{
	ChClassDir() : ChDir(ClassName<ClassT>()){}
};

void copyFile2Path(std::string file_name, std::string path);

std::string actualPath();
char pathSeperator();

class CompressorImpl;
class Compressor
{
public:
	Compressor(std::string compressor_path, std::string compressor_format);
	void operator()(std::string target) const;
	
	const std::string& path() const;
	const std::string& format() const;
private:
	boost::shared_ptr<CompressorImpl> _impl;
	std::string _compressor_path;
	std::string _compressor_format;
};

#endif
