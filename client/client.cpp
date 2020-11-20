#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

int main()
{
	std::cout << "ok\n";

	WSADATA wsaData;

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result)
	{
		std::cout << "Failed to initialize WinSock with error " << result << "\n";
		return 1;
	}

	std::cout << "what is ip\n";
	std::string line;
	std::getline(std::cin, line);


	sockaddr_in sockad;
	//sockad.sin_addr.s_addr = inet_addr("127.0.0.1");
	inet_pton(AF_INET, line.c_str(), &sockad.sin_addr);
	//sockad.sin_addr.s_addr = INADDR_LOOPBACK;
	sockad.sin_family = AF_INET;
	sockad.sin_port = htons(27015);

	SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == INVALID_SOCKET)
	{
		std::cout << "damn\n";
		return 1;
	}

	std::cout << "socket create\n";

	int conneccc = connect(server, (sockaddr*)&sockad, sizeof(sockad));
	if (conneccc)
	{
		std::cout << "FAIL CONNEC!!\n";
		std::cout << WSAGetLastError();
		return 2;
	}

	std::cout << "connect\n";

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

	std::ofstream output("received.mp4", std::ios::binary | std::ios::out | std::ios::trunc);
	if (output.fail())
	{
		std::cout << "couldn't open file\n";
		return 0;
	}

	const int BUFSIZE = 512;
	char buf[BUFSIZE];
	memset(buf, 0, BUFSIZE);

	int totalll = 0;

	while (true)
	{
		int numbyt = recv(server, buf, BUFSIZE, 0);
		if (numbyt > 0)
		{
			totalll += numbyt;

			std::cout << totalll << " Bytes received! " << numbyt << " bytes!\r";
			std::cout.flush();
			//std::cout << buf << "\n";

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
	
	std::cout << "closing file\n";

	output.close();

	std::cout << "over\n";


	closesocket(server);

	WSACleanup();

	std::getchar();
};