
#include "Profile.h"

#define PRECISION 8

std::vector<ProfileData> profileDatas;

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

void ProfileDataOutText(const std::wstring& fileName) {
    std::wofstream file(fileName);
    file << L"\tName\t|\tAverage\t|\tMin\t|\tMax\t|\tCall\n";
    file << L"----------------------------------------------------------------------------------------------------\n";

    double averageTime;

    for (const auto& data : profileDatas) {

        // 호출한 횟수가 설정한 예외범주보다 더 많다면 표준편차에서 벗어난 데이터를 제거 후 평균 구하기
        if (data.callCount > THRESHOLD * 2)
        {
            int originalCallCnt = data.callCount;
            double originalTotalTime = data.totalTime;

            // totalTime에서 과하게 특출난 데이터 제거
            for (UINT8 i = 0; i < THRESHOLD; ++i)
            {
                originalTotalTime -= data.minTime[i];
                originalTotalTime -= data.maxTime[i];

                originalCallCnt -= 2;
            }

            averageTime = originalTotalTime / originalCallCnt;
        }
        // 아니라면 바로 평균 구하기
        else
        {
            averageTime = data.totalTime / data.callCount;
        }

        file << std::left << "\t" << data.name
            << std::fixed << std::setprecision(PRECISION) << "\t" << averageTime
            << std::setw(15) << std::fixed << std::setprecision(PRECISION) << "\t" << data.minTime[0]
            << std::setw(15) << std::fixed << std::setprecision(PRECISION) << "\t" << data.maxTime[0]
            << std::setw(15) << "\t" << data.callCount
            << L'\n';
    }
    file.close();
}

void ProfileReset() {
    profileDatas.clear();
}