
#pragma once

#include <iostream>
#include <vector>

#include "Profile.h"
#include "MemoryPool.h"

// 테스트용 클래스
class TestObject
{
public:
    TestObject() {}
    ~TestObject() {}

private:
    char temp[10000];
};

int main()
{
    ProfileReset();

    for (UINT8 i = 0; i < 100; ++i)
    {
        const size_t numObjects = 10000;

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

            int k = 0;
            for (size_t i = 0; i < numObjects; ++i)
            {
                const auto& obj = memoryPool.Alloc();
                memoryPool.Free(obj);
                k = i;
            }
        }
    }


    std::cout << "테스트 완료";


    ProfileDataOutText(L"profile_data.txt");

    return 0;
}