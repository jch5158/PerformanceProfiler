#pragma once

#define FUNCTION_NAME_SIZE 50

#define FUNCTION_COUNT 100


class CPerformanceProfiler
{
public:
	CPerformanceProfiler(const WCHAR* functionName);

	~CPerformanceProfiler(void);

	friend void PrintPerformance(void);

	// 태그 성능 정보.
	struct stPerformanceInfo
	{
		// 최대로 걸린시간.
		long long max[2];

		// 최소로 걸린시간.
		long long min[2];

		// 시작 시간.
		LARGE_INTEGER startTime;

		// 호출 횟수.
		int callCount;

		// 함수의 누적 로직 시간
		long long totalTIme;

	};

private:	

	stPerformanceInfo* RegisterFunction(const WCHAR* funcName);

	void UpdateFunctionPerformance(stPerformanceInfo* performanceInfo);

	WCHAR *mFunctionName;
};

void PrintPerformance(void);

static WCHAR *functionNameList[FUNCTION_COUNT];

static int functionCout;

static std::unordered_map<WCHAR*, CPerformanceProfiler::stPerformanceInfo*> performanceInfoMap;