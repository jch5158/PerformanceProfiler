#include "stdafx.h"
#include "CPerformanceProfiler.h"



CPerformanceProfiler::CPerformanceProfiler(void)
{

}


CPerformanceProfiler::~CPerformanceProfiler(void)
{

	return;
}


void CPerformanceProfiler::StartPerformanceProfiling(const WCHAR* pFunctionName)
{
	if (findPerformanceInfo(pFunctionName) == nullptr)
	{
		setPerformanceInfo(pFunctionName);
	}

	mpFunctionName = (WCHAR*)pFunctionName;

	// ���� �ð� üũ
	QueryPerformanceCounter(&startTime);

	return;
}



void CPerformanceProfiler::EndPerformanceProfiling(void)
{
	LARGE_INTEGER endTime = { 0, };

	QueryPerformanceCounter(&endTime);

	stPerformanceInfo* performanceInfo = findPerformanceInfo((const WCHAR*)mpFunctionName);

	INT64 elapsedTime = endTime.QuadPart - startTime.QuadPart;

	// �ֻ��� �� 2��, ������ �� 2���� ��´�. �׸��� �ֻ��� 1�� ������ 1���� ���� ����
	for (INT count = 0; count < 2; ++count)
	{
		if (performanceInfo->maxTime[count] < elapsedTime)
		{
			performanceInfo->maxTime[count] = elapsedTime;
			break;
		}
		else if (performanceInfo->minTime[count] > elapsedTime)
		{
			performanceInfo->minTime[count] = elapsedTime;
			break;
		}
	}

	// ����� ���� Total ���� ���Ѵ�.
	performanceInfo->totalTime += elapsedTime;

	// ����� ���� �� Ƚ�� ī����
	performanceInfo->callCount += 1;

	return;
}



CPerformanceProfiler::stPerformanceInfo* CPerformanceProfiler::findPerformanceInfo(const WCHAR* pFunctionName)
{
	auto iterE = mPerformanceInfoMap.end();

	auto iter = mPerformanceInfoMap.find(pFunctionName);

	if (iter == iterE)
	{
		return nullptr;
	}

	return iter->second;
}

CPerformanceProfiler::stPerformanceInfo* CPerformanceProfiler::setPerformanceInfo(const WCHAR* pFunctionName)
{
	stPerformanceInfo* performanceInfo = new stPerformanceInfo;

	ZeroMemory(performanceInfo, sizeof(stPerformanceInfo));

	performanceInfo->minTime[0] = LLONG_MAX;
	performanceInfo->minTime[1] = LLONG_MAX;

	mPerformanceInfoMap.insert(std::pair<const WCHAR*, stPerformanceInfo*>(pFunctionName, performanceInfo));

	return performanceInfo;
}



// TLS �� ����ϸ��� ȣ���� �ʿ䰡 ����
BOOL CPerformanceProfiler::SetPerformanceProfiler(const WCHAR* pTitle)
{
	// strsafe ����ϴ���� nullptr�� Ȯ�� �ʿ��ϴ�.
	if (pTitle == nullptr)
	{
		return FALSE;
	}

	// ������ ���� �̸� ����		
	HRESULT retval;
	retval = StringCbCopyW(mTitle, performanceprofiler::titleLength, pTitle);
	if (FAILED(retval) == TRUE)
	{
		return FALSE;
	}

	// frequency ���� �����Ѵ�.
	QueryPerformanceFrequency(&mFrequencyTime);

	// �ѱ� �����ڵ� ����
	_wsetlocale(LC_ALL, L"");

	return TRUE;
}



void CPerformanceProfiler::FreePerformanceProfiler(void)
{	
	auto iterE = mPerformanceInfoMap.end();

	for (auto iter : mPerformanceInfoMap)
	{
		stPerformanceInfo* pPerformanceInfo = iter.second;

		delete pPerformanceInfo;
	}

	mPerformanceInfoMap.clear();

	return;
}


void CPerformanceProfiler::setLogTitle(WCHAR* pLogTitle)
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


BOOL CPerformanceProfiler::PrintPerformanceProfile(void)
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
	fwprintf_s(fp, L"Function,Average,Max,Min,Call \n");

	for (auto iter : mPerformanceInfoMap)
	{
		DOUBLE avgTime;

		DOUBLE maxTime;

		DOUBLE minTime;

		stPerformanceInfo* performanceInfo = iter.second;

		// �ּ� ���� 4�� �̻��̿��� �Ѵ�.
		if (performanceInfo->callCount <= 4)
		{
			continue;
		}

		// �ֻ���, ������ �� 1���� ��հ����� �����Ѵ�.
		performanceInfo->totalTime -= performanceInfo->maxTime[0];
		performanceInfo->totalTime -= performanceInfo->minTime[0];

		avgTime = (DOUBLE)performanceInfo->totalTime / (DOUBLE)(performanceInfo->callCount - 2) * performanceprofiler::MICRO_SECOND / mFrequencyTime.QuadPart;

		maxTime = (DOUBLE)performanceInfo->maxTime[1] * performanceprofiler::MICRO_SECOND / mFrequencyTime.QuadPart;
		minTime = (DOUBLE)performanceInfo->minTime[1] * performanceprofiler::MICRO_SECOND / mFrequencyTime.QuadPart;

		// ���� �ٽ� print�� ���ؼ� total ���� �ǵ��� ���´�. 
		performanceInfo->totalTime += performanceInfo->maxTime[0];
		performanceInfo->totalTime += performanceInfo->minTime[0];

		fwprintf_s(fp, L"%s,%.6lf��, %.6lf��, %.6lf��, %lld\n", iter.first, avgTime, maxTime, minTime, performanceInfo->callCount - 2);
	}

	fwprintf_s(fp, L"\n");

	// ���� �ݱ�.
	fclose(fp);

	return TRUE;
}
