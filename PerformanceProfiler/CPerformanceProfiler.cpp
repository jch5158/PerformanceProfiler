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

	// 시작 시간 체크
	QueryPerformanceCounter(&startTime);

	return;
}



void CPerformanceProfiler::EndPerformanceProfiling(void)
{
	LARGE_INTEGER endTime = { 0, };

	QueryPerformanceCounter(&endTime);

	stPerformanceInfo* performanceInfo = findPerformanceInfo((const WCHAR*)mpFunctionName);

	INT64 elapsedTime = endTime.QuadPart - startTime.QuadPart;

	// 최상위 값 2개, 최하위 값 2개를 얻는다. 그리고 최상위 1개 최하위 1개는 버릴 예정
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

	// 평균을 위한 Total 값을 구한다.
	performanceInfo->totalTime += elapsedTime;

	// 평균을 위한 콜 횟수 카운팅
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



// TLS 를 사용하면은 호출할 필요가 없음
BOOL CPerformanceProfiler::SetPerformanceProfiler(const WCHAR* pTitle)
{
	// strsafe 사용하더라고 nullptr은 확인 필요하다.
	if (pTitle == nullptr)
	{
		return FALSE;
	}

	// 저장할 파일 이름 셋팅		
	HRESULT retval;
	retval = StringCbCopyW(mTitle, performanceprofiler::titleLength, pTitle);
	if (FAILED(retval) == TRUE)
	{
		return FALSE;
	}

	// frequency 값을 저장한다.
	QueryPerformanceFrequency(&mFrequencyTime);

	// 한글 유니코드 셋팅
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

	// 시스템 클록에 따라 자정 (00:00:00), 1970 년 1 월 1 일 Utc (협정 세계시) 이후 경과 된 시간 (초)을 반환 합니다.
	_time64(&time64);

	// time_t 값으로 저장 된 시간을 변환 하 고 결과를 tm형식의 구조에 저장 합니다.
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

	// 열 출력.
	fwprintf_s(fp, L"Function,Average,Max,Min,Call \n");

	for (auto iter : mPerformanceInfoMap)
	{
		DOUBLE avgTime;

		DOUBLE maxTime;

		DOUBLE minTime;

		stPerformanceInfo* performanceInfo = iter.second;

		// 최소 콜은 4개 이상이여야 한다.
		if (performanceInfo->callCount <= 4)
		{
			continue;
		}

		// 최상위, 최하위 값 1개씩 평균값에서 제외한다.
		performanceInfo->totalTime -= performanceInfo->maxTime[0];
		performanceInfo->totalTime -= performanceInfo->minTime[0];

		avgTime = (DOUBLE)performanceInfo->totalTime / (DOUBLE)(performanceInfo->callCount - 2) * performanceprofiler::MICRO_SECOND / mFrequencyTime.QuadPart;

		maxTime = (DOUBLE)performanceInfo->maxTime[1] * performanceprofiler::MICRO_SECOND / mFrequencyTime.QuadPart;
		minTime = (DOUBLE)performanceInfo->minTime[1] * performanceprofiler::MICRO_SECOND / mFrequencyTime.QuadPart;

		// 추후 다시 print를 위해서 total 값은 되돌려 놓는다. 
		performanceInfo->totalTime += performanceInfo->maxTime[0];
		performanceInfo->totalTime += performanceInfo->minTime[0];

		fwprintf_s(fp, L"%s,%.6lf㎲, %.6lf㎲, %.6lf㎲, %lld\n", iter.first, avgTime, maxTime, minTime, performanceInfo->callCount - 2);
	}

	fwprintf_s(fp, L"\n");

	// 파일 닫기.
	fclose(fp);

	return TRUE;
}
