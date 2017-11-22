#include "log4x.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <list>
#include <vector>
#include <queue>
#include <deque>
#include <map>
#include <algorithm>

/* #include <stdio.h> */
/* #include <stdlib.h> */
/* #include <errno.h> */
/* #include <time.h> */
/* #include <string.h> */
/* #include <math.h> */
/* #include <string> */
/* #include <iostream> */


#include <string.h>

#include <time.h>
#include <sys/timeb.h>
#ifdef WIN32
#include <windows.h>
/* #include <io.h> */
#include <shlwapi.h>
/* #include <process.h> */
#pragma comment(lib, "shlwapi")
/* #pragma comment(lib, "User32.lib") */
#pragma warning(disable:4996)

#else
#include <unistd.h>
#include <fcntl.h>
/* #include <netinet/in.h> */
/* #include <arpa/inet.h> */
/* #include <sys/types.h> */
/* #include <sys/socket.h> */
/* #include <pthread.h> */
#include <sys/stat.h>
#include <dirent.h>
/* #include <semaphore.h> */
/* #include <sys/syscall.h> */
#endif


#ifdef __APPLE__
#include "TargetConditionals.h"
#include <dispatch/dispatch.h>
#if !TARGET_OS_IPHONE
#define LOG4X_HAVE_LIBPROC
#include <libproc.h>
#endif
#endif

namespace log4x
{
static const char *const log4x_level[] =
{
    "TRA",
    "DBG",
    "INF",
    "WAR",
    "ERR",
    "FATL",
};

#ifdef WIN32
const static WORD log4x_color[LOG_LEVEL_FATAL + 1] =
{
    0,
    0,
    FOREGROUND_BLUE | FOREGROUND_GREEN,
    FOREGROUND_GREEN | FOREGROUND_RED,
    FOREGROUND_RED,
    /* FOREGROUND_GREEN, */
    FOREGROUND_RED | FOREGROUND_BLUE
};
#else
const static char *log4x_color[] =
{
    "\e[0m",
    "\e[0m",
    "\e[34m\e[1m",                                         /* hight blue */
    "\e[33m",                                              /* yellow */
    "\e[31m",                                              /* red */
    /* "\e[32m",                                              /1* green *1/ */
    "\e[35m"
};
#endif

#ifndef _MACRO
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
#endif

/**
 * file
 */
class file
{
public:
    file()
    {
        _file = NULL;
    }
    ~file()
    {
        close();
    }
    inline bool isOpen()
    {
        return _file != NULL;
    }

    inline long open(const char *path, const char * mod)
    {
        if (_file != NULL)
        {
            fclose(_file);
            _file = NULL;
        }

        _file = fopen(path, mod);
        if (_file)
        {
            long tel = 0;
            long cur = ftell(_file);
            fseek(_file, 0L, SEEK_END);
            tel = ftell(_file);
            fseek(_file, cur, SEEK_SET);
            return tel;
        }

        return -1;
    }
    inline void clean(int index, int len)
    {
#if !defined(__APPLE__) && !defined(WIN32)
        if (_file != NULL)
        {
            int fd = fileno(_file);
            fsync(fd);
            posix_fadvise(fd, index, len, POSIX_FADV_DONTNEED);
            fsync(fd);
        }
#endif
    }
    inline void close()
    {
        if (_file != NULL)
        {
            clean(0, 0);
            fclose(_file);
            _file = NULL;
        }
    }
    inline void write(const char * data, size_t len)
    {
        if (_file && len > 0)
        {
            if (fwrite(data, 1, len, _file) != len)
            {
                close();
            }
        }
    }
    inline void flush()
    {
        if (_file)
        {
            fflush(_file);
        }
    }

    inline std::string readLine()
    {
        char buf[500] = { 0 };
        if (_file && fgets(buf, 500, _file) != NULL)
        {
            return std::string(buf);
        }
        return std::string();
    }
    inline const std::string readContent();
    inline bool removeFile(const std::string & path)
    {
        return ::remove(path.c_str()) == 0;
    }
public:
    FILE *_file;
};

const std::string file::readContent()
{
    std::string content;

    if (!_file)
    {
        return content;
    }
    char buf[BUFSIZ];
    size_t ret = 0;
    do
    {
        ret = fread(buf, sizeof(char), BUFSIZ, _file);
        content.append(buf, ret);
    }
    while (ret == BUFSIZ);

    return content;
}


struct log4x_value_t
{
    bool           enable;
    std::string    key;
    std::string    path;
    int            level;
    bool           display;
    bool           outfile;
    bool           monthdir;
    long           limitsize;                              /* Mb */
    bool           fileLine;

