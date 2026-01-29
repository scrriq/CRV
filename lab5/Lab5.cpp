#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <string>
#include <clocale>
#include <algorithm>

// Константы
const int MAX_EATEN = 10000;       // Сколько может съесть толстяк до взрыва
const int START_FOOD = 3000;       // Начальное количество еды в каждой тарелке
const int WORK_DAYS = 5;           // Сколько дней (секунд) готов работать повар

// Простейший мьютекс на основе CAS (compare_exchange_strong)
class MyMutex {
    std::atomic<int> locked{ 0 }; // 0 - свободен, 1 - занят
public:
    MyMutex() = default;
    MyMutex(const MyMutex&) = delete;
    MyMutex& operator=(const MyMutex&) = delete;

    void lock() {
        int expected = 0; 
        // Пытаемся сменить 0 -> 1, если неудачно — сброс expected и пробуем снова
        while (!locked.compare_exchange_strong(expected, 1, std::memory_order_acquire)) {
            // если failed, expected теперь содержит текущее значение (==1), нужно сбросить
            expected = 0;
            // Немного уступаем процессору, чтобы не крутить без конца
            std::this_thread::yield();
        }
    }

    void unlock() {
        locked.store(0, std::memory_order_release);
    }
};

// Функция для повара
void cook_work(int cook_speed, std::vector<int>& plates,
    std::atomic<bool>& keep_working, std::atomic<bool>& cook_fired,
    MyMutex& food_mutex) {

    auto start_time = std::chrono::steady_clock::now();

    while (keep_working && !cook_fired) {
        // Проверяем, не проработал ли повар 5 дней (секунд)
        auto now = std::chrono::steady_clock::now();
        auto worked_time = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);

        if (worked_time.count() >= WORK_DAYS) {
            keep_working = false;
            break;
        }

        // Добавляем еду в тарелки (защищено нашим мьютексом)
        food_mutex.lock();
        if (keep_working) {
            for (int i = 0; i < 3; ++i) {
                plates[i] += cook_speed;
            }
        }
        food_mutex.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Функция для толстяка
void fatman_eat(int fatman_id, int eat_speed, std::vector<int>& plates,
    std::vector<int>& eaten, std::atomic<bool>& keep_working,
    std::atomic<bool>& cook_fired, std::string& result,
    MyMutex& food_mutex) {

    while (keep_working && eaten[fatman_id] <= MAX_EATEN) {
        food_mutex.lock();

        // Если тарелка пустая (или почти пустая) и толстяк еще не лопнул
        if (plates[fatman_id] <= eat_speed && eaten[fatman_id] < MAX_EATEN) {
            // Важно: разблокировать перед выходом
            result = "1) Повара уволили! (тарелка опустела)";
            cook_fired = true;
            keep_working = false;
            food_mutex.unlock();
            break;
        }

        // Едим, если есть что
        if (plates[fatman_id] > 0) {
            plates[fatman_id] -= eat_speed;
            eaten[fatman_id] += eat_speed;

            // Проверяем, не лопнул ли толстяк
            if (eaten[fatman_id] > MAX_EATEN) {
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
                    // разблокируем и выйдем
                    food_mutex.unlock();
                    break;
                }
            }
        }

        food_mutex.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Запуск одной симуляции
void run_simulation(int eat_speed, int cook_speed, const std::string& test_name) {
    std::cout << "\n=== " << test_name << " ===" << std::endl;
    std::cout << "Скорость еды: " << eat_speed << std::endl;
    std::cout << "Скорость готовки: " << cook_speed <<


        std::endl;

    // Данные для симуляции
    std::vector<int> plates(3, START_FOOD);  // Тарелки с едой
    std::vector<int> eaten(3, 0);            // Сколько уже съел каждый толстяк

    // Флаги состояния
    std::atomic<bool> keep_working(true);    // Продолжать симуляцию
    std::atomic<bool> cook_fired(false);     // Повара уволили
    std::string result = "3) Повар уволился по истечении 5 дней";     // Результат симуляции

    // Наш мьютекс
    MyMutex food_mutex;

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

    // Ждем завершения потоков
    cook_thread.join();
    for (auto& thread : fatman_threads) {
        thread.join();
    }

    // Выводим результаты
    std::cout << "\nСостояние толстяков:" << std::endl;
    for (int i = 0; i < 3; ++i) {
        std::cout << "Толстяк " << i + 1 << ": съел " << eaten[i] << " наггетсов";
        if (eaten[i] >= MAX_EATEN) {
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
    std::cout << "Лабораторная работа: Толстяки и Повар (с собственным мьютексом)\n" << std::endl;

    // Тест 1: Повара должны уволить
    run_simulation(50, 5, "ТЕСТ 1: Должны уволить повара");

    // Тест 2: Повар не получает зарплату
    run_simulation(100, 200, "ТЕСТ 2: Повар не должен получить зарплату");

    // Тест 3: Повар увольняется сам
    run_simulation(10, 11, "ТЕСТ 3: Повар должен уволиться сам");

    return 0;
}