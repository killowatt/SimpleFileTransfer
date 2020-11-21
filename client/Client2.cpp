#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <WinSock2.h>
#include <WS2tcpip.h>

// old message code
//std::string input;
//while (std::getline(std::cin, input))
//{
//	if (input == "close")
//		break;

//	int sendr = send(server, input.c_str(), input.size(), 0);
//	if (sendr == SOCKET_ERROR)
//	{
//		std::cout << "WOE IS US\n";
//		return 5;
//	}
//}

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

	std::cout << "what is ip\n";
	std::string line;
	std::getline(std::cin, line);


	sockaddr_in sockad;
	//sockad.sin_addr.s_addr = inet_addr("127.0.0.1");
	inet_pton(AF_INET, line.c_str(), &sockad.sin_addr);
	//sockad.sin_addr.s_addr = INADDR_LOOPBACK;
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


	std::ofstream output("received.mp4", std::ios::binary | std::ios::out | std::ios::trunc);
	if (output.fail())
	{
		std::cout << "couldn't open file\n";
		return 1;
	}

	const int BUFSIZE = 512;
	char buf[BUFSIZE];
	memset(buf, 0, BUFSIZE);

	int totalll = 0;

	auto cloc = std::chrono::high_resolution_clock::now();
	while (true)
	{
		int numbyt = recv(server, buf, BUFSIZE, 0);
		if (numbyt > 0)
		{
			totalll += numbyt;

			auto clocnow = std::chrono::high_resolution_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(clocnow - cloc).count() >= 250)
			{
				cloc = std::chrono::high_resolution_clock::now();
				std::cout << "\r" << totalll << " Bytes received! " << numbyt << " bytes!";
				std::cout.flush();
				//std::cout << buf << "\n";
			}

			output.write(buf, numbyt);

			memset(buf, 0, BUFSIZE);

		}
		else if (numbyt == 0)
		{
			std::cout << "disconnect.\n";
			break;
		}
		else if (numbyt == SOCKET_ERROR)
		{
			std::cout << "ERROR!! " << WSAGetLastError() << "\n";
			break;
		}
	}
	std::cout << "received " << totalll << " bytes total.\n";
	
	std::cout << "closing file\n";

	output.close();

	std::cout << "over\n";


	closesocket(server);

	WSACleanup();
};