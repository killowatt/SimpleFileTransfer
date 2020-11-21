#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <thread>
#include <csignal>
#include <WinSock2.h>
#include <WS2tcpip.h>

std::vector<char> fileBytes;

void HandleClient(SOCKET client)
{
	sockaddr_in adds;
	int len = sizeof(adds);
	int sockget = getpeername(client, (sockaddr*)&adds, &len);

	char address[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET, &adds.sin_addr, address, sizeof(address));

	printf("Client connected from %s\n", address);

	int totalBytes = 0;
	while (totalBytes < fileBytes.size())
	{
		int bytesSent = send(client, fileBytes.data() + totalBytes, fileBytes.size() - totalBytes, 0);
		if (bytesSent == SOCKET_ERROR)
		{
			printf("\nError");
			break;
		}

		totalBytes += bytesSent;
		std::cout << "\r" << totalBytes << "/" << fileBytes.size() << " bytes sent.";
		std::cout.flush();
	}
	printf("\n");

	std::cout << "Finish client haha\n";
	
	closesocket(client);
}

int main()
{
	printf("Please enter the name or path of a file you'd like to host.\n");

	std::ifstream file;
	bool opened = false;
	while (!opened)
	{
		std::string inputFilename;
		std::getline(std::cin, inputFilename);

		file = std::ifstream(inputFilename, std::ios::binary | std::ios::ate);
		if (!file)
		{
			printf("Failed to open file, try again.\n");
			continue;
		}

		opened = true;
	}

	printf("Loading file...\t\t");
	int fileSize = file.tellg();
	file.seekg(0, file.beg);

	fileBytes = std::vector<char>(fileSize);
	file.read(fileBytes.data(), fileSize);

	file.close();
	printf("Done\n");


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
	//soin.sin_addr.s_addr = inet_addr("127.0.0.1");
	//inet_pton(AF_INET, "127.0.0.1", &soin.sin_addr);
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