#include "thread.h"
namespace log4x
{
Thread::Thread()
{
    _active = false;
}

Thread::~Thread()
{
}

int
Thread::start()
{
    std::lock_guard<std::mutex> locker(_mutex);
    if (_active)
    {
        return -1;
    }

    std::thread t(&Thread::entry, this);
    _t = std::move(t);

    return 0;
}

void
Thread::stop()
{
    std::lock_guard<std::mutex> locker(_mutex);
    if (!_active)
    {
        return;
    }

    _active = false;
    _t.join();
}

void
Thread::entry()
{
    {
        std::lock_guard<std::mutex> locker(_mutex);
        _active = true;
    }

    run();
}
}
