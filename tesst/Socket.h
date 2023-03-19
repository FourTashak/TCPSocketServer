#pragma once
#include <iostream>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>

#pragma comment (lib, "Ws2_32.lib")

//FD_SET
//std async

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
    }
    void ReadfromSocket()
    {
        int result;
        result = recv(sock_, RecBuffer, sizeof(RecBuffer),0);
        if (result == SOCKET_ERROR)
        {
            std::cout << "Error receiving data from client: "<< (int*)&Clientaddress_.sin_addr << " error code: " << WSAGetLastError() << std::endl;
        }
    }
    void WritetoSocket()
    {
        int result;
        result = recv(sock_, SendBuffer, sizeof(SendBuffer), 0);
        if (result == SOCKET_ERROR)
        {
            std::cout << "Error sending data to client: "<< (int*)&Clientaddress_.sin_addr <<" error code: " << WSAGetLastError() << std::endl;
        }
        else if (result == 0)
        {
            std::cout << "Connection closed by client: "<< (int*)&Clientaddress_.sin_addr << std::endl;
        }
        else
        {
            std::cout << "Error receiving data from: "<< (int*)&Clientaddress_.sin_addr << " error code: " << WSAGetLastError() << std::endl;
        }
    }
private:
    SOCKET sock_;
    sockaddr_in Clientaddress_ ;
    char RecBuffer[1024];
    char SendBuffer[1024];
};

class threadPool //
{
public:
    threadPool()
    {
        Threads th(std::thread::hardware_concurrency());
    }

    class Threads //
    {
    private:
        std::vector<std::thread> threads;
        std::mutex mtx;
        std::condition_variable cv;

    public:
        void run()
        {
            std::cout << std::this_thread::get_id() << std::endl;
        }
        void waitforturn()
        {
            std::unique_lock<std::mutex> lck{ mtx };
            cv.wait(lck);
            run();
        }
        void joinTh()
        {
            for (auto& th : threads)
            {
                th.join();
            }
        }
        Threads(unsigned int numberofthreads)
        {
            numberofthreads = std::thread::hardware_concurrency();
            for (unsigned int i = 0; i < numberofthreads; i++)
            {
                threads.emplace_back(std::thread(&Threads::waitforturn, this));
                Sleep(200);
                cv.notify_one();
                threads[i].join();
            }
        }

    };
    class work //
    {
    private:

    public:
        work()
        {

        }
    };

private:
    std::deque<work> Tasks;
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
    clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddress, &clientAddressSize);
    if (clientSocket != INVALID_SOCKET)
    {
        Cons.emplace_back(Connections(clientSocket,clientAddress));
        std::cout << "connected" << std::endl;
    }
    else if (clientSocket == INVALID_SOCKET) 
    {
        std::cout << "Error accepting connection: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    // Close the socket and clean up
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}