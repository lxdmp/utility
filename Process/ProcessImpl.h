#ifndef _PROCESS_IMPL_H_
#define _PROCESS_IMPL_H_

#include <map>
#include "Process.h"

class ProcessImpl
{
public:
	ProcessImpl(Process *parent);
	virtual ~ProcessImpl();
	
	bool start();
	int wait();
	
	void putenv(std::string key, std::string val);

	virtual void write_to_stdin(const char *msg, size_t len)=0;
protected:
	virtual bool do_start()=0; // 启动进程(true,启动成功;false,启动失败)
	virtual int do_wait()=0; // 阻塞等待进程结束(返回进程退出码)
	virtual void do_delete(){}; // 强制结束进程
	
protected:
	bool m_has_started;
	bool m_has_exited;
	int m_exit_code;
	Process *m_parent;
	std::map<std::string, std::vector<std::string> > m_extended_env;
	void putenvprivate(std::map<std::string, std::vector<std::string> > &env, std::string key, std::string val);
};

#endif
