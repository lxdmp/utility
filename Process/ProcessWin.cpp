#include <boost/bind.hpp>
#include "ProcessWin.h"

#if (defined(_WIN32) || defined(_WIN64))
ProcessWin::ProcessWin(Process *parent) : ProcessImpl(parent), 
	hStdInRead(INVALID_HANDLE_VALUE),
	hStdInWrite(INVALID_HANDLE_VALUE),
	hStdOutRead(INVALID_HANDLE_VALUE),
	hStdOutWrite(INVALID_HANDLE_VALUE),
	hStdErrRead(INVALID_HANDLE_VALUE),
	hStdErrWrite(INVALID_HANDLE_VALUE)
{
}
	
bool ProcessWin::do_start()
{
	SECURITY_ATTRIBUTES saAttr;
	STARTUPINFO siStartInfo;
	std::string name, args, cmdline;
	BOOL bSuccess = FALSE;
	
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL;

	if(!::CreatePipe(&hStdInRead, &hStdInWrite, &saAttr, 0))
		goto ERROR_EXIT;
	else if(!::SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0))
		goto ERROR_EXIT; 

	if(!::CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0))
		goto ERROR_EXIT;
	else if(!::SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0))
		goto ERROR_EXIT; 

	if(!::CreatePipe(&hStdErrRead, &hStdErrWrite, &saAttr, 0))
		goto ERROR_EXIT;
	else if(!::SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0))
		goto ERROR_EXIT; 
	
	
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));  
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
	siStartInfo.hStdError = hStdErrWrite;
	siStartInfo.hStdOutput = hStdOutWrite;
	siStartInfo.hStdInput = hStdInRead;

	name = m_parent->processName();
	args = m_parent->processArgs();
	cmdline = name+' '+args;
	
	char *env_str = NULL;
	if(!m_extended_env.empty())
	{
		std::map<std::string, std::vector<std::string> > env = m_extended_env;
		
		PTSTR key, val;
		key = GetEnvironmentStrings();
		while((val=::strchr(key, '='))!=NULL)
		{
			std::string key_str(key, val-key);
			val++;
			std::string val_str(val);
			this->putenvprivate(env, key_str, val_str);
			
			val += strlen(val);
			key = val+1;

			if(key=='\0')
				break;
		}
		
		const std::string env_sep(";");
		int len = 0;
		for(std::map<std::string, std::vector<std::string> >::const_iterator iter=env.begin(); 
			iter!=env.end(); ++iter)
		{
			std::string key = iter->first;
			std::string val;
			for(size_t i=0; i<iter->second.size(); ++i){
				val += iter->second.at(i);
				if(i+1<iter->second.size())
					val += env_sep;
			}
			len += key.size()+val.size()+2; // one for '=' and one for '\0'
		}
		len += 1;

		env_str = (char*)::malloc(len);
		if(env_str==NULL)
			goto ERROR_EXIT;
		memset(env_str, 0, len);
		char *ptr = env_str;
		
		for(std::map<std::string, std::vector<std::string> >::const_iterator iter=env.begin(); 
			iter!=env.end(); ++iter)
		{
			std::string key = iter->first;
			std::string val;
			for(size_t i=0; i<iter->second.size(); ++i){
				val += iter->second.at(i);
				if(i+1<iter->second.size())
					val += env_sep;
			}
			sprintf(ptr, "%s", key.c_str());
			ptr += strlen(ptr);
			*ptr++ = '=';
			sprintf(ptr, "%s", val.c_str());
			ptr += strlen(ptr);
			
			ptr++;
		}
	}

	bSuccess = ::CreateProcess(
		NULL, 
		const_cast<char*>(cmdline.c_str()), 
		NULL, // process security attributes  
		NULL, // primary thread security attributes  
		TRUE, // handles are inherited  
		0, // creation flags  
		env_str, // use parent's environment  
		NULL, // use parent's current directory  
		&siStartInfo, // STARTUPINFO pointer  
        &piProcInfo); // receives PROCESS_INFORMATION  

	if(env_str!=NULL)
		::free(env_str);

	if(!bSuccess){
		goto ERROR_EXIT;
	}else{
		CloseHandle(piProcInfo.hThread);
		return true;
	}

ERROR_EXIT:
	resetPipeHandle(&hStdInRead);
	resetPipeHandle(&hStdInWrite);
	resetPipeHandle(&hStdOutRead);
	resetPipeHandle(&hStdOutWrite);
	resetPipeHandle(&hStdErrRead);
	resetPipeHandle(&hStdErrWrite);
	return false;
}

