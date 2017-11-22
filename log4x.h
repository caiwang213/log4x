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

#define _MACRO
#ifdef  _MACRO
#include <stdlib.h>
#include <string>
/* #include <errno.h> */
/* #include <stdio.h> */
#include <string.h>
#include <math.h>
#include <cmath>
/* #include <stdlib.h> */
#include <list>
#include <vector>
#include <map>
#include <queue>
#include <deque>

#ifdef WIN32
/* #include <io.h> */
/* #include <shlwapi.h> */
/* #include <process.h> */
/* #pragma comment(lib, "shlwapi") */
/* #pragma comment(lib, "User32.lib") */
#pragma warning(disable:4996)
#endif

namespace log4x
{
struct log4x_t
{
    std::string    key;
    int            type;
    int            value;
    int            level;

    time_t         time;
    int            msec;
    long           tid;
    size_t         len;

    char           buf[0];
};
}
#endif


#define LOG4X_DEFAULT_PATH             ("./log/")
#define LOG4X_DEFAULT_LEVEL            (LOG_LEVEL_DEBUG)
#define LOG4X_DEFAULT_DISPLAY          (true)
#define LOG4X_DEFAULT_OUTFILE          (true)
#define LOG4X_DEFAULT_MONTHDIR         (false)
#define LOG4X_DEFAULT_LIMITSIZE        (100)
#define LOG4X_DEFAULT_FILELINE         (true)

#define LOG4X_FORCE_RESERVE_FILE_COUNT (7)
#define LOG4X_LOG_QUEUE_LIMIT_SIZE     (20000)
#define LOG4X_LOG_BUF_SIZE             (1024*4)
#define LOG4X_LOG_CONTAINER_DEPTH      (5)

/* #define LOG4X_ALL_SYNCHRONOUS_OUTPUT   (false) */
#define LOG4X_ALL_SYNCHRONOUS_OUTPUT   (true)
#define LOG4X_ALL_DEBUGOUTPUT_DISPLAY  (false)

enum
{
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
};



/* base macro. */
#define LOG_STREAM(key, level, func, file, line, log) \
do \
{ \
    if (log4x::ilog4x::instance()->prepush(key, level)) \
    {\
        log4x_t * __log = log4x::ilog4x::instance()->make(key, level); \
        Log4zStream __ss(__log->buf + __log->len, LOG4X_LOG_BUF_SIZE - __log->len); \
        __ss << log; \
        __log->len += __ss.length(); \
        log4x::ilog4x::instance()->push(__log, func, file, line); \
    } \
} while (0)


/* fast macro. */
#define LOG_TRACE(key, log) LOG_STREAM(key, LOG_LEVEL_TRACE, __FUNCTION__, __FILE__, __LINE__, log)
#define LOG_DEBUG(key, log) LOG_STREAM(key, LOG_LEVEL_DEBUG, __FUNCTION__, __FILE__, __LINE__, log)
#define LOG_INFO(key,  log) LOG_STREAM(key, LOG_LEVEL_INFO,  __FUNCTION__, __FILE__, __LINE__, log)
#define LOG_WARN(key,  log) LOG_STREAM(key, LOG_LEVEL_WARN,  __FUNCTION__, __FILE__, __LINE__, log)
#define LOG_ERROR(key, log) LOG_STREAM(key, LOG_LEVEL_ERROR, __FUNCTION__, __FILE__, __LINE__, log)
#define LOG_FATAL(key, log) LOG_STREAM(key, LOG_LEVEL_FATAL, __FUNCTION__, __FILE__, __LINE__, log)

/* super macro. */
#define LOGT(log) LOG_TRACE("main", log )
#define LOGD(log) LOG_DEBUG("main", log )
#define LOGI(log) LOG_INFO ("main", log )
#define LOGW(log) LOG_WARN ("main", log )
#define LOGE(log) LOG_ERROR("main", log )
#define LOGF(log) LOG_FATAL("main", log )


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

    virtual log4x_t  * make(const char * key, int level) = 0;
    virtual void       free(log4x_t * log) = 0;
    virtual int        prepush(const char * key, int level) = 0;
    virtual int        push(log4x_t * log, const char * func, const char * file = "", int line = 0) = 0;
#if 0
    virtual int        find(const char* key) = 0;


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

