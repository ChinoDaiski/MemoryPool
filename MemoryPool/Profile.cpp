
#include "Profile.h"
#include <mutex>

#define PRECISION 8

thread_local std::vector<ProfileData> profileDatas;
static std::vector<ProfileData> g_allProfileDatas;

// 컨테이너 접근을 보호할 뮤텍스
static std::mutex               g_profilesMutex;

ProfileData* findProfileData(const std::wstring& name) {
    for (auto& sample : profileDatas) {
        if (sample.name == name) {
            return &sample;
        }
    }
    return nullptr;
}

// 프로파일링 시작
void ProfileBegin(const std::wstring& name) {
    // 데이터 추출
    ProfileData* data = findProfileData(name);

    // 없다면 추가
    if (!data) {
        profileDatas.push_back(ProfileData(name));
        data = &profileDatas.back();
    }

    // 타이머 시작
    data->timer.start();
}

void UpdateMinTime(ProfileData* data, double elapsedTime) {
    for (UINT8 i = 0; i < THRESHOLD; ++i) {
        if (elapsedTime < data->minTime[i]) {
            for (UINT8 j = THRESHOLD - 1; j > i; --j) {
                data->minTime[j] = data->minTime[j - 1];
            }
            data->minTime[i] = elapsedTime;
            break;
        }
    }
}

void UpdateMaxTime(ProfileData* data, double elapsedTime) {
    for (UINT8 i = 0; i < THRESHOLD; ++i) {
        if (elapsedTime > data->maxTime[i]) {
            for (UINT8 j = THRESHOLD - 1; j > i; --j) {
                data->maxTime[j] = data->maxTime[j - 1];
            }
            data->maxTime[i] = elapsedTime;
            break;
        }
    }
}

// 프로파일링 끝
void ProfileEnd(const std::wstring& name) {
    // 데이터를 찾음
    ProfileData* data = findProfileData(name);

    // 데이터가 존재한다면
    // 측정후 나온 값을 데이터에 추가
    if (data) {
        // 지난 시간 측정
        double elapsedTime = data->timer.stop();

        // 전체 시간에 더하기
        data->totalTime += elapsedTime;

        // 최고 시간 및 최저 시간 갱신
        UpdateMinTime(data, elapsedTime);
        UpdateMaxTime(data, elapsedTime);

        // 함수 호출 횟수 연산
        data->callCount++;
    }
}

//void ProfileDataOutText(const std::wstring& fileName) {
//    std::wofstream file(fileName);
//    file << L"\tName\t|\tAverage\t|\tMin\t|\tMax\t|\tCall\n";
//    file << L"----------------------------------------------------------------------------------------------------\n";
//
//    double averageTime;
//
//    for (const auto& data : profileDatas) {
//
//        // 호출한 횟수가 설정한 예외범주보다 더 많다면 표준편차에서 벗어난 데이터를 제거 후 평균 구하기
//        if (data.callCount > THRESHOLD * 2)
//        {
//            int originalCallCnt = data.callCount;
//            double originalTotalTime = data.totalTime;
//
//            // totalTime에서 과하게 특출난 데이터 제거
//            for (UINT8 i = 0; i < THRESHOLD; ++i)
//            {
//                originalTotalTime -= data.minTime[i];
//                originalTotalTime -= data.maxTime[i];
//
//                originalCallCnt -= 2;
//            }
//
//            averageTime = originalTotalTime / originalCallCnt;
//        }
//        // 아니라면 바로 평균 구하기
//        else
//        {
//            averageTime = data.totalTime / data.callCount;
//        }
//
//        file << std::left << "\t" << data.name
//            << std::fixed << std::setprecision(PRECISION) << "\t" << averageTime
//            << std::setw(15) << std::fixed << std::setprecision(PRECISION) << "\t" << data.minTime[0]
//            << std::setw(15) << std::fixed << std::setprecision(PRECISION) << "\t" << data.maxTime[0]
//            << std::setw(15) << "\t" << data.callCount
//            << L'\n';
//    }
//    file.close();
//}

