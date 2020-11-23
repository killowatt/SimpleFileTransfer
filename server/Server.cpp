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

#define DEFAULT_PORT "27015"	// Default port if none is specified
#define BUFFER_SIZE 512			// Size of the buffer used when receiving files

// Used to receive data until a set amount in total is received
// Returns true on success and false on failure
bool ReceiveAll(SOCKET receiver, char* buffer, int length)
{
	// Keeps track of the total bytes received during this function
	size_t totalBytes = 0;
	while (totalBytes < length)
	{
		// Receive bytes from the socket and check for errors or disconnect
		int bytesReceived = recv(receiver, buffer, length, 0);
		if (bytesReceived == 0)
		{
			std::cout << "Client disconnected while reading from socket\n";
			return false;
		}
		else if (bytesReceived == SOCKET_ERROR)
		{
			std::cout << "Receiving from socket failed with error "
				<< WSAGetLastError() << "\n";
			return false;
		}

		// Track the number of bytes received so far
		totalBytes += bytesReceived;
	}
	return true;
}

/*
*	Main handler for individual clients
*		client			socket our client is on
*		addressString	the human readable address of our client
*/
void HandleClient(SOCKET client, std::string addressString)
{
	// For simplicity
	const char* address = addressString.c_str();
	std::cout << "Client connected from " << address << "\n";

	/*
	*	Receive the name of the file from the client
	*/
	// Receive the size of the filename
	size_t msgSize = 0;
	if (!ReceiveAll(client, (char*)&msgSize, sizeof(msgSize)))
	{
		std::cout << "Failed to receive file name size from "
			<< address << "\n";
		closesocket(client);
		return;
	}
	msgSize = ntohll(msgSize);

	// Receive the filename using the size received
	std::vector<char> fileNameBuffer(msgSize);
	if (!ReceiveAll(client, fileNameBuffer.data(), (int)msgSize))
	{
		std::cout << "Failed to receive file name from "
			<< address << "\n";
		closesocket(client);
		return;
	}

	// Sanitize the filename so its a name only, no path
	std::filesystem::path fileNamePath = 
		std::filesystem::path(fileNameBuffer.data()).filename();
	std::string fileName = fileNamePath.string();

	/*
	*	Attempt to open up the specified file for writing
	*/
	std::cout << "Receiving file \"" << fileName
		<< "\" from " << address << "\n";

	// Attempt to open the file in binary mode while also truncating
	std::ofstream file(fileName, std::ios::binary | std::ios::out | std::ios::trunc);
	if (file.fail())
	{
		// Don't forget to close the socket if things mess up here
		std::cout << "Failed to open file " << fileName << " for writing\n";
		closesocket(client);
		return;
	}

	/*
	*	Receive all of the file's bytes from the client
	*/
	// Create our buffer and zero out the bytes
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));

	// Receive bytes, looping until the client disconnects
	std::streampos totalBytes = 0;
	while (true)
	{
		// Receive the bytes from the client
		int bytesReceived = recv(client, buffer, sizeof(buffer), 0);
		if (bytesReceived > 0)
		{
			// Track the total bytes received
			totalBytes += bytesReceived;

			// Write the received bytes into our file
			file.write(buffer, bytesReceived);
		}
		else if (bytesReceived == 0)
		{
			// The client disconnected, so stop writing
			std::cout << "Received " << totalBytes
				<< " bytes total from " << address << "\n";
			break;
		}
		else if (bytesReceived == SOCKET_ERROR)
		{
			// There was an error, so stop writing
			std::cout << "Error receiving from " << address
				<< " with error " << WSAGetLastError() << "\n";
			break;
		}
	}

	// Close the file and the socket
	file.close();
	closesocket(client);
}

// We have these outside of main so the signal handler can modify them
SOCKET listener = INVALID_SOCKET;
bool running = true;

// Handles interrupts once we enter our main loop
void SignalHandler(int signum)
{
	std::cout << "Shutting down server\n";

	// Close the listener socket, cleanup WinSock, and exit
	running = false;
	closesocket(listener);
	WSACleanup();

	exit(0);
}