#ifdef _MACRO
class Log4zBinary
{
public:
    Log4zBinary(const void * buf, size_t len)
    {
        this->buf = (const char *)buf;
        this->len = len;
    }
    const char * buf;
    size_t  len;
};

class Log4zString
{
public:
    Log4zString(const char * buf, size_t len)
    {
        this->buf = (const char *)buf;
        this->len = len;
    }
    const char * buf;
    size_t  len;
};

class Log4zStream
{
public:
    inline Log4zStream(char * buf, int len);
    inline int length()
    {
        return (int)(_cur - _begin);
    }
public:
    inline Log4zStream & writeLongLong(long long t, int width = 0, int dec = 10);
    inline Log4zStream & writeULongLong(unsigned long long t, int width = 0, int dec = 10);
    inline Log4zStream & writeDouble(double t, bool isSimple);
    inline Log4zStream & writePointer(const void * t);
    inline Log4zStream & writeString(const char * t)
    {
        return writeString(t, strlen(t));
    };
    inline Log4zStream & writeString(const char * t, size_t len);
    inline Log4zStream & writeChar(char ch);
    inline Log4zStream & writeBinary(const Log4zBinary & t);
public:
    inline Log4zStream & operator <<(const void * t)
    {
        return  writePointer(t);
    }

    inline Log4zStream & operator <<(const char * t)
    {
        return writeString(t);
    }

    inline Log4zStream & operator <<(bool t)
    {
        return (t ? writeString("true", 4) : writeString("false", 5));
    }

    inline Log4zStream & operator <<(char t)
    {
        return writeChar(t);
    }

    inline Log4zStream & operator <<(unsigned char t)
    {
        return writeULongLong(t);
    }

    inline Log4zStream & operator <<(short t)
    {
        return writeLongLong(t);
    }

    inline Log4zStream & operator <<(unsigned short t)
    {
        return writeULongLong(t);
    }

    inline Log4zStream & operator <<(int t)
    {
        return writeLongLong(t);
    }

    inline Log4zStream & operator <<(unsigned int t)
    {
        return writeULongLong(t);
    }

    inline Log4zStream & operator <<(long t)
    {
        return writeLongLong(t);
    }

    inline Log4zStream & operator <<(unsigned long t)
    {
        return writeULongLong(t);
    }

    inline Log4zStream & operator <<(long long t)
    {
        return writeLongLong(t);
    }

    inline Log4zStream & operator <<(unsigned long long t)
    {
        return writeULongLong(t);
    }

    inline Log4zStream & operator <<(float t)
    {
        return writeDouble(t, true);
    }

    inline Log4zStream & operator <<(double t)
    {
        return writeDouble(t, false);
    }

    template<class _Elem, class _Traits, class _Alloc> //support std::string, std::wstring
    inline Log4zStream & operator <<(const std::basic_string<_Elem, _Traits, _Alloc> & t)
    {
        return writeString(t.c_str(), t.length());
    }

    inline Log4zStream & operator << (const Log4zBinary & binary)
    {
        return writeBinary(binary);
    }

    inline Log4zStream & operator << (const Log4zString & str)
    {
        return writeString(str.buf, str.len);
    }

    template<class _Ty1, class _Ty2>
    inline Log4zStream & operator <<(const std::pair<_Ty1, _Ty2> & t)
    {
        return *this << "pair(" << t.first << ":" << t.second << ")";
    }

    template<class _Elem, class _Alloc>
    inline Log4zStream & operator <<(const std::vector<_Elem, _Alloc> & t)
    {
        *this << "vector(" << t.size() << ")[";
        int inputCount = 0;
        for (typename std::vector<_Elem, _Alloc>::const_iterator iter = t.begin(); iter != t.end(); iter++)
        {
            inputCount++;
            if (inputCount > LOG4X_LOG_CONTAINER_DEPTH)
            {
                *this << "..., ";
                break;
            }
            *this << *iter << ", ";
        }
        if (!t.empty())
        {
            _cur -= 2;
        }
        return *this << "]";
    }

