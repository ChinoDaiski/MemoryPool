
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

    for (UINT8 i = 0; i < 10; ++i)
    {
        const size_t numObjects = 10000;

        //======================================================================
        // 기본 new/delete
        //======================================================================

        //std::cout << "new/delete 테스트\n";

        std::vector<TestObject*> objects;
        {
            Profile pf(L"new/delete");

            for (size_t i = 0; i < numObjects; ++i)
            {
                objects.push_back(new TestObject());
            }

            for (auto obj : objects)
            {
                delete obj;
            }
        }

        //======================================================================
        // MemoryPool
        //======================================================================

        //std::cout << "MemoryPool 테스트\n";

        MemoryPool<TestObject, true> memoryPool(numObjects);
        objects.clear();
        {
            Profile pf(L"MemoryPool");

            for (size_t i = 0; i < numObjects; ++i)
            {
                objects.push_back(memoryPool.Alloc());
            }

            for (auto obj : objects)
            {
                memoryPool.Free(obj);
            }
        }
    }


    std::cout << "테스트 완료";


    ProfileDataOutText(L"profile_data.txt");

    return 0;
}