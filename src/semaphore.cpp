#include "semaphore.h"
namespace log4x
{
Semaphore::Semaphore(long count)
    : _count(count)
{
}

void
Semaphore::post()
{
    std::unique_lock<std::mutex> lock(_mutex);
    ++_count;
    _cv.notify_one();
}

int
Semaphore::wait(int msec)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (_cv.wait_for(lock, std::chrono::milliseconds(msec), [ = ] { return _count > 0; }))
    {
        --_count;

        return 0;
    }

    return -1;
}
}
