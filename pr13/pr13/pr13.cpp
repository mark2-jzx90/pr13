// pr13.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <string>
#include <windows.h>
#include<cstdio>
using namespace std;

struct Racer{
    int id;
    PROCESS_INFORMATION pi;
    DWORD priority_class;
    ULONGLONG start_time;
    ULONGLONG finish_time;
    int primes_found;
    bool is_finished;
    HANDLE hPipeRead;
};

string get_priority_name(DWORD p_class) {
    if (p_class == IDLE_PRIORITY_CLASS) {
        return "IDLE";
    }
    if (p_class == BELOW_NORMAL_PRIORITY_CLASS) {
        return "BELOW_NORMAL";
    }
    if (p_class == NORMAL_PRIORITY_CLASS) {
        return "NORMAL";
    }
    if (p_class == ABOVE_NORMAL_PRIORITY_CLASS) {
        return "ABOVE_NORMAL";
    }
    if (p_class == HIGH_PRIORITY_CLASS) {
        return "HIGH";
    }
    return "UNKNOWN";
}

DWORD choose_priority() {
    int choise;
    cout << "Выбери приоритет: " << endl;
    cout << "1. IDLE_PRIORITY_CLASS" << endl;
    cout << "2. BELOW_NORMAL_PRIORITY_CLASS" << endl;
    cout << "3. NORMAL_PRIORITY_CLASS" << endl;
    cout << "4. ABOVE_NORMAL_PRIORITY_CLASS" << endl;
    cout << "5. HIGH_PRIORITY_CLASS" << endl;
    cout << "Твой выбор вась: " << endl;
    cin >> choise;
    switch (choise) {
    case 1: return IDLE_PRIORITY_CLASS;
    case 2: return BELOW_NORMAL_PRIORITY_CLASS;
    case 3: return NORMAL_PRIORITY_CLASS;
    case 4: return ABOVE_NORMAL_PRIORITY_CLASS;
    case 5: return HIGH_PRIORITY_CLASS;
    default:return NORMAL_PRIORITY_CLASS;
    }
}

int main()
{
    setlocale(LC_ALL, "Russian");
    cout << "Введи вась диапазон для поиска простых чисел(начало и конец): ";
    int range_start, range_end;
    cin >> range_start >> range_end;
    int count;
    cout << "Сколько процессов запустить вась:  ";
    cin >> count;
    if (count < 2) {
        count = 2;
    }
    if (count > 5) {
        count = 5;
    }
    Racer racers[5];
    int finished_count = 0;
    char cmdArg[256];
    sprintf_s(cmdArg, "%d %d", range_start, range_end);
    cout << "\nЗапуск процессов вась" << endl;
    ULONGLONG global_start_time = GetTickCount64();
    for (int i = 0; i < count; i++) {
        racers[i].id = i + 1;
        racers[i].is_finished = false;
        cout << "\nНастрофка робота вась " << racers[i].id << ":" << endl;
        racers[i].priority_class = choose_priority();

        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;
        HANDLE hPipeWrite = NULL;
        CreatePipe(&racers[i].hPipeRead, &hPipeWrite, &saAttr, 0);
        SetHandleInformation(racers[i].hPipeRead, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hPipeWrite;
        si.hStdError = hPipeWrite;
        ZeroMemory(&racers[i].pi, sizeof(racers[i].pi));
        char fullCmd[512];
        sprintf_s(fullCmd, "worker.exe %s", cmdArg);

        BOOL success = CreateProcessA(
            NULL,           
            fullCmd,       
            NULL, NULL,
            TRUE,           
            CREATE_SUSPENDED,
            NULL, NULL,
            &si, &racers[i].pi
        );

        if (!success) {
            cout << "ОШИБКА: Не удалось запустить процесс " << racers[i].id << "!" << endl;
            CloseHandle(hPipeWrite);
            continue;
        }
        if (!SetPriorityClass(racers[i].pi.hProcess, racers[i].priority_class)) {
            cout << "ОШИБКА: Не удалось установить приоритет!" << endl;
        }
        CloseHandle(hPipeWrite); 
        ResumeThread(racers[i].pi.hThread);
        racers[i].start_time = GetTickCount64() - global_start_time;
    }
    cout << "\nстратр вась" << endl;
    cout << "PID   Приоритет   Время (мс) Найдено чисел" << endl;
    while (finished_count < count) {
        for (int i = 0; i < count; i++) {
            if (!racers[i].is_finished && racers[i].pi.hProcess != NULL) {
                DWORD wait_result = WaitForSingleObject(racers[i].pi.hProcess, 0);
                if (wait_result == WAIT_OBJECT_0) {
                    racers[i].finish_time = GetTickCount64() - global_start_time;
                    racers[i].is_finished = true;
                    finished_count++;
                    char buffer[128] = { 0 };
                    DWORD bytesRead = 0;
                    ReadFile(racers[i].hPipeRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
                    sscanf_s(buffer, "%d", &racers[i].primes_found);
                    printf("PID: %-5d | %-14s | %-10llu | %d\n",
                        racers[i].pi.dwProcessId,
                        get_priority_name(racers[i].priority_class).c_str(),
                        racers[i].finish_time,
                        racers[i].primes_found);
                }
            }
        }
        Sleep(10);
    }
    int places[5] = { 1, 1, 1, 1, 1 };
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < count; j++) {
            if (i != j && racers[i].finish_time > racers[j].finish_time) {
                places[i]++;
            }
        }
    }
    cout << "\nитого" << endl;
    cout << "Место  PID    Приоритет     Время (мс) Найдено чисел" << endl;
    for (int step = 1; step <= count; step++) {
        for (int i = 0; i < count; i++) {
            if (places[i] == step) {
                printf("  %d   | %-5d | %-14s | %-10llu | %d\n",
                    places[i],
                    racers[i].pi.dwProcessId,
                    get_priority_name(racers[i].priority_class).c_str(),
                    racers[i].finish_time,
                    racers[i].primes_found);
            }
        }
    }
    for (int i = 0; i < count; i++) {
        if (racers[i].pi.hProcess != NULL) {
            CloseHandle(racers[i].pi.hProcess);
            CloseHandle(racers[i].pi.hThread);
            CloseHandle(racers[i].hPipeRead);
        }
    }
    return 0;
    }

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
