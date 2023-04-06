#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <conio.h>
#include "Market.h"
#include "SQL.h"

#pragma comment (lib, "Ws2_32.lib")

class threadPool
{
    class Connections;
    class Threads;
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
        SocketMain(th);
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
    //This is the function where the main thread will keep looping
    int SocketMain(Threads &Th)
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
        T.tv_usec = 100000;
        T.tv_sec = 0;

        //The loop where the main thread will loop indefinitely, accepting connections and passing the connections over to the worker threads
        while (true)
        {
            if (_kbhit()) //The main thread will check if there is any input in the console
            {
                std::string In;
                std::getline(std::cin,In);

                //If the shutdown command is entered into the console, the main thread will request the worker threads to finish up what they and wait for them to join
                if (In == "Shutdown")
                {
                    for (int i = 0; i < Th.threads.size(); i++)
                    {
                        Th.JoinFlag[i] = true;
                        Th.threads[i].join();
                    }
                    break;
                }
            }
            FD_ZERO(&acceptFds);
            FD_SET(serverSocket, &acceptFds);

            //select is used to check the server socket if there are any connections waiting to be accepted
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
                //the incoming connection is accepted here
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
                        //The SetManager functions purpose is to evenly distribute incomming connections to each thread
                        //it will always assign the new connection to the thread that has the least connections to manage, this evens the load on all threads.
                        SetManager(clientSocket,clientAddress);
                        char addrStr[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &(clientAddress.sin_addr), addrStr, INET_ADDRSTRLEN);
                        std::cout << "Connected: " << addrStr <<":"<< clientAddress.sin_port <<std::endl;
                    }
                }
                else if (clientSocket == INVALID_SOCKET)
                {
                    char addrStr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(clientAddress.sin_addr), addrStr, INET_ADDRSTRLEN);
                    std::cout << "Error accepting connection from: " << addrStr << ":" << clientAddress.sin_port << " Error code: " << WSAGetLastError() << std::endl;
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
        std::unique_ptr<SOCKET> clientsocketnew(new SOCKET(clientsocket));
        Cons[i].emplace_back(Connections(std::move(clientsocketnew), Clientaddress));
    }

    class Threads
    {
        friend threadPool;
    public:
        Threads(unsigned int numberofthreads, std::vector<std::vector<Connections>>& ConVec, std::vector<fd_set>& ReadVec)
        {
            JoinFlag.resize(numberofthreads);
            for (unsigned int i = 0; i < numberofthreads; i++)
            {
                //Worker threads get emplaced back into the vector and start running the run function
                threads.emplace_back(std::thread(&Threads::run, this, i, std::ref(ConVec), std::ref(ReadVec), std::ref(JoinFlag)));
            }
        }
        //This function is used when authentication is successful
        //it send stock name and prices, and how many stocks the customer has and balance
        int SendCusInfo(SOCKET clientsocket,std::string Cus)
        {
            Sleep(200);
            std::string buffer;
            for (int i = 0; i < Market.size(); i++)
                buffer += (Cus_Map[Cus].Customershares[i].Share_Name + "$" + std::to_string(Cus_Map[Cus].Customershares[i].Share_Quantity) + "$" + std::to_string(Market[i].price) + "$");

            int result = send(clientsocket, buffer.c_str(), buffer.size(), 0);
            if (result == SOCKET_ERROR)
            {
                return false;
            }
            buffer.clear();
            buffer = std::to_string(Cus_Map[Cus].Balance);
            Sleep(1000);
            result = send(clientsocket, buffer.c_str(), buffer.size(), 0);
            if (result == SOCKET_ERROR)
            {
                return false;
            }
            return true;
        }
        //this is the main function for the worker threads, here the worker threads are in an infinite loop where they check on sockets assigned to them
        //and receive data if there is data to be received.
        void run(int number, std::vector<std::vector<Connections>> &ConVec, std::vector<fd_set> &ReadVec, std::vector<bool> &Joinflag)
        {
            timeval t;
            t.tv_usec = 100000;
            t.tv_sec = 0;
            while (true)
            {
                //if the joinflag gets set by the main thread the worker thread will end all client connections and join.
                if (JoinFlag[number] == true)
                {
                    for (int i = 0; i < ConVec[number].size(); i++)
                    {
                        ConVec[number][i].~Connections();
                    }
                    break;
                }
                int Sel = 0;
                Sleep(50);
                //Cleans the fd_set
                FD_ZERO(&(ReadVec[number]));
                //sets the fd_set with assigned sockets
                for (int i = 0; i < ConVec[number].size(); i++)
				{
                    FD_SET(*ConVec[number][i].sock_.get(), &ReadVec[number]);
				}
                if(ReadVec[number].fd_count > 0)
                {
                    //the select function checks if there is data to be received
                    Sel = select(FD_SETSIZE, &ReadVec[number], NULL, NULL, &t);
                }
				if (Sel == SOCKET_ERROR)
				{
					std::cout << "Socket Error at thread number : " << number << std::endl;
					Sleep(50);
				}
				else if (Sel == 0) { Sleep(50); }
				else
				{
                    //loops through every socket and checks which socket has data to be received
					for (int i = 0; i < ReadVec[number].fd_count; i++)
					{
						if (FD_ISSET(*ConVec[number][i].sock_.get(), &ReadVec[number]))
						{
							char buffer[1024];
							int bytes_rec = recv(*ConVec[number][i].sock_.get(), buffer, sizeof(buffer), 0);
							if (bytes_rec == -1) //if theres an error with the socket, the thread will set the customer as not logged in and will remove the socket from the vector containing it
                            {
                                std::cout << "Socket Error, Disconnecting Client" << std::endl;
                                Cus_Map[ConVec[number][i].Name].logged_in = false;
                                FD_CLR(*ConVec[number][i].sock_.get(), &ReadVec[number]);
                                ConVec[number].erase(ConVec[number].begin() + i);
                            }
							else if (bytes_rec == 0) // If connection is no longer alive, the thread will set the customer as not logged in and will remove the socket from the vector containing it
							{
                                std::cout << "Socket Error, Disconnecting Client" << std::endl;
                                Cus_Map[ConVec[number][i].Name].logged_in = false;
                                FD_CLR(*ConVec[number][i].sock_.get(), &ReadVec[number]);
                                ConVec[number].erase(ConVec[number].begin()+i);
							}
							else //If connection is not closed and there is data waiting to be read on socket
							{
								if (ConVec[number][i].DataStream(buffer))//DataStream is called to identify and process the request
								{
                                    //if ther request is successful, the server send '1' as confirmation
									int result = send(*ConVec[number][i].sock_.get(), "1", 3, 0);
									if (result == SOCKET_ERROR)
										std::cout << "Sending confirmation failed, error code :" << WSAGetLastError() << std::endl;
                                    if(buffer[0] == '1')
                                    {
                                        //if the first char of the received data is '1' this means it is a log in request
                                        //the server will reply with the customer information
                                        result = SendCusInfo(*ConVec[number][i].sock_.get(), ConVec[number][i].Name);
                                        if (result == SOCKET_ERROR)
                                            std::cout << "Sending confirmation failed, error code :" << WSAGetLastError() << std::endl;
                                    }
                                }
								else
								{
                                    //if the username and password is wrong or a buy, sell request did not go through 
                                    //the server replies with '0' as a failiure
									int result = send(*ConVec[number][i].sock_.get(), "0", 7, 0);
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
        std::vector<bool> JoinFlag;
    };

    class Connections
    {
        friend Threads;
        friend threadPool;
    public:
        Connections(std::unique_ptr<SOCKET> Socket, sockaddr_in clientaddress)
        {
            sock_.reset(Socket.release());
			Clientaddress_ = clientaddress;
        }
        ~Connections()
        {
            closesocket((SOCKET)sock_.get());
        }
        bool DataStream(char* Received)
        {
        #define Login 1
        #define Buy 2
        #define Sell 3
        #define Refresh 4

            if (std::stoi(&Received[0]) == Login)
            {
                std::string username;
                std::string password;
                for (int i = 1; i < 25; i++)
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
                if (dummy.id==-1 || dummy.logged_in == true ) 
                    return false;
                else
                {
                    Cus_Map[username].logged_in = true;
                    Name = dummy.Name;
                    Name = username;
                    return true;
                }
            }
            else if (std::stoi(&Received[0]) == Buy && Cus_Map[Name].logged_in == true)
            {
                std::string StockName;
                int Amount;
                std::string Buffer;
                for (int i = 1; i < 6; i++)
                {
                    if (Received[i] != '$')
                        Buffer += Received[i];
                    else
                    {
                        StockName = Buffer;
                        Buffer.clear();
                        int j = (i + 6);
                        i++;
                        for (; i < j; i++)
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
                //after the data is extracted the buystock function is called
                if (BuyStock(Amount, StockName, Cus_Map[this->Name]) && Cus_Map[Name].logged_in == true)
                    return true;
                else
                    return false;
            }
            else if (std::stoi(&Received[0]) == Sell)
            {
                std::string StockName;
                int Amount;
                std::string Buffer;
                for (int i = 1; i < 6; i++)
                {
                    if (Received[i] != '$')
                        Buffer += Received[i];
                    else
                    {
                        StockName = Buffer;
                        Buffer.clear();
                        int j = (i + 6);
                        i++;
                        for (; i < j; i++)
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
                //after the data is extracted the sellstock function is called
                if (SellStock(Amount, StockName, Cus_Map[this->Name]) && Cus_Map[Name].logged_in == true)
                    return true;
                else
                    return false;
            }
            else if (std::stoi(&Received[0]) == Refresh)
            {
                //when refresh is requested the server replies with updated stock prices
                std::string Buffer = "4";
                for (int i = 0; i < Market.size(); i++)
                {
                    Buffer += std::to_string(Market[i].price) + "$";
                }
                send(*this->sock_.get(), Buffer.c_str(), Buffer.size(), NULL);
            }
        }
    private:
        std::shared_ptr<SOCKET> sock_;
        sockaddr_in Clientaddress_;
        std::string Name;
    };
};


