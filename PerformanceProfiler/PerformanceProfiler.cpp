#include "CPerformanceProfiler.h"
#include "CTLSPerformanceProfiler.h"
#include <locale.h>
#include <process.h>

constexpr DWORD THREAD_COUNT = 10;

HANDLE gEvent;

void TimeFunc1(void)
{
	CTLSPerformanceProfiler profiler(L"TimeFunc1");

	Sleep(20);
}

void TimeFunc2(void)
{
	CTLSPerformanceProfiler profiler(L"TimeFunc2");

	Sleep(30);
}

DWORD WINAPI WorkerThread1(void* lpParam)
{
	if (WaitForSingleObject(gEvent, INFINITE) != WAIT_OBJECT_0)
	{
		CCrashDump::Crash();
	}

	for (INT count = 0; count < 200; ++count)
	{
		if (count == 100)
		{
			CTLSPerformanceProfiler::PrintPerformanceProfile();

			CTLSPerformanceProfiler::PrinteTotalPerformanceProfile();
		}

		TimeFunc1();
	}

	wprintf_s(L"WorekrThread 1 Finish\n");

	return 0;
}

DWORD WINAPI WorkerThread2(void* lpParam)
{
	if (WaitForSingleObject(gEvent, INFINITE) != WAIT_OBJECT_0)
	{
		CCrashDump::Crash();
	}

	for (INT count = 0; count < 200; ++count)
	{
		if (count == 120)
		{
			CTLSPerformanceProfiler::PrintPerformanceProfile();

			CTLSPerformanceProfiler::PrinteTotalPerformanceProfile();
		}


		TimeFunc2();
	}

	wprintf_s(L"WorekrThread 2 Finish\n");

	return 0;
}


struct stInt
{
	LONG low;
	LONG high;
};


int wmain()
{
	_wsetlocale(LC_ALL, L"");

	timeBeginPeriod(1);

	CCrashDump::GetInstance();

	gEvent = CreateEvent(NULL, TRUE, FALSE, nullptr);

	HANDLE handle[THREAD_COUNT] = { 0, };

	CTLSPerformanceProfiler::SetPerformanceProfiler(L"Test", 10);

	for (INT index = 0; index < THREAD_COUNT / 2; ++index)
	{
		handle[index] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)WorkerThread1, 0, 0, 0);
	}

	for (INT index = THREAD_COUNT / 2; index < THREAD_COUNT; ++index)
	{
		handle[index] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)WorkerThread2, 0, 0, 0);
	}

	Sleep(1000);

	SetEvent(gEvent);

	WaitForMultipleObjects(THREAD_COUNT, handle, TRUE, INFINITE);

	CTLSPerformanceProfiler::PrintPerformanceProfile();

	CTLSPerformanceProfiler::PrinteTotalPerformanceProfile();

	CTLSPerformanceProfiler::FreePerformanceProfiler();

	timeEndPeriod(1);

	return 1;
}
