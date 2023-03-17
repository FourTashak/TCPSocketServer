#pragma once
#include <iostream>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")

int SocketMain()
{
    // Initialize Winsock2
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    // Create a socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Error creating socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Bind the socket to a local address and port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(12345); // choose a port number
    iResult = bind(serverSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
    if (iResult == SOCKET_ERROR) {
        std::cout << "Error binding socket: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    iResult = listen(serverSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cout << "Error listening on socket: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Accept incoming connections
    SOCKET clientSocket;
    sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);
    clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddress, &clientAddressSize);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Error accepting connection: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Send and receive data
    char buffer[1024];
    iResult = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (iResult > 0) {
        std::cout << "Received data: " << buffer << std::endl;
        iResult = send(clientSocket, buffer, iResult, 0);
        if (iResult == SOCKET_ERROR) {
            std::cout << "Error sending data: " << WSAGetLastError() << std::endl;
        }
    }
    else if (iResult == 0) {
        std::cout << "Connection closed by client." << std::endl;
    }
    else {
        std::cout << "Error receiving data: " << WSAGetLastError() << std::endl;
    }

    // Close the socket and clean up
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}