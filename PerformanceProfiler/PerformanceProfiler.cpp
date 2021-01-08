#include "stdafx.h"
#include "CPerformanceProfiler.h"

void TimeFunc1()
{
	CPerformanceProfiler profiler(L"TimeFunc1");

	Sleep(20);
}

void TimeFunc2()
{
	CPerformanceProfiler profiler(L"TimeFunc2");

	Sleep(30);
}

unsigned __stdcall WorkerThread1(void* lpParam)
{
	for (int iCnt = 0; iCnt < 200; ++iCnt)
	{
		TimeFunc1();
		printf_s("%d\n", iCnt + 1);
	}

	return 0;
}

unsigned __stdcall WorkerThread2(void* lpParam)
{
	for (int iCnt = 0; iCnt < 200; ++iCnt)
	{
		TimeFunc2();
		printf_s("%d\n", iCnt + 1);

		if (iCnt == 99)
		{
			CPerformanceProfiler::PrintPerformance();
		}

	}

	return 0;
}



int wmain()
{
	_wsetlocale(LC_ALL, L"");

	timeBeginPeriod(1);

	HANDLE handle[2] = { 0, };

	CPerformanceProfiler::SetPerformanceProfiler(2);

	handle[0] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread1, 0, 0, 0);

	handle[1] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread2, 0, 0, 0);

	WaitForMultipleObjects(2, handle, true, INFINITE);

	CPerformanceProfiler::PrintPerformance();

	CPerformanceProfiler::FreePerformanceProfiler();


	timeEndPeriod(1);
}
