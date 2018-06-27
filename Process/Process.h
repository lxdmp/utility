#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <string>
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>

class ProcessImpl;

class Process : public boost::enable_shared_from_this<Process>, public boost::noncopyable
{
public:
	Process(std::string name, std::vector<std::string> args);
	virtual ~Process();

	bool start();
	int wait();

	void putenv(std::string key, std::string val);

	void writeToStdin(const char *msg, size_t len);
	void writeTostdin(std::string msg);

	virtual void onStdoutReaded(const char *msg, size_t len);
	virtual void onStderrReaded(const char *msg, size_t len);

	std::string processName() const;
	std::string processArgs() const;
	std::vector<std::string> processArgList() const;
	
private:
	boost::shared_ptr<ProcessImpl> m_dptr;
	std::string m_name;
	std::vector<std::string> m_args;
};

#endif
