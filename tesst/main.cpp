#include "Socket.h"

int main()
{
    int Num;
    std::cout << "Enter Thread Count, Enter 0 for supported Max threads" << std::endl;
    std::cin >> Num;

    if (ServerStartup())
    {
        if(Num==0 || Num > std::thread::hardware_concurrency())
			threadPool thpool(std::thread::hardware_concurrency());
        else
        {
            threadPool thpool(Num);
        }
    }
    else
    {
        exit(0);
        system("Pause");
    }
    ServerShutdown();
    system("Echo Server Shutdown Successful");
}
