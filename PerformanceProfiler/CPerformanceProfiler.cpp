#include "stdafx.h"
#include "CPerformanceProfiler.h"


std::unordered_map<WCHAR*, CPerformanceProfiler::stPerformanceInfo*> CPerformanceProfiler::performanceInfoMap;

CPerformanceProfiler::CPerformanceProfiler(const WCHAR* funcName)
{
	stPerformanceInfo* performanceInfo;

	performanceInfo = findFunctionPerformance(funcName);
	if (performanceInfo == nullptr)
	{
		return;
	}

	updateFunctionPerformance(performanceInfo);
}

CPerformanceProfiler::~CPerformanceProfiler(void)
{
	LARGE_INTEGER endTime = { 0, };

	QueryPerformanceCounter(&endTime);

	stPerformanceInfo* performanceInfo = performanceInfoMap.find(mFunctionName)->second;

	long long logicTime = NULL;

	logicTime = endTime.QuadPart - performanceInfo->startTime.QuadPart;

	for (int iCnt = 0; iCnt < 2; ++iCnt)
	{
		if (performanceInfo->maxTime[iCnt] < logicTime)
		{
			performanceInfo->maxTime[iCnt] = logicTime;
			break;
		}

		if (performanceInfo->minTime[iCnt] > logicTime)
		{
			performanceInfo->minTime[iCnt] = logicTime;
			break;
		}
	}

	performanceInfo->totalTIme += logicTime;
}


CPerformanceProfiler::stPerformanceInfo* CPerformanceProfiler::findFunctionPerformance(const WCHAR* funcName)
{
	auto iterE = performanceInfoMap.end();

	mFunctionName = (WCHAR*)funcName;

	auto findIter = performanceInfoMap.find(mFunctionName);

	if (iterE != findIter)
	{
		return findIter->second;
	}

	stPerformanceInfo* performanceInfo = (stPerformanceInfo*)malloc(sizeof(stPerformanceInfo));

	ZeroMemory(performanceInfo, sizeof(stPerformanceInfo));

	performanceInfo->maxTime[0] = 0;
	performanceInfo->maxTime[1] = 0;
	performanceInfo->minTime[0] = LLONG_MAX;
	performanceInfo->minTime[1] = LLONG_MAX;

	performanceInfo->callCount += 1;

	performanceInfoMap.insert(std::pair<WCHAR*, CPerformanceProfiler::stPerformanceInfo*>(mFunctionName, performanceInfo));

	QueryPerformanceCounter(&performanceInfo->startTime);

	return nullptr;
}

void CPerformanceProfiler::updateFunctionPerformance(stPerformanceInfo* performanceInfo)
{
	performanceInfo->callCount += 1;

	QueryPerformanceCounter(&performanceInfo->startTime);
}

bool CPerformanceProfiler::PrintPerformance(void)
{
	LARGE_INTEGER frequencyCount = { 0, };

	QueryPerformanceFrequency(&frequencyCount);

	FILE* fp = nullptr;

	_wfopen_s(&fp, L"profiler.csv", L"a+t");
	if (fp == nullptr)
	{
		return false;
	}

	// 열 출력.
	fwprintf_s(fp, L"FunctionName,Average,Max,Min,Call \n");

	double avgTime = NULL;

	double maxTime = NULL;
	double minTime = NULL;

	CPerformanceProfiler::stPerformanceInfo* performanceInfo = nullptr;

	auto iterE = performanceInfoMap.end();
	for (auto iter = performanceInfoMap.begin(); iter != iterE; ++iter)
	{
		performanceInfo = iter->second;

		// 최소 콜은 4개 이상이여야 한다.
		if (performanceInfo->callCount <= 4)
		{
			continue;
		}

		performanceInfo->callCount -= 4;

		for (int iCntM = 0; iCntM < 2; ++iCntM)
		{
			performanceInfo->totalTIme -= performanceInfo->maxTime[iCntM];
			performanceInfo->totalTIme -= performanceInfo->minTime[iCntM];
		}

		avgTime = ((double)(performanceInfo->totalTIme / (double)performanceInfo->callCount)) / (double)frequencyCount.QuadPart;

		maxTime = (double)performanceInfo->maxTime[1] / (double)frequencyCount.QuadPart;
		minTime = (double)performanceInfo->minTime[1] / (double)frequencyCount.QuadPart;

		fwprintf_s(fp, L"%s,", iter->first);

		fwprintf_s(fp, L"%.7lf㎲, %.7lf㎲, %.7lf㎲, %lld\n", avgTime, maxTime, minTime, performanceInfo->callCount);

		free(iter->second);
	}

	performanceInfoMap.clear();

	// 파일 닫기.
	fclose(fp);

	return true;
}