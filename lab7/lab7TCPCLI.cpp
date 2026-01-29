// TCPClient.cpp
#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main() {
    setlocale(LC_ALL, "ru");
    string ipAddress = "127.0.0.1";
    int port = 55000;

    // 1. Инициализация WinSock
    WSAData data;
    WORD ver = MAKEWORD(2, 2);
    int wsResult = WSAStartup(ver, &data);
    if (wsResult != 0) {
        cerr << "Не удалось запустить Winsock, ошибка #" << wsResult << endl;
        return 1;
    }

    // 2. Создание TCP сокета
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "Не удалось создать сокет, ошибка #" << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // 3. Настройка адреса сервера
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

    // 4. Подключение к серверу
    int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
    if (connResult == SOCKET_ERROR) {
        cerr << "Не удалось подключиться к серверу, ошибка #" << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    cout << "Подключено к серверу " << ipAddress << ":" << port << endl;
    cout << "Доступные команды:" << endl;
    cout << "  time - получить время сервера" << endl;
    cout << "  exit или quit - отключиться" << endl;
    cout << "  Любой другой текст - эхо-ответ" << endl;

    // 5. Буфер для приема данных
    char buf[4096];
    string userInput;
    auto startTime = chrono::steady_clock::now();
    const int SESSION_TIME = 300; // 5 минут максимальное время сессии

    // 6. Основной цикл
    do {
        // 6.1 Проверка времени сессии
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(now - startTime).count();

        if (elapsed > SESSION_TIME) {
            cout << "Время сессии истекло (" << SESSION_TIME << " секунд)" << endl;
            break;
        }

        // 6.2 Ввод сообщения
        cout << "\n> ";
        getline(cin, userInput);

        if (userInput.size() > 0) {
            // 6.3 Отправка сообщения
            int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);
            if (sendResult == SOCKET_ERROR) {
                cerr << "Ошибка отправки, ошибка #" << WSAGetLastError() << endl;
                break;
            }

            // 6.4 Ожидание ответа
            ZeroMemory(buf, 4096);
            int bytesReceived = recv(sock, buf, 4096, 0);

            if (bytesReceived > 0) {
                cout << "СЕРВЕР> " << string(buf, 0, bytesReceived);

                // Если сервер прощается, выходим
                if (string(buf, 0, bytesReceived).find("До свидания") != string::npos) {
                    break;
                }
            }
            else if (bytesReceived == 0) {
                cout << "Сервер отключился" << endl;
                break;
            }
            else {
                cerr << "Ошибка приема, ошибка #" << WSAGetLastError() << endl;
                break;
            }
        }

    } while (userInput != "exit" && userInput != "quit");

    // 7. Корректное отключение
    cout << "Отключение от сервера..." << endl;

    // 8. Очистка
    closesocket(sock);
    WSACleanup();

    cout << "TCP клиент завершил работу." << endl;

    return 0;
}