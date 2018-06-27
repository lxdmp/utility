#include "DebugHelper.h"

#if (defined(_WIN32) || defined(_WIN64))

#include <windows.h>
#include <DbgHelp.h>
#include <direct.h>
#pragma comment(lib, "dbghelp.lib")

static bool isDataSectionNeeded(const WCHAR* pModuleName)
{
	if (pModuleName == 0)
		return false;

	WCHAR szFileName[_MAX_FNAME] = L"";
	_wsplitpath(pModuleName, NULL, NULL, szFileName, NULL);

	if(wcsicmp(szFileName, L"ntdll")==0)
		return true;
	return false;
}

static BOOL CALLBACK miniDumpCallback(
	PVOID                            pParam,
	const PMINIDUMP_CALLBACK_INPUT   pInput,
	PMINIDUMP_CALLBACK_OUTPUT        pOutput
)
{
	if(pInput == 0 || pOutput == 0)
		return FALSE;
	switch (pInput->CallbackType)
	{
	case ModuleCallback:
		if(pOutput->ModuleWriteFlags & ModuleWriteDataSeg)
		{
			if (!isDataSectionNeeded(pInput->Module.FullPath))
				pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg);
		}
		// fall through
	case IncludeModuleCallback:
	case IncludeThreadCallback:
	case ThreadCallback:
	case ThreadExCallback:
		return TRUE;
	default:;
	}
	return FALSE;
}

static void createMiniDump(EXCEPTION_POINTERS* pep, LPCSTR filename)
{
	HANDLE hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		MINIDUMP_EXCEPTION_INFORMATION mdei;
		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pep;
		mdei.ClientPointers = FALSE;
		MINIDUMP_CALLBACK_INFORMATION mci;
		mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)miniDumpCallback;
		mci.CallbackParam = 0;
		MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory |
			MiniDumpWithDataSegs |
			MiniDumpWithHandleData |
			0x00000800 |
			0x00001000 |
			MiniDumpWithUnloadedModules);
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
			hFile, mdt, (pep != 0) ? &mdei : 0, 0, &mci);
		CloseHandle(hFile);
	}
}

LONG __stdcall customUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
{
	char crashFile[_MAX_FNAME];
	memset(crashFile, 0, sizeof(crashFile));
	getcwd(crashFile, sizeof(crashFile));
	strcat(crashFile, "\\crash_file.dmp");
	createMiniDump(pExceptionInfo, crashFile);
	return EXCEPTION_EXECUTE_HANDLER;
}
#else

#endif

void initDebugHandler()
{
#if (defined(_WIN32) || defined(_WIN64))
	SetUnhandledExceptionFilter(customUnhandledExceptionFilter);
#else
#endif
}
