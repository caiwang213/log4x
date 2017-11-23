#include "log4x.h"
#include <stdio.h>
#include <thread>

using namespace log4x;


bool run = true;

void f0(int n, const char *key)
{
    while (run)
    {
        /* LOG_TRACE(key, "thread: " << n); */
        LOGF_TRACE(key, "thread: %d", n);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void f1(int n, const char *key)
{
    while (run)
    {
        /* LOG_DEBUG(key, "thread: " << n); */
        LOGF_DEBUG(key, "thread: %d", n);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void f2(int n, const char *key)
{
    while (run)
    {
        /* LOG_INFO(key, "thread: " << n); */
        LOGF_INFO(key, "thread: %d", n);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void f3(int n, const char *key)
{
    while (run)
    {
        /* LOG_WARN(key, "thread: " << n); */
        LOGF_WARN(key, "thread: %d", n);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void f4(int n, const char *key)
{
    while (run)
    {
        LOG_ERROR(key, "thread: " << n);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void f5(int n, const char *key)
{
    while (run)
    {
        LOG_FATAL(key, "thread: " << n);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main(int argc, char *argv[])
{
    ilog4x *i =  ilog4x::instance();
    i->config("log.ini");
    i->start();

    std::thread t0(f0, 0, "main");
    std::thread t1(f1, 1, "test");
    std::thread t2(f2, 2, "test");
    std::thread t3(f3, 3, "main");
    std::thread t4(f4, 4, "main");
    std::thread t5(f5, 5, "main");

    getchar();

    run = false;

    t0.join();
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    i->stop();

    getchar();
    return 0;
}
