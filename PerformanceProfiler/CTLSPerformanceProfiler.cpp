#include "stdafx.h"
#include "CTLSPerformanceProfiler.h"


CTLSPerformanceProfiler::CTLSPerformanceProfiler(const WCHAR* pFunctionName)
{	
	startPerformanceProfiling(pFunctionName);
}

CTLSPerformanceProfiler::~CTLSPerformanceProfiler(void)
{
	endPerformanceProfiling();
}


// TLS index와 vector 사이즈를 미리 셋팅한다.
BOOL CTLSPerformanceProfiler::SetPerformanceProfiler(const WCHAR* pTitle, DWORD threadCount)
{
	if (pTitle == nullptr)
	{
		return FALSE;
	}
	
	if (setTLSIndex() == FALSE)
	{
		return FALSE;
	}

	HRESULT retval = StringCbCopyW(mTitle, tlsperformanceprofiler::TITLE_LENGTH, pTitle);
	if (FAILED(retval) == TRUE)
	{
		return FALSE;
	}	
	
	QueryPerformanceFrequency(&mFrequencyTime);

	// 한글 유니코드 셋팅
	_wsetlocale(LC_ALL, L"");

	// 벡터 사이즈 초기화 resize는 0으로 초기화한다.
	// resize로 초기화 하지 않으면은 vector에 [] 방식으로 인자를 추가할 수 없다.
	mThreadPerformanceInfoArray.resize(threadCount);

	mbPerformanceProfileSetupFlag = TRUE;

	return TRUE;
}


BOOL CTLSPerformanceProfiler::FreePerformanceProfiler(void)
{
	if (freeTLSIndex() == FALSE)
	{
		return FALSE;
	}

	for (auto iter : mThreadPerformanceInfoArray)
	{
		if (iter == nullptr)
			break;

		iter->performanceProfiler.FreePerformanceProfiler();

		delete iter;
	}

	mThreadPerformanceInfoArray.clear();

	return TRUE;
}

BOOL CTLSPerformanceProfiler::PrintPerformanceProfile(void)
{
	WCHAR pLogTitle[MAX_PATH] = { 0, };

	setLogTitle(pLogTitle);

	FILE* fp = nullptr;

	_wfopen_s(&fp, pLogTitle, L"a+t");
	if (fp == nullptr)
	{
		return FALSE;
	}

	// 열 출력.
	fwprintf_s(fp, L"Thread ID,Function,Average,Max,Min,Call \n");

	for (auto* pThreadPerformanceInfo : mThreadPerformanceInfoArray)
	{
		if (pThreadPerformanceInfo == nullptr)
		{
			break;
		}

		auto pPerformanceInfoMap = pThreadPerformanceInfo->performanceProfiler.mPerformanceInfoMap;

		for (auto iter : pPerformanceInfoMap)
		{
			DOUBLE avgTime;

			DOUBLE maxTime;

			DOUBLE minTime;
			
			auto* pPerformanceInfo = iter.second;

			// 최소 콜은 4개 이상이여야 한다.
			if (pPerformanceInfo->callCount <= 4)
			{
				continue;
			}

			// 최상위, 최하위 값 1개씩 평균값에서 제외한다.
			pPerformanceInfo->totalTime -= pPerformanceInfo->maxTime[0];
			pPerformanceInfo->totalTime -= pPerformanceInfo->minTime[0];

			avgTime = (DOUBLE)pPerformanceInfo->totalTime / (DOUBLE)(pPerformanceInfo->callCount - 2) * performanceprofiler::MICRO_SECOND / mFrequencyTime.QuadPart;

			maxTime = (DOUBLE)pPerformanceInfo->maxTime[1] * performanceprofiler::MICRO_SECOND / mFrequencyTime.QuadPart;
			minTime = (DOUBLE)pPerformanceInfo->minTime[1] * performanceprofiler::MICRO_SECOND / mFrequencyTime.QuadPart;

			// 추후 다시 print를 위해서 total 값은 되돌려 놓는다. 
			pPerformanceInfo->totalTime += pPerformanceInfo->maxTime[0];
			pPerformanceInfo->totalTime += pPerformanceInfo->minTime[0];

			fwprintf_s(fp, L"%d,%s,%.6lf㎲, %.6lf㎲, %.6lf㎲, %lld\n", pThreadPerformanceInfo->threadID, iter.first, avgTime, maxTime, minTime, pPerformanceInfo->callCount - 2);
		}
	}

	fwprintf_s(fp, L"\n");

	// 파일 닫기.
	fclose(fp);

	return TRUE;
}