    file           fhandle;                                /* file handle. */
    time_t         ftime;                                  /* file create time */
    size_t         flen;                                   /* current file length */
    int            findex;                                 /* rolling file index */

    /* time_t         logReserveTime; */

    /* time_t         curFileCreateTime;    //file create time */
    /* time_t         _curFileCreateDay;    //file create day time */

    //!history
    /* std::list<std::pair<time_t, std::string> > _historyLogs; */


    log4x_value_t()
    {
        /* _curFileCreateDay = 0; */
        enable    = false;
        path      = LOG4X_DEFAULT_PATH;
        level     = LOG4X_DEFAULT_LEVEL;
        display   = LOG4X_DEFAULT_DISPLAY;
        outfile   = LOG4X_DEFAULT_OUTFILE;
        monthdir  = LOG4X_DEFAULT_MONTHDIR;
        limitsize = LOG4X_DEFAULT_LIMITSIZE;
        fileLine  = LOG4X_DEFAULT_FILELINE;

        ftime     = 0;
        findex    = 0;
        flen      = 0;
        /* reserveTime = 0; */
    }
};

#ifndef _MACRO
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

/**
 * thread
 */
class thread
{
public:
    thread();
    virtual ~thread();

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
};

thread::thread()
{
    _active = false;
}

thread::~thread()
{
}

int
thread::start()
{
    std::thread t(&thread::entry, this);
    _t = std::move(t);

    return 0;
}

void
thread::stop()
{
    std::lock_guard<std::mutex> locker(_mutex);
    _active = false;
    _t.join();
}

void
thread::entry()
{
    {
        std::lock_guard<std::mutex> locker(_mutex);
        _active = true;
    }

    run();
}

/**
 * log4x
 */

typedef std::map<std::string, log4x_value_t> log4x_keys_t;
typedef std::deque<log4x_t *>                log4x_queue_t;
typedef std::vector<log4x_t *>               log4x_pool_t;
/* typedef std::list<log4x_t *>                 log4x_pool_t; */
class log4x : public thread, public ilog4x
{
public:
    log4x();
    virtual ~log4x();

public:
    virtual int        config(const char * path);
    virtual int        configFromString(const char * str);
#if 0
    virtual int        create(const char * key) = 0;
#endif

    virtual int        start();
    virtual void       stop();

    virtual log4x_t  * make(const char * key, int level);
    virtual void       free(log4x_t * log);
    virtual int        prepush(const char * key, int level);
    virtual int        push(log4x_t * log, const char * func, const char * file = "", int line = 0);
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
protected:
    virtual void       run();

protected:
    int                parse(const std::string &str);
    int                parse(const std::string &str, std::map<std::string, log4x_value_t> &values);
    void               parse(const std::string &line, int number, std::string &key, std::map<std::string, log4x_value_t> &values);

    int                split(const std::string &str, const std::string &delimiter, std::pair<std::string, std::string> &pair);
    void               trim(std::string &str, const std::string &ignore = std::string("\r\n\t "));
    void               showColorText(const char * text, int level);

    int                open(log4x_t * log);
    void               close(const std::string &key);

    std::string        pname()
    {
        std::string name = "process";
        char buf[260] = {0};
#ifdef WIN32
        if (GetModuleFileNameA(NULL, buf, 259) > 0)
        {
            name = buf;
        }
        std::string::size_type pos = name.rfind("\\");
        if (pos != std::string::npos)
        {
            name = name.substr(pos + 1, std::string::npos);
        }
        pos = name.rfind(".");
        if (pos != std::string::npos)
        {
            name = name.substr(0, pos - 0);
        }

#elif defined(LOG4X_HAVE_LIBPROC)
        proc_name(getpid(), buf, 260);
        name = buf;
        return name;;
#else
        sprintf(buf, "/proc/%d/cmdline", (int)getpid());
        file i;
        i.open(buf, "rb");
        if (!i.isOpen())
        {
            return name;
        }
        name = i.readLine();
        i.close();

        std::string::size_type pos = name.rfind("/");
        if (pos != std::string::npos)
        {
            name = name.substr(pos + 1, std::string::npos);
        }
#endif

        return name;
    }

    std::string        pid()
    {
        std::string pid = "0";
        char buf[260] = {0};
#ifdef WIN32
        DWORD winPID = GetCurrentProcessId();
        sprintf(buf, "%06u", winPID);
        pid = buf;
#else
        sprintf(buf, "%06d", getpid());
        pid = buf;
#endif
        return pid;
    }

