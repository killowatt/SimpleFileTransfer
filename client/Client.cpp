#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <chrono>
#include <algorithm>
#include <filesystem>
#include <WinSock2.h>
#include <WS2tcpip.h>

#define DEFAULT_PORT "27015"	// Default port if none is specified
#define CHUNK_SIZE 512			// Size of chunks we read our file into and send

// Used to send repeatedly until all data has been sent
// Returns true on success and false on failure
bool SendAll(SOCKET out, const char* buffer, int size)
{
	// Keeps track of the total bytes sent during this function
	int totalBytes = 0;
	while (totalBytes < size)
	{
		// Send bytes on the socket and check for any error
		int bytesSent = send(out, buffer + totalBytes, size - totalBytes, 0);
		if (bytesSent == SOCKET_ERROR)
			return false;

		// Track the number of bytes sent so far
		totalBytes += bytesSent;
	}
	return true;
}

int main(int argc, char* argv[])
{
	// Ensure all arguments are provided, and if not, print usage statement
	printf("\n");
	if (argc < 3)
	{
		printf("Usage: %s file address [port]\n\n", argv[0]);
		printf("Options:\n");
		printf("\tfile:		input file or path to send to the server\n");
		printf("\taddress:	the destination server to send to\n");
		printf("\tport:		desired port to use when connecting, default %s\n",
			DEFAULT_PORT);

		return 0;
	}

	/*
	*	Attempt to open our file and get relevant information
	*/

	// Get our two main arguments and optional argument
	const char* filePath = argv[1];
	const char* address = argv[2];
	const char* port = argc > 3 ? argv[3] : DEFAULT_PORT;

	// Attempt to open the file provided by the user
	std::ifstream file(filePath, std::ios::binary | std::ios::ate);
	if (!file)
	{
		printf("Failed to open file, exiting\n");
		return 1;
	}

	// Get the size of the file for later
	std::streamoff fileSize = file.tellg();
	file.seekg(0, file.beg);
	std::cout << "Filesize is " << fileSize << " bytes\n";

	// Get just the file's name, not the whole path
	std::filesystem::path fileNamePath = std::filesystem::path(filePath).filename();
	std::string fileName = fileNamePath.string();

	/*
	*	Initialize the WinSock socket library
	*/
	printf("Initializing WinSock...\t");

	WSADATA wsaData;
	int wsaError = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaError)
	{
		printf("Failed\nFailed to initialze WinSock with error %d\n", wsaError);
		return 1;
	}
	printf("Done\n");

	/*
	*	Attempt to connect to the specified destination
	*/

	// Set up the hint addrinfo
	printf("Connecting...\t\t");
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// IPv4 or IPv6, we don't care
	hints.ai_socktype = SOCK_STREAM;	// Reliable byte stream socket

	// Populate our resolved addrinfo with a list of possible socket candidates
	addrinfo* resolved;
	int result = getaddrinfo(address, port, &hints, &resolved);
	if (result)
	{
		printf("Failure somethin %d\n", result);
		return 1;
	}

	// Try to create and connect using each info from each element in the list
	SOCKET server = INVALID_SOCKET;
	addrinfo* node = resolved;
	for (node = resolved; node != nullptr; node = node->ai_next)
	{
		// Try to create a socket, if we fail just move on
		SOCKET attempt = socket(node->ai_family, node->ai_socktype,
			node->ai_protocol);
		if (attempt == SOCKET_ERROR)
			continue;

		// Try to connect using our new socket, move on if we fail
		if (connect(attempt, node->ai_addr, (int)node->ai_addrlen))
		{
			closesocket(attempt);
			continue;
		}

		// If we succeed, set our server socket to this attempt
		server = attempt;
		break;
	}
	// Free our addrinfo linked list from before
	freeaddrinfo(resolved);

	// If we failed to connect, exit out
	if (server == INVALID_SOCKET)
	{
		printf("Failed\nConnection failed, exiting\n");
		return 1;
	}
	printf("Done\n");

	/*
	*	Send the name of our file to the server
	*/

	// Send the size of the filename
	size_t data = htonll(fileName.size() + 1);
	if (!SendAll(server, (char*)&data, sizeof(data)))
	{
		// Make sure to disconnect and clean up if we fail
		printf("Failed to send file name size\n");
		closesocket(server);
		WSACleanup();
		return 1;
	}

	// Send the filename itself
	if (!SendAll(server, fileName.c_str(), (int)fileName.size() + 1))
	{
		// Make sure to disconnect and clean up if we fail
		printf("Failed to send file name\n");
		closesocket(server);
		WSACleanup();
		return 1;
	}

	/*
	*	Send the all of the specified file's bytes to the server
	*/
	auto lastTime = std::chrono::high_resolution_clock::now();

	char buffer[CHUNK_SIZE];
	std::streamoff totalBytes = 0;
	while (totalBytes < fileSize)
	{
		// Determine the size of the chunk, then read that amount from file
		size_t chunkSize = std::min<std::streamoff>((std::streamoff)(fileSize - totalBytes), CHUNK_SIZE);
		file.read(buffer, chunkSize);

		// Send all the bytes from the chunk we just read
		if (!SendAll(server, buffer, (int)chunkSize))
		{
			printf("\nSending failed with error %d\n", WSAGetLastError());
			break;
		}
		totalBytes += chunkSize;

		// This updates the console log of number of bytes sent
		// We only update the timer every half-second for performance
		auto currentTime = std::chrono::high_resolution_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(
			currentTime - lastTime).count() > 500)
		{
			// Use carriage returns so the ticker stays in place
			lastTime = std::chrono::high_resolution_clock::now();
			std::cout << "\r" << totalBytes << "/" << fileSize << " bytes sent";
			std::cout.flush();
		}
	}
	// Once we're done, print the total number of bytes sent for our user
	std::cout << "\r" << totalBytes << "/" << fileSize << " bytes sent";
	std::cout.flush();
	printf("\n");

	// Disconnect and shut down WinSock
	printf("File successfully sent!\n");
	printf("Disconnecting\n");
	closesocket(server);

	WSACleanup();
};