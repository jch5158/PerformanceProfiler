#include "stdafx.h"
#include "CPerformanceProfiler.h"

void TimeFunc()
{
	CPerformanceProfiler profiler(L"TimeFunc");

	Sleep(20);
}


int wmain()
{
	_wsetlocale(LC_ALL, L"");

	timeBeginPeriod(1);

	CPerformanceProfiler::SetPerformanceProfiler(3);

	for (int iCnt = 0; iCnt < 200; ++iCnt)
	{
		TimeFunc();
		printf_s("%d\n", iCnt + 1);
	}

	CPerformanceProfiler::PrintPerformance();

	CPerformanceProfiler::FreePerformanceProfiler();


	timeEndPeriod(1);
}