    bool isdir(std::string &path)
    {
#ifdef WIN32
        return PathIsDirectoryA(path.c_str()) ? true : false;
#else
        DIR * pdir = opendir(path.c_str());
        if (pdir == NULL)
        {
            return false;
        }
        else
        {
            closedir(pdir);
            pdir = NULL;
            return true;
        }
#endif
    }

    int mkdir(std::string &path)
    {
        if (0 == path.length())
        {
            return 0;
        }
    
        for (std::string::iterator iter = path.begin(); iter != path.end(); ++iter)
        {
            if (*iter == '\\')
            {
                *iter = '/';
            }
        }
        if (path.at(path.length() - 1) != '/')
        {
            path.append("/");
        }

        std::string::size_type pos = path.find('/');
        while (pos != std::string::npos)
        {
            std::string cur = path.substr(0, pos - 0);
            if (cur.length() > 0 && !isdir(cur))
            {
                bool result = false;
#ifdef WIN32
                result = CreateDirectoryA(cur.c_str(), NULL) ? true : false;
#else
                result = (::mkdir(cur.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0);
#endif
                if (!result)
                {
                    return -1;
                }
            }

            pos = path.find('/', pos + 1);
        }

        return true;
    }

private:
    int               _hotInterval;

    long              _totalCount;
    long              _totalBytes;
    long              _totalPush;
    long              _totalPop;
    int               _activeCount;

    std::string       _pname;
    std::string       _pid;
    std::string       _path;

    unsigned long     _checksum;


    log4x_keys_t      _loggers;
    std::mutex        _loggers_mtx;

    log4x_queue_t     _logs;
    std::mutex        _logs_mtx;

    log4x_queue_t     _cache;
    std::mutex        _cache_mtx;

    log4x_pool_t      _pool;
    std::mutex        _pool_mtx;

    std::mutex        _show_mtx;
};

log4x::log4x()
{
    _hotInterval = 0;
    _totalCount  = 0;
    _totalBytes  = 0;
    _totalPush   = 0;
    _totalPop    = 0;
    _activeCount = 0;

    _pname       = pname();
    _pid         = pid();
}

log4x::~log4x()
{

}

class file;
int
log4x::config(const char * path)
{
    if (!_path.empty())
    {
        printf(" !!! !!! !!! !!!\r\n");
        printf(" !!! !!! log4z configure error: too many calls to Config. the old config file=%s,  the new config file=%s !!! !!! \r\n"
               , _path.c_str(), path);
        printf(" !!! !!! !!! !!!\r\n");

        return -1;
    }
    _path = path;

    file f;
    f.open(_path.c_str(), "rb");
    if (!f.isOpen())
    {
        printf(" !!! !!! !!! !!!\r\n");
        printf(" !!! !!! log4z load config file error. filename=%s  !!! !!! \r\n", path);
        printf(" !!! !!! !!! !!!\r\n");

        return -1;
    }

    return parse(f.readContent());
}

int log4x::configFromString(const char * str)
{
    return parse(str);
}

int log4x::parse(const std::string &str)
{
    unsigned long sum = 0;
    for (std::string::const_iterator iter = str.begin(); iter != str.end(); ++iter)
    {
        sum += (unsigned char) * iter;
    }

    if (sum == _checksum)
    {
        /* no changed */
        return true;
    }

    _checksum = sum;

    std::map<std::string, log4x_value_t> values;
    if (parse(str, values) < 0)
    {
        printf(" !!! !!! !!! !!!\r\n");
        printf(" !!! !!! log4z load config file error \r\n");
        printf(" !!! !!! !!! !!!\r\n");

        return -1;
    }

    /* parse ok, we copy it */
    {
        std::lock_guard<std::mutex> locker(_loggers_mtx);
        _loggers.clear();

        _loggers = values;

#if 1
        for (std::map<std::string, log4x_value_t>::const_iterator iter = _loggers.begin(); iter != _loggers.end(); ++iter)
        {
            std::cout << "key: " << iter->second.key.c_str() << "\tlevel: " << iter->second.level << std::endl;
        }
#endif
    }

    return 0;
}

int log4x::parse(const std::string &content, std::map<std::string, log4x_value_t> &values)
{
    std::string key;
    int curLine = 1;
    std::string line;
    std::string::size_type curPos = 0;

    if (content.empty())
    {
        return -1;
    }

    do
    {
        std::string::size_type pos = std::string::npos;
        for (std::string::size_type i = curPos; i < content.length(); ++i)
        {
            /* support linux/unix/windows LRCF */
            if (content[i] == '\r' || content[i] == '\n')
            {
                pos = i;
                break;
            }
        }

        line = content.substr(curPos, pos - curPos);
        parse(line, curLine, key, values);

        curLine++;

        if (pos == std::string::npos)
        {
            break;
        }

        curPos = pos + 1;
    }
    while (1);

    return 0;
}

void log4x::parse(const std::string& line, int number, std::string & key, std::map<std::string, log4x_value_t> & values)
{
    /* std::cout << "line: " << line << "number: " << number << std::endl; */
    std::pair<std::string, std::string> kv;
    split(line, "=", kv);

    if (kv.first.empty())
    {
        return;
    }

    trim(kv.first);
    trim(kv.second);

    if (kv.first.empty() || kv.first.at(0) == '#')
    {
        return;
    }

    /* std::cout << "kv.first: " << kv.first << "\tkv.second: " << kv.second << std::endl; */
    if (kv.first.at(0) == '[')
    {
        trim(kv.first, "[]");

        key = kv.first;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        /* std::cout << "key: " << key << std::endl; */

        std::map<std::string, log4x_value_t>::iterator iter = values.find(key);
        if (iter == values.end())
        {
            log4x_value_t val;
            val.enable  = true;
            val.key     = key;
            values[key] = val;
        }
        else
        {
            printf("log4z configure warning: duplicate logger key:[%s] at line: %d \r\n", key.c_str(), number);
        }

        return;
    }

    trim(kv.first);
    trim(kv.second);

    std::map<std::string, log4x_value_t>::iterator iter = values.find(key);
    if (iter == values.end())
    {
        printf("log4z configure warning: not found current logger name:[%s] at line:%d, key=%s, value=%s \r\n",
               key.c_str(), number, kv.first.c_str(), kv.second.c_str());
        return;
    }

    std::transform(kv.first.begin(), kv.first.end(), kv.first.begin(), ::tolower);

    /* path */
    if (kv.first == "path")
    {
        iter->second.path = kv.second;
        return;
    }
    else if (kv.first == "name")
    {
        /* iter->second._name = kv.second; */
        return;
    }

    std::transform(kv.second.begin(), kv.second.end(), kv.second.begin(), ::tolower);

    /* level */
    if (kv.first == "level")
    {
        if (kv.second == "trace" || kv.second == "all")
        {
            iter->second.level = LOG_LEVEL_TRACE;
        }
        else if (kv.second == "debug")
        {
            iter->second.level = LOG_LEVEL_DEBUG;
        }
        else if (kv.second == "info")
        {
            iter->second.level = LOG_LEVEL_INFO;
        }
        else if (kv.second == "warn" || kv.second == "warning")
        {
            iter->second.level = LOG_LEVEL_WARN;
        }
        else if (kv.second == "error")
        {
            iter->second.level = LOG_LEVEL_ERROR;
        }
        else if (kv.second == "fatal")
        {
            iter->second.level = LOG_LEVEL_FATAL;
        }
    }

    /* display */
    else if (kv.first == "display")
    {
        if (kv.second == "false" || kv.second == "0")
        {
            iter->second.display = false;
        }
        else
        {
            iter->second.display = true;
        }
    }

    /* output to file */
    else if (kv.first == "outfile")
    {
        if (kv.second == "false" || kv.second == "0")
        {
            iter->second.outfile = false;
        }
        else
        {
            iter->second.outfile = true;
        }
    }

    /* monthdir */
    else if (kv.first == "monthdir")
    {
        if (kv.second == "false" || kv.second == "0")
        {
            iter->second.monthdir = false;
        }
        else
        {
            iter->second.monthdir = true;
        }
    }

    /* limit file size */
    else if (kv.first == "limitsize")
    {
        iter->second.limitsize = atoi(kv.second.c_str());
    }

    /* display log in file line */
    else if (kv.first == "fileline")
    {
        if (kv.second == "false" || kv.second == "0")
        {
            iter->second.fileLine = false;
        }
        else
        {
            iter->second.fileLine = true;
        }
    }

    /* enable/disable one logger */
    else if (kv.first == "enable")
    {
        if (kv.second == "false" || kv.second == "0")
        {
            iter->second.enable = false;
        }
        else
        {
            iter->second.enable = true;
        }
    }

    /* set reserve time */
    else if (kv.first == "reserve")
    {
        /* iter->second._logReserveTime = atoi(kv.second.c_str()); */
    }
}

int
log4x::split(const std::string &str, const std::string &delimiter, std::pair<std::string, std::string> &pair)
{
    std::string::size_type pos = str.find(delimiter.c_str());
    if (pos == std::string::npos)
    {
        pair = std::make_pair(str, "");
        return 0;
    }

    pair = std::make_pair(str.substr(0, pos), str.substr(pos + delimiter.length()));

    return 0;
}

void
log4x::trim(std::string &str, const std::string &ignore)
{
    if (str.empty())
    {
        return;
    }

    size_t length = str.length();
    size_t begin  = 0;
    size_t end    = 0;

    /* trim utf8 file bom */
    if (str.length() >= 3
            && (unsigned char)str[0] == 0xef
            && (unsigned char)str[1] == 0xbb
            && (unsigned char)str[2] == 0xbf)
    {
        begin = 3;
    }

    /* trim character */
    for (size_t i = begin ; i < length; i++)
    {
        bool ischk = false;
        for (size_t j = 0; j < ignore.length(); j++)
        {
            if (str[i] == ignore[j])
            {
                ischk = true;
            }
        }
        if (ischk)
        {
            if (i == begin)
            {
                begin ++;
            }
        }
        else
        {
            end = i + 1;
        }
    }
    if (begin < end)
    {
        str = str.substr(begin, end - begin);
    }
    else
    {
        str.clear();
    }
}

int
log4x::start()
{
    return thread::start();
}

void
log4x::stop()
{
    thread::stop();
}

int
log4x::prepush(const char * key, int level)
{
    if (!key || !_active)
    {
        return -1;
    }

    log4x_value_t value;
    {
        std::lock_guard<std::mutex> locker(_loggers_mtx);

        std::map<std::string, log4x_value_t>::const_iterator iter = _loggers.find(key);
        if (iter == _loggers.end())
        {
            printf("[%s] do not find the key: %s\n",  __FUNCTION__, key);

            return -1;
        }

        value = iter->second;
    }

    if (level < value.level)
    {
        return -1;
    }

    {
        std::lock_guard<std::mutex> locker(_logs_mtx);
        size_t count = _logs.size();

        if (count > LOG4X_LOG_QUEUE_LIMIT_SIZE)
        {
            printf("[%s] count > LOG4X_LOG_QUEUE_LIMIT_SIZE\n", __FUNCTION__);

            size_t rate = (count - LOG4X_LOG_QUEUE_LIMIT_SIZE) * 100 / LOG4X_LOG_QUEUE_LIMIT_SIZE;
            if (rate > 100)
            {
                rate = 100;
            }

            if ((size_t)rand() % 100 < rate)
            {
                if (rate > 50)
                {
                    count = _logs.size();
                }

                if (count > LOG4X_LOG_QUEUE_LIMIT_SIZE)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(rate));
                }
            }
        }
    }

