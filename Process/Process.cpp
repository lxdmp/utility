#include <boost/thread.hpp>
#include <stdio.h>
#include "Process.h"

#if (defined(_WIN32) || defined(_WIN64))
#include "ProcessWin.h"
#else
#include "ProcessPosix.h"
#endif

Process::Process(std::string name, std::vector<std::string> args) : 
	m_name(name), m_args(args), 
#if (defined(_WIN32) || defined(_WIN64))
	m_dptr(new ProcessWin(this))
#else
	m_dptr(new ProcessPosix(this))
#endif
{
}

Process::~Process()
{
}

bool Process::start()
{
	return m_dptr->start();
}

int Process::wait()
{
	return m_dptr->wait();
}

void Process::putenv(std::string key, std::string val)
{
	m_dptr->putenv(key, val);
}

void Process::writeToStdin(const char *msg, size_t len)
{
	m_dptr->write_to_stdin(msg, len);
}

void Process::writeTostdin(std::string msg)
{
	this->writeToStdin(msg.c_str(), msg.size());
}

void Process::onStdoutReaded(const char *msg, size_t len)
{
	fprintf(stdout, "%s", msg);
}

void Process::onStderrReaded(const char *msg, size_t len)
{
	fprintf(stdout, "%s", msg);
}

std::string Process::processName() const
{
	return m_name;
}

std::string Process::processArgs() const
{
	std::string ret;
	for(std::vector<std::string>::const_iterator iter=m_args.begin(); iter!=m_args.end(); ++iter){
		if(iter!=m_args.begin())
			ret += " ";
		ret += *iter;
	}
	return ret;
}

std::vector<std::string> Process::processArgList() const
{
	return m_args;
}
