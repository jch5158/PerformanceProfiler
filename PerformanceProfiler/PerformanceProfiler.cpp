#include "stdafx.h"
#include "CPerformanceProfiler.h"

void TimeFunc()
{
	CPerformanceProfiler profiler(L"TimeFunc");

	Sleep(200);
}


int wmain()
{
	timeBeginPeriod(1);


	for (int iCnt = 0; iCnt < 200; ++iCnt)
	{
		TimeFunc();
		printf_s("%d\n", iCnt + 1);
	}

	PrintPerformance();


	timeEndPeriod(1);
}
