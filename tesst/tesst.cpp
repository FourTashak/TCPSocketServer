#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>
#include "Socket.h"

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

int main()
{
    /*threadPool thpool;
    system("pause");*/
    SocketMain();
}
