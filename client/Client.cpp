#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <WinSock2.h>
#include <WS2tcpip.h>

//addrinfo hints;
//memset(&hints, 0, sizeof(hints));
//hints.ai_family = AF_UNSPEC;
//hints.ai_socktype = SOCK_STREAM;

//addrinfo* somethin;
//int result = getaddrinfo(line.c_str(), "27015", &hints, &somethin);
//if (result)
//{
//	printf("Failure somethin %d\n", result);
//	return 1;
//}

//addrinfo* t = somethin;
//while (t != nullptr)
//{
//	if 
//}


int main()
{
	printf("Please enter the name or path of a file you'd like to send.\n");

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

	std::vector<char> fileBytes(fileSize);
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


	std::cout << "what is ip\n";
	std::string line;
	std::getline(std::cin, line);

	sockaddr_in sockad;
	inet_pton(AF_INET, line.c_str(), &sockad.sin_addr);
	sockad.sin_family = AF_INET;
	sockad.sin_port = htons(27015);

	printf("Creating socket...\t");
	SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == INVALID_SOCKET)
	{
		printf("Failed\nSocket creation failed with error %d\n", WSAGetLastError());
		return 1;
	}
	printf("Done\n");

	printf("Connecting...\t\t");
	if (connect(server, (sockaddr*)&sockad, sizeof(sockad)))
	{
		printf("Failed\nConnecting failed with error %d\n", WSAGetLastError());
		return 1;
	}
	printf("Done\n");


	int totalBytes = 0;
	while (totalBytes < fileBytes.size())
	{
		int bytesSent = send(server, fileBytes.data() + totalBytes, fileBytes.size() - totalBytes, 0);
		if (bytesSent == SOCKET_ERROR)
		{
			printf("\nError");
			break;
		}

		totalBytes += bytesSent;
		std::cout << "\r" << totalBytes << "/" << fileBytes.size() << " bytes sent";
		std::cout.flush();
	}

	printf("File successfully sent!\n");
	printf("Disconnecting\n");
	closesocket(server);

	WSACleanup();
};