BOOL CTLSPerformanceProfiler::PrinteTotalPerformanceProfile(void)
{
	WCHAR pLogTitle[MAX_PATH] = { 0, };

	setTotalLogTitle(pLogTitle);

	FILE* fp = nullptr;

	_wfopen_s(&fp, pLogTitle, L"a+t");
	if (fp == nullptr)
	{
		return FALSE;
	}

	INT64 frequencyTime = mFrequencyTime.QuadPart;

	// 프로파일링 셈플을 합산 출력하기 위함
	std::unordered_map<const WCHAR*, CPerformanceProfiler::stPerformanceInfo*> totalProfileMap;

	// 열 출력.
	fwprintf_s(fp, L"FunctionName,Average,Call \n");	

	for (auto pThreadPerformanceInfo : mThreadPerformanceInfoArray)
	{
		if (pThreadPerformanceInfo == nullptr)
		{
			break;
		}

		CPerformanceProfiler::stPerformanceInfo* pPerformanceInfo;

		CPerformanceProfiler::stPerformanceInfo* pTotalPerformanceInfo;

		auto pPerformanceInfoMap = pThreadPerformanceInfo->performanceProfiler.mPerformanceInfoMap;

		for (auto performanceIter : pPerformanceInfoMap)
		{
			auto totalPerformanceIter = totalProfileMap.find(performanceIter.first);

			if (totalPerformanceIter == totalProfileMap.end())
			{
				pTotalPerformanceInfo = new CPerformanceProfiler::stPerformanceInfo;

				ZeroMemory(pTotalPerformanceInfo, sizeof(CPerformanceProfiler::stPerformanceInfo));

				totalProfileMap.insert(std::make_pair(performanceIter.first, pTotalPerformanceInfo));
			}
			else
			{
				pTotalPerformanceInfo = totalPerformanceIter->second;
			}

			pPerformanceInfo = performanceIter.second;

			// 최소 콜은 4개 이상이여야 한다.
			if (pPerformanceInfo->callCount <= 4)
			{
				continue;
			}

			pTotalPerformanceInfo->callCount += pPerformanceInfo->callCount - 2;

			// min, max 는 버린다.
			pTotalPerformanceInfo->totalTime += pPerformanceInfo->totalTime - pPerformanceInfo->maxTime[0] - pPerformanceInfo->minTime[0];
		}
	}

	for (auto iter : totalProfileMap)
	{
		fwprintf_s(fp, L"%s,%.6lf㎲, %lld\n", iter.first, (DOUBLE)iter.second->totalTime * performanceprofiler::MICRO_SECOND / (DOUBLE)iter.second->callCount / frequencyTime, iter.second->callCount);

		delete iter.second;
	}

	fwprintf_s(fp, L"\n");

	// 파일 닫기.
	fclose(fp);

	// 프로파일링 셈플 합산 출력 unordered_map clear
	totalProfileMap.clear();

	return TRUE;
}




BOOL CTLSPerformanceProfiler::setTLSIndex(void)
{
	// 이미 TLS index를 셋팅했는데 다시 셋팅하려하면 return flase
	if (mTLSIndex != UINT_MAX)
	{
		return FALSE;
	}

	mTLSIndex = TlsAlloc();
	if (mTLSIndex == TLS_OUT_OF_INDEXES)
	{
		wprintf(L"TlsAlloc() Error Value : %d\n", GetLastError());

		CCrashDump::Crash();
	}

	return TRUE;
}


