#pragma once
#include <iostream>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "Market.h"

#pragma comment (lib, "Ws2_32.lib")

//FD_SET
//std async

bool LoginAuthReceive(char* Received)
{
    std::string Username;
    std::string Password;
    for (int i = 0; i < 25; i++)
    {
        if (Received[i] != '$')
            Username += Received[i];
        else
        {
            for (; i < (i + 64); i++)
            {
                Password += Received[i];
            }
        }
    }
    if (Authenticate(Username, Password))
        return true;
    else
        return false;
}

class FileDescManager
{
public:
    FileDescManager(int numberofthreads)
    {
        fd_vec.reserve(numberofthreads);
        SOCKET sok;
        FD_SET(sok, &fd_vec[0]); //last here
    }
    class Connections
    {
    public:
        Connections(SOCKET Socket, sockaddr_in clientaddress)
        {
            sock_ = Socket;
            Clientaddress_ = clientaddress;
        }
        ~Connections()
        {
            closesocket(sock_);
            delete RecBuffer;
            delete SendBuffer;
        }
        void Alive()
        {
            if (LoginAuthReceive(RecBuffer) == 1)
            {
                while (true)
                {
                    ReadfromSocket();
                }
            }
        }
        char* ReadfromSocket()
        {
            int result;
            result = recv(sock_, RecBuffer, sizeof(RecBuffer), 0);
            if (result == SOCKET_ERROR)
            {
                std::cout << "Error receiving data, error code: " << WSAGetLastError() << std::endl;
            }
            else
            {
                return RecBuffer;
            }
        }
        void WritetoSocket()
        {
            int result;
            result = recv(sock_, SendBuffer, sizeof(SendBuffer), 0);
            if (result == SOCKET_ERROR)
            {
                std::cout << "Error sending data, error code: " << WSAGetLastError() << std::endl;
            }
            else if (result == 0)
            {
                std::cout << "Connection closed by client " << std::endl;
            }
            else
            {
                std::cout << "Error code: " << WSAGetLastError() << std::endl;
            }
        }
    private:
        SOCKET sock_;
        sockaddr_in Clientaddress_;
        char RecBuffer[1024];
        char SendBuffer[1024];
    };
private:
    std::vector<fd_set> fd_vec;
};

int SocketMain()
{
    // Initialize Winsock2
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) 
    {
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
    u_long nonblockingmode = 1;
    iResult = ioctlsocket(serverSocket, FIONBIO, &nonblockingmode);
    if (iResult == SOCKET_ERROR) 
    {
        std::cout << "Error setting socket to nonblocking mode: " << WSAGetLastError();
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Bind the socket to a local address and port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(12345); // choose a port number
    iResult = bind(serverSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
    if (iResult == SOCKET_ERROR) 
    {
        std::cout << "Error binding socket: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    // Listen for incoming connections
    iResult = listen(serverSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) 
    {
        std::cout << "Error listening on socket: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Accept incoming connections
    std::vector<Connections> Cons;
    SOCKET clientSocket;
    sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);
    std::cout << "waiting for clients" << std::endl;
    while (true) 
    {
        clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddress, &clientAddressSize);
        if (clientSocket != INVALID_SOCKET)
        {
            if (ioctlsocket(clientSocket, FIONBIO, &nonblockingmode) != 0)
            {
                std::cout << "failed to set client socket to nonblocking mode: " << WSAGetLastError() << std::endl;
                closesocket(clientSocket);
            }
            else
            {
                Cons.emplace_back(Connections(clientSocket, clientAddress));
                std::cout << "connected: " << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port) << std::endl;
            }
        }
        else if (clientSocket == INVALID_SOCKET)
        {
            std::cout << "Error accepting connection from: " << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port) << " Error code: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
        }
    }
    // Close the socket and clean up
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}