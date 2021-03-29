                    Programming Assignment 2
										Ramzey Ghanaim

In this assignment, I created a client server customized protocol on top of UDP
protocol. The goal is to have clients mimic mobile subscribers and request to
access the network.

                     Directory Structure
All source files are in the src/ directory.
All executables are in their respective bin/ directories.
Main directory of this project contains a Makefile for compiling and cleaning
the project files.

Database of phone numbers is included in the serverBin/ directory.

                     Usage Instructions
In the main directory run the following commands

To compile:
make

Once compiled, executables are in their respetive bin directories.

To run the server, specify a port number (required):
./serverBin/server 1111

To run the client specify a port number (required) and client id (optional):
./clientBin/client 1111 66

NOTE: Port number used MUST match on client and server

To remove executables once done:
make clean

