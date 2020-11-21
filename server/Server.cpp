#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <thread>
#include <csignal>
#include <WinSock2.h>
#include <WS2tcpip.h>

void HandleClient(SOCKET client)
{
	sockaddr_in adds;
	int len = sizeof(adds);
	int sockget = getpeername(client, (sockaddr*)&adds, &len);

	char address[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &adds.sin_addr, address, sizeof(address));

	printf("Client connected from %s\n", address);

	size_t msgSize = 0;
	int recvd = 0;
	while (recvd < sizeof(size_t))
	{
		int bytesReceived = recv(client, (char*)&msgSize + recvd, sizeof(size_t) - recvd, 0);

		if (bytesReceived > 0)
		{
			recvd += bytesReceived;
		}
		else if (bytesReceived == 0)
		{
			// conn closed
			return;
		}
		else if (bytesReceived == SOCKET_ERROR)
		{
			// err
			return;
		}
	}

	std::cout << msgSize << "\n";

	std::vector<char> lol(msgSize);
	int tottt = 0;
	while (tottt < msgSize)
	{
		int bytesReceived = recv(client, lol.data() + tottt, msgSize - tottt, 0);
		if (bytesReceived > 0)
		{
			tottt += bytesReceived;
		}
		else if (bytesReceived == 0)
		{
			// conn closed
			return;
		}
		else if (bytesReceived == SOCKET_ERROR)
		{
			// err
			return;
		}
	}

	for (char c : lol)
	{
		std::cout << c;
	}
	std::cout << "\n";

	std::string finalnam(lol.data());
	std::cout << "FILE NAME! " << finalnam << "\n";

	std::ofstream file(finalnam, std::ios::binary | std::ios::out | std::ios::trunc);
	if (file.fail())
	{
		printf("Failed to open file for writing\n");
		closesocket(client);
		return;
	}

	const int BUFSIZE = 512;
	char buffer[BUFSIZE];
	memset(buffer, 0, BUFSIZE);

	std::streampos totalBytes = 0;
	while (true)
	{
		int bytesReceived = recv(client, buffer, BUFSIZE, 0);
		if (bytesReceived > 0)
		{
			totalBytes += bytesReceived;

			file.write(buffer, bytesReceived);
			memset(buffer, 0, BUFSIZE);
		}
		else if (bytesReceived == 0)
		{
			std::cout << "Received " << totalBytes;
			printf(" bytes total from %s\n", address);
			break;
		}
		else if (bytesReceived == SOCKET_ERROR)
		{
			printf("Error receiving from %s with error %d",
				address, WSAGetLastError());
			break;
		}
	}

	file.close();

	closesocket(client);
}

int main()
{
	printf("Initializing WinSock...\t");

	WSADATA wsaData;
	int wsaError = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaError)
	{
		printf("Failed\nFailed to initialze WinSock with error %d\n", wsaError);
		return 1;
	}
	printf("Done\n");


	printf("Creating socket...\t");
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
	{
		printf("Failed\nSocket creation failed with error %d\n", WSAGetLastError());
		return 1;
	}
	printf("Done\n");

	// OLD STYLE !
	sockaddr_in soin;
	soin.sin_addr.s_addr = INADDR_ANY;
	soin.sin_family = AF_INET;
	soin.sin_port = htons(27015);

	if (bind(listenSocket, (sockaddr*)&soin, sizeof(soin)))
	{
		printf("Socket binding failed with error %d\n", WSAGetLastError());
		return 1;
	}

	if (listen(listenSocket, SOMAXCONN))
	{
		printf("Socket listen failed with error %d\n", WSAGetLastError());
		return 1;
	}

	SOCKET acceptSocket = INVALID_SOCKET;
	while (true)
	{
		acceptSocket = accept(listenSocket, nullptr, nullptr);
		if (acceptSocket == INVALID_SOCKET)
		{
			printf("Failed to accept connection with error %d", WSAGetLastError());
			continue;
		}
		else
		{
			std::thread clientThread(HandleClient, acceptSocket);
			clientThread.detach();

			acceptSocket = INVALID_SOCKET;
		}
	}

	// WE NEVER REACH THIS!!
	// AAAAAAAAAAAAAAAAAA
	// AAAAAAAAAAAAAAAA
	printf("Server ending\n");
	WSACleanup();
};