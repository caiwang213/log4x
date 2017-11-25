/**
 * @file      thread.h
 * @copyright Copyright (c) 2017, UT Technology Co., Ltd. All Rights Reserved.
 * @brief     brief
 * @author    caiwang213@qq.com
 * @date      2017-11-25 22:15:45
 *   
 * @note
 *  thread.h defines 
 */
#ifndef __THREAD_H__
#define __THREAD_H__
#include "semaphore.h"
#include <thread>

namespace log4x
{
class Thread
{
public:
    Thread();
    virtual ~Thread();

public:
    int                start();
    void               stop();

    int                wait();

protected:
    virtual void       run() = 0;

protected:
    void               entry();

protected:
    bool               _active;
    std::thread        _t;
    std::mutex         _mutex;
    Semaphore          _sem;
};

}
#endif
