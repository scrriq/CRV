#include <iostream>
#include <WS2tcpip.h> // WINAPI библиотека сокетов 

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void main()
{
	WSADATA data;

	WORD version = MAKEWORD(2, 2);

	int wsOk = WSAStartup(version, &data); // запускаем WinSock 
	if (wsOk != 0) {
		cout << "Can't start Winsock" << endl;
		return;
	}
	else cout << "Service is running\nEnter message\n";

	// Подключение к серверу 

	// Создаем hint сервера 
	sockaddr_in server; 
	server.sin_family = AF_INET; // AD_INET = PIv4 address
	server.sin_port = htons(54000); // задаем порт

	// Задаем ip локально машиной 
	// Функция преобразует строку ip в массив байт 
	inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

	// Создаем сокет с типом данных SOCK_DGRAM 
	SOCKET out = socket(AF_INET, SOCK_DGRAM, 0);

	// Создаем жизненный цикл
	string msg = "";

	while (true) {
		msg.clear();
		cin >> msg; // читаем сообщение из консоли 

		// Отправляем сообщение 
		int sendOk = sendto(out, msg.c_str(), msg.size() + 1, 0, (sockaddr*)&server, sizeof(server));
		if (sendOk == SOCKET_ERROR) {
			cout << "That didnt work!" << WSAGetLastError() << endl;
		}
	}

	// Закрываем socket 
	closesocket(out);
	WSACleanup();

}

