#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <GLFW/glfw3.h>
#include "../libs/httplib.h"
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>

// Структура для хранения точки данных
struct TemperaturePoint {
    std::string timestamp;
    double temperature;
};

// Глобальные данные (защищены мьютексом)
std::mutex data_mutex;
double current_temp = 0.0;
double avg_temp = 0.0;
std::vector<TemperaturePoint> history;
bool has_current = false;
bool has_avg = false;
bool has_history = false;

// Простой парсер JSON для {"temperature": 22.5}
double parse_json_temperature(const std::string& json) {
    size_t pos = json.find("\"temperature\":");
    if (pos == std::string::npos) return 0.0;
    
    pos += 14; // длина "\"temperature\":"
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n')) pos++;
    
    size_t end = json.find_first_not_of("0123456789.-", pos);
    if (end == std::string::npos) end = json.size();
    
    return std::stod(json.substr(pos, end - pos));
}

// Простой парсер JSON-массива для /history
std::vector<TemperaturePoint> parse_json_history(const std::string& json) {
    std::vector<TemperaturePoint> result;
    size_t pos = 0;
    
    while ((pos = json.find("\"timestamp\":", pos)) != std::string::npos) {
        TemperaturePoint pt;
        
        // Парсим timestamp
        pos += 12;
        size_t start_quote = json.find('"', pos);
        if (start_quote == std::string::npos) break;
        size_t end_quote = json.find('"', start_quote + 1);
        if (end_quote == std::string::npos) break;
        pt.timestamp = json.substr(start_quote + 1, end_quote - start_quote - 1);
        
        // Парсим temperature
        size_t temp_pos = json.find("\"temperature\":", end_quote);
        if (temp_pos == std::string::npos) break;
        temp_pos += 14;
        while (temp_pos < json.size() && (json[temp_pos] == ' ' || json[temp_pos] == '\n')) temp_pos++;
        size_t temp_end = json.find_first_not_of("0123456789.-", temp_pos);
        if (temp_end == std::string::npos) temp_end = json.size();
        pt.temperature = std::stod(json.substr(temp_pos, temp_end - temp_pos));
        
        result.push_back(pt);
        pos = temp_end;
    }
    
    return result;
}

// Фоновый поток для обновления данных
void data_fetcher_thread(const std::string& server_url) {
    httplib::Client cli(server_url.c_str());
    
    while (true) {
        // Текущая температура
        auto res = cli.Get("/current");
        if (res && res->status == 200) {
            std::lock_guard<std::mutex> lock(data_mutex);
            current_temp = parse_json_temperature(res->body);
            has_current = true;
        }
        
        // История за последний час (для примера)
        auto now = std::chrono::system_clock::now();
        auto hour_ago = now - std::chrono::hours(1);
        
        auto t_now = std::chrono::system_clock::to_time_t(now);
        auto t_hour_ago = std::chrono::system_clock::to_time_t(hour_ago);
        
        char now_str[20], hour_ago_str[20];
        strftime(now_str, sizeof(now_str), "%Y-%m-%d %H:%M:%S", localtime(&t_now));
        strftime(hour_ago_str, sizeof(hour_ago_str), "%Y-%m-%d %H:%M:%S", localtime(&t_hour_ago));
        
        std::string url = "/history?start=" + std::string(hour_ago_str) + "&end=" + std::string(now_str);
        res = cli.Get(url.c_str());
        if (res && res->status == 200) {
            std::lock_guard<std::mutex> lock(data_mutex);
            history = parse_json_history(res->body);
            has_history = true;
        }
        
        // Средняя за период (последний час)
        res = cli.Get(("/average?start=" + std::string(hour_ago_str) + "&end=" + std::string(now_str)).c_str());
        if (res && res->status == 200) {
            std::lock_guard<std::mutex> lock(data_mutex);
            avg_temp = parse_json_temperature(res->body);
            has_avg = true;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    // Инициализация GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(1024, 768, "Temperature Monitor", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    // Инициализация ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    // Немного синеватая тема
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.09f, 0.16f, 1.00f); // #0f172a
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.47f, 0.64f, 0.84f, 1.00f);  // #60a5fa
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // Запуск фонового потока
    std::thread fetcher(data_fetcher_thread, "http://localhost:8080");
    fetcher.detach();
    
    // Выбор периода
    char start_date[20] = "2026-01-25 00:00:00";
    char end_date[20] = "2026-01-25 23:59:59";
    
    // Основной цикл
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Главное окно
        ImGui::Begin("Temperature Monitor", nullptr, ImGuiWindowFlags_NoResize);
        
        // Текущая температура
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            if (has_current) {
                ImGui::SetWindowFontScale(2.0f);
                ImGui::Text("Current: %.2f °C", current_temp);
                ImGui::SetWindowFontScale(1.0f);
            } else {
                ImGui::Text("Current: -- °C");
            }
        }
        
        ImGui::Separator();
        
        // Выбор периода
        ImGui::InputText("Start (YYYY-MM-DD HH:MM:SS)", start_date, sizeof(start_date));
        ImGui::InputText("End (YYYY-MM-DD HH:MM:SS)", end_date, sizeof(end_date));
        
        if (ImGui::Button("Update")) {
            httplib::Client cli("http://localhost:8080");
            
            // Запрос среднего
            auto res = cli.Get(("/average?start=" + std::string(start_date) + "&end=" + std::string(end_date)).c_str());
            if (res && res->status == 200) {
                std::lock_guard<std::mutex> lock(data_mutex);
                avg_temp = parse_json_temperature(res->body);
                has_avg = true;
            }
            
            // Запрос истории
            res = cli.Get(("/history?start=" + std::string(start_date) + "&end=" + std::string(end_date)).c_str());
            if (res && res->status == 200) {
                std::lock_guard<std::mutex> lock(data_mutex);
                history = parse_json_history(res->body);
                has_history = true;
            }
        }
        
        // Средняя температура
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            if (has_avg) {
                ImGui::Text("Average: %.2f °C", avg_temp);
            } else {
                ImGui::Text("Average: -- °C");
            }
        }
        
        ImGui::Separator();
        
        // График
        if (ImPlot::BeginPlot("Temperature History", "Time", "°C", ImVec2(-1, 300))) {
            std::lock_guard<std::mutex> lock(data_mutex);
            if (has_history && !history.empty()) {
                std::vector<double> xs(history.size());
                std::vector<double> ys(history.size());
                
                // Для простоты используем индексы как X (можно парсить время для реального масштаба)
                for (size_t i = 0; i < history.size(); i++) {
                    xs[i] = static_cast<double>(i);
                    ys[i] = history[i].temperature;
                }
                
                ImPlot::PlotLine("Temp", xs.data(), ys.data(), static_cast<int>(xs.size()));
            }
            ImPlot::EndPlot();
        }
        
        ImGui::End();
        
        // Рендеринг
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.06f, 0.09f, 0.16f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
    
    // Очистка
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
