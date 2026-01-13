/*
На windows эмулируем работу портов с помощью com0com
*/

#define _USE_MATH_DEFINES // для числа пи

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

#ifdef _WIN32
    #define sleep_ms(ms) Sleep(ms)
#else // POSIX
    static inline void sleep_ms(unsigned long ms) {
        struct timespec ts;
        ts.tv_sec = ms / 1000;
        ts.tv_nsec = (ms % 1000) * 1000000;
        nanosleep(&ts, NULL);
    }
#endif



int main(int argc, char** argv) {
    srand(time(NULL));

    // Открываем порт для записи
    FILE* port = fopen(argv[1], "w");
    if (!port) {
        perror("Не удалось открыть порт");
        return 1;
    }

    while (1) {
        time_t now = time(NULL);
        // Генерируем температуру как синусоиду с периодом 86400
        // со случайными девиациями в пределах половины градуса
        double temp = 10 + 5 * sin(M_PI * now / 86400) +
            ((double) (rand() % 100) / 10.0 - 0.5);

        fprintf(port, "%.2f\n", temp);
        fflush(port); // Сразу отправляем данные
        printf("Отправлено: %.2f\n", temp);
        sleep_ms(1000); // Пауза 1 секунда
    }

    fclose(port);
    return 0;
}