    template<class _Elem, class _Alloc>
    inline Log4zStream & operator <<(const std::list<_Elem, _Alloc> & t)
    {
        *this << "list(" << t.size() << ")[";
        int inputCount = 0;
        for (typename std::list<_Elem, _Alloc>::const_iterator iter = t.begin(); iter != t.end(); iter++)
        {
            inputCount++;
            if (inputCount > LOG4X_LOG_CONTAINER_DEPTH)
            {
                *this << "..., ";
                break;
            }
            *this << *iter << ", ";
        }
        if (!t.empty())
        {
            _cur -= 2;
        }
        return *this << "]";
    }

    template<class _Elem, class _Alloc>
    inline Log4zStream & operator <<(const std::deque<_Elem, _Alloc> & t)
    {
        *this << "deque(" << t.size() << ")[";
        int inputCount = 0;
        for (typename std::deque<_Elem, _Alloc>::const_iterator iter = t.begin(); iter != t.end(); iter++)
        {
            inputCount++;
            if (inputCount > LOG4X_LOG_CONTAINER_DEPTH)
            {
                *this << "..., ";
                break;
            }
            *this << *iter << ", ";
        }
        if (!t.empty())
        {
            _cur -= 2;
        }
        return *this << "]";
    }
    template<class _Elem, class _Alloc>
    inline Log4zStream & operator <<(const std::queue<_Elem, _Alloc> & t)
    {
        *this << "queue(" << t.size() << ")[";
        int inputCount = 0;
        for (typename std::queue<_Elem, _Alloc>::const_iterator iter = t.begin(); iter != t.end(); iter++)
        {
            inputCount++;
            if (inputCount > LOG4X_LOG_CONTAINER_DEPTH)
            {
                *this << "..., ";
                break;
            }
            *this << *iter << ", ";
        }
        if (!t.empty())
        {
            _cur -= 2;
        }
        return *this << "]";
    }
    template<class _K, class _V, class _Pr, class _Alloc>
    inline Log4zStream & operator <<(const std::map<_K, _V, _Pr, _Alloc> & t)
    {
        *this << "map(" << t.size() << ")[";
        int inputCount = 0;
        for (typename std::map < _K, _V, _Pr, _Alloc>::const_iterator iter = t.begin(); iter != t.end(); iter++)
        {
            inputCount++;
            if (inputCount > LOG4X_LOG_CONTAINER_DEPTH)
            {
                *this << "..., ";
                break;
            }
            *this << *iter << ", ";
        }
        if (!t.empty())
        {
            _cur -= 2;
        }
        return *this << "]";
    }

private:
    Log4zStream() {}
    Log4zStream(Log4zStream &) {}
    char *  _begin;
    char *  _end;
    char *  _cur;
};

inline Log4zStream::Log4zStream(char * buf, int len)
{
    _begin = buf;
    _end = buf + len;
    _cur = _begin;
}



inline Log4zStream & Log4zStream::writeLongLong(long long t, int width, int dec)
{
    if (t < 0)
    {
        t = -t;
        writeChar('-');
    }
    writeULongLong((unsigned long long)t, width, dec);
    return *this;
}