void ProfileDataOutText(const std::wstring& fileName)
{
    std::wofstream file(fileName);
    file << L"Name\t|\tAverage\t|\tMin\t|\tMax\t|\tCalls\n";
    file << L"-----------------------------------------------------------\n";

    for (const auto& data : g_allProfileDatas)
    {
        double averageTime;

        // 이상치 제거 후 평균
        if (data.callCount > THRESHOLD * 2)
        {
            int cnt = data.callCount;
            double tot = data.totalTime;
            // 상위/하위 THRESHOLD 개씩 빼기
            for (int i = 0; i < THRESHOLD; ++i)
            {
                tot -= data.minTime[i];
                tot -= data.maxTime[i];
                cnt -= 2;
            }
            averageTime = tot / cnt;
        }
        else
        {
            averageTime = data.totalTime / data.callCount;
        }

        file << std::left << data.name
            << L"\t" << std::fixed << std::setprecision(PRECISION) << averageTime
            << L"\t" << data.minTime[0]
            << L"\t" << data.maxTime[0]
            << L"\t" << data.callCount
            << L"\n";
    }

    file.close();
}


void ProfileReset() {
    profileDatas.clear();
}

// TLS → 전역으로 옮기고, name 기준으로 합산
void FlushThreadProfileData()
{
    std::lock_guard<std::mutex> lk(g_profilesMutex);

    for (auto& td : profileDatas)
    {
        // 같은 name 항목 찾기
        auto it = std::find_if(
            g_allProfileDatas.begin(), g_allProfileDatas.end(),
            [&](const ProfileData& pd) {
                return pd.name == td.name;
            });

        if (it == g_allProfileDatas.end())
        {
            // 신규 태그면 그대로 move
            g_allProfileDatas.push_back(std::move(td));
        }
        else
        {
            // 기존 태그면 시간 합산, 호출 횟수 합산
            it->totalTime += td.totalTime;
            it->callCount += td.callCount;

            // (선택) 필요하다면 min/max 통계도 갱신할 수 있음
            for (int i = 0; i < THRESHOLD; ++i) {
                it->minTime[i] = std::min(it->minTime[i], td.minTime[i]);
                it->maxTime[i] = std::max(it->maxTime[i], td.maxTime[i]);
            }
        }
    }

    // TLS 벡터 비우기
    profileDatas.clear();
}

void ProfileDataOutTextMultiThread(const std::wstring& fileName)
{
    std::wofstream file(fileName);
    if (!file.is_open()) return;

    // 헤더
    file
        << std::left << std::setw(24) << L"Name"
        << L" | " << std::right << std::setw(12) << L"Average"
        << L" | " << std::setw(8) << L"Calls"
        << L" | " << std::setw(12) << L"Total"
        << L" | " << std::setw(12) << L"Min"
        << L" | " << std::setw(12) << L"Max"
        << L"\n";

    // 구분선
    file << std::wstring(24 + 3 + 12 + 3 + 8 + 3 + 12 + 3 + 12 + 3 + 12, L'-') << L"\n";

    // 데이터 행
    for (const auto& pd : g_allProfileDatas)
    {
        double average = pd.callCount > 0
            ? pd.totalTime / pd.callCount
            : 0.0;

        // 전체 호출 중 최소·최대 시간 계산
        double minVal = DBL_MAX;
        double maxVal = DBL_MIN;
        for (int i = 0; i < THRESHOLD; ++i) {
            minVal = std::min(minVal, pd.minTime[i]);
            maxVal = std::max(maxVal, pd.maxTime[i]);
        }
        // 만약 실제 호출이 없었다면 0으로
        if (pd.callCount == 0) {
            minVal = maxVal = 0.0;
        }

        file
            // Name (좌측 정렬, 24칸)
            << std::left << std::setw(24) << pd.name
            << L" | "
            // Average (우측 정렬, 소수점 6자리, 12칸)
            << std::right << std::setw(12) << std::fixed << std::setprecision(6) << average
            << L" | "
            // Calls (우측 정렬, 8칸)
            << std::setw(8) << pd.callCount
            << L" | "
            // Total (우측 정렬, 소수점 6자리, 12칸)
            << std::setw(12) << std::fixed << std::setprecision(6) << pd.totalTime
            << L" | "
            // Min (우측 정렬, 소수점 6자리, 12칸)
            << std::setw(12) << std::fixed << std::setprecision(6) << minVal
            << L" | "
            // Max (우측 정렬, 소수점 6자리, 12칸)
            << std::setw(12) << std::fixed << std::setprecision(6) << maxVal
            << L"\n";
    }

    file.close();
}
