#pragma once

//#define FUNCTION_NAME_SIZE 50

//#define FUNCTION_COUNT 100


class CPerformanceProfiler
{
public:

	CPerformanceProfiler(const WCHAR* functionName);

	~CPerformanceProfiler(void);

	static bool PrintPerformance(void);

	static bool SetPerformanceProfiler(int threadCount);

	static bool FreePerformanceProfiler(void);

private:

	// 태그 성능 정보.
	struct stPerformanceInfo
	{	
		// 호출 횟수.
		long long callCount;

		// 함수의 누적 로직 시간
		// 평균 로직 시간을 구할 때 사용함
		long long totalTIme;

		// 시작 시간.
		LARGE_INTEGER startTime;

		// 최대로 걸린시간.
		long long maxTime[2];

		// 최소로 걸린시간.
		long long minTime[2];
	};

	
	struct stThreadPerformanceSample
	{
		DWORD mThreadId;

		SRWLOCK mSrwLock;

		std::unordered_map<WCHAR*, CPerformanceProfiler::stPerformanceInfo*> mPerformanceInfoMap;
	};


	// 이름이 없으면 저장
	stPerformanceInfo* findFunctionPerformance(const WCHAR* funcName);

	void updateFunctionPerformance(stPerformanceInfo* performanceInfo);

	static bool setTlsIndex(void);

	static bool freeTlsIndex(void);

	
	WCHAR* mFunctionName;

	static std::vector<stThreadPerformanceSample*> mThreadPerformanceSampleArray;

	static int mTlsIndex;
};
