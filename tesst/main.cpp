#include "Socket.h"

int main()
{
    ServerStartup();
    threadPool thpool(std::thread::hardware_concurrency());
    ServerShutdown();
    system("Echo Server Shutdown Successful");
}
