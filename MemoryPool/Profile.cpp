
#include "Profile.h"
#include <mutex>

#define PRECISION 8

thread_local std::vector<ProfileData> profileDatas;
static std::vector<ProfileData> g_allProfileDatas;

// �����̳� ������ ��ȣ�� ���ؽ�
static std::mutex               g_profilesMutex;

ProfileData* findProfileData(const std::wstring& name) {
    for (auto& sample : profileDatas) {
        if (sample.name == name) {
            return &sample;
        }
    }
    return nullptr;
}

// �������ϸ� ����
void ProfileBegin(const std::wstring& name) {
    // ������ ����
    ProfileData* data = findProfileData(name);

    // ���ٸ� �߰�
    if (!data) {
        profileDatas.push_back(ProfileData(name));
        data = &profileDatas.back();
    }

    // Ÿ�̸� ����
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

// �������ϸ� ��
void ProfileEnd(const std::wstring& name) {
    // �����͸� ã��
    ProfileData* data = findProfileData(name);

    // �����Ͱ� �����Ѵٸ�
    // ������ ���� ���� �����Ϳ� �߰�
    if (data) {
        // ���� �ð� ����
        double elapsedTime = data->timer.stop();

        // ��ü �ð��� ���ϱ�
        data->totalTime += elapsedTime;

        // �ְ� �ð� �� ���� �ð� ����
        UpdateMinTime(data, elapsedTime);
        UpdateMaxTime(data, elapsedTime);

        // �Լ� ȣ�� Ƚ�� ����
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
//        // ȣ���� Ƚ���� ������ ���ܹ��ֺ��� �� ���ٸ� ǥ���������� ��� �����͸� ���� �� ��� ���ϱ�
//        if (data.callCount > THRESHOLD * 2)
//        {
//            int originalCallCnt = data.callCount;
//            double originalTotalTime = data.totalTime;
//
//            // totalTime���� ���ϰ� Ư�Ⳮ ������ ����
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
//        // �ƴ϶�� �ٷ� ��� ���ϱ�
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

        // �̻�ġ ���� �� ���
        if (data.callCount > THRESHOLD * 2)
        {
            int cnt = data.callCount;
            double tot = data.totalTime;
            // ����/���� THRESHOLD ���� ����
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

// TLS �� �������� �ű��, name �������� �ջ�
void FlushThreadProfileData()
{
    std::lock_guard<std::mutex> lk(g_profilesMutex);

    for (auto& td : profileDatas)
    {
        // ���� name �׸� ã��
        auto it = std::find_if(
            g_allProfileDatas.begin(), g_allProfileDatas.end(),
            [&](const ProfileData& pd) {
                return pd.name == td.name;
            });

        if (it == g_allProfileDatas.end())
        {
            // �ű� �±׸� �״�� move
            g_allProfileDatas.push_back(std::move(td));
        }
        else
        {
            // ���� �±׸� �ð� �ջ�, ȣ�� Ƚ�� �ջ�
            it->totalTime += td.totalTime;
            it->callCount += td.callCount;

            // (����) �ʿ��ϴٸ� min/max ��赵 ������ �� ����
            for (int i = 0; i < THRESHOLD; ++i) {
                it->minTime[i] = std::min(it->minTime[i], td.minTime[i]);
                it->maxTime[i] = std::max(it->maxTime[i], td.maxTime[i]);
            }
        }
    }

    // TLS ���� ����
    profileDatas.clear();
}

void ProfileDataOutTextMultiThread(const std::wstring& fileName)
{
    std::wofstream file(fileName);
    if (!file.is_open()) return;

    // ���
    file
        << std::left << std::setw(24) << L"Name"
        << L" | " << std::right << std::setw(12) << L"Average"
        << L" | " << std::setw(8) << L"Calls"
        << L" | " << std::setw(12) << L"Total"
        << L" | " << std::setw(12) << L"Min"
        << L" | " << std::setw(12) << L"Max"
        << L"\n";

    // ���м�
    file << std::wstring(24 + 3 + 12 + 3 + 8 + 3 + 12 + 3 + 12 + 3 + 12, L'-') << L"\n";

    // ������ ��
    for (const auto& pd : g_allProfileDatas)
    {
        double average = pd.callCount > 0
            ? pd.totalTime / pd.callCount
            : 0.0;

        // ��ü ȣ�� �� �ּҡ��ִ� �ð� ���
        double minVal = DBL_MAX;
        double maxVal = DBL_MIN;
        for (int i = 0; i < THRESHOLD; ++i) {
            minVal = std::min(minVal, pd.minTime[i]);
            maxVal = std::max(maxVal, pd.maxTime[i]);
        }
        // ���� ���� ȣ���� �����ٸ� 0����
        if (pd.callCount == 0) {
            minVal = maxVal = 0.0;
        }

        file
            // Name (���� ����, 24ĭ)
            << std::left << std::setw(24) << pd.name
            << L" | "
            // Average (���� ����, �Ҽ��� 6�ڸ�, 12ĭ)
            << std::right << std::setw(12) << std::fixed << std::setprecision(6) << average
            << L" | "
            // Calls (���� ����, 8ĭ)
            << std::setw(8) << pd.callCount
            << L" | "
            // Total (���� ����, �Ҽ��� 6�ڸ�, 12ĭ)
            << std::setw(12) << std::fixed << std::setprecision(6) << pd.totalTime
            << L" | "
            // Min (���� ����, �Ҽ��� 6�ڸ�, 12ĭ)
            << std::setw(12) << std::fixed << std::setprecision(6) << minVal
            << L" | "
            // Max (���� ����, �Ҽ��� 6�ڸ�, 12ĭ)
            << std::setw(12) << std::fixed << std::setprecision(6) << maxVal
            << L"\n";
    }

    file.close();
}
