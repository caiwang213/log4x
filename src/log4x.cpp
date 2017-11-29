#include "log4x.h"
#include "file.h"
#include "stream.h"
#include "semaphore.h"
#include "thread.h"
#include <sstream>
#include <stdarg.h>
#include <sys/timeb.h>

#ifdef WIN32
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi")
#pragma warning(disable:4200)
#pragma warning(disable:4996)
#else
#include <sys/stat.h>
#include <dirent.h>
#endif

#ifdef __APPLE__
#include "TargetConditionals.h"
#include <dispatch/dispatch.h>
#if !TARGET_OS_IPHONE
#define LOG4X_HAVE_LIBPROC
#include <libproc.h>
#endif
#endif

/* base macro. */
#define LOG_STREAM(key, level, func, file, line, log) \
do \
{ \
    if (0 == prepush(key, level)) \
    {\
        log4x_t * __log = make(key, level); \
        Stream __ss(__log->buf + __log->len, LOG4X_LOG_BUF_SIZE - __log->len); \
        __ss << log; \
        __log->len += __ss.length(); \
        push(__log, func, file, line); \
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
static const char *const log4x_level[] =
{
    "TRA",
    "DBG",
    "INF",
    "WAR",
    "ERR",
    "FAT",
};

#ifdef WIN32
const static WORD log4x_color[] =
{
    0,
    FOREGROUND_GREEN,
    FOREGROUND_BLUE | FOREGROUND_GREEN,
    FOREGROUND_GREEN | FOREGROUND_RED,
    FOREGROUND_RED,
    FOREGROUND_RED | FOREGROUND_BLUE
};
#else
const static char *log4x_color[] =
{
    "\e[0m",
    "\e[32m",                                              /* green */
    "\e[34m\e[1m",                                         /* hight blue */
    "\e[33m",                                              /* yellow */
    "\e[31m",                                              /* red */
    "\e[35m"
};
#endif

/* log4x_t */
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


typedef std::list<std::pair<time_t, std::string> > log4x_history_t;
struct log4x_value_t
{
    bool           enable;
    std::string    key;
    std::string    path;
    int            level;
    bool           display;
    bool           outfile;
    bool           monthdir;
    long           limitsize;                              /* limit file's size. Mb */
    bool           fileline;
    long           reserve;                                /* history logs reserve time. second. */

    bool           fromfile;                               /* is from log.cfg file */
    File           fhandle;                                /* file handle. */
    time_t         ftime;                                  /* file create time */
    size_t         flen;                                   /* current file length */
    int            findex;                                 /* rolling file index */

    log4x_history_t history;                               /* history log files */

    log4x_value_t()
    {
        /* _curFileCreateDay = 0; */
        enable    = false;
        fromfile  = false;
        path      = LOG4X_DEFAULT_PATH;
        level     = LOG4X_DEFAULT_LEVEL;
        display   = LOG4X_DEFAULT_DISPLAY;
        outfile   = LOG4X_DEFAULT_OUTFILE;
        monthdir  = LOG4X_DEFAULT_MONTHDIR;
        limitsize = LOG4X_DEFAULT_LIMITSIZE;
        fileline  = LOG4X_DEFAULT_FILELINE;
        reserve   = LOG4X_DEFAULT_RESERVE;

        ftime     = 0;
        findex    = 0;
        flen      = 0;
    }
};

/**
 * log4x
 */

typedef std::map<std::string, log4x_value_t> log4x_keys_t;
typedef std::deque<log4x_t *>                log4x_queue_t;
typedef std::vector<log4x_t *>               log4x_pool_t;
/* typedef std::list<log4x_t *>                 log4x_pool_t; */
class log4x : public Thread, public ilog4x
{
public:
    log4x();
    virtual ~log4x();

protected:
    virtual int        config(const char * path);
    virtual int        configFromString(const char * str);

    virtual int        start();
    virtual void       stop();

    virtual log4x_t  * make(const char * key, int level);
    virtual void       free(log4x_t * log);
    virtual int        prepush(const char * key, int level);
    virtual int        push(log4x_t * log, const char * func, const char * file, int line);

    virtual int        push(const char * key, int level, const char * func, const char * file, int line, const char* fmt, ...);

    virtual int        enable(const char * key, bool enable);
    virtual int        setpath(const char * key, const char * path);
    virtual int        setlevel(const char * key, int level);
    virtual int        setfileLine(const char * key, bool enable);
    virtual int        setdisplay(const char * key, bool enable);
    virtual int        setoutFile(const char * key, bool enable);
    virtual int        setlimit(const char * key, unsigned int limitsize);
    virtual int        setmonthdir(const char * key, bool enable);
    virtual int        setReserve(const char * key, unsigned int sec);

    virtual void       setAutoUpdate(int interval);
    virtual int        update();
#if 0

    virtual bool       isEnable(const char * key) = 0;

    virtual long       totalCount()  = 0;
    virtual long       totalBytes()  = 0;
    virtual long       totalPush()   = 0;
    virtual long       totalPop()    = 0;
    virtual int        activeCount() = 0;
#endif
    virtual bool       isff(const char * key);
protected:
    virtual void       run();

protected:
    log4x_t          * pop();
    int                parse(const std::string &str);
    int                parse(const std::string &str, std::map<std::string, log4x_value_t> &values);
    void               parse(const std::string &line, int number, std::string &key, std::map<std::string, log4x_value_t> &values);

    int                split(const std::string &str, const std::string &delimiter, std::pair<std::string, std::string> &pair);
    void               trim(std::string &str, const std::string &ignore = std::string("\r\n\t "));

    int                open(log4x_t * log);
    void               close(const std::string &key);

    std::string        pname();
    std::string        pid();

    inline void        showColorText(const char * text, int level);
    inline bool        isdir(std::string &path);
    inline int         mkdir(std::string &path);

    inline tm          time2tm(time_t t);

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


    log4x_keys_t      _keys;
    std::mutex        _keys_mtx;

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
    _hotInterval = 30;
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
    stop();
}

int
log4x::config(const char * path)
{
    int result = -1;

    do
    {
        if (!_path.empty())
        {
            printf(" !!! !!! !!! !!!\r\n");
            printf(" !!! !!! log4x configure error: too many calls to Config. the old config file=%s,  the new config file=%s !!! !!! \r\n"
                   , _path.c_str(), path);
            printf(" !!! !!! !!! !!!\r\n");

            break;
        }

        _path = path;

        File f;
        f.open(_path.c_str(), "rb");
        if (!f.isOpen())
        {
            printf(" !!! !!! !!! !!!\r\n");
            printf(" !!! !!! log4x load config file error. filename=%s  !!! !!! \r\n", path);
            printf(" !!! !!! !!! !!!\r\n");

            break;
        }

        result = parse(f.readContent());
    }
    while (0);

    {
        /* add the main key default */
        std::lock_guard<std::mutex> locker(_keys_mtx);
        if (result < 0 || _keys.find("main") == _keys.end())
        {
            log4x_value_t val;

            val.enable    = true;
            val.level     = LOG_LEVEL_TRACE;
            val.key       = "main";
            _keys["main"] = val;
        }
    }

    return result;
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
        return 0;
    }

    _checksum = sum;

    std::map<std::string, log4x_value_t> values;
    if (parse(str, values) < 0)
    {
        printf(" !!! !!! !!! !!!\r\n");
        printf(" !!! !!! log4x load config file error \r\n");
        printf(" !!! !!! !!! !!!\r\n");

        return -1;
    }

    /* parse ok, we copy it */
    {
        std::lock_guard<std::mutex> locker(_keys_mtx);
        _keys.clear();

        _keys = values;
        /* _keys.swap(values); */

#if 0
        for (std::map<std::string, log4x_value_t>::const_iterator iter = _keys.begin(); iter != _keys.end(); ++iter)
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
            val.enable   = true;
            val.fromfile = true;
            val.key      = key;
            values[key]  = val;
        }
        else
        {
            printf("log4x configure warning: duplicate logger key:[%s] at line: %d \r\n", key.c_str(), number);
        }

        return;
    }

    trim(kv.first);
    trim(kv.second);

    std::map<std::string, log4x_value_t>::iterator iter = values.find(key);
    if (iter == values.end())
    {
        printf("log4x configure warning: not found current logger name:[%s] at line:%d, key=%s, value=%s \r\n",
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
            iter->second.fileline = false;
        }
        else
        {
            iter->second.fileline = true;
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
        iter->second.reserve = atol(kv.second.c_str());
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
    if (Thread::start() < 0)
    {
        return -1;
    }

    return _sem.wait(3000);
}

void
log4x::stop()
{
    Thread::stop();
}

int
log4x::prepush(const char * key, int level)
{
    if (!key || !_active)
    {
        return -1;
    }

    {
        std::lock_guard<std::mutex> locker(_keys_mtx);

        std::map<std::string, log4x_value_t>::const_iterator iter = _keys.find(key);
        if (iter == _keys.end())
        {
            char text[256] = {0};
            sprintf(text, "[%s] do not find the key: %s [%s:%d]\r\n",  __FUNCTION__, key, __FILE__, __LINE__);
            showColorText(text, LOG_LEVEL_WARN);

            log4x_value_t val;

            val.enable = true;
            val.key    = key;
            _keys[key] = val;
        }
        else
        {
            if (level < iter->second.level)
            {
                return -1;
            }
        }
    }

    {
        std::lock_guard<std::mutex> locker(_logs_mtx);
        size_t count = _logs.size();

        if (count > LOG4X_LOG_QUEUE_LIMIT_SIZE)
        {
            char text[256] = {0};
            sprintf(text, "[%s] count > LOG4X_LOG_QUEUE_LIMIT_SIZE [%s:%d]\r\n",  __FUNCTION__, __FILE__, __LINE__);
            showColorText(text, LOG_LEVEL_WARN);

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

    return 0;
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


    {
#if 0
#ifdef WIN32
        log->tid = GetCurrentThreadId();
#elif defined(__APPLE__)
        unsigned long long tid = 0;
        pthread_threadid_np(NULL, &tid);
        log->tid = (long)tid;
#else
        log->tid = (long)syscall(SYS_gettid);
#endif
#else
        /* note include <sstream> */
        std::stringstream ss;
        ss << std::this_thread::get_id();
        log->tid   = std::stol(ss.str()) & 0xFFFF;
#endif
        /* append precise time to log */
        timeb tb;
        ftime(&tb);

        log->key   = key;
        log->level = level;
        log->type  = LDT_GENERAL;
        log->value = 0;
        log->time  = tb.time;
        log->msec  = tb.millitm;
    }

    /* format log */
    {
        tm lt = time2tm(log->time);

        Stream ls(log->buf, LOG4X_LOG_BUF_SIZE);
        ls.writeChar('[');
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
        ls.writeChar(']');
        ls.writeChar('[');
        ls.writeULongLong(log->tid, 4);
        ls.writeChar(']');
        ls.writeChar('[');
        ls.writeString(log4x_level[log->level], strlen(log4x_level[log->level]));
        ls.writeChar(']');
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
    std::map<std::string, log4x_value_t>::iterator iter;
    {
        std::lock_guard<std::mutex> locker(_keys_mtx);
        iter = _keys.find(log->key);
        if (iter == _keys.end())
        {
            char text[256] = {0};
            sprintf(text, "[%s] do not find the key: %s [%s:%d]\r\n",  __FUNCTION__, log->key.c_str(), __FILE__, __LINE__);
            showColorText(text, LOG_LEVEL_WARN);

            free(log);

            return -1;
        }
    }

    log4x_value_t &value = iter->second;
    if (!_active || !value.enable)
    {
        free(log);

        return -1;
    }

    /* filter log */
    if (log->level < value.level)
    {
        free(log);

        return -1;
    }

    if (value.fileline && file)
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

        Stream ss(log->buf + log->len, LOG4X_LOG_BUF_SIZE - log->len);
        ss.writeChar(' ');
        ss.writeChar('[');
        ss.writeString(func, strlen(func));
        ss.writeChar(':');
        ss.writeString(begin, end - begin);
        ss.writeChar(':');
        ss.writeULongLong((unsigned long long)line);
        ss.writeChar(']');
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


    if (value.display && LOG4X_ALL_SYNCHRONOUS_OUTPUT)
    {
        showColorText(log->buf, log->level);
    }

    if (LOG4X_ALL_DEBUGOUTPUT_DISPLAY && LOG4X_ALL_SYNCHRONOUS_OUTPUT)
    {
#ifdef WIN32
        OutputDebugStringA(log->buf);
#endif
    }

    if (value.outfile && LOG4X_ALL_SYNCHRONOUS_OUTPUT)
    {
        std::lock_guard<std::mutex> locker(_keys_mtx);
        if (0 == open(log))
        {
            value.fhandle.write(log->buf, log->len);
            value.flen += log->len;

#if 0
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

int
log4x::push(const char * key, int level, const char * func, const char * file, int line, const char* fmt, ...)
{
    va_list args;

    if (0 == prepush(key, level))
    {
        log4x_t * log = make(key, level);

        va_start(args, fmt);
        int length = vsnprintf(log->buf + log->len, LOG4X_LOG_BUF_SIZE - log->len, fmt, args);
        va_end(args);

        if (length < 0)
        {
            length = 0;
        }

        if (length > (int)(LOG4X_LOG_BUF_SIZE - log->len))
        {
            length = LOG4X_LOG_BUF_SIZE - log->len;
        }
        log->len += length;

        return push(log, func, file, line);
    }

    return -1;
}

log4x_t *
log4x::pop()
{
    std::lock_guard<std::mutex> locker(_cache_mtx);
    log4x_t * log = NULL;
    if (_cache.empty())
    {
        if (!_logs.empty())
        {
            if (_logs.empty())
            {
                return NULL;
            }

            std::lock_guard<std::mutex> locker(_logs_mtx);
            _cache.swap(_logs);
        }
    }

    if (!_cache.empty())
    {
        log = _cache.front();
        _cache.pop_front();

        return log;
    }

    return NULL;
}

inline void
log4x::showColorText(const char *text, int level)
{
#if defined(WIN32) && defined(LOG4X_OEM_CONSOLE)
    char oem[LOG4X_LOG_BUF_SIZE] = { 0 };
    CharToOemBuffA(text, oem, LOG4X_LOG_BUF_SIZE);
#endif

    if (level < LOG_LEVEL_DEBUG || level > LOG_LEVEL_FATAL)
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
        std::map<std::string, log4x_value_t>::const_iterator iter = _keys.find(log->key);
        if (iter == _keys.end())
        {
            showColorText("log4x: openLogger can not open, invalide logger id! \r\n", LOG_LEVEL_FATAL);

            return -1;
        }
    }

    log4x_value_t * value = &_keys[log->key];

    if (!value->enable || !value->outfile || log->level < value->level)
    {
        return -1;
    }

    tm lt = time2tm(log->time);
    tm ct = time2tm(value->ftime);
    bool sameday = (
                       (lt.tm_year == ct.tm_year) &&
                       (lt.tm_mon  == ct.tm_mon)  &&
                       (lt.tm_mday == ct.tm_mday)
                   );

    bool newfile = value->flen > (size_t)(value->limitsize * 1024 * 1024);
    if (!sameday || newfile)
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

    if (!value->fhandle.isOpen())
    {
        value->ftime = log->time;
        value->flen  = 0;

        tm lt = time2tm(log->time);

        std::string key;
        std::string path;

        key  = value->key;
        path = value->path;


        char buf[500] = {0};
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
        if (!value->fhandle.isOpen() || flen < 0)
        {
            sprintf(buf, "log4x: can not open log file %s. \r\n", path.c_str());
            showColorText("!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n", LOG_LEVEL_FATAL);
            showColorText(buf, LOG_LEVEL_FATAL);
            showColorText("!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n", LOG_LEVEL_FATAL);
            value->outfile = false;

            return -1;
        }

        value->flen = flen;

        if (value->reserve > 0)
        {
            if (value->history.size() > LOG4X_RESERVE_FILE_COUNT)
            {
                while (!value->history.empty() && value->history.front().first < time(NULL) - value->reserve)
                {
                    value->fhandle.removeFile(value->history.front().second.c_str());
                    value->history.pop_front();
                }
            }

            if (value->history.empty() || value->history.back().second != path)
            {
                value->history.push_back(std::make_pair(time(NULL), path));
            }
        }

        return 0;
    }

    return 0;
}

void log4x::close(const std::string &key)
{
    {
        std::map<std::string, log4x_value_t>::const_iterator iter = _keys.find(key);
        if (iter == _keys.end())
        {
            showColorText("log4x: openLogger can not open, invalide logger id! \r\n", LOG_LEVEL_FATAL);

            return;
        }
    }

    log4x_value_t * value = &_keys[key];
    if (value->fhandle.isOpen())
    {
        value->fhandle.close();
    }
}


void
log4x::run()
{
    LOGI("-----------------  log4x thread started!   ----------------------------");

    for (std::map<std::string, log4x_value_t>::const_iterator iter = _keys.begin(); iter != _keys.end(); ++iter)
    {
        if (_keys[iter->first].enable)
        {
            LOGI("key: " << _keys[iter->first].key << "\t"
                 << "path: " << _keys[iter->first].path << "\t"
                 << "level: " << _keys[iter->first].level << "\t"
                 << "display: " << _keys[iter->first].display);
        }
    }

    _sem.post();

    log4x_t * log     = NULL;
    time_t    lastupd = time(NULL);

    std::map<std::string, bool> fflags;

    while (true)
    {
        while ((log = pop()))
        {
            std::map<std::string, log4x_value_t>::iterator iter;
            {
                std::lock_guard<std::mutex> locker(_keys_mtx);

                iter = _keys.find(log->key);
                if (iter == _keys.end())
                {
                    free(log);
                    continue;
                }
            }

            log4x_value_t &value = iter->second;
            if (log->type != LDT_GENERAL)
            {
                /* onHotChange(log->key, (log4x_tType)log->_type, log->_typeval, std::string(log->buf, log->len)); */
                /* value.fhandle.close(); */
                /* free(log); */
                continue;
            }

            /* _ullStatusTotalPolog ++; */

            /* discard */
            if (!value.enable || log->level < value.level)
            {
                free(log);
                continue;
            }


            if (value.display)
            {
                showColorText(log->buf, log->level);
            }

            if (LOG4X_ALL_DEBUGOUTPUT_DISPLAY)
            {
#ifdef WIN32
                OutputDebugStringA(log->buf);
#endif
            }

            if (value.outfile)
            {
                if (open(log) < 0)
                {
                    free(log);
                    continue;
                }

                value.fhandle.write(log->buf, log->len);
                value.flen += (unsigned int)log->len;
                fflags[log->key] = true;
                /* _ullStatusTotalWriteFileCount++; */
                /* _ullStatusTotalWriteFileBytes += log->len; */
            }
            else
            {
                /* _ullStatusTotalWriteFileCount++; */
                /* _ullStatusTotalWriteFileBytes += log->len; */
            }

            free(log);
        }

        {
            std::lock_guard<std::mutex> locker(_keys_mtx);
            for (std::map<std::string, log4x_value_t>::const_iterator iter = _keys.begin(); iter != _keys.end(); ++iter)
            {
                if (iter->second.enable && fflags[iter->second.key])
                {
                    _keys[iter->second.key].fhandle.flush();
                    fflags[iter->second.key] = false;
                }

                if (!iter->second.enable && fflags[iter->second.key])
                {
                    _keys[iter->second.key].fhandle.close();
                }
            }
        }

        if (!_active && _logs.empty())
        {
            break;
        }

        if (_hotInterval != 0 && (time(NULL) - lastupd) > _hotInterval)
        {
            update();
            lastupd = time(NULL);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}


int
log4x::enable(const char * key, bool enable)
{
    if (!key)
    {
        LOGE("invalid args");
    }

    /* The lock has been locked in LOGE, and it will deadlock */
    /* std::lock_guard<std::mutex> locker(_keys_mtx); */
    std::map<std::string, log4x_value_t>::iterator iter = _keys.find(key);
    if (iter == _keys.end())
    {
        LOGE("the key: " << key << " is not exist");

        return -1;
    }

    iter->second.enable = enable;

    return 0;
}

int
log4x::setpath(const char * key, const char * path)
{
    if (!key || !path)
    {
        LOGE("invalid args");
    }

    std::map<std::string, log4x_value_t>::iterator iter = _keys.find(key);
    if (iter == _keys.end())
    {
        LOGE("the key: " << key << " is not exist");

        return -1;
    }

    iter->second.path = path;

    return 0;
}

int
log4x::setlevel(const char * key, int level)
{
    if (!key)
    {
        LOGE("invalid args");
    }

    if (level < LOG_LEVEL_TRACE)
    {
        LOGW("invalid level value: " << level);
        level = LOG_LEVEL_TRACE;
    }

    if (level > LOG_LEVEL_FATAL)
    {
        LOGW("invalid level value: " << level);
        level = LOG_LEVEL_FATAL;
    }

    std::map<std::string, log4x_value_t>::iterator iter = _keys.find(key);
    if (iter == _keys.end())
    {
        LOGE("the key: " << key << " is not exist");

        return -1;
    }

    iter->second.level = level;

    return 0;
}

int
log4x::setfileLine(const char * key, bool enable)
{
    if (!key)
    {
        LOGE("invalid args");
    }

    std::lock_guard<std::mutex> locker(_keys_mtx);
    std::map<std::string, log4x_value_t>::iterator iter = _keys.find(key);
    if (iter == _keys.end())
    {
        LOGE("the key: " << key << " is not exist");

        return -1;
    }

    iter->second.fileline = enable;

    return 0;
}

int
log4x::setdisplay(const char * key, bool enable)
{
    if (!key)
    {
        LOGE("invalid args");
    }

    std::map<std::string, log4x_value_t>::iterator iter = _keys.find(key);
    if (iter == _keys.end())
    {
        LOGE("the key: " << key << " is not exist");

        return -1;
    }

    iter->second.display = enable;

    return 0;
}

int
log4x::setoutFile(const char * key, bool enable)
{
    if (!key)
    {
        LOGE("invalid args");
    }

    std::map<std::string, log4x_value_t>::iterator iter = _keys.find(key);
    if (iter == _keys.end())
    {
        LOGE("the key: " << key << " is not exist");

        return -1;
    }

    iter->second.outfile = enable;

    return 0;
}

int
log4x::setlimit(const char * key, unsigned int limitsize)
{
    if (!key)
    {
        LOGE("invalid args");
    }

    if (limitsize < 1)
    {
        LOGW("invalid limitsize value: " << limitsize);
        limitsize = 1;
    }

    if (limitsize > LOG4X_DEFAULT_LIMITSIZE)
    {
        LOGW("invalid limitsize value: " << limitsize);
        limitsize = LOG4X_DEFAULT_LIMITSIZE;
    }

    std::map<std::string, log4x_value_t>::iterator iter = _keys.find(key);
    if (iter == _keys.end())
    {
        LOGE("the key: " << key << " is not exist");

        return -1;
    }

    iter->second.limitsize = limitsize;

    return 0;
}

int
log4x::setmonthdir(const char * key, bool enable)
{
    if (!key)
    {
        LOGE("invalid args");
    }

    std::map<std::string, log4x_value_t>::iterator iter = _keys.find(key);
    if (iter == _keys.end())
    {
        LOGE("the key: " << key << " is not exist");

        return -1;
    }

    iter->second.monthdir = enable;

    return 0;
}

int
log4x::setReserve(const char * key, unsigned int sec)
{
    if (!key)
    {
        LOGE("invalid args");
    }

    if (sec < 30)
    {
        LOGW("invalid sec value: " << sec);
        sec = 30;
    }

    if (sec > LOG4X_DEFAULT_RESERVE * 7)
    {
        LOGW("invalid sec value: " << sec);
        sec = LOG4X_DEFAULT_RESERVE * 7;
    }

    std::map<std::string, log4x_value_t>::iterator iter = _keys.find(key);
    if (iter == _keys.end())
    {
        LOGE("the key: " << key << " is not exist");

        return -1;
    }

    iter->second.reserve = sec;

    return 0;
}

void
log4x::setAutoUpdate(int interval)
{
    _hotInterval = interval;
}

int
log4x::update()
{
    if (_path.empty())
    {
        return -1;
    }

    File f;
    f.open(_path.c_str(), "rb");
    if (!f.isOpen())
    {
        LOGE("!!! !!! log4x load config file error. filename " << _path);

        return -1;
    }

    return parse(f.readContent());
}

bool
log4x::isff(const char * key)
{
    if (!key)
    {
        LOGE("invalid args");
    }

    std::map<std::string, log4x_value_t>::iterator iter = _keys.find(key);
    if (iter == _keys.end())
    {
        LOGE("the key: " << key << " is not exist");

        return false;
    }

    return iter->second.fromfile;
}

std::string
log4x::pname()
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
    File i;
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

std::string
log4x::pid()
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

inline bool
log4x::isdir(std::string &path)
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

inline int
log4x::mkdir(std::string &path)
{
    if (0 == path.length())
    {
        return -1;
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

    return 0;
}

inline tm
log4x::time2tm(time_t t)
{
#ifdef WIN32
    tm tt = {0};
    localtime_s(&tt, &t);
    return tt;
#else
    tm tt = {0};
    localtime_r(&t, &tt);
    return tt;
#endif
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
