#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <filesystem>
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

#define CHUNK_SIZE 512

bool SendAll(SOCKET out, const char* buffer, size_t size)
{
	size_t totalBytes = 0;
	while (totalBytes < size)
	{
		int bytesSent = send(out, buffer + totalBytes, size - totalBytes, 0);
		if (bytesSent == SOCKET_ERROR)
			return false;
		if (bytesSent == 0)
		{
			printf("WAT\n");
			exit(3);
		}

		totalBytes += bytesSent;
	}
	return true;
}

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

	std::filesystem::path prr = std::filesystem::path(fileName).filename();
	std::string mystr = prr.string();


	//int jesus = 0;
	//size_t data = mystr.size() + 1;
	//while (jesus < sizeof(size_t))
	//{
	//	int bytesSent = send(server, (char*)&data + jesus, sizeof(size_t) - jesus, 0);
	//	jesus += bytesSent;

	//	if (bytesSent == SOCKET_ERROR)
	//	{
	//		printf("socket errr\n %d\n", WSAGetLastError());
	//		return 1;
	//	}
	//}

	//int tempott = 0;
	//while (tempott < mystr.size() + 1)
	//{
	//	int bytesSent = send(server, mystr.c_str() + tempott, mystr.size() + 1 - tempott, 0);
	//	if (bytesSent == SOCKET_ERROR)
	//	{
	//		printf("error on filename send\n");
	//		return 1;
	//	}
	//	
	//	tempott += bytesSent;
	//}

	size_t data = mystr.size() + 1;
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

	auto clock = std::chrono::high_resolution_clock::now();

	char buffer[CHUNK_SIZE];

	std::streamoff totalBytes = 0;
	while (totalBytes < fileSize)
	{
		file.read(buffer, CHUNK_SIZE);

		int chunkBytes = 0;
		size_t oksize = std::min<std::streamoff>((std::streamoff)(fileSize - totalBytes), CHUNK_SIZE);
		while (chunkBytes < oksize)
		{
			int bytesSent = send(server, buffer + chunkBytes, oksize - chunkBytes, 0);
			if (bytesSent == SOCKET_ERROR)
			{
				printf("\nError..%d", WSAGetLastError());
				break;
			}

			chunkBytes += bytesSent;
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
	std::cout << "\r" << totalBytes << "/" << fileSize << " bytes sent";
	std::cout.flush();
	printf("\n");

	printf("File successfully sent!\n");
	printf("Disconnecting\n");
	closesocket(server);

	WSACleanup();
};