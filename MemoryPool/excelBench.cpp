//#include <iostream>
//#include <fstream>
//#include <vector>
//#include <thread>
//#include <chrono>
//#include <cstddef>
//#include "MemoryPool.h"
//
//// 1) 런타임 크기별 구조체
//template <size_t N>
//struct TestStruct {
//    alignas(std::max_align_t) char data[N];
//};
//
//// 2) 벤치마크: new/delete vs tlsMemoryPool
//template <size_t N>
//void runBenchmark(int threads, std::ofstream& csv) {
//    using Foo = TestStruct<N>;
//    constexpr size_t perThreadCount = 20000;
//
//    // 포인터 저장 공간 (thread‑indexed)
//    std::vector<std::vector<Foo*>> ptrsNew(threads);
//    std::vector<std::vector<Foo*>> ptrsPool(threads);
//
//    // --- new/delete 할당 측정 ---
//    auto t0 = std::chrono::high_resolution_clock::now();
//    {
//        std::vector<std::thread> ths; ths.reserve(threads);
//        for (int t = 0; t < threads; ++t) {
//            ths.emplace_back([t, &ptrsNew]() {
//                auto& v = ptrsNew[t];
//                v.reserve(perThreadCount);
//                for (size_t i = 0; i < perThreadCount; ++i)
//                    v.push_back(new Foo());
//                });
//        }
//        for (auto& th : ths) th.join();
//    }
//    auto t1 = std::chrono::high_resolution_clock::now();
//
//    // --- new/delete 해제 측정 ---
//    {
//        std::vector<std::thread> ths; ths.reserve(threads);
//        for (int t = 0; t < threads; ++t) {
//            ths.emplace_back([t, &ptrsNew]() {
//                for (Foo* p : ptrsNew[t])
//                    delete p;
//                });
//        }
//        for (auto& th : ths) th.join();
//    }
//    auto t2 = std::chrono::high_resolution_clock::now();
//
//    double allocNewMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
//    double freeNewMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
//
//    // --- tlsMemoryPool Alloc 측정 ---
//    auto p0 = std::chrono::high_resolution_clock::now();
//    {
//        std::vector<std::thread> ths; ths.reserve(threads);
//        for (int t = 0; t < threads; ++t) {
//            ths.emplace_back([t, &ptrsPool]() {
//                // 쓰레드마다 독립 풀 인스턴스
//                thread_local tlsMemoryPool<Foo, true> localPool;
//                auto& v = ptrsPool[t];
//                v.reserve(perThreadCount);
//                for (size_t i = 0; i < perThreadCount; ++i)
//                    v.push_back(localPool.Alloc());
//                });
//        }
//        for (auto& th : ths) th.join();
//    }
//    auto p1 = std::chrono::high_resolution_clock::now();
//
//    // --- tlsMemoryPool Free 측정 ---
//    {
//        std::vector<std::thread> ths; ths.reserve(threads);
//        for (int t = 0; t < threads; ++t) {
//            ths.emplace_back([t, &ptrsPool]() {
//                thread_local tlsMemoryPool<Foo, true> localPool;
//                for (Foo* p : ptrsPool[t])
//                    localPool.Free(p);
//                });
//        }
//        for (auto& th : ths) th.join();
//    }
//    auto p2 = std::chrono::high_resolution_clock::now();
//
//    double allocPoolMs = std::chrono::duration<double, std::milli>(p1 - p0).count();
//    double freePoolMs = std::chrono::duration<double, std::milli>(p2 - p1).count();
//
//    // --- 결과 출력 & CSV 기록 ---
//    std::cout
//        << N << " bytes, " << threads << " threads : "
//        << "new alloc=" << allocNewMs << " ms, "
//        << "new free=" << freeNewMs << " ms, "
//        << "pool alloc=" << allocPoolMs << " ms, "
//        << "pool free=" << freePoolMs << " ms\n";
//
//    csv
//        << N << ',' << threads << ','
//        << allocNewMs << ',' << freeNewMs << ','
//        << allocPoolMs << ',' << freePoolMs << '\n';
//}
//
//int main() {
//    std::vector<size_t> sizes = { 4,16,64,256,1024,4096,8192 };
//    std::vector<int>    threadsL = { 1,2,4,8,16 };
//
//    std::ofstream csv("benchmark_results.csv");
//    if (!csv) {
//        std::cerr << "CSV 파일 열기 실패\n";
//        return 1;
//    }
//    csv << "size,threads,alloc_new_ms,free_new_ms,alloc_pool_ms,free_pool_ms\n";
//
//    for (auto sz : sizes) {
//        for (auto th : threadsL) {
//            switch (sz) {
//            case 4:    runBenchmark<4>(th, csv);    break;
//            case 16:   runBenchmark<16>(th, csv);   break;
//            case 64:   runBenchmark<64>(th, csv);   break;
//            case 256:  runBenchmark<256>(th, csv);  break;
//            case 1024: runBenchmark<1024>(th, csv); break;
//            case 4096: runBenchmark<4096>(th, csv); break;
//            case 8192: runBenchmark<8192>(th, csv); break;
//            }
//        }
//    }
//
//    csv.close();
//    std::cout << "\nCSV 저장 완료: benchmark_results.csv\n";
//    return 0;
//}