BOOL CTLSPerformanceProfiler::freeTLSIndex(void)
{
	// 아직 TLS를 셋팅하지 않았는데 호출할 경우 return FALSE
	if (mTLSIndex == UINT_MAX)
	{
		CSystemLog::GetInstance()->Log(FALSE, CSystemLog::eLogLevel::LogLevelError, L"TLSPerformanceProfiler", L"[freeTLSIndex] TLS index Error");

		return FALSE;
	}

	if (TlsFree(mTLSIndex) == FALSE)
	{
		CSystemLog::GetInstance()->Log(FALSE, CSystemLog::eLogLevel::LogLevelError, L"TLSPerformanceProfiler", L"[freeTLSIndex] TLS index Free Error");

		return FALSE;
	}

	mTLSIndex = UINT_MAX;

	return TRUE;
}




void CTLSPerformanceProfiler::setLogTitle(WCHAR* pLogTitle)
{
	tm nowTime = { 0, };

	INT64 time64 = NULL;

	// 시스템 클록에 따라 자정 (00:00:00), 1970 년 1 월 1 일 Utc (협정 세계시) 이후 경과 된 시간 (초)을 반환 합니다.
	_time64(&time64);

	// time_t 값으로 저장 된 시간을 변환 하 고 결과를 tm형식의 구조에 저장 합니다.
	_localtime64_s(&nowTime, &time64);

	StringCchPrintfW(pLogTitle, MAX_PATH, L"[%s Profile]_[%d-%02d-%02d_%02d-%02d-%02d].csv", mTitle, nowTime.tm_year + 1900, nowTime.tm_mon + 1, nowTime.tm_mday, nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);

	return;
}

void CTLSPerformanceProfiler::setTotalLogTitle(WCHAR* pLogTitle)
{
	tm nowTime = { 0, };

	INT64 time64 = NULL;

	// 시스템 클록에 따라 자정 (00:00:00), 1970 년 1 월 1 일 Utc (협정 세계시) 이후 경과 된 시간 (초)을 반환 합니다.
	_time64(&time64);

	// time_t 값으로 저장 된 시간을 변환 하 고 결과를 tm형식의 구조에 저장 합니다.
	_localtime64_s(&nowTime, &time64);

	StringCchPrintfW(pLogTitle, MAX_PATH, L"[%s Total Profile]_[%d-%02d-%02d_%02d-%02d-%02d].csv", mTitle, nowTime.tm_year + 1900, nowTime.tm_mon + 1, nowTime.tm_mday, nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);

	return;
}

void CTLSPerformanceProfiler::startPerformanceProfiling(const WCHAR* pFunctionName)
{
	if (mbPerformanceProfileSetupFlag == FALSE)
	{
		CSystemLog::GetInstance()->Log(FALSE, CSystemLog::eLogLevel::LogLevelError, L"TLSPerformanceProfiler", L"[startPerformanceProfiling] mbPerformanceProfileSetupFlag is FALSE");

		CCrashDump::Crash();

		return;
	}

	stThreadPerformanceInfo* pThreadPerformanceInfo = (stThreadPerformanceInfo*)TlsGetValue(mTLSIndex);
	if (pThreadPerformanceInfo == nullptr)
	{
		static LONG threadInfoIndex = -1;

		pThreadPerformanceInfo = new stThreadPerformanceInfo;

		pThreadPerformanceInfo->threadID = GetCurrentThreadId();

		mThreadPerformanceInfoArray[InterlockedIncrement(&threadInfoIndex)] = pThreadPerformanceInfo;

		TlsSetValue(mTLSIndex, (LPVOID)pThreadPerformanceInfo);
	}

	pThreadPerformanceInfo->performanceProfiler.StartPerformanceProfiling(pFunctionName);

	return;
}

void CTLSPerformanceProfiler::endPerformanceProfiling(void)
{
	stThreadPerformanceInfo* pThreadPerformanceInfo = (stThreadPerformanceInfo*)TlsGetValue(mTLSIndex);
	if (pThreadPerformanceInfo == nullptr)
	{
		CSystemLog::GetInstance()->Log(FALSE, CSystemLog::eLogLevel::LogLevelError, L"TLSPerformanceProfiler", L"[endPerformanceProfiling] pThreadPerformanceInfo is nullptr");

		CCrashDump::Crash();

		return;
	}

	pThreadPerformanceInfo->performanceProfiler.EndPerformanceProfiling();

	return;
}

