#define _WIN32_WINNT 0x0A00
// TODO
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libs/httplib.h"
#include "libs/sqlite3.h"
#include <thread>
#include <mutex>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
#else // POSIX
    #include <unistd.h>
    #include <fcntl.h>
    // #include <termios.h>
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



std::mutex db_mutex;



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
    FILE* fp = _fdopen(_open_osfhandle((intptr_t)hPort, _O_RDONLY), "r");
    setvbuf(fp, NULL, _IONBF, 0); // Отключить буферизацию
    return fp;

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

void init_db_maybe(sqlite3** db) {
    // Инициализация БД
    sqlite3_open("temperature.db", db);
    const char* sql = 
        "CREATE TABLE IF NOT EXISTS temps ("
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "temperature REAL);";
    sqlite3_exec(*db, sql, nullptr, nullptr, nullptr);
}

void log_temperature(sqlite3* db, double temp) {
    // Запись температуры в БД
    std::lock_guard<std::mutex> lock(db_mutex); // автоматически разблокирует мьютекс при выходе из области видимости
    sqlite3_stmt* stmt;
    const char* query = "INSERT INTO temps (temperature) VALUES (?)";
    sqlite3_prepare_v2(db, query, -1, &stmt, nullptr); // компилирует SQL-запрос
    sqlite3_bind_double(stmt, 1, temp); // подставляет значение temp вместо ?
    sqlite3_step(stmt); // выполняет запрос
    sqlite3_finalize(stmt); // освобождает ресурсы, связанные с подготовленным запросом.
}

void get_current(httplib::Response& res, sqlite3* db) {
    // Обработчик: текущая температура
    sqlite3_stmt* stmt;
    const char* sql = "SELECT temperature FROM temps ORDER BY timestamp DESC LIMIT 1";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    double last_temp = 0.0;
    bool found = false;

    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            last_temp = sqlite3_column_double(stmt, 0);
            found = true;
        }
    }

    sqlite3_finalize(stmt);

    if (found) {
        std::string json = "{\"temperature\":" + std::to_string(last_temp) + "}";
        res.set_content(json, "application/json");
    } else {
        res.status = 404;
        res.set_content("{\"error\":\"No data available\"}", "application/json");
    }
}

void get_average(const httplib::Request& req, httplib::Response& res, sqlite3* db) {
    // Обработчик: среднее за период
    std::string start = req.get_param_value("start");
    std::string end = req.get_param_value("end");

    // Проверка формата даты (ISO 8601: YYYY-MM-DDTHH:MM[:SS])
    if (start.empty() || end.empty()) {
        res.status = 400;
        res.set_content("{\"error\":\"Wrong 'start' or 'end' parameter\"}", "application/json");
        return;
    }

    sqlite3_stmt* stmt;
    const char* sql = "SELECT AVG(temperature) FROM temps WHERE timestamp BETWEEN ? AND ?";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, start.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, end.c_str(), -1, SQLITE_STATIC);

    double avg_temp = 0.0;
    bool found = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            avg_temp = sqlite3_column_double(stmt, 0);
            found = true;
        }
    }

    sqlite3_finalize(stmt);

    if (found) {
        std::string json = "{\"temperature\":" + std::to_string(avg_temp) + "}";
        res.set_content(json, "application/json");
    } else {
        res.status = 404;
        res.set_content("{\"error\":\"No data available\"}", "application/json");
    }
}
// TODO
void get_history(const httplib::Request& req, httplib::Response& res, sqlite3* db) {
    // Обработчик: все измерения за период
    std::string start = req.get_param_value("start");
    std::string end = req.get_param_value("end");

    if (start.empty() || end.empty()) {
        res.status = 400;
        res.set_content("{\"error\":\"Missing 'start' or 'end' parameter\"}", "application/json");
        return;
    }

    sqlite3_stmt* stmt;
    const char* sql = "SELECT timestamp, temperature FROM temps WHERE timestamp BETWEEN ? AND ? ORDER BY timestamp";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        res.status = 500;
        res.set_content("{\"error\":\"Database query failed\"}", "application/json");
        return;
    }

    sqlite3_bind_text(stmt, 1, start.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, end.c_str(), -1, SQLITE_STATIC);

    std::ostringstream json;
    json << "[";
    bool first = true;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        double temp = sqlite3_column_double(stmt, 1);

        if (!first) json << ",";
        json << "{\"timestamp\":\"" << ts << "\",\"temperature\":" << temp << "}";
        first = false;
    }

    json << "]";

    sqlite3_finalize(stmt);

    res.set_content(json.str(), "application/json");
}

void start_server(sqlite3* db) {
    // HTTP-сервер
    using namespace httplib;
    Server server;

    // Обработчик: текущая температура
    server.Get("/current", [&](const Request&, Response& res) {get_current(res, db);});

    // Обработчик: среднее за период
    server.Get("/average", [&](const Request& req, Response& res) {get_average(req, res, db);});

    // Обработчик: все измерения за период
    server.Get("/history", [&](const Request& req, Response& res) {get_history(req, res, db);});

    // Отдача статики (веб-интерфейс)
    server.set_base_dir("./web");

    // Запуск сервера
    server.listen("0.0.0.0", 8080);
}



int main(int argc, char** argv) {
    FILE* port = open_serial_port(argv[1]);
    sqlite3* db;
    init_db_maybe(&db);

    // Запуск сервера в отдельном потоке
    std::thread server_thread(start_server, db);
    server_thread.detach();

    char buffer[32];
    while (1) {
        double temp;

        fgets(buffer, sizeof(buffer), port);
        sscanf(buffer, "%lf", &temp);
        printf("Received: %.2f\n", temp);

        // Логируем текущее значение
        log_temperature(db, temp);

        sleep_ms(1000);
    }

    sqlite3_close(db);
    fclose(port);
    return 0;
}