//#include <iostream>
//#include <vector>
//#include <thread>
//#include <cstddef>
//
//#include "MemoryPool.h"
//#include "Profile.h"
//
//// 1) 런타임 크기별 구조체
//template <size_t N>
//struct TestStruct {
//    alignas(std::max_align_t) char data[N];
//};
//
//// 2) 프로파일링 벤치마크 함수
//template <size_t N>
//void runProfiledBenchmark(int threads)
//{
//    using Foo = TestStruct<N>;
//    constexpr size_t totalCount = 20000;              // 전체 작업 횟수 고정
//    size_t baseCount = totalCount / threads;
//    size_t extra = totalCount % threads;          // 나머지는 앞쪽 스레드에 1씩 분배
//
//    // "0004bytes, 4threads" 식의 접두어
//    wchar_t buf[5];
//    swprintf(buf, 5, L"%04zu", N);
//    std::wstring prefix = std::wstring(buf) + L"bytes, " +
//        std::to_wstring(threads) + L"threads";
//
//    // --- 1) new/delete ---
//    {
//        std::wstring labelAlloc = prefix + L" new          ";
//        std::wstring labelFree = prefix + L" delete       ";
//
//        auto worker = [&](int t) {
//            // 이 스레드가 처리할 작업 수
//            size_t myCount = baseCount + (t < extra ? 1 : 0);
//
//            std::vector<Foo*> v;
//            v.reserve(myCount);
//
//            // 할당
//            for (size_t i = 0; i < myCount; ++i) {
//                Profile pf(labelAlloc);
//                v.push_back(new Foo());
//            }
//            // 해제
//            for (Foo* p : v) {
//                Profile pf(labelFree);
//                delete p;
//            }
//            FlushThreadProfileData();  // 반드시 스레드 종료 전에
//            };
//
//        std::vector<std::thread> ths;
//        ths.reserve(threads);
//        for (int t = 0; t < threads; ++t)
//            ths.emplace_back(worker, t);
//        for (auto& th : ths) th.join();
//    }
//
//    // --- 2) tlsMemoryPool ---
//    {
//        std::wstring labelAlloc = prefix + L" pool alloc";
//        std::wstring labelFree = prefix + L" pool free";
//
//        auto worker = [&](int t) {
//            // 미리 예열용: 총 totalCount 크기로 풀 생성
//            thread_local tlsMemoryPool<Foo, true> tlsPool(totalCount);
//
//            size_t myCount = baseCount + (t < extra ? 1 : 0);
//            std::vector<Foo*> v;
//            v.reserve(myCount);
//
//            // 풀 할당
//            for (size_t i = 0; i < myCount; ++i) {
//                Profile pf(labelAlloc);
//                v.push_back(tlsPool.Alloc());
//            }
//            // 풀 해제
//            for (Foo* p : v) {
//                Profile pf(labelFree);
//                tlsPool.Free(p);
//            }
//            FlushThreadProfileData();
//            };
//
//        std::vector<std::thread> ths;
//        ths.reserve(threads);
//        for (int t = 0; t < threads; ++t)
//            ths.emplace_back(worker, t);
//        for (auto& th : ths) th.join();
//    }
//}
//
//int main()
//{
//    ProfileReset();
//
//    std::vector<size_t> sizes = { 16,256,4096,16384 };
//    std::vector<int>    threadsL = { 1,2,4,8 };
//
//    for (size_t sz : sizes) {
//        for (int th : threadsL) {
//            std::wcout
//                << L"== " << sz << L"bytes, "
//                << th << L"threads ==\n";
//            switch (sz) {
//            case 4:    runProfiledBenchmark<4>(th); break;
//            case 16:   runProfiledBenchmark<16>(th); break;
//            case 64:   runProfiledBenchmark<64>(th); break;
//            case 256:  runProfiledBenchmark<256>(th); break;
//            case 1024: runProfiledBenchmark<1024>(th); break;
//            case 4096: runProfiledBenchmark<4096>(th); break;
//            //case 16384: runProfiledBenchmark<16384>(th); break;
//            }
//        }
//    }
//
//    ProfileDataOutTextMultiThread(L"full_profile_data.txt");
//    std::wcout << L"프로파일 데이터 출력 완료\n";
//    return 0;
//}


