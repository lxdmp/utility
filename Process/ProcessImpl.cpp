#include "ProcessImpl.h"

ProcessImpl::ProcessImpl(Process *parent) : 
	m_parent(parent), 
	m_has_started(false), 
	m_has_exited(false), 
	m_exit_code(0)
{
}

ProcessImpl::~ProcessImpl()
{
	if(m_has_started && !m_has_exited){
		do_delete();
		m_has_exited = true;
	}
}

bool ProcessImpl::start()
{
	if(m_has_started)
		return false;
	m_has_started = do_start();
	return m_has_started;
}

int ProcessImpl::wait()
{
	if(m_has_started && !m_has_exited){
		m_exit_code = do_wait();
		m_has_exited = true;
	}
	return m_exit_code;
}

void ProcessImpl::putenv(std::string key, std::string val)
{
	this->putenvprivate(m_extended_env, key, val);
}

void ProcessImpl::putenvprivate(std::map<std::string, std::vector<std::string> > &env, std::string key, std::string val)
{
	std::map<std::string, std::vector<std::string> >::iterator iter = env.find(key);
	if(iter==env.end()){
		std::vector<std::string> tmp;
		tmp.push_back(val);
		env.insert(std::make_pair<std::string, std::vector<std::string> >(key, tmp));
	}else{
		iter->second.push_back(val);
	}
}

