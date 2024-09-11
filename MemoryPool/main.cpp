
#pragma once

#include <iostream>
#include <vector>

#include "Profile.h"
#include "MemoryPool.h"

// 테스트용 클래스
class TestObject
{
public:
    TestObject() {
        i++;
    }
    ~TestObject() {}

private:
    volatile int i = 0;
};

int main()
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

    MemoryPool<TestObject, false> memoryPool(numObjects);
    {
        Profile pf(L"MemoryPool");
        for (size_t i = 0; i < numObjects; ++i)
        {
            TestObject* obj = memoryPool.Alloc();
            memoryPool.Free(obj);
        }
    }

    std::cout << "테스트 완료";

    ProfileDataOutText(L"profile_data.txt");

    return 0;
}