#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstddef>
#include <cstdio>

#include "MemoryPool.h"
#include "Profile.h"

// 1) 할당 사이즈 512바이트 고정 구조체
struct Foo {
    alignas(std::max_align_t) char data[512];
};

template <int THREADS>
void runProfiledBenchmark()
{
    constexpr int phases = 1000;  // 반복 횟수
    constexpr int batch = 1000;  // 한 번에 할당/해제 개수

    // 4자리 0패딩 크기 문자열 ("0512")
    wchar_t buf[5];
    swprintf(buf, 5, L"%04d", 512);
    std::wstring prefix = std::wstring(buf) + L"bytes, " +
        std::to_wstring(THREADS) + L"threads";

    // worker 쓰레드 함수
    auto worker = [&](int /*tid*/) {
        // TLS 풀 예열 (버킷당 1000개 노드)
        thread_local tlsMemoryPool<Foo, true> tlsPool(1000);

        for (int it = 0; it < phases; ++it) {
            // (1) new 할당 1000개
            {
                Profile pf(prefix + L" new      ");
                for (int i = 0; i < batch; ++i) {
                    Foo* p = new Foo();
                    (void)p;
                }
            }

            // (2) delete 해제 1000개
            {
                Profile pf(prefix + L" delete   ");
                for (int i = 0; i < batch; ++i) {
                    // 실제로는 포인터를 저장했다가 delete 해야 하지만
                    // 여기선 예제를 단순화하기 위해 즉시 delete
                    delete new Foo();
                }
            }

            // (3) pool 할당 1000개
            {
                Profile pf(prefix + L" pool alloc");
                for (int i = 0; i < batch; ++i) {
                    Foo* p = tlsPool.Alloc();
                    (void)p;
                }
            }

            // (4) pool 해제 1000개
            {
                Profile pf(prefix + L" pool free ");
                for (int i = 0; i < batch; ++i) {
                    // 예제를 단순화하기 위해 Alloc→Free 쌍 보장
                    Foo* p = tlsPool.Alloc();
                    tlsPool.Free(p);
                }
            }
        }

        // 스레드 종료 전에 반드시 플러시
        FlushThreadProfileData();
        };

    // THREADS 개의 워커 실행
    std::vector<std::thread> ths;
    ths.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t) {
        ths.emplace_back(worker, t);
    }
    for (auto& th : ths) th.join();
}

int main()
{
    ProfileReset();

    // 스레드 수 조합: 1, 2, 4, 8, 16
    runProfiledBenchmark<1>();
    runProfiledBenchmark<2>();
    runProfiledBenchmark<4>();
    runProfiledBenchmark<8>();
    runProfiledBenchmark<16>();

    // 모든 스레드가 플러시한 다음, 결과를 텍스트로 출력
    ProfileDataOutTextMultiThread(L"benchmark_detailed_profile.txt");
    std::wcout << L"프로파일 데이터 출력 완료: benchmark_detailed_profile.txt\n";
    return 0;
}
