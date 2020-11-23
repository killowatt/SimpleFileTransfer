# Simple File Transfer

# Author
William Yates
CS371-002

# Info
Simple file transfer application that can transfer any type of file,
replicating filenames on both ends. The program should support even
large files gigabytes in size.

# Building
Use the included solution file, and build using either Debug or Release mode
Requires Visual Studio 2019

# Usage
Server - ./server [port]
    You can run the server using just ./server, though it takes a port number
    as an optional argument

Client - ./client file address [port]
	file:		input file or path to send to the server
	address:	the destination server to send to
	port:		desired port to use when connecting, default 27015

# Technical


# Notes
files can be written at same time from server if multiple clients

we could just look for a null terminator on receive instead of sending the size of the string