#pragma once

#include "CPerformanceProfiler.h"

namespace tlsperformanceprofiler
{
	constexpr DWORD TITLE_LENGTH = MAX_PATH;

}

class CTLSPerformanceProfiler
{
public:

	CTLSPerformanceProfiler(const WCHAR* pFunctionName);

	~CTLSPerformanceProfiler(void);

	static BOOL SetPerformanceProfiler(const WCHAR* pTitle, DWORD threadCount);

	static BOOL FreePerformanceProfiler(void);

	static BOOL PrintPerformanceProfile(void);

	static BOOL PrinteTotalPerformanceProfile(void);


private:
	
	struct stThreadPerformanceInfo
	{
		DWORD threadID;

		CPerformanceProfiler performanceProfiler;
	};

	static BOOL setTLSIndex(void);

	static BOOL freeTLSIndex(void);

	static void setLogTitle(WCHAR* pLogTitle);

	static void setTotalLogTitle(WCHAR* pLogTitle);

	void startPerformanceProfiling(const WCHAR* pFunctionName);

	void endPerformanceProfiling(void);

	// SetPerformanceInfo 호출 여부 확인
	inline static BOOL mbPerformanceProfileSetupFlag = FALSE;

	inline static DWORD mTLSIndex = UINT_MAX;

	inline static LARGE_INTEGER mFrequencyTime = { 0, };

	inline static WCHAR mTitle[tlsperformanceprofiler::TITLE_LENGTH] = { 0, };

	inline static std::vector<stThreadPerformanceInfo*> mThreadPerformanceInfoArray;
};

