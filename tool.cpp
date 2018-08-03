#include "tool.h"
#include <boost/filesystem.hpp>

#if defined(_WIN32) || defined(_WIN64) // 统一用posix名
#include <direct.h>
#define getcwd _getcwd
#define chdir _chdir
#define strcasecmp _stricmp
#else
#include <dirent.h>
#endif

StringSplitter::StringSplitter(char ch) : _ch(ch)
{
}

std::vector<std::string> StringSplitter::operator()(const std::string &s)
{
	std::vector<std::string> res;
	size_t start = 0, end  = 0;

	end = s.find_first_of(_ch, start);
	while(end!=std::string::npos){
		if(end>=start)
			res.push_back(s.substr(start, end-start));
		start = end+1;
		end = s.find_first_of(_ch, start);
	}
	res.push_back(s.substr(start));

	return res;
}

StringJoiner::StringJoiner(char ch) : _ch(ch)
{
}

std::string StringJoiner::operator()(const std::vector<std::string> &l)
{
	std::string res;
	for(size_t i=0; i<l.size(); ++i)
		res += l[i]+_ch;
	if(res.length()>0)
		res.erase(res.size()-1);
	return res;
}

StringStripper::StringStripper(char ch) : _ch(ch)
{
}

std::string StringStripper::operator()(const std::string &s)
{
	size_t start = s.find_first_not_of(_ch);
	if(start==std::string::npos)
		return std::string("");
	size_t end = s.find_last_not_of(_ch);
	if(end==std::string::npos)
		return std::string("");
	return s.substr(start, end-start+1);
}

bool StringIgnoreCase::operator()(const std::string &s1, const std::string &s2) const
{
	return StringIgnoreCase::compare(s1, s2)<0;
}

int StringIgnoreCase::compare(const std::string &s1, const std::string &s2)
{
	return strcasecmp(s1.c_str(), s2.c_str());
}

ChDir::ChDir(std::string target)
{
	boost::filesystem::path current_path = boost::filesystem::current_path();
	_backup = current_path.string();

	boost::filesystem::path target_path(target);
	if(!boost::filesystem::exists(target_path))
		boost::filesystem::create_directories(target_path);

	::chdir(target.c_str());
}

ChDir::~ChDir()
{
	::chdir(_backup.c_str());
}

void RealSplitter::operator()(double val)
{
	int hour_part = (int)(val);
	int min_part = (int)((val-hour_part)*60.0);
	this->hour_part=  hour_part;
	this->min_part = min_part;
}

// 区间[start1, end1)与区间[start2, end2)是否相交
bool timeRangeCrossed(
	boost::posix_time::time_duration start1, boost::posix_time::time_duration end1, 
	boost::posix_time::time_duration start2, boost::posix_time::time_duration end2
)
{
	assert(start1<end1 && start2<end2);

	/*
	 * 存在以下几种相交情况:
	 * - start1<=start2<end1<=end2;
	 * - start1<=start2<end2<=end1;
	 * - start2<=start1<end1<=end2;
	 * - start2<=start1<end2<=end1.
	 */
	if(start1<=start2 && start2<end1 && end1<=end2)
		return true;
	else if(start1<=start2 && start2<end2 && end2<=end1)
		return true;
	else if(start2<=start1 && start1<end1 && end1<=end2)
		return true;
	else if(start2<=start1 && start1<end2 && end2<=end1)
		return true;
	else
		return false;
}

// 区间[start1, end1)是否覆盖区间[start2, end2)
bool timeRangeCovered(
	boost::posix_time::time_duration start1, boost::posix_time::time_duration end1, 
	boost::posix_time::time_duration start2, boost::posix_time::time_duration end2
)
{
	assert(start1<end1 && start2<end2);

	/*
	 * 存在以下覆盖情况:
	 * - start1<=start2<end2<=end1
	 */
	return (start1<=start2 && start2<end2 && end2<=end1);
}

void copyFile2Path(std::string file_name, std::string path)
{
	std::ostringstream s;
	s<<path<<"/"<<file_name;
	boost::filesystem::path from(file_name), to(s.str());
	if(boost::filesystem::exists(to))
		boost::filesystem::remove(to);
	boost::filesystem::copy(from, to);
}

std::string actualPath()
{
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	::getcwd(buf, sizeof(buf));
	return std::string(buf, strlen(buf));
}

char pathSeperator()
{
#if defined(_WIN32) || defined(_WIN64)
	return '\\';
#else
	return '/';
#endif
}

/*
 * Compressor : 调用系统安装的压缩工具进行压缩(指定压缩工具的路径与压缩的格式)
 */
#include "ObjLoader.h"
#include "Process/Process.h"
class CompressorImpl
{
public:
	virtual ~CompressorImpl(){}
	virtual void operator()(std::string target) const = 0;
	void setParent(const Compressor *parent){_parent=parent;}
protected:
	const Compressor *_parent;
};

class Compressor7z : public CompressorImpl
{
public:
	DECL_OBJ_WITHOUT_PARAMETER(Compressor7z);
	Compressor7z(){}
public:
	virtual void operator()(std::string target) const
	{
		if(!boost::filesystem::exists(target))
		{
			std::ostringstream s;
			s<<target<<" does not exist.";
			throw std::runtime_error(s.str());
		}

		std::string format = _parent->format();
		std::transform(format.begin(), format.end(), format.begin(), ::tolower);
																
		// 7z a -t7z file.7z file
		std::vector<std::string> args;
		args.push_back("a");
		args.push_back("-t"+format);
		args.push_back(target+"."+format);
		args.push_back(target);

		boost::shared_ptr<Process> process = boost::shared_ptr<Process>(new Process(_parent->path(), args));
		process->start();
		process->wait();
	}
};

Compressor::Compressor(std::string compressor_path, std::string compressor_format) : 
	_compressor_path(compressor_path), _compressor_format(compressor_format)
{
	// - 注册实现
	ObjLoader<CompressorImpl> compressor_loader;
	compressor_loader.reg<Compressor7z>("7z");

	// - 载入实现
	std::string compressor_routine_with_suffix = StringSplitter(pathSeperator())(_compressor_path).back();
	std::string compressor_routine_name = StringSplitter('.')(compressor_routine_with_suffix).front();
	std::transform(compressor_routine_name.begin(), compressor_routine_name.end(), compressor_routine_name.begin(), ::tolower);

	std::string load_param;
	{
		std::ostringstream s;
		s<<"{\""<<compressor_routine_name<<"\":{}}";
		load_param = s.str();
	}

	try{
		std::vector<CompressorImpl*> out;
		compressor_loader.load(load_param, out);
		assert(out.size()==1);
		_impl = boost::shared_ptr<CompressorImpl>(out.front());
		_impl->setParent(this);
	}catch(std::exception &e){
		std::ostringstream s;
		s<<"invalid compressor with path "<<_compressor_path<<" and with format "<<_compressor_format<<", no related implementation.";
		throw std::runtime_error(s.str());
	}

	// - 检查程序是否存在
	boost::filesystem::path path(_compressor_path);
	if(!boost::filesystem::exists(path))
	{
		std::ostringstream s;
		s<<"compressor routine "<<_compressor_path<<" does not exist.";
		throw std::runtime_error(s.str());
	}
}

void Compressor::operator()(std::string target) const
{
	assert(_impl.get());
	_impl->operator()(target);
}

const std::string& Compressor::path() const
{
	return _compressor_path;
}

const std::string& Compressor::format() const
{
	return _compressor_format;
}

