#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>

int main()
{
	std::cout << "ok\n";

	WSADATA wsaData;

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result)
	{
		std::cout << "Failed to initialize WinSock with error " << result << "\n";
		return 1;
	}


	sockaddr_in sockad;
	//sockad.sin_addr.s_addr = inet_addr("127.0.0.1");
	inet_pton(AF_INET, "127.0.0.1", &sockad.sin_addr);
	//sockad.sin_addr.s_addr = INADDR_LOOPBACK;
	sockad.sin_family = AF_INET;
	sockad.sin_port = htons(27015);

	SOCKET clientsock = socket(AF_INET, SOCK_STREAM, 0);
	if (clientsock == INVALID_SOCKET)
	{
		std::cout << "damn\n";
		return 1;
	}

	std::cout << "socket create\n";

	int conneccc = connect(clientsock, (sockaddr*)&sockad, sizeof(sockad));
	if (conneccc)
	{
		std::cout << "FAIL CONNEC!!\n";
		std::cout << WSAGetLastError();
		return 2;
	}

	std::cout << "connect\n";

	std::string input;
	while (std::getline(std::cin, input))
	{
		int sendr = send(clientsock, input.c_str(), input.size(), 0);
		if (sendr == SOCKET_ERROR)
		{
			std::cout << "WOE IS US\n";
			return 5;
		}
	}

	std::cout << "send\n";


	closesocket(clientsock);

	WSACleanup();


};