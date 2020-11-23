#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <thread>
#include <filesystem>
#include <csignal>
#include <list>
#include <WinSock2.h>
#include <WS2tcpip.h>

#define DEFAULT_PORT "27015"
#define FILE_BUFFER_SIZE 512

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

void HandleClient(SOCKET client, const char* address)
{
	printf("Client connected from %s\n", address);

	size_t msgSize = 0;
	if (!ReceiveAll(client, (char*)&msgSize, sizeof(msgSize)))
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

	char buffer[FILE_BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));

	std::streampos totalBytes = 0;
	while (true)
	{
		int bytesReceived = recv(client, buffer, sizeof(buffer), 0);
		if (bytesReceived > 0)
		{
			totalBytes += bytesReceived;

			file.write(buffer, bytesReceived);
			memset(buffer, 0, sizeof(buffer));
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

SOCKET listener = INVALID_SOCKET;
bool running = true;
void siggg(int signum)
{
	printf("Shutting down server\n");

	running = false;
	closesocket(listener);
	WSACleanup();

	exit(0);
}

int main(int argc, char* argv[])
{
	const char* port = argc > 1 ? argv[1] : DEFAULT_PORT;

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


	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	addrinfo* resolved;
	int result = getaddrinfo(nullptr, port, &hints, &resolved);
	if (result)
	{
		printf("Failure somethin %d\n", result);
		return 1;
	}

	addrinfo* node = resolved;
	for (node = resolved; node != nullptr; node = node->ai_next)
	{
		SOCKET attempt = socket(node->ai_family, node->ai_socktype,
			node->ai_protocol);
		if (attempt == SOCKET_ERROR)
			continue;

		int option = 0;
		setsockopt(attempt, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&option, sizeof(option));

		if (bind(attempt, node->ai_addr, node->ai_addrlen))
		{
			closesocket(attempt);
			continue;
		}

		listener = attempt;
		break;
	}
	freeaddrinfo(resolved);

	if (listener == INVALID_SOCKET)
	{
		printf("No available sockets found, exiting\n");
		return 1;
	}

	if (listen(listener, SOMAXCONN))
	{
		printf("Socket listen failed with error %d\n", WSAGetLastError());
		return 1;
	}

	std::signal(SIGINT, siggg);

	printf("Server is now accepting connections\n");
	printf("Press Ctrl + C to stop the server\n");

	SOCKET acceptSocket = INVALID_SOCKET;
	sockaddr_storage acceptStorage;
	int storageSize = sizeof(acceptStorage);
	while (running)
	{
		acceptSocket = accept(listener, (sockaddr*)&acceptStorage, &storageSize);
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
			char address[INET6_ADDRSTRLEN] = "unknown address";

			sockaddr* socketAddress = nullptr;
			if (acceptStorage.ss_family == AF_INET)
			{
				socketAddress = (sockaddr*)&(((sockaddr_in*)&acceptStorage)->sin_addr);
			}
			else if (acceptStorage.ss_family == AF_INET6)
			{
				socketAddress = (sockaddr*)&(((sockaddr_in6*)&acceptStorage)->sin6_addr);
			}

			if (socketAddress)
				inet_ntop(acceptStorage.ss_family, socketAddress, address, sizeof(address));

			std::thread clientThread(HandleClient, acceptSocket, address);
			clientThread.detach();

			acceptSocket = INVALID_SOCKET;
		}
	}
};