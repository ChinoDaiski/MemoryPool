#pragma once

#include <iostream>
#include <chrono>
#include <vector>
#include <new>
#include <Windows.h>

#include "Profile.h"

// Node 구조체 정의
#pragma pack(1)
template<typename T>
struct Node
{
    // x64 환경에서 빌드될것을 고려해서 UINT64 형을 사용
#ifdef _DEBUG
    UINT64 BUFFER_GUARD_FRONT;
#endif // _DEBUG

    T data;

#ifdef _DEBUG
    UINT64 BUFFER_GUARD_END;
#endif // _DEBUG

    Node* next;
};

// MemoryPool 클래스 정의
template<typename T, bool bPlacementNew>
class MemoryPool
{
public:
    // 생성자
    MemoryPool(UINT32 sizeInitialize = 0);

    // 소멸자
    virtual ~MemoryPool(void);
    
    // 풀에 있는 객체를 넘겨주거나 새로 할당해 넘김
    T* Alloc(void);
    
    // 객체를 풀에 반환
    bool Free(T* ptr);
   

private:
    Node<T>* m_freeNode;
    UINT32 m_countPool;

#ifdef _DEBUG
    UINT64 m_bufferGuardValue;
#endif // _DEBUG
};

template<typename T, bool bPlacementNew>
inline MemoryPool<T, bPlacementNew>::MemoryPool(UINT32 sizeInitialize)
{
#ifdef _DEBUG
    m_bufferGuardValue = reinterpret_cast<UINT64>(this);
#endif // _DEBUG

    m_freeNode = nullptr;
    m_countPool = 0;

    // 초기 메모리 공간 준비
    if (sizeInitialize > 0)
    {
        for (UINT32 i = 0; i < sizeInitialize; i++)
        {
            // 새로운 객체 할당
            Node<T>* newNode = new Node<T>();

#ifdef _DEBUG
            newNode->BUFFER_GUARD_FRONT = m_bufferGuardValue;
            newNode->BUFFER_GUARD_END = m_bufferGuardValue;
#endif // _DEBUG

            // placement New 옵션이 켜져있다면 생성자 호출
            if constexpr (bPlacementNew)
            {
                new (&(newNode->data)) T();
            }

            // m_freeNode를 다음 노드로 설정. 마치 stack을 사용하듯이 사용.
            newNode->next = m_freeNode;
            m_freeNode = newNode;

            // 풀에서 관리하는 객체 갯수 증가
            m_countPool++;
        }
    }
}

template<typename T, bool bPlacementNew>
inline MemoryPool<T, bPlacementNew>::~MemoryPool(void)
{
    // 모든 노드 해제
    Node<T>* deleteNode = m_freeNode;
    while (deleteNode != nullptr)
    {
        Node<T>* nextNode = deleteNode->next;
        delete deleteNode;
        deleteNode = nextNode;
        m_countPool--;
    }

    // 만약 할당 해제가 전부 완료되지 않았다면
    if (m_countPool != 0)
    {
        // 음... 어떻게할지 나중에 정하자구.
    }
}

template<typename T, bool bPlacementNew>
inline T* MemoryPool<T, bPlacementNew>::Alloc(void)
{
    Node<T>* returnNode;

    // m_freeNode가 nullptr이 아니라는 것은 풀에 객체가 존재한다는 의미이므로 하나 뽑아서 넘겨줌
    if (m_freeNode != nullptr)
    {
        returnNode = m_freeNode;
        m_freeNode = m_freeNode->next;

        // placement New 옵션이 켜져있다면 생성자 호출
        if constexpr (bPlacementNew)
        {
            new (&(returnNode->data)) T();
        }

        // 객체의 T타입 데이터 반환
        return &returnNode->data;
    }

    // m_freeNode가 nullptr이라면 풀에 객체가 존재하지 않는다는 의미이므로 새로운 객체 할당

    // 새 노드 할당
    Node<T>* newNode = new Node<T>();

#ifdef _DEBUG
    // 디버깅용 가드. 가드 값도 확인하고, 반환되는 풀의 정보가 올바른지 확인하기 위해 사용
    newNode->BUFFER_GUARD_FRONT = m_bufferGuardValue;
    newNode->BUFFER_GUARD_END = m_bufferGuardValue;
#endif // _DEBUG

    newNode->next = nullptr;    // 정확힌 m_freeNode를 대입해도 된다. 근데 이 자체가 nullptr이니 보기 쉽게 nullptr 넣음.

    // placement New 옵션이 켜져있다면 생성자 호출
    if (bPlacementNew)
    {
        new (&(newNode->data)) T();
    }

    // 객체의 T타입 데이터 반환
    return &newNode->data;
}

template<typename T, bool bPlacementNew>
inline bool MemoryPool<T, bPlacementNew>::Free(T* ptr)
{
    // 반환하는 값이 존재하지 않는다면
    if (ptr == nullptr)
    {
        // 실패
        return false;
    }

    // offsetof(Node<T>, data)) => Node 구조체의 data 변수와 Node 구조체와의 주소 차이값을 계산
    // 여기선 debug 모드일때 가드가 있으므로 4, release일 경우 0으로 처리
    Node<T>* pNode = reinterpret_cast<Node<T>*>(reinterpret_cast<char*>(ptr) - offsetof(Node<T>, data));

#ifdef _DEBUG 
    // 스택 오버, 언더 플로우 감지 및 풀 반환이 올바른지 검사
    // 만약 다르다면 false 반환
    if (
        pNode->BUFFER_GUARD_FRONT != m_bufferGuardValue ||
        pNode->BUFFER_GUARD_END != m_bufferGuardValue
        )
    {
        // 이 시점에서 어떻게 할지... 나중에 결정.

        return false;
    }
#endif // _DEBUG

    // 만약 placement new 옵션이 켜져 있다면, 생성을 placement new로 했을 것이므로 해당 객체의 소멸자를 수동으로 불러줌
    if constexpr (bPlacementNew)
    {
        pNode->data.~T();
    }

    // 프리 리스트 스택에 정보를 넣고
    pNode->next = m_freeNode;
    m_freeNode = pNode;

    // 풀 갯수를 1 증가
    m_countPool++;

    // 반환 성공
    return true;
}