    return true;
}

enum
{
    LDT_GENERAL,
    LDT_ENABLE_LOGGER,
    LDT_SET_LOGGER_NAME,
    LDT_SET_LOGGER_PATH,
    LDT_SET_LOGGER_LEVEL,
    LDT_SET_LOGGER_FILELINE,
    LDT_SET_LOGGER_DISPLAY,
    LDT_SET_LOGGER_OUTFILE,
    LDT_SET_LOGGER_LIMITSIZE,
    LDT_SET_LOGGER_MONTHDIR,
    LDT_SET_LOGGER_RESERVETIME,
};

log4x_t *
log4x::make(const char * key, int level)
{
    log4x_t * log = NULL;

    {
        std::lock_guard<std::mutex> locker(_pool_mtx);
        if (!_pool.empty())
        {
            log = _pool.back();
            _pool.pop_back();
        }
    }

    if (!log)
    {
        /* log = (log4x_t *)malloc(sizeof(log4x_t) + LOG4X_LOG_BUF_SIZE); */
        log = new (malloc(sizeof(log4x_t) + LOG4X_LOG_BUF_SIZE))log4x_t();
    }



    /* append precise time to log */
    {
        /* note include <sstream> */
        std::stringstream ss;
        ss << std::this_thread::get_id();

        timeb tb;
        ftime(&tb);

        log->key   = key;
        log->level = level;
        log->type  = LDT_GENERAL;
        log->value = 0;
        log->tid   = std::stol(ss.str()) & 0xFFFF;

        log->time  = tb.time;
        log->msec  = tb.millitm;
    }

    /* format log */
    {
        tm lt;
#ifdef _WIN32
        localtime_s(&lt, &log->time);
#else
        localtime_r(&log->time, &lt);
#endif
#if 0
        printf("%d-%d-%d %d:%d:%d.%d\n",
               lt.tm_year + 1900,
               lt.tm_mon + 1,
               lt.tm_mday,
               lt.tm_hour,
               lt.tm_min,
               lt.tm_sec,
               log->msec);
#endif
        Log4zStream ls(log->buf, LOG4X_LOG_BUF_SIZE);
        ls.writeULongLong(lt.tm_year + 1900, 4);
        ls.writeChar('-');
        ls.writeULongLong(lt.tm_mon + 1, 2);
        ls.writeChar('-');
        ls.writeULongLong(lt.tm_mday, 2);
        ls.writeChar(' ');
        ls.writeULongLong(lt.tm_hour, 2);
        ls.writeChar(':');
        ls.writeULongLong(lt.tm_min, 2);
        ls.writeChar(':');
        ls.writeULongLong(lt.tm_sec, 2);
        ls.writeChar('.');
        ls.writeULongLong(log->msec, 3);
        ls.writeChar(' ');
        ls.writeChar('[');
        ls.writeULongLong(log->tid, 4);
        ls.writeChar(']');

        ls.writeChar(' ');
        ls.writeString(log4x_level[log->level], strlen(log4x_level[log->level]));
        ls.writeChar(' ');

        log->len = ls.length();
    }

    return log;
}

