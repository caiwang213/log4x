/**
 * @file      log4x.h
 * @copyright Copyright (c) 2017, UT Technology Co., Ltd. All Rights Reserved.
 * @brief     brief
 * @author    caiwang213@qq.com
 * @date      2017-11-21 10:08:58
 *
 * @note
 *  log4x.h defines
 */
#ifndef __LOG4X_H__
#define __LOG4X_H__

#include <stdlib.h>
/* #include <string> */
/* #include <errno.h> */
/* #include <stdio.h> */
/* #include <string.h> */
/* #include <math.h> */
/* #include <cmath> */
/* #include <stdlib.h> */
/* #include <vector> */
/* #include <map> */
/* #include <list> */
/* #include <queue> */
/* #include <deque> */

#ifdef _WIN32
#include <windows.h>
#endif

enum ENUM_LOG_LEVEL
{
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
};

#define LOG4X_DEFAULT_PATH             ("./log/")
#define LOG4X_DEFAULT_LEVEL            (LOG_LEVEL_DEBUG)
#define LOG4X_DEFAULT_DISPLAY          (true)
#define LOG4X_DEFAULT_OUTFILE          (true)
#define LOG4X_DEFAULT_MONTHDIR         (false)
#define LOG4X_DEFAULT_LIMITSIZE        (100)
#define LOG4X_DEFAULT_FILELINE         (true)
#define LOG4X_FORCE_RESERVE_FILE_COUNT (7)

/* log4x_info_t: */

/* stream; */
/* binary; */

namespace log4x
{

struct log4x_t;

class ilog4x
{
public:
    ilog4x() {};
    virtual ~ilog4x() {};

public:
    virtual int        config(const char * path)          = 0;
    virtual int        configFromString(const char * str) = 0;
#if 0
    virtual int        create(const char * key) = 0;
#endif

    virtual int        start() = 0;
    virtual void       stop()  = 0;

#if 0
    virtual int        find(const char* key) = 0;
    virtual log4x_t  * make(const char * key, int level) = 0;
    virtual void       free(log4x_t * log) = 0;

    virtual int        prepush(const char * key, int level) = 0;
    virtual int        push(log4x_t * log, const char * func, const char * file = "", int line = 0) = 0;

    virtual int        enable(const char * key, bool enable) = 0;
    virtual int        setname(const char * key, const char * name) = 0;
    virtual int        setpath(const char * key, const char * path) = 0;
    virtual int        setlevel(const char * key, int level) = 0;
    virtual int        setfileLine(const char * key, bool enable) = 0;
    virtual int        setdisplay(const char * key, bool enable) = 0;
    virtual int        setoutFile(const char * key, bool enable) = 0;
    virtual int        setlimit(const char * key, unsigned int limitsize) = 0;
    virtual int        setMonthdir(const char * key, bool enable) = 0;
    virtual int        setReserveTime(const char * key, time_t sec) = 0;
    virtual int        setAutoUpdate(int interval) = 0;
    virtual int        updateConfig() = 0;

    virtual bool       isEnable(const char * key) = 0;

    virtual long       totalCount()  = 0;
    virtual long       totalBytes()  = 0;
    virtual long       totalPush()   = 0;
    virtual long       totalPop()    = 0;
    virtual int        activeCount() = 0;
#endif

public:
    static ilog4x    * instance();
    static ilog4x    & reference()
    {
        return *instance();
    }
};
}

#endif
