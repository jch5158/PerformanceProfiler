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

	// �������ϸ� ������ ��� �����.
	void FreePerformanceProfiler(void);

	void StartPerformanceProfiling(const WCHAR* pFunctionName);

	void EndPerformanceProfiling(void);

private:

	// �±� ���� ����.
	struct stPerformanceInfo
	{	
		// ȣ�� Ƚ��.
		INT64 callCount;

		// �Լ��� ���� ���� �ð�
		// ��� ���� �ð��� ���� �� �����
		INT64 totalTime;

		// �ִ�� �ɸ��ð�.
		INT64 maxTime[2];

		// �ּҷ� �ɸ��ð�.
		INT64 minTime[2];
	};


	stPerformanceInfo* findPerformanceInfo(const WCHAR* pFunctionName);

	// ù ���������ϸ� �Լ��� ��� ������ �����ϱ� ���� ����ü�� �Ҵ�޴´�.
	stPerformanceInfo* setPerformanceInfo(const WCHAR* pFunctionName);

	void setLogTitle(WCHAR* pLogTitle);

	WCHAR* mpFunctionName;

	// ���� �ð�.
	LARGE_INTEGER startTime;

	LARGE_INTEGER mFrequencyTime = { 0, };

	WCHAR mTitle[performanceprofiler::titleLength] = { 0, };

	std::unordered_map<const WCHAR*, stPerformanceInfo*> mPerformanceInfoMap;

};
