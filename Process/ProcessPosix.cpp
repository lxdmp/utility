#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ProcessPosix.h"

#if (!defined(_WIN32) && !defined(_WIN64))
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>
#endif

#if (!defined(_WIN32) && !defined(_WIN64))
ProcessPosix::ProcessPosix(Process *parent) : ProcessImpl(parent), 
	m_child(-1)
{
	for(int i=0; i<sizeof(pipe_stdin)/sizeof(pipe_stdin[0]); ++i)
		pipe_stdin[i] = -1;
	for(int i=0; i<sizeof(pipe_stdout)/sizeof(pipe_stdout[0]); ++i)
		pipe_stdout[i] = -1;
	for(int i=0; i<sizeof(pipe_stderr)/sizeof(pipe_stderr[0]); ++i)
		pipe_stderr[i] = -1;
	for(int i=0; i<sizeof(exit_notify_pipe)/sizeof(exit_notify_pipe[0]); ++i)
		exit_notify_pipe[i] = -1;
}
	
bool ProcessPosix::do_start()
{
	pid_t ret;
	
	if(pipe(exit_notify_pipe))
		return false;
	
	if(pipe(pipe_stdin))
		goto ERROR_EXIT;
	else if(pipe(pipe_stdout))
		goto ERROR_EXIT;
	else if(pipe(pipe_stderr))
		goto ERROR_EXIT;

	ret = fork();
	if(ret<0){
		goto ERROR_EXIT;
	}else if(ret>0){
		reset_pipe(&pipe_stdin[0]);
		reset_pipe(&pipe_stdout[1]);
		reset_pipe(&pipe_stderr[1]);
		m_child = ret;
		boost::thread(boost::bind(&ProcessPosix::block_to_wait_child, this));
		return true;
	}else{
		reset_pipe(&pipe_stdin[1]);
		dup2(pipe_stdin[0], STDIN_FILENO);
		reset_pipe(&pipe_stdin[0]);
		
		reset_pipe(&pipe_stdout[0]);
		dup2(pipe_stdout[1], STDOUT_FILENO);
		reset_pipe(&pipe_stdout[1]);
		
		reset_pipe(&pipe_stderr[0]);
		dup2(pipe_stderr[1], STDERR_FILENO);
		reset_pipe(&pipe_stderr[1]);
		
		const std::string env_sep(":");
		for(std::map<std::string, std::vector<std::string> >::const_iterator iter=m_extended_env.begin(); 
			iter!=m_extended_env.end(); ++iter)
		{
			std::string key = iter->first;
			std::string val;
			char *existed = ::getenv(key.c_str());
			if(existed!=NULL){
				val = existed;
				val += env_sep;
			}
			for(size_t i=0; i<iter->second.size(); ++i){
				val += iter->second.at(i);
				if(i+1<iter->second.size())
					val += env_sep;
			}
			::putenv(const_cast<char*>((key+"="+val).c_str()));
		}
		
		std::string name = m_parent->processName();
		std::vector<std::string> arg = m_parent->processArgList();
		char* args[1024] = {NULL};
		args[0] = const_cast<char*>(name.c_str());
		for(int i=0; i<arg.size(); ++i)
			args[1+i] = const_cast<char*>(arg[i].c_str());
		::execvp(args[0], args);
		::perror("exec failed\n");
		::exit(1);
	}

ERROR_EXIT:
	for(int i=0; i<sizeof(pipe_stdin)/sizeof(pipe_stdin[0]); ++i)
		reset_pipe(&pipe_stdin[i]);
	for(int i=0; i<sizeof(pipe_stdout)/sizeof(pipe_stdout[0]); ++i)
		reset_pipe(&pipe_stdout[i]);
	for(int i=0; i<sizeof(pipe_stderr)/sizeof(pipe_stderr[0]); ++i)
		reset_pipe(&pipe_stderr[i]);
	for(int i=0; i<sizeof(exit_notify_pipe)/sizeof(exit_notify_pipe[0]); ++i)
		reset_pipe(&exit_notify_pipe[i]);
	return false;
}

