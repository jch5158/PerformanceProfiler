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


// TLS index�� vector ����� �̸� �����Ѵ�.
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

	// �ѱ� �����ڵ� ����
	_wsetlocale(LC_ALL, L"");

	// ���� ������ �ʱ�ȭ resize�� 0���� �ʱ�ȭ�Ѵ�.
	// resize�� �ʱ�ȭ ���� �������� vector�� [] ������� ���ڸ� �߰��� �� ����.
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

	// �� ���.
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

			// �ּ� ���� 4�� �̻��̿��� �Ѵ�.
			if (pPerformanceInfo->callCount <= 4)
			{
				continue;
			}

			// �ֻ���, ������ �� 1���� ��հ����� �����Ѵ�.
			pPerformanceInfo->totalTime -= pPerformanceInfo->maxTime[0];
			pPerformanceInfo->totalTime -= pPerformanceInfo->minTime[0];

			avgTime = (DOUBLE)pPerformanceInfo->totalTime / (DOUBLE)(pPerformanceInfo->callCount - 2) * performanceprofiler::MICRO_SECOND / mFrequencyTime.QuadPart;

			maxTime = (DOUBLE)pPerformanceInfo->maxTime[1] * performanceprofiler::MICRO_SECOND / mFrequencyTime.QuadPart;
			minTime = (DOUBLE)pPerformanceInfo->minTime[1] * performanceprofiler::MICRO_SECOND / mFrequencyTime.QuadPart;

			// ���� �ٽ� print�� ���ؼ� total ���� �ǵ��� ���´�. 
			pPerformanceInfo->totalTime += pPerformanceInfo->maxTime[0];
			pPerformanceInfo->totalTime += pPerformanceInfo->minTime[0];

			fwprintf_s(fp, L"%d,%s,%.6lf��, %.6lf��, %.6lf��, %lld\n", pThreadPerformanceInfo->threadID, iter.first, avgTime, maxTime, minTime, pPerformanceInfo->callCount - 2);
		}
	}

	fwprintf_s(fp, L"\n");

	// ���� �ݱ�.
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

	// �������ϸ� ������ �ջ� ����ϱ� ����
	std::unordered_map<const WCHAR*, CPerformanceProfiler::stPerformanceInfo*> totalProfileMap;

	// �� ���.
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

			// �ּ� ���� 4�� �̻��̿��� �Ѵ�.
			if (pPerformanceInfo->callCount <= 4)
			{
				continue;
			}

			pTotalPerformanceInfo->callCount += pPerformanceInfo->callCount - 2;

			// min, max �� ������.
			pTotalPerformanceInfo->totalTime += pPerformanceInfo->totalTime - pPerformanceInfo->maxTime[0] - pPerformanceInfo->minTime[0];
		}
	}

	for (auto iter : totalProfileMap)
	{
		fwprintf_s(fp, L"%s,%.6lf��, %lld\n", iter.first, (DOUBLE)iter.second->totalTime * performanceprofiler::MICRO_SECOND / (DOUBLE)iter.second->callCount / frequencyTime, iter.second->callCount);

		delete iter.second;
	}

	fwprintf_s(fp, L"\n");

	// ���� �ݱ�.
	fclose(fp);

	// �������ϸ� ���� �ջ� ��� unordered_map clear
	totalProfileMap.clear();

	return TRUE;
}




BOOL CTLSPerformanceProfiler::setTLSIndex(void)
{
	// �̹� TLS index�� �����ߴµ� �ٽ� �����Ϸ��ϸ� return flase
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
	// ���� TLS�� �������� �ʾҴµ� ȣ���� ��� return FALSE
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

	// �ý��� Ŭ�Ͽ� ���� ���� (00:00:00), 1970 �� 1 �� 1 �� Utc (���� �����) ���� ��� �� �ð� (��)�� ��ȯ �մϴ�.
	_time64(&time64);

	// time_t ������ ���� �� �ð��� ��ȯ �� �� ����� tm������ ������ ���� �մϴ�.
	_localtime64_s(&nowTime, &time64);

	StringCchPrintfW(pLogTitle, MAX_PATH, L"[%s Profile]_[%d-%02d-%02d_%02d-%02d-%02d].csv", mTitle, nowTime.tm_year + 1900, nowTime.tm_mon + 1, nowTime.tm_mday, nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);

	return;
}

void CTLSPerformanceProfiler::setTotalLogTitle(WCHAR* pLogTitle)
{
	tm nowTime = { 0, };

	INT64 time64 = NULL;

	// �ý��� Ŭ�Ͽ� ���� ���� (00:00:00), 1970 �� 1 �� 1 �� Utc (���� �����) ���� ��� �� �ð� (��)�� ��ȯ �մϴ�.
	_time64(&time64);

	// time_t ������ ���� �� �ð��� ��ȯ �� �� ����� tm������ ������ ���� �մϴ�.
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

