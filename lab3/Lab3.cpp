#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

const int insert_coins = 101; // констант переменная coind 
int coins = insert_coins; // динамическая переменная coins
int Bob_coins = 0;
int Tom_coins = 0;
std::mutex mtx;

// Функция дележки монет
void coin_sharing(const std::string& thief_name, int& thief_coins, int& other_coins) {
    while (true) {
        // Захватываем мьютекс для синхронизации доступа к общим данным
        mtx.lock();

        // Условие выхода: когда осталась 1 монета (её отдадут покойнику)
        if (coins <= 1 && thief_coins == other_coins) {
            mtx.unlock();
            break;
        }

        // Условие для взятия монеты: у вора должно быть <= монет, чем у подельника
        // Или если у подельника 0 монет (начало дележа)
        if (thief_coins <= other_coins) {
            // Берем одну монету из общего мешка
            coins--;
            // Добавляем монету вору
            thief_coins++;

            // Выводим информацию о взятии монеты
            std::cout << thief_name << " взял монету. ";
            std::cout << "У " << thief_name << ": " << thief_coins;
            std::cout << ", у другого: " << other_coins;
            std::cout << ", осталось монет: " << coins << std::endl;
            // Освобождаем мьютекс и делаем небольшую паузу
            mtx.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        else {
            // Если у вора больше монет, он пропускает ход
            std::cout << thief_name << " пропускает ход (у него больше монет)" << std::endl;
            mtx.unlock();
            // Даем возможность другому вору поработать
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

// Функция для Боба
void bob_turn() {
    coin_sharing("Боб", Bob_coins, Tom_coins);
}

// Функция для Тома
void tom_turn() {
    coin_sharing("Том", Tom_coins, Bob_coins);
}

int main() {
    std::locale::global(std::locale("ru_RU.UTF-8"));

    std::cout << "Начинается дележ " << coins << " монет..." << std::endl;
    std::cout << "=========================================" << std::endl;

    // Создаем потоки для воров
    std::thread bob_thread(bob_turn);
    std::thread tom_thread(tom_turn);

    // Ждем завершения работы потоков
    bob_thread.join();
    tom_thread.join();

    std::cout << "=========================================" << std::endl;
    std::cout << "Дележ завершен!" << std::endl;
    std::cout << "Итог:" << std::endl;
    std::cout << "У Боба: " << Bob_coins << " монет" << std::endl;
    std::cout << "У Тома: " << Tom_coins << " монет" << std::endl;
    std::cout << "Отдано покойнику: " << (insert_coins - Bob_coins - Tom_coins) << " монета" << std::endl;

    return 0;
}