# Simple File Transfer

# Author
William Yates
CS371-002

# Info
Simple file transfer application that can transfer any type of file, replicating
filenames on both ends.

The program will attempt name resolution and supports both IPv4 and IPv6.

The program should support even large files gigabytes in size.

# Building
Requires Visual Studio 2019.
Use the included solution file, and build using either Debug or Release mode

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
Nothing is preventing multiple clients from sending a file with the same name
at the same time. This shouldn't crash the server, but there is no guarantee
the server version of the file will not be corrupted.

It's possible that when sending the file name string, we could just look for a
null terminator, instead of sending the size of the filename before the
filename itself.

The source tends to mix use of cout and printf, because in my opinion most of
the time printf is cleaner and shorter, however sometimes cout is preferable.