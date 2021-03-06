#include "CPerformanceProfiler.h"


//int CPerformanceProfiler::mTlsIndex = -1;
//
//WCHAR* CPerformanceProfiler::mTitle = nullptr;
//
//std::vector<CPerformanceProfiler::stThreadPerformanceSample*> CPerformanceProfiler::mThreadPerformanceSampleArray;
//

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

bool CPerformanceProfiler::SetPerformanceProfiler(const WCHAR* pTitle, int threadCount)
{
	if (setTlsIndex() == false)
	{
		return false;
	}

	mTitle = (WCHAR*)pTitle;

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


void CPerformanceProfiler::setLogTitle(WCHAR* pLogTitle)
{
	tm nowTime = { 0, };

	__time64_t time64 = NULL;

	errno_t retval;

	// 시스템 클록에 따라 자정 (00:00:00), 1970 년 1 월 1 일 Utc (협정 세계시) 이후 경과 된 시간 (초)을 반환 합니다.
	_time64(&time64);

	// time_t 값으로 저장 된 시간을 변환 하 고 결과를 tm형식의 구조에 저장 합니다.
	retval = _localtime64_s(&nowTime, &time64);

	StringCchPrintfW(pLogTitle, MAX_PATH, L"[%s Profile]_[%d-%02d-%02d_%02d-%02d-%02d].csv", mTitle,nowTime.tm_year + 1900, nowTime.tm_mon + 1, nowTime.tm_mday, nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);
	
	return;
}


bool CPerformanceProfiler::PrintPerformance(void)
{
	LARGE_INTEGER frequencyCount = { 0, };

	QueryPerformanceFrequency(&frequencyCount);

	WCHAR pLogTitle[MAX_PATH] = { 0, };

	setLogTitle(pLogTitle);

	FILE* fp = nullptr;

	_wfopen_s(&fp, pLogTitle, L"a+t");
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