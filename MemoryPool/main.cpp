
#pragma once

#include <iostream>
#include <vector>

#include "Profile.h"
#include "MemoryPool.h"

// �׽�Ʈ�� Ŭ����
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

int main()
{
    ProfileReset();

    const size_t numObjects = 100000;

    //======================================================================
    // �⺻ new/delete
    //======================================================================

    //std::cout << "new/delete �׽�Ʈ\n";

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

    //std::cout << "MemoryPool �׽�Ʈ\n";

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

    std::cout << "�׽�Ʈ �Ϸ�";

    ProfileDataOutText(L"profile_data.txt");

    return 0;
}