int ProcessPosix::do_wait()
{
	int array[] = {
		exit_notify_pipe[0], 
		pipe_stdout[0], 
		pipe_stderr[0]
	};
	int exit_code = 0;
	bool child_has_exit = false;
	
	while(!child_has_exit)
	{
		int max_fd = -1;
		fd_set set;
		FD_ZERO(&set);
		for(int i=0; i<sizeof(array)/sizeof(array[0]); ++i){
			if(array[i]>max_fd)
				max_fd = array[i];
			FD_SET(array[i], &set);
		}
	
		int ret = ::select(max_fd+1, &set, NULL, NULL, NULL);
		for(int i=0; i<sizeof(array)/sizeof(array[0]) && ret>0; ++i){
			if(!FD_ISSET(array[i], &set))
				continue;
			--ret;
			
			switch(i){
			case 0:
				if(read(array[i], &exit_code, sizeof(exit_code))>0)
					child_has_exit = true;
				break;
			case 1:
				try_read_pipe(array[i], boost::bind(&Process::onStdoutReaded, m_parent, _1, _2));
				break;
			case 2:
				try_read_pipe(array[i], boost::bind(&Process::onStderrReaded, m_parent, _1, _2));
				break;
			}
		}
	}

	for(int i=0; i<sizeof(pipe_stdin)/sizeof(pipe_stdin[0]); ++i)
		reset_pipe(&pipe_stdin[i]);
	for(int i=0; i<sizeof(pipe_stdout)/sizeof(pipe_stdout[0]); ++i)
		reset_pipe(&pipe_stdout[i]);
	for(int i=0; i<sizeof(pipe_stderr)/sizeof(pipe_stderr[0]); ++i)
		reset_pipe(&pipe_stderr[i]);
	for(int i=0; i<sizeof(exit_notify_pipe)/sizeof(exit_notify_pipe[0]); ++i)
		reset_pipe(&exit_notify_pipe[i]);
	return exit_code;
}

void ProcessPosix::do_delete()
{
	if(m_child>0){
		kill(m_child, SIGTERM);

		int buf;
		while(read(exit_notify_pipe[0], &buf, sizeof(buf))<=0);
	}
	
	for(int i=0; i<sizeof(pipe_stdin)/sizeof(pipe_stdin[0]); ++i)
		reset_pipe(&pipe_stdin[i]);
	for(int i=0; i<sizeof(pipe_stdout)/sizeof(pipe_stdout[0]); ++i)
		reset_pipe(&pipe_stdout[i]);
	for(int i=0; i<sizeof(pipe_stderr)/sizeof(pipe_stderr[0]); ++i)
		reset_pipe(&pipe_stderr[i]);
	for(int i=0; i<sizeof(exit_notify_pipe)/sizeof(exit_notify_pipe[0]); ++i)
		reset_pipe(&exit_notify_pipe[i]);
}

void ProcessPosix::write_to_stdin(const char *msg, size_t len)
{
	if(pipe_stdin[1]>0)
		write(pipe_stdin[1], msg, len);
}

void ProcessPosix::reset_pipe(int *pipe)
{
	if(*pipe>=0){
		close(*pipe);
		*pipe = -1;
	}
}

void ProcessPosix::try_read_pipe(int fd, boost::function<void(const char *, size_t)> notify)
{
	char buf[4096];
	memset(buf, 0, sizeof(buf));
	size_t read_count = read(fd, buf, sizeof(buf));
	if(read_count>0)
		notify(buf, read_count);
}

void ProcessPosix::block_to_wait_child()
{
	int status, code;
	if(waitpid(m_child, &status, 0)<=0){
		code = 1;
	}else{
		code = WEXITSTATUS(status);
	}
	write(exit_notify_pipe[1], &code, sizeof(code));
}
#endif