void
log4x::free(log4x_t * log)
{
    if (_pool.size() < 200)
    {
        std::lock_guard<std::mutex> locker(_pool_mtx);
        _pool.push_back(log);
    }
    else
    {
        log->~log4x_t();
        ::free(log);
    }
}

int
log4x::push(log4x_t * log, const char * func, const char * file, int line)
{
    /* discard log */
    {
        std::lock_guard<std::mutex> locker(_loggers_mtx);
        std::map<std::string, log4x_value_t>::const_iterator iter = _loggers.find(log->key);
        if (iter == _loggers.end())
        {
            printf("[%s] do not find the key: %s\n",  __FUNCTION__, log->key.c_str());

            free(log);

            return -1;
        }
    }

    if (!_active || !_loggers[log->key].enable)
    {
        free(log);

        return -1;
    }

    /* filter log */
    if (log->level < _loggers[log->key].level)
    {
        free(log);

        return -1;
    }

    if (_loggers[log->key].fileLine && file)
    {
        const char * end   = file + strlen(file);
        const char * begin = end;
        do
        {
            if (*begin == '\\' || *begin == '/')
            {
                begin++;
                break;
            }
            if (begin == file)
            {
                break;
            }
            begin--;
        }
        while (true);

        Log4zStream ss(log->buf + log->len, LOG4X_LOG_BUF_SIZE - log->len);
        ss.writeChar(' ');
        ss.writeString(begin, end - begin);
        ss.writeChar(':');
        ss.writeULongLong((unsigned long long)line);
        log->len += ss.length();
    }

    if (log->len + 3 > LOG4X_LOG_BUF_SIZE)
    {
        log->len = LOG4X_LOG_BUF_SIZE - 3;
    }
    log->buf[log->len + 0] = '\r';
    log->buf[log->len + 1] = '\n';
    log->buf[log->len + 2] = '\0';
    log->len += 2;


    if (_loggers[log->key].display && LOG4X_ALL_SYNCHRONOUS_OUTPUT)
    {
        showColorText(log->buf, log->level);
    }

    if (LOG4X_ALL_DEBUGOUTPUT_DISPLAY && LOG4X_ALL_SYNCHRONOUS_OUTPUT)
    {
#ifdef WIN32
        OutputDebugStringA(log->buf);
#endif
    }

    if (_loggers[log->key].outfile && LOG4X_ALL_SYNCHRONOUS_OUTPUT)
    {
        std::lock_guard<std::mutex> locker(_loggers_mtx);
        if (0 == open(log))
        {
#if 0
            _loggers[log->key].fhandle.write(log->buf, log->len);
            _loggers[log->key].flen += log->len;

            /* close(log->key); */
            /* _ullStatusTotalWriteFileCount++; */
            /* _ullStatusTotalWriteFileBytes += log->len; */
#endif
        }
    }

    if (LOG4X_ALL_SYNCHRONOUS_OUTPUT)
    {
        free(log);

        return 0;
    }

    {
        std::lock_guard<std::mutex> locker(_logs_mtx);
        _logs.push_back(log);
        /* _ullStatusTotalPushLog ++; */
    }


    return 0;
}

