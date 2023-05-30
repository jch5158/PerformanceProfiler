#pragma once

#include <strsafe.h>
#include <iostream>
#include <Windows.h>
#include <time.h>
#include <locale.h>
#include <unordered_map>
#include <vector>
#include "SystemLog/SystemLog/CSystemLog.h"

#pragma comment(lib,"Winmm.lib")

namespace performanceprofiler
{
	constexpr DOUBLE MICRO_SECOND = 1000000.0f;

	constexpr DWORD titleLength = MAX_PATH;
}

class CTLSPerformanceProfiler;

class CPerformanceProfiler
{
public:

	friend class CTLSPerformanceProfiler;

	CPerformanceProfiler(void);

	~CPerformanceProfiler(void);

	BOOL PrintPerformanceProfile(void);

	BOOL SetPerformanceProfiler(const WCHAR* pTitle);

	// 프로파일링 정보를 모두 지운다.
	void FreePerformanceProfiler(void);

	void StartPerformanceProfiling(const WCHAR* pFunctionName);

	void EndPerformanceProfiling(void);

private:

	// 태그 성능 정보.
	struct stPerformanceInfo
	{	
		// 호출 횟수.
		INT64 callCount;

		// 함수의 누적 로직 시간
		// 평균 로직 시간을 구할 때 사용함
		INT64 totalTime;

		// 최대로 걸린시간.
		INT64 maxTime[2];

		// 최소로 걸린시간.
		INT64 minTime[2];
	};


	stPerformanceInfo* findPerformanceInfo(const WCHAR* pFunctionName);

	// 첫 프로파파일링 함수일 경우 정보를 저장하기 위한 구조체를 할당받는다.
	stPerformanceInfo* setPerformanceInfo(const WCHAR* pFunctionName);

	void setLogTitle(WCHAR* pLogTitle);

	WCHAR* mpFunctionName;

	// 시작 시간.
	LARGE_INTEGER startTime;

	LARGE_INTEGER mFrequencyTime = { 0, };

	WCHAR mTitle[performanceprofiler::titleLength] = { 0, };

	std::unordered_map<const WCHAR*, stPerformanceInfo*> mPerformanceInfoMap;

};