int main(int argc, char* argv[])
{
	// Get our one optional argument, the port number
	const char* port = argc > 1 ? argv[1] : DEFAULT_PORT;

	/*
	*	Initialize the WinSock socket library
	*/
	std::cout << "\nInitializing WinSock...\t";

	WSADATA wsaData;
	int wsaError = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaError)
	{
		std::cout << "Failed\nFailed to initialze WinSock with error "
			<< wsaError << "\n";
		return 1;
	}
	std::cout << "Done\n";

	/*
	*	Attempt to bind a socket so we can start listening
	*/
	// Set up the hint addrinfo
	std::cout << "Creating socket...\t";
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// IPv4 or IPv6, we don't care
	hints.ai_socktype = SOCK_STREAM;	// Reliable byte stream socket
	hints.ai_flags = AI_PASSIVE;		// Use any address

	// Populate our resolved addrinfo with a list of possible socket candidates
	addrinfo* resolved;
	int result = getaddrinfo(nullptr, port, &hints, &resolved);
	if (result)
	{
		std::cout << "Failed\nFailed to resolve address with code "
			<< result << "\n";
		return 1;
	}

	// Iterate through the linked list, trying to create and bind sockets
	addrinfo* node = resolved;
	for (node = resolved; node != nullptr; node = node->ai_next)
	{
		// Try to create a socket, if we fail just move on
		SOCKET attempt = socket(node->ai_family, node->ai_socktype,
			node->ai_protocol);
		if (attempt == SOCKET_ERROR)
			continue;

		// This enables the dual-stack socket so we can use a single socket
		// for both IPv4 and IPv6 connections
		int option = 0;
		setsockopt(attempt, IPPROTO_IPV6, IPV6_V6ONLY,
			(char*)&option, sizeof(option));

		// Attempt to bind our socket and move on if we fail
		if (bind(attempt, node->ai_addr, (int)node->ai_addrlen))
		{
			closesocket(attempt);
			continue;
		}

		// If we succeed at all of the above, set our listener to this attempt
		listener = attempt;
		break;
	}
	// Free our addrinfo linked list from before
	freeaddrinfo(resolved);

	// If we failed to create a socket, error and exit out
	if (listener == INVALID_SOCKET)
	{
		std::cout << "Failed\nNo available sockets found, exiting\n";
		return 1;
	}
	std::cout << "Done\n";

	/*
	*	Start listening on our socket and accept incoming connections
	*/
	// Try to start listening on our new socket
	if (listen(listener, SOMAXCONN))
	{
		std::cout << "Socket listen failed with error "
			<< WSAGetLastError() << "\n";
		return 1;
	}

	// Setup interrupt signal handler for main loop below
	std::signal(SIGINT, SignalHandler);

	std::cout << "Server is now listening on port " << port << "\n"
		<< "Press Ctrl + C to stop the server\n\n";

	// Main loop, accept any client connections and start threads for them
	SOCKET acceptSocket = INVALID_SOCKET;
	sockaddr_storage acceptStorage;
	int storageSize = sizeof(acceptStorage);
	while (running)
	{
		// Accept an incoming connection, checking for errors
		acceptSocket = accept(listener, (sockaddr*)&acceptStorage, &storageSize);
		if (acceptSocket == INVALID_SOCKET)
		{
			int lastError = WSAGetLastError();
			// Suppress annoying error when closing the socket
			if (true || lastError != WSAEINTR)
			{
				std::cout << "Failed to accept connection with error "
					<< WSAGetLastError() << "\n";
			}
			continue;
		}

		// Create a buffer for our address string
		char address[INET6_ADDRSTRLEN] = "unknown address";

		// Get the appropriate sockaddr structure based on if we're IPv4 or IPv6
		sockaddr* socketAddress = nullptr;
		if (acceptStorage.ss_family == AF_INET)
		{
			socketAddress = 
				(sockaddr*)&(((sockaddr_in*)&acceptStorage)->sin_addr);
		}
		else if (acceptStorage.ss_family == AF_INET6)
		{
			socketAddress = 
				(sockaddr*)&(((sockaddr_in6*)&acceptStorage)->sin6_addr);
		}

		// If socket address got set, then get the readable address string
		if (socketAddress)
			inet_ntop(acceptStorage.ss_family, socketAddress, address, sizeof(address));

		// Create a thread for our client
		// Detach it so it doesn't end when it goes out of scope
		std::thread clientThread(HandleClient, acceptSocket, address);
		clientThread.detach();

		// Reset our accept socket for the next one
		acceptSocket = INVALID_SOCKET;
	}
	return 0;
};