void
log4x::showColorText(const char *text, int level)
{

#if defined(WIN32) && defined(LOG4X_OEM_CONSOLE)
    char oem[LOG4X_LOG_BUF_SIZE] = { 0 };
    CharToOemBuffA(text, oem, LOG4X_LOG_BUF_SIZE);
#endif

    if (level <= LOG_LEVEL_DEBUG || level > LOG_LEVEL_FATAL)
    {
#if defined(WIN32) && defined(LOG4X_OEM_CONSOLE)
        printf("%s", oem);
#else
        printf("%s", text);
#endif
        return;
    }
#ifndef WIN32
    printf("%s%s\e[0m", log4x_color[level], text);
#else
    std::lock_guard<std::mutex> locker(_show_mtx);
    HANDLE std = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (std == INVALID_HANDLE_VALUE)
    {
        return;
    }
    CONSOLE_SCREEN_BUFFER_INFO oldInfo;
    if (!GetConsoleScreenBufferInfo(std, &oldInfo))
    {
        return;
    }
    else
    {
        SetConsoleTextAttribute(std, log4x_color[level]);
#ifdef LOG4X_OEM_CONSOLE
        printf("%s", oem);
#else
        printf("%s", text);
#endif
        SetConsoleTextAttribute(std, oldInfo.wAttributes);
    }
#endif
    return;
}

