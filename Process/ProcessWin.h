#ifndef _PROCESS_WIN_H_
#define _PROCESS_WIN_H_

#include <boost/function.hpp>
#include "ProcessImpl.h"

#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>
#endif

#if (defined(_WIN32) || defined(_WIN64))
class ProcessWin : public ProcessImpl
{
public:
	ProcessWin(Process *parent);
	
	virtual void write_to_stdin(const char *msg, size_t len);
protected:
	virtual bool do_start();
	virtual int do_wait();
	virtual void do_delete();

private:
	HANDLE hStdInRead;
	HANDLE hStdInWrite;
    
	HANDLE hStdOutRead;
	HANDLE hStdOutWrite;

	HANDLE hStdErrRead;
	HANDLE hStdErrWrite;

	PROCESS_INFORMATION piProcInfo;
	
	void resetPipeHandle(HANDLE *pHandle);

	void checkChildProcess(BOOL *child_exited, long *exit_code);
	void tryReadPipe(HANDLE fd, boost::function<void(const char *, size_t)> notify);
};
#endif

#endif
