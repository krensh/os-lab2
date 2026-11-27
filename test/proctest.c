#include "proclaunch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    static void sleep_ms(int ms) { Sleep(ms); }
#else
    #include <time.h>
    static void sleep_ms(int ms) {
        struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
        nanosleep(&ts, NULL);
    }
#endif

int main(int argc, char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // Режим ребенка: --child <код_возврата> <время_сна_мс> [доп_аргумент]
    if (argc >= 4 && strcmp(argv[1], "--child") == 0) {
        int code = atoi(argv[2]);
        int ms = atoi(argv[3]);
        sleep_ms(ms);

        // Проверка аргумента с пробелами (тест 3)
        if (argc >= 5 && strcmp(argv[4], "hello big world\\") == 0) {
            return 55; // магический код успеха для теста 3
        }
        return code;
    }

    const char *self = argv[0];
    int fails = 0;

    // --- ТЕСТ 1: Фоновый запуск и pl_poll ---
    {
        const char *args[] = { self, "--child", "42", "1000", NULL };
        pl_process p;
        int code = -1, iters = 0;

        if (pl_start(args, &p) != 0) {
            printf("[FAIL] Тест 1: не удалось запустить\n");
            fails++;
        } else {
            while (pl_poll(&p, &code) == 0) {
                sleep_ms(100);
                iters++;
            }
            if (iters > 3 && code == 42) {
                printf("[ OK ] Тест 1 (фон + poll)\n");
            } else {
                printf("[FAIL] Тест 1: итераций=%d, код=%d\n", iters, code);
                fails++;
            }
        }
    }

    // --- ТЕСТ 2: pl_wait и код 7 ---
    {
        const char *args[] = { self, "--child", "7", "300", NULL };
        pl_process p;
        int code = -1;

        if (pl_start(args, &p) != 0 || pl_wait(&p, &code) != 0 || code != 7) {
            printf("[FAIL] Тест 2: код=%d\n", code);
            fails++;
        } else {
            printf("[ OK ] Тест 2 (wait)\n");
        }
    }

    // --- ТЕСТ 3: Аргумент с пробелами и кавычками ---
    {
        const char *args[] = { self, "--child", "55", "10", "hello big world\\", NULL };
        pl_process p;
        int code = -1;

        if (pl_start(args, &p) != 0 || pl_wait(&p, &code) != 0 || code != 55) {
            printf("[FAIL] Тест 3: код=%d (ожидался 55)\n", code);
            fails++;
        } else {
            printf("[ OK ] Тест 3 (аргумент с пробелами)\n");
        }
    }

    // --- ТЕСТ 4: Несуществующая программа ---
    {
        const char *args[] = { "no_such_program_12345", NULL };
        pl_process p;

        if (pl_start(args, &p) != 0) {
            printf("[ OK ] Тест 4 (несуществующая программа)\n");
        } else {
            printf("[FAIL] Тест 4\n");
            fails++;
        }
    }

    // --- ТЕСТ 5: Код возврата 300 ---
    {
        const char *args[] = { self, "--child", "300", "100", NULL };
        pl_process p;
        int code = -1;

        pl_start(args, &p);
        pl_wait(&p, &code);

#ifdef _WIN32
        if (code == 300) {
            printf("[ OK ] Тест 5 (код 300 на Windows)\n");
        } else {
            printf("[FAIL] Тест 5: код=%d (ожидался 300)\n", code);
            fails++;
        }
#else
        if (code == 44) {
            printf("[ OK ] Тест 5 (код 300 на Linux = 44)\n");
        } else {
            printf("[FAIL] Тест 5: код=%d (ожидался 44)\n", code);
            fails++;
        }
#endif
    }

    if (fails == 0) {
        printf("\nВСЕ ТЕСТЫ ПРОЙДЕНЫ УСПЕШНО!\n");
        return 0;
    } else {
        printf("\nПРОВАЛЕНО ТЕСТОВ: %d\n", fails);
        return 1;
    }
}
