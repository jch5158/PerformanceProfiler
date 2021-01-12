#include "stdafx.h"
#include "CPerformanceProfiler.h"



int CPerformanceProfiler::mTlsIndex = -1;

std::vector<CPerformanceProfiler::stThreadPerformanceSample*> CPerformanceProfiler::mThreadPerformanceSampleArray;


CPerformanceProfiler::CPerformanceProfiler(const WCHAR* funcName)
{
	stPerformanceInfo* performanceInfo = nullptr;

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

	stThreadPerformanceSample* pThreadPerformanceSample = (stThreadPerformanceSample*)TlsGetValue(mTlsIndex);

	std::unordered_map<WCHAR*, CPerformanceProfiler::stPerformanceInfo*>* pPerformanceInfoMap = nullptr;

	pPerformanceInfoMap = &pThreadPerformanceSample->mPerformanceInfoMap;

	stPerformanceInfo* performanceInfo = pPerformanceInfoMap->find(mFunctionName)->second;

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

	performanceInfo->callCount += 1;

	ReleaseSRWLockExclusive(&pThreadPerformanceSample->mSrwLock);
}


CPerformanceProfiler::stPerformanceInfo* CPerformanceProfiler::findFunctionPerformance(const WCHAR* funcName)
{
	stThreadPerformanceSample* pThreadPerformanceSample = nullptr;
		
	pThreadPerformanceSample = (stThreadPerformanceSample*)TlsGetValue(mTlsIndex);	
	if (pThreadPerformanceSample == nullptr)
	{
		static long threadIndex = -1;

		pThreadPerformanceSample = new stThreadPerformanceSample;

		pThreadPerformanceSample->mThreadId = GetCurrentThreadId();

		InitializeSRWLock(&pThreadPerformanceSample->mSrwLock);
	
		mThreadPerformanceSampleArray[InterlockedIncrement(&threadIndex)] = pThreadPerformanceSample;

		TlsSetValue(mTlsIndex, (LPVOID*)pThreadPerformanceSample);
	}

	AcquireSRWLockExclusive(&pThreadPerformanceSample->mSrwLock);

	std::unordered_map<WCHAR*, CPerformanceProfiler::stPerformanceInfo*> *pPerformanceInfoMap = nullptr;
	pPerformanceInfoMap = &pThreadPerformanceSample->mPerformanceInfoMap;

	auto iterE = pPerformanceInfoMap->end();

	mFunctionName = (WCHAR*)funcName;

	auto findIter = pPerformanceInfoMap->find(mFunctionName);

	if (iterE != findIter)
	{
		return findIter->second;
	}

	stPerformanceInfo* performanceInfo = new stPerformanceInfo;

	ZeroMemory(performanceInfo, sizeof(stPerformanceInfo));

	performanceInfo->maxTime[0] = 0;
	performanceInfo->maxTime[1] = 0;
	performanceInfo->minTime[0] = LLONG_MAX;
	performanceInfo->minTime[1] = LLONG_MAX;

	pPerformanceInfoMap->insert(std::pair<WCHAR*, CPerformanceProfiler::stPerformanceInfo*>(mFunctionName, performanceInfo));

	QueryPerformanceCounter(&performanceInfo->startTime);

	return nullptr;
}

void CPerformanceProfiler::updateFunctionPerformance(stPerformanceInfo* performanceInfo)
{
	QueryPerformanceCounter(&performanceInfo->startTime);
}




bool CPerformanceProfiler::setTlsIndex(void)
{
	if (mTlsIndex != -1)
	{
		return false;
	}

	mTlsIndex = TlsAlloc();
	if (mTlsIndex == TLS_OUT_OF_INDEXES)
	{
		wprintf(L"TlsAlloc() Error Value : %d\n", GetLastError());

		return false;
	}

	return true;
}

bool CPerformanceProfiler::SetPerformanceProfiler(int threadCount)
{
	if (setTlsIndex() == false)
	{
		return false;
	}

	mThreadPerformanceSampleArray.resize(threadCount);

	return true;
}


bool CPerformanceProfiler::freeTlsIndex(void)
{

	if (mTlsIndex == -1)
	{
		return false;
	}

	if (TlsFree(mTlsIndex) == false)
	{
		wprintf(L"TlsFree() Error Value : %d\n", GetLastError());

		return false;
	}

	return true;
}


bool CPerformanceProfiler::FreePerformanceProfiler()
{
	if (freeTlsIndex() == false)
	{
		return false;
	}

	auto threadIterE = mThreadPerformanceSampleArray.end();

	for(auto threadIter = mThreadPerformanceSampleArray.begin(); threadIter != threadIterE; ++threadIter)
	{
		if (*threadIter == nullptr)
		{
			break;
		}

		std::unordered_map<WCHAR*, CPerformanceProfiler::stPerformanceInfo*>* pPerformanceInfoMap = &(*threadIter)->mPerformanceInfoMap;

		auto iterE = pPerformanceInfoMap->end();

		for (auto iter = pPerformanceInfoMap->begin(); iter != iterE;)
		{
			delete iter->second;

			iter = pPerformanceInfoMap->erase(iter);
		}	

		delete *threadIter;
	}

	mThreadPerformanceSampleArray.clear();

	return true;
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
	fwprintf_s(fp, L"Thread ID,FunctionName,Average,Max,Min,Call \n");

	double avgTime = NULL;

	double maxTime = NULL;
	
	double minTime = NULL;

	stPerformanceInfo* performanceInfo = nullptr;

	for (stThreadPerformanceSample* pThreadPerformanceSample : mThreadPerformanceSampleArray)
	{
		if (pThreadPerformanceSample == nullptr)
		{
			break;
		}

		AcquireSRWLockExclusive(&pThreadPerformanceSample->mSrwLock);

		for (auto iter : pThreadPerformanceSample->mPerformanceInfoMap)
		{
			performanceInfo = iter.second;

			// 최소 콜은 4개 이상이여야 한다.
			if (performanceInfo->callCount <= 4)
			{
				continue;
			}
		
			performanceInfo->totalTIme -= performanceInfo->maxTime[0];
			performanceInfo->totalTIme -= performanceInfo->minTime[0];


			avgTime = ((double)(performanceInfo->totalTIme / (double)performanceInfo->callCount-2)) / (double)frequencyCount.QuadPart;

			maxTime = (double)performanceInfo->maxTime[1] / (double)frequencyCount.QuadPart;
			minTime = (double)performanceInfo->minTime[1] / (double)frequencyCount.QuadPart;


			fwprintf_s(fp, L"%d, %s,%.6lf㎲, %.6lf㎲, %.6lf㎲, %lld\n", pThreadPerformanceSample->mThreadId, iter.first, avgTime, maxTime, minTime, performanceInfo->callCount-2);
		}

		fwprintf_s(fp, L"\n");

		ReleaseSRWLockExclusive(&pThreadPerformanceSample->mSrwLock);
	}

	// 파일 닫기.
	fclose(fp);

	return true;
}