int log4x::open(log4x_t * log)
{
    {
        std::map<std::string, log4x_value_t>::const_iterator iter = _loggers.find(log->key);
        if (iter == _loggers.end())
        {
            showColorText("log4z: openLogger can not open, invalide logger id! \r\n", LOG_LEVEL_FATAL);

            return -1;
        }
    }

    log4x_value_t * value = &_loggers[log->key];

    if (!value->enable || !value->outfile || log->level < value->level)
    {
        return -1;
    }

#if 0
    bool sameday = log->_time >= value->_curFileCreateDay && log->_time - value->_curFileCreateDay < 24 * 3600;
    bool needChageFile = value->flen > value->_limitsize * 1024 * 1024;
    if (!sameday || needChageFile)
    {
        if (!sameday)
        {
            value->findex = 0;
        }
        else
        {
            value->findex++;
        }
        if (value->fhandle.isOpen())
        {
            value->fhandle.close();
        }
    }
#endif

    if (!value->fhandle.isOpen())
    {
        value->ftime = log->time;
        value->flen  = 0;

#if 0
        tm t = timeToTm(value->ftime);
        if (true) //process day time
        {
            tm day = t;
            day.tm_hour = 0;
            day.tm_min = 0;
            day.tm_sec = 0;
            value->_curFileCreateDay = mktime(&day);
        }
#endif
        tm lt;
#ifdef _WIN32
        localtime_s(&lt, &log->time);
#else
        localtime_r(&log->time, &lt);
#endif

        std::string key;
        std::string path;

        key  = value->key;
        path = value->path;


        char buf[500] = { 0 };
        if (value->monthdir)
        {
            sprintf(buf, "%04d_%02d/", lt.tm_year + 1900, lt.tm_mon + 1);
            path += buf;
        }

        if (!isdir(path))
        {
            mkdir(path);
        }

        if (LOG4X_ALL_SYNCHRONOUS_OUTPUT)
        {
            sprintf(buf, "%s_%s_%04d%02d%02d%02d_%s_%03u.log",
                    _pname.c_str(), key.c_str(), lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
                    lt.tm_hour, _pid.c_str(), value->findex);
        }
        else
        {

            sprintf(buf, "%s_%s_%04d%02d%02d%02d%02d_%s_%03u.log",
                    _pname.c_str(), key.c_str(), lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
                    lt.tm_hour, lt.tm_min, _pid.c_str(), value->findex);
        }

        path += buf;

        long flen = value->fhandle.open(path.c_str(), "ab");
        printf("path -----------------%s\n", path.c_str());
#if 0
        if (!value->fhandle.isOpen() || flen < 0)
        {
            sprintf(buf, "log4z: can not open log file %s. \r\n", path.c_str());
            showColorText("!!!!!!!!!!!!!!!!!!!!!!!!!! \r\n", LOG_LEVEL_FATAL);
            showColorText(buf, LOG_LEVEL_FATAL);
            showColorText("!!!!!!!!!!!!!!!!!!!!!!!!!! \r\n", LOG_LEVEL_FATAL);
            value->outfile = false;

            return -1;
        }

        value->flen = flen;

#if 0
        if (value->_logReserveTime > 0)
        {
            if (value->_historyLogs.size() > LOG4X_FORCE_RESERVE_FILE_COUNT)
            {
                while (!value->_historyLogs.empty() && value->_historyLogs.front().first < time(NULL) - value->_logReserveTime)
                {
                    value->fhandle.removeFile(value->_historyLogs.front().second.c_str());
                    value->_historyLogs.pop_front();
                }
            }
            if (value->_historyLogs.empty() || value->_historyLogs.back().second != path)
            {
                value->_historyLogs.push_back(std::make_pair(time(NULL), path));
            }
        }
        return true;
#endif
#endif
    }

    return 0;
}