inline Log4zStream & Log4zStream::writeULongLong(unsigned long long t, int width, int dec)
{
    static const char * lut =
        "0123456789abcdef";

    static const char *lutDec =
        "00010203040506070809"
        "10111213141516171819"
        "20212223242526272829"
        "30313233343536373839"
        "40414243444546474849"
        "50515253545556575859"
        "60616263646566676869"
        "70717273747576777879"
        "80818283848586878889"
        "90919293949596979899";

    static const char *lutHex =
        "000102030405060708090A0B0C0D0E0F"
        "101112131415161718191A1B1C1D1E1F"
        "202122232425262728292A2B2C2D2E2F"
        "303132333435363738393A3B3C3D3E3F"
        "404142434445464748494A4B4C4D4E4F"
        "505152535455565758595A5B5C5D5E5F"
        "606162636465666768696A6B6C6D6E6F"
        "707172737475767778797A7B7C7D7E7F"
        "808182838485868788898A8B8C8D8E8F"
        "909192939495969798999A9B9C9D9E9F"
        "A0A1A2A3A4A5A6A7A8A9AAABACADAEAF"
        "B0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
        "C0C1C2C3C4C5C6C7C8C9CACBCCCDCECF"
        "D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
        "E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF"
        "F0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF";

    const unsigned long long cacheSize = 64;

    if ((unsigned long long)(_end - _cur) > cacheSize)
    {
        char buf[cacheSize];
        unsigned long long val = t;
        unsigned long long i = cacheSize;
        unsigned long long digit = 0;



        if (dec == 10)
        {
            do
            {
                const unsigned long long m2 = (unsigned long long)((val % 100) * 2);
                *(buf + i - 1) = lutDec[m2 + 1];
                *(buf + i - 2) = lutDec[m2];
                i -= 2;
                val /= 100;
                digit += 2;
            }
            while (val && i >= 2);
            if (digit >= 2 && buf[cacheSize - digit] == '0')
            {
                digit--;
            }
        }
        else if (dec == 16)
        {
            do
            {
                const unsigned long long m2 = (unsigned long long)((val % 256) * 2);
                *(buf + i - 1) = lutHex[m2 + 1];
                *(buf + i - 2) = lutHex[m2];
                i -= 2;
                val /= 256;
                digit += 2;
            }
            while (val && i >= 2);
            if (digit >= 2 && buf[cacheSize - digit] == '0')
            {
                digit--;
            }
        }
        else
        {
            do
            {
                buf[--i] = lut[val % dec];
                val /= dec;
                digit++;
            }
            while (val && i > 0);
        }

        while (digit < (unsigned long long)width)
        {
            digit++;
            buf[cacheSize - digit] = '0';
        }

        writeString(buf + (cacheSize - digit), (size_t)digit);
    }
    return *this;
}
inline Log4zStream & Log4zStream::writeDouble(double t, bool isSimple)
{

#if __cplusplus >= 201103L
    using std::isnan;
    using std::isinf;
#endif
    if (isnan(t))
    {
        writeString("nan", 3);
        return *this;
    }
    else if (isinf(t))
    {
        writeString("inf", 3);
        return *this;
    }



    size_t count = _end - _cur;
    double fabst = fabs(t);
    if (count > 30)
    {
        if (fabst < 0.0001 || (!isSimple && fabst > 4503599627370495ULL) || (isSimple && fabst > 8388607))
        {
            gcvt(t, isSimple ? 7 : 16, _cur);
            size_t len = strlen(_cur);
            if (len > count)
            {
                len = count;
            }
            _cur += len;
            return *this;
        }
        else
        {
            if (t < 0.0)
            {
                writeChar('-');
            }
            double intpart = 0;
            unsigned long long fractpart = (unsigned long long)(modf(fabst, &intpart) * 10000);
            writeULongLong((unsigned long long)intpart);
            if (fractpart > 0)
            {
                writeChar('.');
                writeULongLong(fractpart, 4);
            }
        }
    }

    return *this;
}

inline Log4zStream & Log4zStream::writePointer(const void * t)
{
    sizeof(t) == 8 ?  writeULongLong((unsigned long long)t, 16, 16) : writeULongLong((unsigned long long)t, 8, 16);
    return *this;
}

inline Log4zStream & Log4zStream::writeBinary(const Log4zBinary & t)
{
    writeString("\r\n\t[");
    for (size_t i = 0; i < (t.len / 32) + 1; i++)
    {
        writeString("\r\n\t");
        *this << (void*)(t.buf + i * 32);
        writeString(": ");
        for (size_t j = i * 32; j < (i + 1) * 32 && j < t.len; j++)
        {
            if (isprint((unsigned char)t.buf[j]))
            {
                writeChar(' ');
                writeChar(t.buf[j]);
                writeChar(' ');
            }
            else
            {
                *this << " . ";
            }
        }
        writeString("\r\n\t");
        *this << (void*)(t.buf + i * 32);
        writeString(": ");
        for (size_t j = i * 32; j < (i + 1) * 32 && j < t.len; j++)
        {
            writeULongLong((unsigned long long)(unsigned char)t.buf[j], 2, 16);
            writeChar(' ');
        }
    }

    writeString("\r\n\t]\r\n\t");
    return *this;
}
inline Log4zStream & Log4zStream::writeChar(char ch)
{
    if (_end - _cur > 1)
    {
        _cur[0] = ch;
        _cur++;
    }
    return *this;
}

inline Log4zStream & Log4zStream::writeString(const char * t, size_t len)
{
    size_t count = _end - _cur;
    if (len > count)
    {
        len = count;
    }
    if (len > 0)
    {
        memcpy(_cur, t, len);
        _cur += len;
    }

    return *this;
}
#endif

}

#endif
