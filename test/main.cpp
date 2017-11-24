#include "log4x.h"
#include <stdio.h>
#include <thread>

#define LOG_LEVEL(key, level) \
do \
{ \
    LOGF_FATAL(key, "set %s debug level %d", key, level); \
    log4x::ilog4x::instance()->setlevel(key, level); \
} while (0)

#define LOG_RELEASE_LEVEL(key, level) \
do \
{ \
    if (!log4x::ilog4x::instance()->isff(key)) \
    { \
        LOG_LEVEL(key, level); \
    } \
} while (0)


#ifdef _DEBUG
/* setlevel > fromfile */
#define LOG4X_DEBUG_LEVEL(key, level) LOG_LEVEL(key, level)
#else
/* fromfile > setlevel */
#define LOG4X_DEBUG_LEVEL(key, level) LOG_RELEASE_LEVEL(key, level)
#endif

using namespace log4x;

bool run = true;

void f0(int n, const char *key)
{
    while (run)
    {
        LOGF_TRACE(key, "thread: %d", n);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void f1(int n, const char *key)
{
    while (run)
    {
        LOGF_DEBUG(key, "thread: %d", n);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void f2(int n, const char *key)
{
    while (run)
    {
        LOGF_INFO(key, "thread: %d", n);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void f3(int n, const char *key)
{
    while (run)
    {
        LOGF_WARN(key, "thread: %d", n);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void f4(int n, const char *key)
{
    while (run)
    {
        LOGF_ERROR(key, "thread: %d", n);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void f5(int n, const char *key)
{
    while (run)
    {
        LOGF_FATAL(key, "thread: %d", n);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main(int argc, char *argv[])
{
    ilog4x *i =  ilog4x::instance();
    i->config("log.ini");
    int result = 0;
    result = i->start();
    result = i->start();

    /* LOG4X_DEBUG_LEVEL("main", LOG_LEVEL_DEBUG); */

    /* std::thread t0(f0, 0, "main"); */
    /* std::thread t1(f1, 1, "main"); */
    /* std::thread t2(f2, 2, "main"); */
    /* std::thread t3(f3, 3, "main"); */
    /* std::thread t4(f4, 4, "main"); */
    /* std::thread t5(f5, 5, "main"); */

    LOG4X_DEBUG_LEVEL("test", LOG_LEVEL_TRACE);
    std::thread tt0(f0, 0, "test");
    std::thread tt1(f1, 1, "test");
    std::thread tt2(f2, 2, "test");
    std::thread tt3(f3, 3, "test");
    std::thread tt4(f4, 4, "test");
    std::thread tt5(f5, 5, "test");

    getchar();

    run = false;

    /* t0.join(); */
    /* t1.join(); */
    /* t2.join(); */
    /* t3.join(); */
    /* t4.join(); */
    /* t5.join(); */

    tt0.join();
    tt1.join();
    tt2.join();
    tt3.join();
    tt4.join();
    tt5.join();

    i->stop();

    getchar();
    return 0;
}
