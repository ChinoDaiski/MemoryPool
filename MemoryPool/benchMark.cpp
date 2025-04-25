
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <cstddef>
#include <new>
#include "MemoryPool.h"
#include "Profile.h"
#include <string>

#define REPEAT_COUNT 200

// 간단한 테스트 구조체
struct Foo {
    int x[512];
};

// 1) 일반 new/delete 벤치마크 함수
typedef void (*TestFunc)(size_t, int);

//MemoryPool<Foo, true> tlsPool(200000);                        // 전역 풀 인스턴스
thread_local tlsMemoryPool<Foo, false> tlsPool(200000);      // TLS 풀: 각 스레드별로 독립 인스턴스

// 1) new/delete 테스트
void testNewDelete(size_t count, int threadCnt) {
    std::vector<Foo*> v[REPEAT_COUNT];

    for (int i = 0; i < REPEAT_COUNT; ++i)
    {
        v[i].reserve(count);
    }

    std::wstring threads = std::to_wstring(threadCnt);
    threads += {L" threads "};

    std::wstring alloc = { L"new" };
    alloc += std::to_wstring(count);
    std::wstring free = { L"delete" };
    free += std::to_wstring(count);

    alloc = threads + alloc;
    free = threads + free;

    auto start = std::chrono::high_resolution_clock::now();

    for (int k = 0; k < REPEAT_COUNT; ++k)
    {
        Profile pf(alloc);
        for (size_t i = 0; i < count; ++i) {
            v[k].push_back(new Foo{int(i)});
        }
    }

    for (int k = 0; k < REPEAT_COUNT; ++k)
    {
        Profile pf(free);
        for (Foo* p : v[k]) {
            delete p;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[new/delete] " << ms << " ms\n";

    FlushThreadProfileData();
}

//// 2) 글로벌 풀 테스트
//void testGlobalPool(size_t count, int threadCnt) {
//    std::vector<Foo*> v;
//    v.reserve(count);
//
//    for (size_t i = 0; i < count; ++i) {
//        v.push_back(gPool.Alloc());
//    }
//
//    for (Foo* p : v) {
//        p->x[0] += 1;
//    }
//
//    for (Foo* p : v) {
//        gPool.Free(p);
//    }
//    v.clear();
//
//    std::wstring threads = std::to_wstring(threadCnt);
//    threads += {L" threads "};
//
//    std::wstring alloc = { L"memoryPool Alloc" };
//    alloc += std::to_wstring(count);
//    std::wstring free = { L"memoryPool Free" };
//    free += std::to_wstring(count);
//
//    alloc = threads + alloc;
//    free = threads + free;
//
//    auto start = std::chrono::high_resolution_clock::now();
//
//    for (size_t i = 0; i < count; ++i) {
//        Profile pf(alloc);
//        v.push_back(gPool.Alloc());
//    }
//    for (Foo* p : v) {
//        Profile pf(free);
//        gPool.Free(p);
//    }
//    auto end = std::chrono::high_resolution_clock::now();
//    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//    std::cout << "[global pool] " << ms << " ms\n";
//
//    FlushThreadProfileData();
//}

// 3) TLS 풀 테스트
void testTlsPool(size_t count, int threadCnt) {
    std::vector<Foo*> v[REPEAT_COUNT];

    for (int k = 0; k < REPEAT_COUNT; ++k)
    {
        v[k].reserve(count);

        for (size_t i = 0; i < count; ++i) {
            v[k].push_back(tlsPool.Alloc());
        }
        for (Foo* p : v[k]) {
            tlsPool.Free(p);
        }
        
        v[k].clear();
    }

    std::cout << tlsPool.m_maxPoolCount << "\n";

    std::wstring threads = std::to_wstring(threadCnt);
    threads += {L" threads "};

    std::wstring alloc = { L"Alloc" };
    alloc += std::to_wstring(count);
    std::wstring free = { L"Free" };
    free += std::to_wstring(count);

    alloc = threads + alloc;
    free = threads + free;

    auto start = std::chrono::high_resolution_clock::now();

    for (int k = 0; k < REPEAT_COUNT; ++k)
    {
        Profile pf(alloc);
        for (size_t i = 0; i < count; ++i) {
            v[k].push_back(tlsPool.Alloc());
        }
    }

    for (int k = 0; k < REPEAT_COUNT; ++k)
    {
        {
            Profile pf(free);
            for (Foo* p : v[k]) {
                tlsPool.Free(p);
            }
        }
        v[k].clear();
    }


    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[tls pool] " << ms << " ms\n";

    FlushThreadProfileData();
}

// 멀티스레드 벤치마크 헬퍼
void runInThreads(TestFunc fn, size_t total, int threads, const char* name) {
    size_t chunk = total / threads;
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> ths;
    ths.reserve(threads);
    for (int t = 0; t < threads; ++t) {
        ths.emplace_back(fn, chunk, threads);
    }
    for (auto& th : ths) th.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[" << name << " x" << threads << "] " << ms << " ms\n";
}

int main() {
    ProfileReset();

    const size_t count = 1000;

    std::cout << "--- Single-thread benchmark ---\n";
    testNewDelete(count, 1);
    testTlsPool(count, 1);

    std::cout << "\n\n--- Multi-thread benchmark ---\n";
    for (int t : {2, 4, 8, 16}) {
        std::cout << "\n--- " << t << " threads ---\n";
        runInThreads(testNewDelete, count * t, t, "new/delete");
        runInThreads(testTlsPool, count * t, t, "tls pool");
    }

    std::cout << "테스트 완료. 파일 작성중...\n";

    ProfileDataOutTextMultiThread(L"profile_data.txt");

    std::cout << "파일 작성 완료\n";

    return 0;
}
