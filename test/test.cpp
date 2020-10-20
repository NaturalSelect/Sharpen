#include <cstdio>
#include <iostream>
#include <sharpen/AwaitableFuture.hpp>
#include <sharpen/AsyncOps.hpp>
#include <thread>
#include <ctime>

#ifdef SHARPEN_IS_WIN

namespace sharpen
{
    DWORD GetCurrentThreadId()
    {
        return ::GetCurrentThreadId();
    }
}

#else

#include <unistd.h>
#include <sys/syscall.h>

#define gettid() syscall(__NR_gettid)

namespace sharpen
{
    int GetCurrentThreadId()
    {
        return gettid();
    }
}

#endif

#define TEST_COUNT 1000000

void LaunchTest()
{
    for(size_t i = 0;i < TEST_COUNT;++i)
    {
        sharpen::Launch([](){
            //do nothing
        });
    }
}

void AwaitTest()
{
    for(size_t i = 0;i < TEST_COUNT;i++)
    {
        sharpen::AwaitableFuture<void> future;
        sharpen::Launch([&future](){
            future.Complete();
        });
        future.Await();
    }
}

int main(int argc, char const *argv[])
{
    std::printf("running in machine with %d cores\n",std::thread::hardware_concurrency());
    std::clock_t begin,end;
    //await test
    begin = std::clock();
    AwaitTest();
    end = std::clock();
    std::clock_t time = (end - begin)/CLOCKS_PER_SEC;
    std::printf("AwaitTest using %d sec\n",time);
    //launch test
    begin = std::clock();
    LaunchTest();
    end = std::clock();
    time = (end - begin)/CLOCKS_PER_SEC;
    std::printf("LaunchTest using %d sec\n",time);
    //multithreaded await test
    
    return 0;
}
