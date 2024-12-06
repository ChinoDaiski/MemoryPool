
#pragma once

#include <iostream>
#include <vector>

#include "Profile.h"
#include "MemoryPool.h"

#include <process.h>

// 테스트용 클래스
class TestObject
{
public:
    TestObject() {}
    ~TestObject() {}

public:
    void Clear(void)
    {
        i = 0;
    }

private:
    volatile int i = 0;
};


// 성능 테스트용 함수
void TestMemoryPoolPerformance(void)
{
    ProfileReset();

    const size_t numObjects = 100000;

    //======================================================================
    // 기본 new/delete
    //======================================================================

    //std::cout << "new/delete 테스트\n";

    {
        Profile pf(L"new/delete");
        for (size_t i = 0; i < numObjects; ++i)
        {
            TestObject* obj = new TestObject();
            delete obj;
        }
    }

    //======================================================================
    // MemoryPool
    //======================================================================

    //std::cout << "MemoryPool 테스트\n";

    MemoryPool<TestObject, true> memoryPool(numObjects);
    {
        Profile pf(L"MemoryPool");
        for (size_t i = 0; i < numObjects; ++i)
        {
            TestObject* obj = memoryPool.Alloc();
            obj->Clear();
            memoryPool.Free(obj);
        }
    }

    std::cout << "테스트 완료";

    ProfileDataOutText(L"profile_data.txt");
}

void TestMemoryPoolForBugs(void)
{
    const size_t numObjects = 100000;
    MemoryPool<TestObject, false> memoryPool(numObjects);
    {
        for (size_t i = 0; i < numObjects; ++i)
        {
            TestObject* obj = memoryPool.Alloc();
            obj->Clear();
            memoryPool.Free(obj);
        }
    }

    std::cout << memoryPool.GetCurPoolCount() << "\n";
    std::cout << memoryPool.GetMaxPoolCount() << "\n";

}



struct _tagTestNode
{
    UINT32 data = 0;
};


MemoryPool<_tagTestNode, false> testPool(0);

bool bExitWorker = false;
bool bExitMonitor = false;

unsigned int WINAPI Worker(void* pArg)
{
    const int testCount = 10000;

    _tagTestNode* pNode[testCount];
    UINT32 data1, data2, data3, data4;

    while (!bExitWorker)
    {
        for (int i = 0; i < testCount; ++i)
        {
            pNode[i] = testPool.Alloc();

            data1 = InterlockedExchange(&pNode[i]->data, 0x5555);
            if (data1 != 0)
            {
                DebugBreak();
            }
        }

        for (int i = 0; i < testCount; ++i)
        {
            data2 = InterlockedIncrement(&pNode[i]->data);
            if (data2 != 0x5556)
            {
                DebugBreak();
            }
        }

        for (int i = 0; i < testCount; ++i)
        {
            data3 = InterlockedDecrement(&pNode[i]->data);
            if (data3 != 0x5555)
            {
                DebugBreak();
            }
        }

        for (int i = 0; i < testCount; ++i)
        {
            data4 = InterlockedExchange(&pNode[i]->data, 0);
            if (data4 != 0x5555)
            {
                DebugBreak();
            }

            testPool.Free(pNode[i]);
        }
    }

    return 0;
}



// 인자로 받은 서버의 TPS 및 보고 싶은 정보를 1초마다 감시하는 스레드
unsigned int WINAPI MonitorThread(void* pArg)
{
    while (!bExitMonitor)
    {
        std::cout << "===================================\n";

        std::cout << "CurPoolCount : " << testPool.GetCurPoolCount() << "\n";
        std::cout << "MaxPoolCount : " << testPool.GetMaxPoolCount() << "\n";

        std::cout << "===================================\n\n";

        // 1초간 Sleep
        Sleep(1000);
    }

    return 0;
}







int main()
{
    const int ThreadCnt = 4;
    HANDLE hHandle[ThreadCnt + 1];

    for (int i = 1; i <= ThreadCnt; ++i) {
        hHandle[i] = (HANDLE)_beginthreadex(NULL, 0, Worker, NULL, 0, NULL);
    }

    // 모니터 스레드 생성
    hHandle[0] = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, NULL, 0, NULL);

    WCHAR ControlKey;

    while (1)
    {
        ControlKey = _getwch();
        if (ControlKey == L'q' || ControlKey == L'Q')
        {
            bExitWorker = true;
            break;
        }
    }

    WaitForMultipleObjects(ThreadCnt, &hHandle[1], TRUE, INFINITE);

    bExitMonitor = true;

    WaitForSingleObject(hHandle[0], INFINITE);


    std::cout << "===================================\n";

    std::cout << "CurPoolCount : " << testPool.GetCurPoolCount() << "\n";
    std::cout << "MaxPoolCount : " << testPool.GetMaxPoolCount() << "\n";

    std::cout << "===================================\n\n";
    std::cout << "프로세스 종료" << "\n";

    return 0;
}