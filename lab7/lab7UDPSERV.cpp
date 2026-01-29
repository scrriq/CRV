#include <iostream>
#include <WS2tcpip.h> // WINAPI библиотека сокетов 

#pragma comment(lib, "ws2_32.lib")

using namespace std; 

void main() {

	WSADATA data;

	WORD version = MAKEWORD(2, 2);

	int wsOk = WSAStartup(version, &data); // запускаем WinSock 
	if (wsOk != 0) {
		cout << "Can't start Winsock" << endl;
		return;
	}
	else cout << "Service is running\nWaiting message\n";

	SOCKET in = socket(AF_INET, SOCK_DGRAM, 0); // создаем сокет 

	// создаем hint сервиса 

	sockaddr_in serverHint; 
	serverHint.sin_addr.S_un.S_addr = ADDR_ANY; // Любой доступный IP на машине 
	serverHint.sin_family = AF_INET; // Задаем формат адресов IPv4
	
	// Закидываем порт, сконвертировав обратный порядок байтов в прямой (little to big endian) 
	serverHint.sin_port = htons(54000);

	// Пытаемся связать сокет с IP и портом 
	if (bind(in, (sockaddr*)&serverHint, sizeof(serverHint)) == SOCKET_ERROR) {
		cout << "Can't bind socket" << WSAGetLastError() << endl;
		return;
	}

	sockaddr_in client; // Используется для хранении информации о клиенте (port/ip адресс) 
	int clientLength = sizeof(client);
	char buf[1024]; // буфер для приема информации 

	// Запускаем жизненный цикл 

	while (true) {
		ZeroMemory(&client, clientLength); // Очистим структуру клиента 
		ZeroMemory(buf, 1024); // Очистим буфер приема ( переопределим)

		int bytesIn = recvfrom(in, buf, 1024, 0, (sockaddr*)&client, &clientLength);
		if (bytesIn == SOCKET_ERROR) {
			cout << "Error receiving from client" << WSAGetLastError() << endl; 
			continue;
		}
		char clientIp[256]; // Выделяем 256 байт для конвертации адреса клиента в строку 
		ZeroMemory(clientIp, 256); // заполняем память нулями 

		// Коневертируем массив в байт и символы 
		inet_ntop(AF_INET, &client.sin_addr, clientIp, 256);
		
		// Показываем сообщение и кто его отправлял 
		cout << "Message recv from" << clientIp << ":" << buf << endl; 

	}

	// Закрываем Socket 
	closesocket(in);
	// Отключаем Winsock
	WSACleanup();


}