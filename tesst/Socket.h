#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <mutex>
#include <condition_variable>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <vector>
//#include "Market.h"

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
    /*if (Authenticate(Username, Password))
        return true;
    else
        return false;*/
    return true;
}

class threadPool ////////////////////////////////////////////
{
    class Connections;
private:
    std::vector<fd_set> Readsets;
    std::vector<std::vector<Connections>> Cons;
public:
    threadPool(unsigned int numberofthreads)
    {
        Cons.resize(numberofthreads);
        Threads th(numberofthreads,&Cons,&Readsets);
        Readsets.resize(numberofthreads);
        SocketMain();
    }
    timeval Timeout(long Second,long millisecond)
    {
        timeval Timeout;
        Timeout.tv_sec = Second;
        Timeout.tv_usec = millisecond;
        return Timeout;
    }
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
        SOCKET clientSocket;
        sockaddr_in clientAddress;
        int clientAddressSize = sizeof(clientAddress);
        std::cout << "waiting for clients" << std::endl;
        
        fd_set acceptFds;
        FD_ZERO(&acceptFds);
        FD_SET(serverSocket, &acceptFds);
        timeval T;
        T.tv_usec = 500000;
        T.tv_sec = 0;
        while (true)
        {
            FD_ZERO(&acceptFds);
            FD_SET(serverSocket, &acceptFds);
            iResult = select(serverSocket + 1, &acceptFds, NULL, NULL, &T);
            if (iResult == SOCKET_ERROR)
            {
                std::cout << "Socket error occured, Error code: " << WSAGetLastError() << std::endl;
                continue;
            }
            else if (iResult == 0)
            {
                continue;
            }
            else
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
                        SetManager(clientSocket,clientAddress);
                        wchar_t buffer[1024];
                        DWORD bufferLen = sizeof(buffer) / sizeof(wchar_t);
                        [&clientSocket, &buffer, &bufferLen]() {WSAAddressToStringW((LPSOCKADDR)clientSocket, sizeof(clientSocket), NULL, buffer, &bufferLen); };
                        std::cout << "connected: " << buffer << ":" << ntohs(clientAddress.sin_port) << std::endl;
                    }
                }
                else if (clientSocket == INVALID_SOCKET)
                {
                    wchar_t buffer[1024];
                    DWORD bufferLen = sizeof(buffer) / sizeof(wchar_t);
                    WSAAddressToStringW((LPSOCKADDR)clientSocket, sizeof(clientSocket), NULL, buffer, &bufferLen);
                    std::cout << "Error accepting connection from: " << buffer << ":" << ntohs(clientAddress.sin_port) << " Error code: " << WSAGetLastError() << std::endl;
                    closesocket(serverSocket);
                }
            }
        }
        // Close the socket and clean up
        closesocket(serverSocket);
        WSACleanup();

        return 0;
    }

    int SetSizeFinder() //will loop through the Readsets to deduce which one has the least amount of sockets
    {
        auto min = Readsets[0].fd_count;
        int setindentifier = 0;
        for (int i = 1 ;i<Readsets.size();i++)
        {
            if (min > Readsets[i].fd_count)
            {
                min = Readsets[i].fd_count;
                setindentifier = i;
            }
        }
        return setindentifier;
    }
    void SetManager(SOCKET clientsocket,sockaddr_in Clientaddress)
    {
        int i = SetSizeFinder();
        FD_SET(clientsocket, &Readsets[i]);
        Cons[i].push_back(Connections(clientsocket, Clientaddress));
    }

    class Threads ///////////////////////////////////////////
    {
    public:
        Threads(unsigned int numberofthreads, std::vector<std::vector<Connections>> *ConVec,std::vector<fd_set> *ReadVec)
        {
            for (unsigned int i = 0; i < numberofthreads; i++)
            {
                threads.emplace_back(std::thread(&Threads::run, this, i,ConVec,ReadVec));
                Sleep(200);
            }
        }
        void run(int number, std::vector<std::vector<Connections>> *ConVec, std::vector<fd_set> *ReadVec)
        {
            ThreadNumber = number;
            timeval t;
            t.tv_usec = 100000;
            t.tv_sec = 0;
            /*Sleep(100000000);*/
            while (true)
            {
                select(0, &(*ReadVec)[ThreadNumber], NULL, NULL, &t);
            }
        }
    private:
        std::vector<std::thread> threads;
        std::mutex mtx;
        std::condition_variable cv;
        int ThreadNumber = 0;
    };

    class Connections ////////////////////////////////////////////////////////////////////////////////////////////////
    {
        friend Threads;
        friend threadPool;
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
        u_int FileDesc;
    };
};


