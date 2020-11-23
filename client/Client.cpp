#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <chrono>
#include <algorithm>
#include <filesystem>
#include <WinSock2.h>
#include <WS2tcpip.h>

#define CHUNK_SIZE 512

bool SendAll(SOCKET out, const char* buffer, int size)
{
	int totalBytes = 0;
	while (totalBytes < size)
	{
		int bytesSent = send(out, buffer + totalBytes, size - totalBytes, 0);
		if (bytesSent == SOCKET_ERROR)
			return false;

		totalBytes += bytesSent;
	}
	return true;
}

int main(int argc, char* argv[])
{
	printf("\n");
	if (argc < 3)
	{
		printf("Usage: %s file address [port]\n\n", argv[0]);
		printf("Options:\n");
		printf("\tfile:		input file or path to send to the server\n");
		printf("\taddress:	the destination server to send to\n");
		printf("\tport:		desired port to use when connecting\n");
		return 0;
	}

	const char* fileName = argv[1];
	const char* address = argv[2];
	const char* port = argc > 3 ? argv[3] : "27015";

	//
	//
	//
	std::ifstream file(fileName, std::ios::binary | std::ios::ate);
	if (!file)
	{
		printf("Failed to open file, exiting\n");
		return 1;
	}

	std::streamoff fileSize = file.tellg();
	file.seekg(0, file.beg);
	std::cout << "Filesize is " << fileSize << " bytes\n";


	printf("Initializing WinSock...\t");

	WSADATA wsaData;
	int wsaError = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaError)
	{
		printf("Failed\nFailed to initialze WinSock with error %d\n", wsaError);
		return 1;
	}
	printf("Done\n");


	//printf("Creating socket...\t");
	//SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
	//if (server == INVALID_SOCKET)
	//{
	//	printf("Failed\nSocket creation failed with error %d\n", WSAGetLastError());
	//	return 1;
	//}
	//printf("Done\n");

	//sockaddr_in sockad;
	//inet_pton(AF_INET, address, &sockad.sin_addr);
	//sockad.sin_family = AF_INET;
	//sockad.sin_port = htons(27015);

	//printf("Connecting...\t\t");
	//if (connect(server, (sockaddr*)&sockad, sizeof(sockad)))
	//{
	//	printf("Failed\nConnecting failed with error %d\n", WSAGetLastError());
	//	return 1;
	//}
	//printf("Done\n");

	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	addrinfo* somethin;
	int result = getaddrinfo(address, "27015", &hints, &somethin);
	if (result)
	{
		printf("Failure somethin %d\n", result);
		return 1;
	}

	SOCKET server;

	addrinfo* t = somethin;
	for (t = somethin; t != nullptr; t = t->ai_next)
	{
		server = socket(t->ai_family, t->ai_socktype,
			t->ai_protocol);
		if (server == SOCKET_ERROR)
		{
			printf("tried to socket but no\n");
			continue;
		}

		if (connect(server, t->ai_addr, t->ai_addrlen) == SOCKET_ERROR)
		{
			printf("tried connect but fugg\n");
			closesocket(server);
			continue;
		}

		break;
	}
	freeaddrinfo(somethin);

	if (server == INVALID_SOCKET)
	{
		printf("NO CONNECT\n");
		return 1;
	}

	std::filesystem::path prr = std::filesystem::path(fileName).filename();
	std::string mystr = prr.string();

	size_t data = htonll(mystr.size() + 1);
	if (!SendAll(server, (char*)&data, sizeof(size_t)))
	{
		printf("woopsie daisy!\n");
		return 1;
	}

	if (!SendAll(server, mystr.c_str(), mystr.size() + 1))
	{
		printf("Failedfailedlfla\n");
		return 1;
	}

	auto lastTime = std::chrono::high_resolution_clock::now();

	char buffer[CHUNK_SIZE];
	std::streamoff totalBytes = 0;
	while (totalBytes < fileSize)
	{
		size_t chunkSize = std::min<std::streamoff>((std::streamoff)(fileSize - totalBytes), CHUNK_SIZE);

		file.read(buffer, chunkSize);

		if (!SendAll(server, buffer, chunkSize))
		{
			printf("\nSending failed with error %d\n", WSAGetLastError());
			return 1;
		}
		totalBytes += chunkSize;

		auto currentTime = std::chrono::high_resolution_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(
			currentTime - lastTime).count() > 500)
		{
			lastTime = std::chrono::high_resolution_clock::now();
			std::cout << "\r" << totalBytes << "/" << fileSize << " bytes sent";
			std::cout.flush();
		}
	}
	std::cout << "\r" << totalBytes << "/" << fileSize << " bytes sent";
	std::cout.flush();
	printf("\n");

	printf("File successfully sent!\n");
	printf("Disconnecting\n");
	closesocket(server);

	WSACleanup();
};