#ifndef _PROCESS_POSIX_H_
#define _PROCESS_POSIX_H_

#include <boost/function.hpp>
#include "ProcessImpl.h"

#if (!defined(_WIN32) && !defined(_WIN64))
#include <unistd.h>
#endif

#if (!defined(_WIN32) && !defined(_WIN64))
class ProcessPosix : public ProcessImpl
{
public:
	ProcessPosix(Process *parent);
	
	virtual void write_to_stdin(const char *msg, size_t len);
protected:
	virtual bool do_start();
	virtual int do_wait();
	virtual void do_delete();
private:
	int pipe_stdin[2],pipe_stdout[2],pipe_stderr[2];
	int exit_notify_pipe[2];
	pid_t m_child;

	void reset_pipe(int *pipe);
	void try_read_pipe(int fd, boost::function<void(const char *, size_t)> notify);
	void block_to_wait_child();
};
#endif

#endif

