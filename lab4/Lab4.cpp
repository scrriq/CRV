#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <atomic>
#include <algorithm>

// Константы
const int MAX_EATEN = 10000;       // Сколько может съесть толстяк до взрыва
const int START_FOOD = 3000;       // Начальное количество еды в каждой тарелке
const int WORK_DAYS = 5;           // Сколько дней (секунд) готов работать повар

// Функция для повара
void cook_work(int cook_speed, std::vector<int>& plates,
    std::atomic<bool>& keep_working, std::atomic<bool>& cook_fired,
    std::mutex& food_mutex) {

    auto start_time = std::chrono::steady_clock::now();

    while (keep_working && !cook_fired) {
        // Проверяем, не проработал ли повар 5 дней (секунд)
        auto now = std::chrono::steady_clock::now();
        auto worked_time = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);

        if (worked_time.count() >= WORK_DAYS) {
            keep_working = false;
            break;
        }

        // Добавляем еду в тарелки
        if(keep_working){
            std::lock_guard<std::mutex> lock(food_mutex);
            for (int i = 0; i < 3; ++i) {
                plates[i] += cook_speed;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Функция для толстяка
void fatman_eat(int fatman_id, int eat_speed, std::vector<int>& plates,
    std::vector<int>& eaten, std::atomic<bool>& keep_working,
    std::atomic<bool>& cook_fired, std::string& result,
    std::mutex& food_mutex) {

    while (keep_working && eaten[fatman_id] < MAX_EATEN) {
        {
            std::lock_guard<std::mutex> lock(food_mutex);

            // Если тарелка пустая и толстяк еще не лопнул
            if (plates[fatman_id] + cook_fired <= eat_speed && eaten[fatman_id] < MAX_EATEN) {
                result = "1) Повара уволили! (тарелка опустела)";
                cook_fired = true;
                keep_working = false;
                break;
            }

            // Едим, если есть что
            if (plates[fatman_id] > 0) {
                plates[fatman_id] -= eat_speed;
                eaten[fatman_id] += eat_speed;

                // Проверяем, не лопнул ли толстяк
                if (eaten[fatman_id] >= MAX_EATEN) {
                    // Проверяем, все ли толстяки лопнули
                    bool all_exploded = true;
                    for (int i = 0; i < 3; ++i) {
                        if (eaten[i] < MAX_EATEN) {
                            all_exploded = false;
                            break;
                        }
                    }

                    if (all_exploded) {
                        result = "2) Повар не получил зарплату! (все толстяки лопнули)";
                        keep_working = false;
                        break;
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Запуск одной симуляции
void run_simulation(int eat_speed, int cook_speed, const std::string& test_name) {
    std::cout << "\n=== " << test_name << " ===" << std::endl;
    std::cout << "Скорость еды: " << eat_speed << std::endl;
    std::cout << "Скорость готовки: " << cook_speed << std::endl;

    // Данные для симуляции
    std::vector<int> plates(3, START_FOOD);  // Тарелки с едой
    std::vector<int> eaten(3, 0);            // Сколько уже съел каждый толстяк

    // Флаги состояния
    std::atomic<bool> keep_working(true);    // Продолжать симуляцию
    std::atomic<bool> cook_fired(false);     // Повара уволили
    std::string result = "3) Повар уволился по истечении 5 дней";     // Результат симуляции

    // Мьютекс для защиты данных
    std::mutex food_mutex;

    // Создаем поток для повара
    std::thread cook_thread(cook_work, cook_speed, std::ref(plates),
        std::ref(keep_working), std::ref(cook_fired),
        std::ref(food_mutex));

    // Создаем потоки для толстяков
    std::vector<std::thread> fatman_threads;
    for (int i = 0; i < 3; ++i) {
        fatman_threads.emplace_back(fatman_eat, i, eat_speed, std::ref(plates),
            std::ref(eaten), std::ref(keep_working),
            std::ref(cook_fired), std::ref(result),
            std::ref(food_mutex));
    }

    // Ждем завершения потока кука, а затем заврешения потоков толстяков 
    cook_thread.join();
    for (auto& thread : fatman_threads) {
        thread.join();
    }
    // Выводим результаты
    std::cout << "\nСостояние толстяков:" << std::endl;
    for (int i = 0; i < 3; ++i) {
        std::cout << "Толстяк " << i + 1 << ": съел " << eaten[i] << " наггетсов";
        if (eaten[i] > MAX_EATEN) {
            std::cout << " (лопнул!)";
        }
        else if (plates[i] <= 0) {
            std::cout << " (голоден)";
        }
        else {
            std::cout << ", осталось: " << plates[i];
        }
        std::cout << std::endl;
    }

    std::cout << "\nИтог: " << result << std::endl;
    std::cout << "================================\n" << std::endl;
}

int main() {
    setlocale(LC_ALL, "ru");
    std::cout << "Лабораторная работа: Толстяки и Повар\n" << std::endl;

    // Тест 1: Повара должны уволить
    run_simulation(50, 5, "ТЕСТ 1: Должны уволить повара");

    // Тест 2: Повар не получает зарплату
    run_simulation(100, 200, "ТЕСТ 2: Повар не должен получить зарплату");

    // Тест 3: Повар увольняется сам
    run_simulation(10, 11, "ТЕСТ 3: Повар должен уволиться сам");

    return 0;
}