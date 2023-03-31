#include "Socket.h"
#include "SQL.h"

int main()
{
    threadPool thpool(std::thread::hardware_concurrency());
    system("pause");
}
