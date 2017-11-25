/**
 * @file      semaphore.h
 * @copyright Copyright (c) 2017, UT Technology Co., Ltd. All Rights Reserved.
 * @brief     brief
 * @author    caiwang213@qq.com
 * @date      2017-11-25 22:11:59
 *
 * @note
 *  semaphore.h defines
 */
#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__
#include <mutex>
#include <condition_variable>

typedef std::condition_variable cv_t;
namespace log4x
{
class Semaphore
{
public:
    Semaphore(long count = 0);

public:
    void               post();
    int                wait(int msec);

private:
    std::mutex         _mutex;
    cv_t               _cv;
    long               _count;
};
}
#endif
