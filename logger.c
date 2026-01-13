#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include <math.h>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
#else // POSIX
    #include <unistd.h>
    #include <fcntl.h>
    // #include <termios.h>
#endif



#define TIME_STR_SIZE 32
#define ALL_LOG_FILE "all.log"
#define HOURLY_LOG_FILE "hourly.log"
#define DAILY_LOG_FILE "daily.log"



char* get_time_str() {
    time_t now;
    struct tm tm_info;
    char* time_str = (char*) malloc(TIME_STR_SIZE * sizeof(char));

    time(&now);

    // Потокобезопасное получение структуры даты и времени
#ifdef _WIN32
    localtime_s(&tm_info, &now);
#else // POSIX
    localtime_r(&now, &tm_info);
#endif

    strftime(time_str, TIME_STR_SIZE, "%Y-%m-%d %H:%M:%S", &tm_info);

    return time_str;
}

time_t time_str_to_int(char* time_str) {
    struct tm tm = {0};
    sscanf(time_str, "%d-%d-%d %d:%d:%d", 
        &tm.tm_year, &tm.tm_mon, &tm.tm_mday, 
        &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    tm.tm_year -= 1900;
    tm.tm_mon -= 1; // январь - это 0 лол
    return mktime(&tm);
}

FILE* open_serial_port(char* port_name) {
#ifdef _WIN32

    HANDLE hPort = CreateFileA(
        port_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL
    );
    if (hPort == INVALID_HANDLE_VALUE) {
        printf("Ошибка открытия порта\n");
        return NULL;
    }

    // Превращаем Windows-дескриптор (HANDLE) в POSIX-дескриптор (int), а затем в FILE*
    return _fdopen(_open_osfhandle((intptr_t)hPort, _O_RDONLY), "r");

#else // POSIX

    FILE* fp = fopen(port_name, "r");
    if (!fp) {
        perror("Ошибка открытия порта\n");
        return NULL;
    }

    /*
    // Настраиваем параметры порта
    struct termios tty;
    tcgetattr(fileno(fp), &tty);
    cfsetospeed(&tty, B9600); // Скорость: 9600 бит/с
    tty.c_cflag &= ~PARENB; // отключить чётность
    tty.c_cflag &= ~CSTOPB; // 1 стоп-бит
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;     // 8 бит данных
    tcsetattr(fileno(fp), TCSANOW, &tty);
    //*/

    return fp;

#endif
}

void log_t(char* file_name, double temp) {
    FILE* f = fopen(file_name, "a");
    if (!f) {
        perror("Couldn't open the file!");
        return;
    }
    char* time_str = get_time_str();
    fprintf(f, "[%s] T: %.2f\n", time_str, temp);
    free(time_str);
    fclose(f);
}

void trim_general_log() {
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);

    FILE* temp = tmpfile();
    FILE* orig = fopen(ALL_LOG_FILE, "r");

    char line[128];
    while (fgets(line, sizeof(line), orig)) {
        // Находим в строке время
        char* start = strchr(line, '[');
        char* end = strchr(line, ']');
        int len = end - start;

        char time_str[TIME_STR_SIZE];
        strncpy(time_str, start, len);
        time_str[len] = '\0';

        // Конвертим строку времени в целое число
        time_t time = time_str_to_int(time_str);

        if (now - time <= 86400)
            fputs(line, temp);
    }
    fclose(orig);

    // Перезаписываем файл
    FILE* newf = freopen(ALL_LOG_FILE, "w", temp);
    fclose(newf);
}

void trim_hourly_log() {
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);

    FILE* temp = tmpfile();
    FILE* orig = fopen(HOURLY_LOG_FILE, "r");

    char line[128];
    while (fgets(line, sizeof(line), orig)) {
        // Находим в строке время
        char* start = strchr(line, '[');
        char* end = strchr(line, ']');
        int len = end - start;

        char time_str[TIME_STR_SIZE];
        strncpy(time_str, start, len);
        time_str[len] = '\0';

        // Конвертим строку времени в целое число
        time_t time = time_str_to_int(time_str);

        if (now - time <= 86400 * 31)
            fputs(line, temp);
    }
    fclose(orig);

    // Перезаписываем файл
    FILE* newf = freopen(HOURLY_LOG_FILE, "w", temp);
    fclose(newf);
}

void trim_daily_log() {
    time_t now = time(NULL);
    struct tm* tm_now = localtime(&now);
    int year_now = tm_now->tm_year + 1900;

    FILE* temp = tmpfile();
    FILE* orig = fopen(HOURLY_LOG_FILE, "r");

    char line[128];
    while (fgets(line, sizeof(line), orig)) {
        int written_year;
        sscanf(line, "%lf", &written_year);
        if (written_year == year_now)
            fputs(line, temp);
    }
    fclose(orig);

    // Перезаписываем файл
    FILE* newf = freopen(HOURLY_LOG_FILE, "w", temp);
    fclose(newf);
}



int main(int argc, char** argv) {
    FILE* port = open_serial_port(argv[1]);

    double hourly_sum = 0;
    int hourly_count = 0;
    time_t last_hour_log_t = time(NULL);

    double daily_sum = 0;
    int daily_count = 0;
    time_t last_day_log_t = time(NULL);

    char buffer[32];
    while (fgets(buffer, sizeof(buffer), port)) {
        double temp;
        if (sscanf(buffer, "%lf", &temp) != 1) continue;

        time_t now = time(NULL);

        // Логируем текущее значение
        log_t(ALL_LOG_FILE, temp);
        trim_general_log();

        hourly_sum += temp;
        hourly_count++;
        daily_sum += temp;
        daily_count++;

        // Проверяем смену часа
        if (now - last_hour_log_t >= 3600) {
            double hourly_avg = hourly_sum / hourly_count;
            log_t(HOURLY_LOG_FILE, hourly_avg);
            trim_hourly_log();

            hourly_sum = 0.0;
            hourly_count = 0;
            last_hour_log_t = now;
        }

        // Проверяем смену дня
        if (now - last_day_log_t >= 86400) {
            double daily_avg = daily_sum / daily_count;
            log_t(DAILY_LOG_FILE, daily_avg);
            trim_daily_log();

            daily_sum = 0.0;
            daily_count = 0;
            last_day_log_t = now;
        }
    }

    fclose(port);
    return 0;
}
