#include "stdafx.h"
#include "CPerformanceProfiler.h"

void TimeFunc()
{
	CPerformanceProfiler profiler(L"TimeFunc");

	Sleep(200);
}

void TimeFunc2()
{
	CPerformanceProfiler profiler(L"TimeFunc2");

	Sleep(100);
}


int wmain()
{
	timeBeginPeriod(1);


	for (int iCnt = 0; iCnt < 200; ++iCnt)
	{
		TimeFunc();
		TimeFunc2();
		printf_s("%d\n", iCnt + 1);
	}

	PrintPerformance();


	timeEndPeriod(1);
}
