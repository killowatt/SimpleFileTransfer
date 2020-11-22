#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <thread>
#include <filesystem>
#include <csignal>
#include <WinSock2.h>
#include <WS2tcpip.h>

bool ReceiveAll(SOCKET receiver, char* buffer, int length)
{
	size_t totalBytes = 0;
	while (totalBytes < length)
	{
		int bytesReceived = recv(receiver, buffer, length, 0);
		if (bytesReceived == SOCKET_ERROR)
		{
			printf("Receiving from socket failed with error %d\n", WSAGetLastError());
			return false;
		}
		if (bytesReceived == 0)
		{
			printf("Client disconnected while reading from socket\n");
			return false;
		}

		totalBytes += bytesReceived;
	}
	return true;
}

void HandleClient(SOCKET client)
{
	sockaddr_in adds;
	int len = sizeof(adds);
	int sockget = getpeername(client, (sockaddr*)&adds, &len);

	char address[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &adds.sin_addr, address, sizeof(address));

	printf("Client connected from %s\n", address);

	size_t msgSize = 0;
	if (!ReceiveAll(client, (char*)&msgSize, sizeof(size_t)))
		return;
	msgSize = ntohll(msgSize);

	std::vector<char> fileNameBuffer(msgSize);
	if (!ReceiveAll(client, fileNameBuffer.data(), msgSize))
		return;

	std::filesystem::path path = std::filesystem::path(fileNameBuffer.data()).filename();
	std::string fileName = path.string();

	std::cout << "Receiving file " << fileName;
	printf(" from %s\n", address);

	std::ofstream file(fileName, std::ios::binary | std::ios::out | std::ios::trunc);
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
			printf("Error receiving from %s with error %d\n",
				address, WSAGetLastError());
			break;
		}
	}

	file.close();

	closesocket(client);
}

SOCKET listenSocket = INVALID_SOCKET;
bool running = true;
void siggg(int signum)
{
	printf("Shutting down server\n");

	running = false;
	closesocket(listenSocket);
	WSACleanup();

	exit(0);
}

int main()
{
	printf("\n");
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
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
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

	std::signal(SIGINT, siggg);

	printf("Server is now accepting connections\n");
	printf("Press Ctrl + C to stop the server\n");

	SOCKET acceptSocket = INVALID_SOCKET;
	while (running)
	{
		acceptSocket = accept(listenSocket, nullptr, nullptr);
		if (acceptSocket == INVALID_SOCKET)
		{
			int lastError = WSAGetLastError();
			// Suppress annoying error when closing the socket
			if (lastError != WSAEINTR)
				printf("Failed to accept connection with error %d\n", WSAGetLastError());
			continue;
		}
		else
		{
			std::thread clientThread(HandleClient, acceptSocket);
			clientThread.detach();

			acceptSocket = INVALID_SOCKET;
		}
	}
};