int ProcessWin::do_wait()
{
#define CHILD_PROCESS_INDEX 0 // child process
#define STDOUT_INDEX 1 // stdout
#define STDERR_INDEX 2 // stderr
#define WAIT_OBJ_NUM 3 // total number

	HANDLE array[WAIT_OBJ_NUM] = {
		piProcInfo.hProcess, // child process, must positioned in the most front, probably data still in data when process exited signal triggered.
		hStdOutRead, // stdout
		hStdErrRead // stderr
	};
	BOOL child_exited = FALSE;
	long exit_code;
	DWORD ret;

	BOOL check_more_signal_state = FALSE; // adapted to "WaitForMultipleObjects", multi objects signaled possible.
	int base_index = 0;

	for(;;)
	{
		int objNum = WAIT_OBJ_NUM-base_index;
		ret = ::WaitForMultipleObjects(objNum, &array[base_index], FALSE, (check_more_signal_state?0:INFINITE));

		if(ret<WAIT_OBJECT_0 || ret>WAIT_OBJECT_0+objNum-1){ // no handle in signal state
			if(check_more_signal_state && !child_exited){
				check_more_signal_state = FALSE;
				base_index = 0;
				continue;
			}else{
				break;
			}
		}

		int index = ret-WAIT_OBJECT_0+base_index;
		switch(index)
		{
		case CHILD_PROCESS_INDEX:
			checkChildProcess(&child_exited, &exit_code);
			check_more_signal_state = TRUE;
			++base_index;
			break;
		case STDOUT_INDEX:
			tryReadPipe(hStdOutRead, boost::bind(&Process::onStdoutReaded, this->m_parent, _1, _2));
			check_more_signal_state = TRUE;
			++base_index;
			break;
		case STDERR_INDEX:
			tryReadPipe(hStdErrRead, boost::bind(&Process::onStderrReaded, this->m_parent, _1, _2));
			check_more_signal_state = FALSE;
			base_index = 0;
			break;
		}

		if(!check_more_signal_state && child_exited)
			break;
	}
	
	resetPipeHandle(&hStdInWrite);
	resetPipeHandle(&hStdOutRead);
	resetPipeHandle(&hStdErrRead);

	CloseHandle(piProcInfo.hProcess);

	return (int)exit_code;
}

void ProcessWin::do_delete()
{
	HANDLE hprocess = ::OpenProcess(PROCESS_TERMINATE, false, piProcInfo.dwProcessId);
	BOOL result = ::TerminateProcess(hprocess,0);
	::CloseHandle(hprocess);
	
	resetPipeHandle(&hStdInWrite);
	resetPipeHandle(&hStdOutRead);
	resetPipeHandle(&hStdErrRead);
	
	CloseHandle(piProcInfo.hProcess);
}

void ProcessWin::write_to_stdin(const char *msg, size_t len)
{
	DWORD dwWritten;  
	BOOL bSuccess = FALSE;
	bSuccess = WriteFile( hStdInWrite, msg, len, &dwWritten, NULL);  
}

void ProcessWin::resetPipeHandle(HANDLE *pHandle)
{
	if(*pHandle!=INVALID_HANDLE_VALUE){
		CloseHandle(*pHandle);
		*pHandle = INVALID_HANDLE_VALUE;
	}
}

void ProcessWin::checkChildProcess(BOOL *child_exited, long *exit_code)
{
	DWORD process_exit_code;
	
	if(::GetExitCodeProcess(piProcInfo.hProcess, &process_exit_code))
	{
		if (process_exit_code!=STILL_ACTIVE){
			*child_exited = TRUE;
			*exit_code = (long)process_exit_code;
		}
	}
}

void ProcessWin::tryReadPipe(HANDLE fd, boost::function<void(const char *, size_t)> notify)
{
	DWORD dwRead;
	char chBuf[4096]; 
	BOOL bSuccess = FALSE;
	DWORD available = 0;
	
	for(;;)
	{
		ZeroMemory(chBuf, sizeof(chBuf));
		if(!PeekNamedPipe(fd, chBuf, sizeof(chBuf), NULL, &available, NULL)) // ReadFile will block if no data in pipe, so use PeekNamedPipe to determine whether there are data in pipe
			break;
		if(available<=0)
			break;
		bSuccess = ReadFile(fd, chBuf, sizeof(chBuf), &dwRead, NULL);
		if(!bSuccess || dwRead<=0)
			break;
		notify(chBuf, dwRead);
	}
}
#endif
