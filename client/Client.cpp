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


int main(int argc, char *argv[])
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

	std::streampos fileSize = file.tellg();
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


	printf("Creating socket...\t");
	SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == INVALID_SOCKET)
	{
		printf("Failed\nSocket creation failed with error %d\n", WSAGetLastError());
		return 1;
	}
	printf("Done\n");

	sockaddr_in sockad;
	inet_pton(AF_INET, address, &sockad.sin_addr);
	sockad.sin_family = AF_INET;
	sockad.sin_port = htons(27015);

	printf("Connecting...\t\t");
	if (connect(server, (sockaddr*)&sockad, sizeof(sockad)))
	{
		printf("Failed\nConnecting failed with error %d\n", WSAGetLastError());
		return 1;
	}
	printf("Done\n");

	char buffer[512];

	auto clock = std::chrono::high_resolution_clock::now();

	std::streampos totalBytes = 0;
	while (totalBytes < fileSize)
	{
		file.read(buffer, 512);

		int okk = 0;
		while (okk < 512)
		{
			int bytesSent = send(server, buffer + okk, 512 - okk, 0);
			if (bytesSent == SOCKET_ERROR)
			{
				printf("\nError");
				break;
			}

			okk += bytesSent;
			totalBytes += bytesSent;
		}

		auto noww = std::chrono::high_resolution_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(noww - clock).count() > 500)
		{
			clock = std::chrono::high_resolution_clock::now();
			std::cout << "\r" << totalBytes << "/" << fileSize << " bytes sent";
			std::cout.flush();
		}
	}
	printf("\n");

	printf("File successfully sent!\n");
	printf("Disconnecting\n");
	closesocket(server);

	WSACleanup();
};