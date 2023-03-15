#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <Windows.h>
#include <deque>

class Threads
{
public:
    void run()
    {
        std::cout << std::this_thread::get_id() << std::endl;
    }
    void waitforturn()
    {
        std::unique_lock<std::mutex> lck{mtx};
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
        for (int i = 0; i < numberofthreads; i++)
        {
            threads.emplace_back(std::thread(&Threads::waitforturn, this));
            Sleep(200);
            cv.notify_one();
        }
    }
private:
    std::vector<std::thread> threads;
    std::mutex mtx;
    std::condition_variable cv;
};

class work
{
public:

private:

};

class threadPool
{
public:

private:
    std::deque<work> Tasks;
};

int main()
{
    Threads Th(std::thread::hardware_concurrency());
    Th.joinTh();
    system("pause");
}
