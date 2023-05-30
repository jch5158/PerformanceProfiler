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

	// �±� ���� ����.
	struct stPerformanceInfo
	{	
		// ȣ�� Ƚ��.
		long long callCount;

		// �Լ��� ���� ���� �ð�
		// ��� ���� �ð��� ���� �� �����
		long long totalTIme;

		// ���� �ð�.
		LARGE_INTEGER startTime;

		// �ִ�� �ɸ��ð�.
		long long maxTime[2];

		// �ּҷ� �ɸ��ð�.
		long long minTime[2];
	};

	
	struct stThreadPerformanceSample
	{
		DWORD mThreadId;

		SRWLOCK mSrwLock;

		std::unordered_map<WCHAR*, CPerformanceProfiler::stPerformanceInfo*> mPerformanceInfoMap;
	};


	// �̸��� ������ ����
	stPerformanceInfo* findFunctionPerformance(const WCHAR* funcName);

	void updateFunctionPerformance(stPerformanceInfo* performanceInfo);

	static bool setTlsIndex(void);

	static bool freeTlsIndex(void);

	
	WCHAR* mFunctionName;

	static std::vector<stThreadPerformanceSample*> mThreadPerformanceSampleArray;

	static int mTlsIndex;
};
