#pragma once

//#define FUNCTION_NAME_SIZE 50

#define FUNCTION_COUNT 100


class CPerformanceProfiler
{
public:
	CPerformanceProfiler(const WCHAR* functionName);

	~CPerformanceProfiler(void);

	static bool PrintPerformance(void);

	// 태그 성능 정보.
	struct stPerformanceInfo
	{

		// 최대로 걸린시간.
		long long maxTime[2] = { 0, };

		// 최소로 걸린시간.
		long long minTime[2] = { 0, };

		// 시작 시간.
		LARGE_INTEGER startTime = { 0, };

		// 호출 횟수.
		long long callCount = 0;

		// 함수의 누적 로직 시간
		// 평균 로직 시간을 구할 때 사용함
		long long totalTIme = 0;
	};

private:	

	// 이름이 없으면 저장
	stPerformanceInfo* findFunctionPerformance(const WCHAR* funcName);

	void updateFunctionPerformance(stPerformanceInfo* performanceInfo);

	WCHAR *mFunctionName;

	static std::unordered_map<WCHAR*, CPerformanceProfiler::stPerformanceInfo*> performanceInfoMap;
};
