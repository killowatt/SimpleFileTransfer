#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <vector>

DWORD WINAPI goshdarnthread(LPVOID arrr)
{
	SOCKET recvsock = (SOCKET)arrr;

	std::cout << "THREAD!!!\n";

	char buf[64];
	memset(buf, 0, 64);

	while (true)
	{
		int numbyt = recv(recvsock, buf, 64, 0);
		if (numbyt > 0)
		{
			std::cout << "Bytes received! " << numbyt << " bytes!\n";
			std::cout << buf << "\n";
			memset(buf, 0, 64);

		}
	}
}

int main()
{
	WSADATA wsaData;

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result)
	{
		std::cout << "Failed to initialize WinSock with error " << result << "\n";
		return 1;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		std::cout << "Failed to create socket with error " << WSAGetLastError() << "\n";
		return 2;
	}

	// OLD STYLE !
	sockaddr_in soin;
	//soin.sin_addr.s_addr = inet_addr("127.0.0.1");
	//inet_pton(AF_INET, "127.0.0.1", &soin.sin_addr);
	soin.sin_addr.s_addr = INADDR_ANY;
	soin.sin_family = AF_INET;
	soin.sin_port = htons(27015);

	int binndres = bind(sock, (sockaddr*)&soin, sizeof(soin));
	if (binndres)
	{
		std::cout << "aaa bind\n";
		std::cout << WSAGetLastError();
		return 4;
	}

	int listr = listen(sock, 10); // 10 is queued connections max
	if (listr)
	{
		std::cout << "cant listen\n";
		return 5;
	}

	char buf[64];
	memset(buf, 0, 64);

	SOCKET recvsockz = INVALID_SOCKET;
	while (true)
	{
		int addrle = 0;
		recvsockz = accept(sock, NULL, NULL);
		if (recvsockz == INVALID_SOCKET)
		{
			//std::cout << "noaccept\n";
			continue;
		}
		else
		{
			DWORD threadid;
			CreateThread(NULL, 0, goshdarnthread, (LPVOID)recvsockz, 0, &threadid);

			recvsockz = INVALID_SOCKET;
			std::cout << "new connection\n";
		}
	}

	std::cout << "Connected\n";

	WSACleanup();

	std::getchar();
};
