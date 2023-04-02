#include "Socket.h"

int main()
{
    ServerStartup();
    /*threadPool thpool(std::thread::hardware_concurrency());*/
    ServerShutdown();
    system("pause");
}
