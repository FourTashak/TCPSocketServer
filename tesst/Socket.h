#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <mutex>
#include <condition_variable>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include "Market.h"

#pragma comment (lib, "Ws2_32.lib")

//FD_SET
//std async

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
        Readsets.reserve(numberofthreads);
        Construct_Vecs(numberofthreads);
        Threads th(numberofthreads,Cons,Readsets);
        SocketMain();
    }
    void Construct_Vecs(int size)
    {
        fd_set Temp;
        FD_ZERO(&Temp);
        for (int i = 0; i < size; i++)
        {
            Cons[i].reserve(FD_SETSIZE);
            Readsets.push_back(Temp);
        }
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
        Cons[i].emplace_back(Connections(clientsocket, Clientaddress));
    }

    class Threads ///////////////////////////////////////////
    {
    public:
        Threads(unsigned int numberofthreads, std::vector<std::vector<Connections>> &ConVec,std::vector<fd_set> &ReadVec)
        {
            for (unsigned int i = 0; i < numberofthreads; i++)
            {
                threads.emplace_back(std::thread(&Threads::run, this, i, std::ref(ConVec), std::ref(ReadVec)));
                threads[i].detach();
            }
        }
        void run(int number, std::vector<std::vector<Connections>> &ConVec, std::vector<fd_set> &ReadVec)
        {
            timeval t;
            t.tv_usec = 100000;
            t.tv_sec = 0;
            while (true)
            {
                int Sel = 0;
                Sleep(50);
                FD_ZERO(&(ReadVec[number]));
                for (int i = 0; i < ConVec[number].size(); i++)
				{
                    FD_SET(ConVec[number][i].sock_, &ReadVec[number]);
				}
                if(ReadVec[number].fd_count > 0)
                {
                    Sel = select(FD_SETSIZE, &ReadVec[number], NULL, NULL, &t);
                }
				if (Sel == SOCKET_ERROR)
				{
					std::cout << "Socket Error at thread number : " << number<< std::endl;
					Sleep(50);
				}
				else if (Sel == 0) { Sleep(50); }
				else
				{
					for (int i = 0; i < ReadVec.size(); i++)
					{
						if (FD_ISSET(ConVec[number][i].sock_, &ReadVec[number]))
						{
							char buffer[1024];
							int bytes_rec = recv(ConVec[number][i].sock_, buffer, sizeof(buffer), 0);
							if (bytes_rec == -1) { std::cout << "Socket Error" << std::endl; }
							else if (bytes_rec == 0) // If connection is no longer alive
							{
                                ConVec[number].erase(ConVec[number].begin()+i);
								FD_CLR(ConVec[number][i].sock_, &ReadVec[number]);
							}
							else //If connection is not closed and there is data waiting to be read on socket
							{
								if (ConVec[number][i].DataStream(buffer))
								{
									int result = send(ConVec[number][i].sock_, "Success", 7, 0);
									if (result == SOCKET_ERROR)
										std::cout << "Sending confirmation failed, error code :" << WSAGetLastError() << std::endl;
								}
								else
								{
									int result = send(ConVec[number][i].sock_, "Success", 7, 0);
									if (result == SOCKET_ERROR)
										std::cout << "Sending error of confirmation failed, error code: " << WSAGetLastError() << std::endl;
								}
							}
						}
					}
				}
            }
        }
    private:
        std::vector<std::thread> threads;
        std::mutex mtx;
        std::condition_variable cv;
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
            ThisCustomer.logged_in = false;
            Cus_Map.insert({Name,ThisCustomer});
        }
        bool DataStream(char* Received)
        {
        #define Login 1
        #define Buy 2
        #define Sell 3

            if (std::stoi(&Received[0]) == Login)
            {
                std::string username;
                std::string password;
                for (int i = 2; i < 25; i++)
                {
                    if (Received[i] != '$')
                        username += Received[i];
                    else
                    {
                        for (i += 1; i < (i + 64); i++)
                        {
                            if (Received[i] != '$')
                                password += Received[i];
                            else
                                break;
                        }
                        break;
                    }
                }
                Customer dummy;
                dummy = Authenticate(username, password);
                if (dummy.id==-1)
                    return false;
                else
                {
                    Cus_Map[username].logged_in = true;
                    ThisCustomer = dummy;
                    Name = username;
                    return true;
                }
            }
            else if (std::stoi(&Received[0]) == Buy)
            {
                std::string StockName;
                int Amount;
                std::string Buffer;
                for (int i = 2; i < 6; i++)
                {
                    if (Received[i] != '$')
                        Buffer += Received[i];
                    else
                    {
                        StockName = Buffer;
                        Buffer.clear();
                        for (i += 1; i < (i + 6); i++)
                        {
                            if (Received[i] != '$')
                                Buffer += Received[i];
                            else
                                break;
                        }
                        Amount = std::stoi(Buffer);
                        break;
                    }
                }
                if (BuyStock(Amount, StockName, this->ThisCustomer))
                    return true;
                else
                    return false;
            }
            else if (Received[0] == Sell)
            {
                std::string StockName;
                int Amount;
                std::string Buffer;
                for (int i = 2; i < 6; i++)
                {
                    if (Received[i] != '$')
                        Buffer += Received[i];
                    else
                    {
                        StockName = Buffer;
                        Buffer.clear();
                        for (i += 1; i < (i + 6); i++)
                        {
                            if (Received[i] != '$')
                                Buffer += Received[i];
                            else
                                break;
                        }
                        Amount = std::stoi(Buffer);
                        break;
                    }
                }
                if (SellStock(Amount, StockName, this->ThisCustomer))
                    return true;
                else
                    return false;
            }
        }
    private:
        SOCKET sock_;
        sockaddr_in Clientaddress_;
        std::string Name;
        Customer ThisCustomer;
    };
};


