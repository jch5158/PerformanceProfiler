#include "stdafx.h"
#include "CPerformanceProfiler.h"



CPerformanceProfiler::CPerformanceProfiler(const WCHAR* funcName)
{	
	stPerformanceInfo* performanceInfo;

	performanceInfo = RegisterFunction(funcName);	
	if (performanceInfo == nullptr)
	{
		return;
	}

	UpdateFunctionPerformance(performanceInfo);
}

CPerformanceProfiler::~CPerformanceProfiler(void)
{
	LARGE_INTEGER endTime;

	QueryPerformanceCounter(&endTime);

	stPerformanceInfo* performanceInfo = performanceInfoMap.find(mFunctionName)->second;

	long long logicTime;

	logicTime = endTime.QuadPart - performanceInfo->startTime.QuadPart;

	for (int iCnt = 0; iCnt < 2; ++iCnt)
	{
		if (performanceInfo->max[iCnt] < logicTime)
		{
			performanceInfo->max[iCnt] = logicTime;
			break;
		}

		if (performanceInfo->min[iCnt] > logicTime)
		{
			performanceInfo->min[iCnt] = logicTime;
			break;
		}
	}

	performanceInfo->totalTIme += logicTime;
}


CPerformanceProfiler::stPerformanceInfo* CPerformanceProfiler::RegisterFunction(const WCHAR* funcName)
{
	auto iterE = performanceInfoMap.end();

	mFunctionName = (WCHAR*)funcName;

	auto findIter = performanceInfoMap.find(mFunctionName);

	if (iterE != findIter)
	{
		return findIter->second;
	}
	
	stPerformanceInfo* performanceInfo = (stPerformanceInfo*)malloc(sizeof(stPerformanceInfo));

	memset(performanceInfo, 0, sizeof(stPerformanceInfo));

	QueryPerformanceCounter(&performanceInfo->startTime);

	performanceInfo->max[0] = 0;
	performanceInfo->max[1] = 0;
	performanceInfo->min[0] = LLONG_MAX;
	performanceInfo->min[1] = LLONG_MAX;

	functionNameList[functionCout] = mFunctionName;
	
	functionCout += 1;

	performanceInfo->callCount += 1;

	performanceInfoMap.insert(std::pair<WCHAR*, CPerformanceProfiler::stPerformanceInfo*>(mFunctionName, performanceInfo));
	
	return nullptr;
}

void CPerformanceProfiler::UpdateFunctionPerformance(stPerformanceInfo* performanceInfo)
{
	performanceInfo->callCount += 1;
	
	QueryPerformanceCounter(&performanceInfo->startTime);
}

void PrintPerformance(void)
{

	LARGE_INTEGER frequencyCount;

	QueryPerformanceFrequency(&frequencyCount);

	FILE* fp;

	fopen_s(&fp, "profiler.csv", "a+t");

	// 열 출력.
	fwprintf(fp, L"TagName,Average,Max,Min,Call \n");

	double avgTime;

	double max;
	double min;

	CPerformanceProfiler::stPerformanceInfo* performanceInfo;

	for (int iCnt = 0; iCnt < functionCout; ++iCnt)
	{
		performanceInfo = performanceInfoMap.find(functionNameList[iCnt])->second;

		// 최소 콜은 4개 이상이여야 한다.
		if (performanceInfo->callCount <= 4)
		{
			continue;
		}

		performanceInfo->callCount -= 4;

		for (int iCntM = 0; iCntM < 2; ++iCntM)
		{
			performanceInfo->totalTIme -= performanceInfo->max[iCntM];
			performanceInfo->totalTIme -= performanceInfo->min[iCntM];
		}

		avgTime = ((double)(performanceInfo->totalTIme / (double)performanceInfo->callCount)) / (double)frequencyCount.QuadPart;

		max = (double)performanceInfo->max[1] / (double)frequencyCount.QuadPart;
		min = (double)performanceInfo->min[1] / (double)frequencyCount.QuadPart;

		fwprintf(fp, L"%s,", functionNameList[iCnt]);

		fprintf(fp, "%lf㎲, %lf㎲, %lf㎲, %ld\n",avgTime, max, min, performanceInfo->callCount);

	}

	performanceInfoMap.clear();

	// 파일 닫기.
	fclose(fp);
}