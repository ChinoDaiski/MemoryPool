#pragma once

#include <iostream>
#include <chrono>
#include <vector>
#include <new>
#include <Windows.h>

#include "Profile.h"

// Node ����ü ����
#pragma pack(1)
template<typename T>
struct Node
{
    // x64 ȯ�濡�� ����ɰ��� ����ؼ� UINT64 ���� ���
#ifdef _DEBUG
    UINT64 BUFFER_GUARD_FRONT;
#endif // _DEBUG

    T data;

#ifdef _DEBUG
    UINT64 BUFFER_GUARD_END;
#endif // _DEBUG

    Node* next;
};

// MemoryPool Ŭ���� ����
template<typename T, bool bPlacementNew>
class MemoryPool
{
public:
    // ������
    MemoryPool(UINT32 sizeInitialize = 0);

    // �Ҹ���
    virtual ~MemoryPool(void);
    
    // Ǯ�� �ִ� ��ü�� �Ѱ��ְų� ���� �Ҵ��� �ѱ�
    T* Alloc(void);
    
    // ��ü�� Ǯ�� ��ȯ
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

    // �ʱ� �޸� ���� �غ�
    if (sizeInitialize > 0)
    {
        for (UINT32 i = 0; i < sizeInitialize; i++)
        {
            // ���ο� ��ü �Ҵ�
            Node<T>* newNode = new Node<T>();

#ifdef _DEBUG
            newNode->BUFFER_GUARD_FRONT = m_bufferGuardValue;
            newNode->BUFFER_GUARD_END = m_bufferGuardValue;
#endif // _DEBUG

            // placement New �ɼ��� �����ִٸ� ������ ȣ��
            if constexpr (bPlacementNew)
            {
                new (&(newNode->data)) T();
            }

            // m_freeNode�� ���� ���� ����. ��ġ stack�� ����ϵ��� ���.
            newNode->next = m_freeNode;
            m_freeNode = newNode;

            // Ǯ���� �����ϴ� ��ü ���� ����
            m_countPool++;
        }
    }
}

template<typename T, bool bPlacementNew>
inline MemoryPool<T, bPlacementNew>::~MemoryPool(void)
{
    // ��� ��� ����
    Node<T>* deleteNode = m_freeNode;
    while (deleteNode != nullptr)
    {
        Node<T>* nextNode = deleteNode->next;
        delete deleteNode;
        deleteNode = nextNode;
        m_countPool--;
    }

    // ���� �Ҵ� ������ ���� �Ϸ���� �ʾҴٸ�
    if (m_countPool != 0)
    {
        // ��... ������� ���߿� �����ڱ�.
    }
}

template<typename T, bool bPlacementNew>
inline T* MemoryPool<T, bPlacementNew>::Alloc(void)
{
    Node<T>* returnNode;

    // m_freeNode�� nullptr�� �ƴ϶�� ���� Ǯ�� ��ü�� �����Ѵٴ� �ǹ��̹Ƿ� �ϳ� �̾Ƽ� �Ѱ���
    if (m_freeNode != nullptr)
    {
        returnNode = m_freeNode;
        m_freeNode = m_freeNode->next;

        // placement New �ɼ��� �����ִٸ� ������ ȣ��
        if constexpr (bPlacementNew)
        {
            new (&(returnNode->data)) T();
        }

        // ��ü�� TŸ�� ������ ��ȯ
        return &returnNode->data;
    }

    // m_freeNode�� nullptr�̶�� Ǯ�� ��ü�� �������� �ʴ´ٴ� �ǹ��̹Ƿ� ���ο� ��ü �Ҵ�

    // �� ��� �Ҵ�
    Node<T>* newNode = new Node<T>();

#ifdef _DEBUG
    // ������ ����. ���� ���� Ȯ���ϰ�, ��ȯ�Ǵ� Ǯ�� ������ �ùٸ��� Ȯ���ϱ� ���� ���
    newNode->BUFFER_GUARD_FRONT = m_bufferGuardValue;
    newNode->BUFFER_GUARD_END = m_bufferGuardValue;
#endif // _DEBUG

    newNode->next = nullptr;    // ��Ȯ�� m_freeNode�� �����ص� �ȴ�. �ٵ� �� ��ü�� nullptr�̴� ���� ���� nullptr ����.

    // placement New �ɼ��� �����ִٸ� ������ ȣ��
    if (bPlacementNew)
    {
        new (&(newNode->data)) T();
    }

    // ��ü�� TŸ�� ������ ��ȯ
    return &newNode->data;
}

template<typename T, bool bPlacementNew>
inline bool MemoryPool<T, bPlacementNew>::Free(T* ptr)
{
    // ��ȯ�ϴ� ���� �������� �ʴ´ٸ�
    if (ptr == nullptr)
    {
        // ����
        return false;
    }

    // offsetof(Node<T>, data)) => Node ����ü�� data ������ Node ����ü���� �ּ� ���̰��� ���
    // ���⼱ debug ����϶� ���尡 �����Ƿ� 4, release�� ��� 0���� ó��
    Node<T>* pNode = reinterpret_cast<Node<T>*>(reinterpret_cast<char*>(ptr) - offsetof(Node<T>, data));

#ifdef _DEBUG 
    // ���� ����, ��� �÷ο� ���� �� Ǯ ��ȯ�� �ùٸ��� �˻�
    // ���� �ٸ��ٸ� false ��ȯ
    if (
        pNode->BUFFER_GUARD_FRONT != m_bufferGuardValue ||
        pNode->BUFFER_GUARD_END != m_bufferGuardValue
        )
    {
        // �� �������� ��� ����... ���߿� ����.

        return false;
    }
#endif // _DEBUG

    // ���� placement new �ɼ��� ���� �ִٸ�, ������ placement new�� ���� ���̹Ƿ� �ش� ��ü�� �Ҹ��ڸ� �������� �ҷ���
    if constexpr (bPlacementNew)
    {
        pNode->data.~T();
    }

    // ���� ����Ʈ ���ÿ� ������ �ְ�
    pNode->next = m_freeNode;
    m_freeNode = pNode;

    // Ǯ ������ 1 ����
    m_countPool++;

    // ��ȯ ����
    return true;
}