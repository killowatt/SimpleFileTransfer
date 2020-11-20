#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>


std::vector<char> coolbytes;

DWORD WINAPI goshdarnthread(LPVOID arrr)
{
	SOCKET client = (SOCKET)arrr;

	sockaddr_in adds;
	int len = sizeof(adds);
	int sockget = getpeername(client, (sockaddr*)&adds, &len);

	char ipadd[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET, &adds.sin_addr, ipadd, sizeof(ipadd));

	std::cout << "THREAD!!!\n";
	std::cout << "client @ " << ipadd << "\n";


	int sentbytes = 0;
	int totalbytes = coolbytes.size();
	while (sentbytes < totalbytes)
	{
		int sendr = send(client, coolbytes.data() + sentbytes, coolbytes.size() - sentbytes, 0);
		if (sendr == SOCKET_ERROR)
		{
			std::cout << "WOE IS US\n";
			return 5;
		}

		sentbytes += sendr;
		std::cout << "\r" << sentbytes << "/" << totalbytes << " bytes sent.";
		std::cout.flush();
	}


	std::cout << "Finish client haha\n";
	
	closesocket(client);
	return 0;
}


int main()
{
	std::cout << "tell filename\n";

	std::string input;
	std::getline(std::cin, input);

	std::ifstream file(input, std::ios::binary | std::ios::ate);
	if (file.fail())
	{
		std::cout << "failed to open\n";
		return 1;
	}

	int filesize = file.tellg();
	file.seekg(0, file.beg);

	coolbytes = std::vector<char>(filesize);
	file.read(coolbytes.data(), filesize);

	file.close();

	std::cout << "finished loading file\n";









	WSADATA wsaData;

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result)
	{
		std::cout << "Failed to initialize WinSock with error " << result << "\n";
		return 1;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		std::cout << "Failed to create socket with error " << WSAGetLastError() << "\n";
		return 2;
	}

	// OLD STYLE !
	sockaddr_in soin;
	//soin.sin_addr.s_addr = inet_addr("127.0.0.1");
	//inet_pton(AF_INET, "127.0.0.1", &soin.sin_addr);
	soin.sin_addr.s_addr = INADDR_ANY;
	soin.sin_family = AF_INET;
	soin.sin_port = htons(27015);

	int binndres = bind(sock, (sockaddr*)&soin, sizeof(soin));
	if (binndres)
	{
		std::cout << "aaa bind\n";
		std::cout << WSAGetLastError();
		return 4;
	}

	int listr = listen(sock, 10); // 10 is queued connections max
	if (listr)
	{
		std::cout << "cant listen\n";
		return 5;
	}

	SOCKET recvsockz = INVALID_SOCKET;
	while (true)
	{
		int addrle = 0;
		recvsockz = accept(sock, NULL, NULL);
		if (recvsockz == INVALID_SOCKET)
		{
			//std::cout << "noaccept\n";
			continue;
		}
		else
		{
			DWORD threadid;
			CreateThread(NULL, 0, goshdarnthread, (LPVOID)recvsockz, 0, &threadid);

			recvsockz = INVALID_SOCKET;
			std::cout << "new connection\n";
		}
	}

	std::cout << "Connected\n";

	WSACleanup();

	std::getchar();
};