void log4x::close(const std::string &key)
{
    {
        std::map<std::string, log4x_value_t>::const_iterator iter = _loggers.find(key);
        if (iter == _loggers.end())
        {
            showColorText("log4z: openLogger can not open, invalide logger id! \r\n", LOG_LEVEL_FATAL);

            return;
        }
    }

    log4x_value_t * value = &_loggers[key];
    if (value->fhandle.isOpen())
    {
        value->fhandle.close();
    }
}

void
log4x::run()
{

#if 0
    while (_active)
    {
        std::stringstream ss;
        ss << std::this_thread::get_id();

        long tid = std::stol(ss.str());

        /* note: std::this_thread::get_id() will Consume a lot of time in windows. */
        /* std::cout << std::this_thread::get_id() << " " << tid << std::endl; */

        std::this_thread::sleep_for(std::chrono::seconds(1));

        /* make("test", 1); */
    }
#endif

    LOGI("-----------------  log4x thread started!   ----------------------------");
    LOGI("-----------------  log4x thread started!   ----------------------------");

    /* for (std::map<std::string, log4x_value_t>::const_iterator iter = _loggers.begin(); iter != _loggers.end(); ++iter) */
    /* { */
    /*     if (_loggers[iter->first].enable) */
    /*     { */
    /*         LOGI("key\t" << _loggers[iter->first].key << "\n" */
    /*              << "path\t" << _loggers[iter->first].path << "\n" */
    /*              << "level\t" << _loggers[iter->first].level << "\n" */
    /*              << "display\t" << _loggers[iter->first].display << "\n"); */
    /*     } */
    /* } */
#if 0
    _semaphore.post();


    LogData * log = NULL;
    int needFlush[LOG4X_LOGGER_MAX] = {0};
    time_t lastCheckUpdate = time(NULL);


    while (_active)
    {
        while (polog(log))
        {
            if (log->key < 0 || log->key > _lastId)
            {
                free(log);
                continue;
            }
            LoggerInfo & curLogger = _loggers[log->key];

            if (log->_type != LDT_GENERAL)
            {
                onHotChange(log->key, (LogDataType)log->_type, log->_typeval, std::string(log->buf, log->len));
                curLogger.fhandle.close();
                free(log);
                continue;
            }

            //
            _ullStatusTotalPolog ++;
            //discard

            if (!curLogger._enable || log->level < curLogger.level)
            {
                free(log);
                continue;
            }


            if (curLogger._display)
            {
                showColorText(log->buf, log->level);
            }
            if (LOG4X_ALL_DEBUGOUTPUT_DISPLAY)
            {
#ifdef WIN32
                OutputDebugStringA(log->buf);
#endif
            }


            if (curLogger.outfile)
            {
                if (!openLogger(log))
                {
                    free(log);
                    continue;
                }

                curLogger.fhandle.write(log->buf, log->len);
                curLogger.flen += (unsigned int)log->len;
                needFlush[log->key] ++;
                _ullStatusTotalWriteFileCount++;
                _ullStatusTotalWriteFileBytes += log->len;
            }
            else
            {
                _ullStatusTotalWriteFileCount++;
                _ullStatusTotalWriteFileBytes += log->len;
            }

            free(log);
        }

        for (int i = 0; i <= _lastId; i++)
        {
            if (_loggers[i]._enable && needFlush[i] > 0)
            {
                _loggers[i].fhandle.flush();
                needFlush[i] = 0;
            }
            if (!_loggers[i]._enable && _loggers[i].fhandle.isOpen())
            {
                _loggers[i].fhandle.close();
            }
        }

        //! delay.
        sleepMillisecond(50);

        //! quit
        if (!_runing && _logs.empty())
        {
            break;
        }

        if (_hotUpdateInterval != 0 && time(NULL) - lastCheckUpdate > _hotUpdateInterval)
        {
            updateConfig();
            lastCheckUpdate = time(NULL);
        }



    }

#endif
    {

        std::lock_guard<std::mutex> locker(_loggers_mtx);
        for (std::map<std::string, log4x_value_t>::const_iterator iter = _loggers.begin(); iter != _loggers.end(); ++iter)
        {
            close(iter->first);
        }
    }
}

/**
 * ilog4x
 */
ilog4x *
ilog4x::instance()
{
    static log4x log;
    return &log;
}


}
