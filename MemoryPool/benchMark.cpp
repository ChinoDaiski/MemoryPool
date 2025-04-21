
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

// ������ �׽�Ʈ ����ü
struct Foo {
    int x[2000];
};

// 1) �Ϲ� new/delete ��ġ��ũ �Լ�
typedef void (*TestFunc)(size_t);

// 2) �۷ι� Ǯ: lock-free ���� ��� ���� ����
template<typename T>
class GlobalPool {
    struct Node { Node* next; alignas(T) unsigned char storage[sizeof(T)]; };
    std::atomic<Node*> head{ nullptr };
public:
    T* alloc() {
        Node* n = head.load(std::memory_order_acquire);
        while (n) {
            Node* next = n->next;
            if (head.compare_exchange_weak(n, next,
                std::memory_order_acq_rel, std::memory_order_acquire)) {
                return new (&n->storage) T();
            }
        }
        // ���� ������� ���� �Ҵ�
        void* mem = ::operator new(sizeof(Node));
        return new (mem) T();
    }
    void free(T* p) {
        p->~T();
        Node* n = reinterpret_cast<Node*>(
            reinterpret_cast<char*>(p) - offsetof(Node, storage));
        Node* old = head.load(std::memory_order_acquire);
        do {
            n->next = old;
        } while (!head.compare_exchange_weak(old, n,
            std::memory_order_release, std::memory_order_relaxed));
    }
};

MemoryPool<Foo, true> gPool;                        // ���� Ǯ �ν��Ͻ�
thread_local tlsMemoryPool<Foo, true> tlsPool;      // TLS Ǯ: �� �����庰�� ���� �ν��Ͻ�


// 1) new/delete �׽�Ʈ
void testNewDelete(size_t count) {
    std::vector<Foo*> v;
    v.reserve(count);

    std::wstring alloc = { L"new" };
    alloc += std::to_wstring(count);
    std::wstring free = { L"delete" };
    free += std::to_wstring(count);

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < count; ++i) {
        Profile pf(alloc);
        v.push_back(new Foo{ int(i) });
    }
    for (Foo* p : v) {
        Profile pf(free);
        delete p;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[new/delete] " << ms << " ms\n";

    FlushThreadProfileData();
}

// 2) �۷ι� Ǯ �׽�Ʈ
void testGlobalPool(size_t count) {
    std::vector<Foo*> v;
    v.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        v.push_back(gPool.Alloc());
    }
    for (Foo* p : v) {
        gPool.Free(p);
    }
    v.clear();

    auto start = std::chrono::high_resolution_clock::now();

    std::wstring alloc = { L"memoryPool Alloc" };
    alloc += std::to_wstring(count);
    std::wstring free = { L"memoryPool Free" };
    free += std::to_wstring(count);

    for (size_t i = 0; i < count; ++i) {
        Profile pf(alloc);
        v.push_back(gPool.Alloc());
    }
    for (Foo* p : v) {
        Profile pf(free);
        gPool.Free(p);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[global pool] " << ms << " ms\n";

    FlushThreadProfileData();
}

// 3) TLS Ǯ �׽�Ʈ
void testTlsPool(size_t count) {
    std::vector<Foo*> v;
    v.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        v.push_back(tlsPool.Alloc());
    }
    for (Foo* p : v) {
        tlsPool.Free(p);
    }
    v.clear();

    auto start = std::chrono::high_resolution_clock::now();

    std::wstring alloc = { L"tlsPool Alloc" };
    alloc += std::to_wstring(count);
    std::wstring free = { L"tlsPool Free" };
    free += std::to_wstring(count);

    for (size_t i = 0; i < count; ++i) {
        Profile pf(alloc);
        v.push_back(tlsPool.Alloc());
    }
    for (Foo* p : v) {
        Profile pf(free);
        tlsPool.Free(p);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[tls pool] " << ms << " ms\n";

    FlushThreadProfileData();
}

// ��Ƽ������ ��ġ��ũ ����
void runInThreads(TestFunc fn, size_t total, int threads, const char* name) {
    size_t chunk = total / threads;
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> ths;
    ths.reserve(threads);
    for (int t = 0; t < threads; ++t) {
        ths.emplace_back(fn, chunk);
    }
    for (auto& th : ths) th.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[" << name << " x" << threads << "] " << ms << " ms\n";
}

int main() {
    ProfileReset();

    const size_t count = 500000;

    std::cout << "--- Single-thread benchmark ---\n";
    testNewDelete(count);
    testGlobalPool(count);
    testTlsPool(count);

    std::cout << "\n\n--- Multi-thread benchmark ---\n";
    for (int t : {2, 4, 8, 16}) {
        std::cout << "\n--- " << t << " threads ---\n";
        runInThreads(testNewDelete, count, t, "new/delete");
        runInThreads(testGlobalPool, count, t, "global pool");
        runInThreads(testTlsPool, count, t, "tls pool");
    }

    std::cout << "�׽�Ʈ �Ϸ�. ���� �ۼ���...\n";

    ProfileDataOutTextMultiThread(L"profile_data.txt");

    std::cout << "���� �ۼ� �Ϸ�\n";

    return 0;
}
