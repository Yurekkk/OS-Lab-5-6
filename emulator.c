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

#ifdef _WIN32
    HANDLE port = CreateFileA(argv[1], GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (!port) {
        perror("Couldn't open the port.\n");
        return 1;
    }
#else // POSIX
    int port = open(argv[1], O_WRONLY | O_NOCTTY);
    if (port < 0) {
        perror("Couldn't open the port.\n");
        return 1;
    }
#endif

    while (1) {
        time_t now = time(NULL);
        // Генерируем температуру как синусоиду с периодом 86400
        // со случайными девиациями в пределах половины градуса
        double temp = 10 + 5 * sin(M_PI * now / 86400) +
            ((double) (rand() % 101) / 100 - 0.5);
        char buffer[8];
        int len = snprintf(buffer, sizeof(buffer), "%.2f\n", temp);

#ifdef _WIN32
        DWORD bytesWritten;
        WriteFile(port, buffer, len, &bytesWritten, NULL);
#else // POSIX
        write(port, buffer, len);
#endif

        printf("Sent: %.2f\n", temp);
        sleep_ms(1000); // Пауза 1 секунда
    }

#ifdef _WIN32
    CloseHandle(port);
#else // POSIX
    close(port);
#endif

    return 0;
}
