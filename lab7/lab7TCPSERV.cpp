// TCPServer.cpp - исправленная версия
#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <chrono>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Безопасная функция для получения строки времени
string getCurrentTimeString() {
    time_t now = time(nullptr);
    char timeStr[26];
    ctime_s(timeStr, sizeof(timeStr), &now);
    // Удаляем символ новой строки в конце
    string result(timeStr);
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

int main() {
    setlocale(LC_ALL, "ru");
    // 1. Инициализация WinSock
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);

    int wsOk = WSAStartup(ver, &wsData);
    if (wsOk != 0) {
        cerr << "Не удалось инициализировать WinSock! Код ошибки: " << wsOk << endl;
        return 1;
    }
    cout << "WinSock инициализирован успешно!" << endl;

    // 2. Создание TCP сокета
    SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == INVALID_SOCKET) {
        cerr << "Не удалось создать сокет! Код ошибки: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }
    cout << "TCP сокет создан успешно!" << endl;

    // 3. Настройка адреса сервера
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(55000); // Другой порт для TCP
    hint.sin_addr.S_un.S_addr = INADDR_ANY;

    // 4. Привязка сокета
    if (bind(listening, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
        cerr << "Не удалось привязать сокет! Код ошибки: " << WSAGetLastError() << endl;
        closesocket(listening);
        WSACleanup();
        return 1;
    }

    // 5. Прослушивание порта
    if (listen(listening, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Ошибка listen! Код ошибки: " << WSAGetLastError() << endl;
        closesocket(listening);
        WSACleanup();
        return 1;
    }
    cout << "Сервер слушает порт 55000..." << endl;

    // 6. Ожидание подключения
    sockaddr_in client;
    int clientSize = sizeof(client);

    SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Ошибка accept! Код ошибки: " << WSAGetLastError() << endl;
        closesocket(listening);
        WSACleanup();
        return 1;
    }

    // 7. Информация о клиенте
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    ZeroMemory(host, NI_MAXHOST);
    ZeroMemory(service, NI_MAXSERV);

    if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST,
        service, NI_MAXSERV, 0) == 0) {
        cout << host << " подключен на порту " << service << endl;
    }
    else {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        cout << host << " подключен на порту " << ntohs(client.sin_port) << endl;
    }

    // Закрываем слушающий сокет (больше не нужен)
    closesocket(listening);

    // 8. Буфер для приема данных
    const int BUFFER_SIZE = 4096;
    char buf[BUFFER_SIZE];
    auto lastActivity = chrono::steady_clock::now();
    const int TIMEOUT_SECONDS = 30; // Таймаут 30 секунд

    // 9. Основной цикл обработки сообщений
    while (true) {
        // 9.1 Проверка таймаута
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(now - lastActivity).count();

        if (elapsed > TIMEOUT_SECONDS) {
            cout << "Таймаут " << TIMEOUT_SECONDS << " секунд. Отключение клиента..." << endl;
            break;
        }

        // 9.2 Очистка буфера
        ZeroMemory(buf, BUFFER_SIZE);

        // 9.3 Ожидание данных с таймаутом (неблокирующий режим)
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(clientSocket, &readSet);

        timeval timeout;
        timeout.tv_sec = 1; // Проверяем каждую секунду
        timeout.tv_usec = 0;

        int selectResult = select(0, &readSet, NULL, NULL, &timeout);

        if (selectResult == SOCKET_ERROR) {
            cerr << "Ошибка select! Код ошибки: " << WSAGetLastError() << endl;
            break;
        }

        if (selectResult == 0) {
            // Таймаут - продолжаем цикл для проверки общего таймаута
            continue;
        }

        // 9.4 Прием данных
        int bytesReceived = recv(clientSocket, buf, BUFFER_SIZE, 0);

        if (bytesReceived == SOCKET_ERROR) {
            cerr << "Ошибка в recv(). Код ошибки: " << WSAGetLastError() << endl;
            break;
        }

        if (bytesReceived == 0) {
            cout << "Клиент отключился" << endl;
            break;
        }

        // 9.5 Обновление времени последней активности
        lastActivity = chrono::steady_clock::now();

        // 9.6 Обработка сообщения
        string message(buf, 0, bytesReceived);
        cout << "Клиент: " << message << endl;

        // 9.7 Проверка специальных команд
        if (message == "exit" || message == "quit") {
            cout << "Клиент запросил отключение" << endl;
            string goodbye = "Сервер завершает работу. До свидания!";
            int sendResult = send(clientSocket, goodbye.c_str(), static_cast<int>(goodbye.size()) + 1, 0);
            if (sendResult == SOCKET_ERROR) {
                cerr << "Ошибка отправки прощания: " << WSAGetLastError() << endl;
            }
            break;
        }

        if (message == "time") {
            // Отправляем текущее время с использованием безопасной функции
            string timeStr = getCurrentTimeString();
            int sendResult = send(clientSocket, timeStr.c_str(), static_cast<int>(timeStr.size()) + 1, 0);
            if (sendResult == SOCKET_ERROR) {
                cerr << "Ошибка отправки времени: " << WSAGetLastError() << endl;
            }
            continue;
        }

        // 9.8 Эхо-ответ
        string response = "Эхо: " + message;
        int sendResult = send(clientSocket, response.c_str(), static_cast<int>(response.size()) + 1, 0);
        if (sendResult == SOCKET_ERROR) {
            cerr << "Ошибка отправки эхо-ответа: " << WSAGetLastError() << endl;
            break;
        }
    }

    // 10. Очистка
    closesocket(clientSocket);
    WSACleanup();
    cout << "TCP сервер завершил работу." << endl;

    return 0;
}