
#pragma once

#include <iostream>
#include <Windows.h>

#include <vector>
#include <string>

#include <fstream>
#include <float.h> // DBL_MAX, DBL_MIN
#include <iomanip>

class CProfileTimer
{
public:
    CProfileTimer() {
        QueryPerformanceFrequency(&freq);
        start();
    }

    void start() {
        QueryPerformanceCounter(&startTime);
    }

    double stop() {
        LARGE_INTEGER endTime;
        QueryPerformanceCounter(&endTime);
        return 1.f * (endTime.QuadPart - startTime.QuadPart) / freq.QuadPart; // microseconds
    }

private:
    LARGE_INTEGER freq;
    LARGE_INTEGER startTime;
};

// 프로파일링용 구조체
#define THRESHOLD 10

typedef struct _tagProfileData {
    std::wstring name;
    double totalTime = 0;
    double minTime[THRESHOLD];
    double maxTime[THRESHOLD];
    int callCount = 0;
    CProfileTimer timer; // 타이머

    _tagProfileData(const std::wstring& _name)
    {
        for (UINT8 i = 0; i < THRESHOLD; ++i)
        {
            minTime[i] = DBL_MAX;
            maxTime[i] = DBL_MIN;
        }

        name = _name;
    }
}ProfileData;

ProfileData* findProfileData(const std::wstring& name);
void ProfileBegin(const std::wstring& name);
void UpdateMinTime(ProfileData* data, double elapsedTime);
void UpdateMaxTime(ProfileData* data, double elapsedTime);
void ProfileEnd(const std::wstring& name);
void ProfileDataOutText(const std::wstring& fileName);
void ProfileReset();

#define PROFILE

#ifdef PROFILE
#define PRO_BEGIN(tagName) ProfileBegin(tagName)
#define PRO_END(tagName) ProfileEnd(tagName)
#else
#define PRO_BEGIN(tagName)
#define PRO_END(tagName)
#endif

class Profile {
public:
    Profile(const std::wstring& tag) : m_tagName(tag) {
        PRO_BEGIN(m_tagName);
    }
    ~Profile() {
        PRO_END(m_tagName);
    }

private:
    std::wstring m_tagName;
};

//void Func()
//{
//    {
//        Profile p1(L"Func A");
//
//        Sleep(100);
//    }
//
//    {
//        Profile p2(L"Func B");
//
//        Sleep(20);
//    }
//}
//
//int main() {
//    ProfileReset();
//
//    for (int i = 0; i < 10; ++i)
//        Func();
//
//    ProfileDataOutText(L"profile_data.txt");
//
//    return 0;
//}
