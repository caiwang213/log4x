#include "log4x.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>

/* #include <stdio.h> */
/* #include <stdlib.h> */
/* #include <errno.h> */
/* #include <time.h> */
/* #include <string.h> */
/* #include <math.h> */
/* #include <string> */
/* #include <vector> */
#include <map>
#include <list>
/* #include <algorithm> */
/* #include <iostream> */


#ifdef WIN32
/* #include <io.h> */
/* #include <shlwapi.h> */
/* #include <process.h> */
/* #pragma comment(lib, "shlwapi") */
/* #pragma comment(lib, "User32.lib") */
/* #pragma warning(disable:4996) */

#else
#include <unistd.h>
#include <fcntl.h>
/* #include <netinet/in.h> */
/* #include <arpa/inet.h> */
/* #include <sys/types.h> */
/* #include <sys/socket.h> */
/* #include <pthread.h> */
/* #include <sys/time.h> */
/* #include <sys/stat.h> */
/* #include <dirent.h> */
/* #include <semaphore.h> */
/* #include <sys/syscall.h> */
#endif


#ifdef __APPLE__
#include "TargetConditionals.h"
#include <dispatch/dispatch.h>
#if !TARGET_OS_IPHONE
#define LOG4Z_HAVE_LIBPROC
#include <libproc.h>
#endif
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


struct log4x_key_t
{
    bool           enable;
    std::string    key;
    std::string    path;
    int            level;
    bool           display;
    bool           outfile;
    bool           monthdir;
    long           limitsize;
    bool           fileLine;

    /* time_t         logReserveTime; */

    /* time_t         curFileCreateTime;    //file create time */
    /* time_t         _curFileCreateDay;    //file create day time */
    /* unsigned int   _curFileIndex; //rolling file index */
    /* unsigned int   _curWriteLen;  //current file length */

    file           handle;        //file handle.
    //!history
    /* std::list<std::pair<time_t, std::string> > _historyLogs; */


    log4x_key_t()
    {
        /* _curFileCreateTime = 0; */
        /* _curFileCreateDay = 0; */
        /* _curFileIndex = 0; */
        /* _curWriteLen = 0; */
        /* _logReserveTime = 0; */
        enable    = false;
        path      = LOG4X_DEFAULT_OUTFILE;
        level     = LOG4X_DEFAULT_LEVEL;
        display   = LOG4X_DEFAULT_DISPLAY;
        outfile   = LOG4X_DEFAULT_OUTFILE;
        monthdir  = LOG4X_DEFAULT_MONTHDIR;
        limitsize = LOG4X_DEFAULT_LIMITSIZE;
        fileLine  = LOG4X_DEFAULT_FILELINE;
    }
};



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

typedef std::map<std::string, log4x_key_t> loggers;

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
protected:
    virtual void       run();

protected:
    int                parse(const std::string &str);
    int                parse(const std::string &str, std::map<std::string, log4x_key_t> &keys);
    int                parse(const std::string &line, int number, std::string &key, std::map<std::string, log4x_key_t> &keys);
    int                parse(const std::string &line, int number, std::string &key, log4x_key_t &val);
    std::string        process()
    {
        return "";
    }

private:
    loggers           _loggers;
    int               _hotInterval;

    long              _totalCount;
    long              _totalBytes;
    long              _totalPush;
    long              _totalPop;
    int               _activeCount;

    std::string       _process;
    std::string       _path;

    unsigned long     _checksum;
};

log4x::log4x()
{
    _hotInterval = 0;
    _process     = process();
    _totalCount  = 0;
    _totalBytes  = 0;
    _totalPush   = 0;
    _totalPop    = 0;
    _activeCount = 0;
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
        return true;
    }
    _checksum = sum;


    std::map<std::string, log4x_key_t> keys;
    if (!parse(str, keys))
    {
        printf(" !!! !!! !!! !!!\r\n");
        printf(" !!! !!! log4z load config file error \r\n");
        printf(" !!! !!! !!! !!!\r\n");

        return -1;
    }

    for (std::map<std::string, log4x_key_t>::const_iterator iter = keys.begin(); iter != keys.end(); ++iter)
    {
        std::map<std::string, log4x_key_t>::iterator i = _loggers.find(iter->first);
        if (i != _loggers.end())
        {
            /* i->second.key = iter->second.key; */
            i->second = iter->second;
            continue;
        }

#if 0
        id = findLogger(iter->second._key.c_str());
        if (id == LOG4Z_INVALID_LOGGER_ID)
        {
            if (isUpdate)
            {
                continue;
            }
            else
            {
                id = createLogger(iter->second._key.c_str());
                if (id == LOG4Z_INVALID_LOGGER_ID)
                {
                    continue;
                }
            }
        }

        enableLogger(id, iter->second._enable);
        setLoggerName(id, iter->second._name.c_str());
        setLoggerPath(id, iter->second._path.c_str());
        setLoggerLevel(id, iter->second._level);
        setLoggerFileLine(id, iter->second._fileLine);
        setLoggerDisplay(id, iter->second._display);
        setLoggerOutFile(id, iter->second._outfile);
        setLoggerLimitsize(id, iter->second._limitsize);
        setLoggerMonthdir(id, iter->second._monthdir);
#endif
    }

    return 0;
}

int log4x::parse(const std::string &content, std::map<std::string, log4x_key_t> &keys)
{
    std::string key;
    int curLine = 1;
    std::string line;
    std::string::size_type curPos = 0;
    if (content.empty())
    {
        return 0;
    }
    do
    {
        std::string::size_type pos = std::string::npos;
        for (std::string::size_type i = curPos; i < content.length(); ++i)
        {
            //support linux/unix/windows LRCF
            if (content[i] == '\r' || content[i] == '\n')
            {
                pos = i;
                break;
            }
        }
        line = content.substr(curPos, pos - curPos);
        /* parseConfigLine(line, curLine, key, keys); */
        parse(line, curLine, key, keys);

        curLine++;

        if (pos == std::string::npos)
        {
            break;
        }
        else
        {
            curPos = pos + 1;
        }
    }
    while (1);

    return 0;
}

int
log4x::parse(const std::string &line, int number, std::string &key, log4x_key_t &val)
{

    return 0;
}

int log4x::parse(const std::string& line, int number, std::string & key, std::map<std::string, log4x_key_t> & keys)
{
#if 0
    std::pair<std::string, std::string> kv = splitPairString(line, "=");
    if (kv.first.empty())
    {
        return false;
    }

    trimLogConfig(kv.first);
    trimLogConfig(kv.second);
    if (kv.first.empty() || kv.first.at(0) == '#')
    {
        return true;
    }

    if (kv.first.at(0) == '[')
    {
        trimLogConfig(kv.first, "[]");
        key = kv.first;
        {
            std::string tmpstr = kv.first;
            std::transform(tmpstr.begin(), tmpstr.end(), tmpstr.begin(), ::tolower);
            if (tmpstr == "main")
            {
                key = "main";
            }
        }
        std::map<std::string, log4x_key_t>::iterator iter = keys.find(key);
        if (iter == keys.end())
        {
            log4x_key_t li;
            li._enable = true;
            li._key = key;
            li._name = key;
            keys.insert(std::make_pair(li._key, li));
        }
        else
        {
            printf("log4z configure warning: duplicate logger key:[%s] at line: %d \r\n", key.c_str(), number);
        }
        return true;
    }
    trimLogConfig(kv.first);
    trimLogConfig(kv.second);
    std::map<std::string, log4x_key_t>::iterator iter = keys.find(key);
    if (iter == keys.end())
    {
        printf("log4z configure warning: not found current logger name:[%s] at line:%d, key=%s, value=%s \r\n",
               key.c_str(), number, kv.first.c_str(), kv.second.c_str());
        return true;
    }
    std::transform(kv.first.begin(), kv.first.end(), kv.first.begin(), ::tolower);
    //! path
    if (kv.first == "path")
    {
        iter->second._path = kv.second;
        return true;
    }
    else if (kv.first == "name")
    {
        iter->second._name = kv.second;
        return true;
    }
    std::transform(kv.second.begin(), kv.second.end(), kv.second.begin(), ::tolower);
    //! level
    if (kv.first == "level")
    {
        if (kv.second == "trace" || kv.second == "all")
        {
            iter->second._level = LOG_LEVEL_TRACE;
        }
        else if (kv.second == "debug")
        {
            iter->second._level = LOG_LEVEL_DEBUG;
        }
        else if (kv.second == "info")
        {
            iter->second._level = LOG_LEVEL_INFO;
        }
        else if (kv.second == "warn" || kv.second == "warning")
        {
            iter->second._level = LOG_LEVEL_WARN;
        }
        else if (kv.second == "error")
        {
            iter->second._level = LOG_LEVEL_ERROR;
        }
        else if (kv.second == "alarm")
        {
            iter->second._level = LOG_LEVEL_ALARM;
        }
        else if (kv.second == "fatal")
        {
            iter->second._level = LOG_LEVEL_FATAL;
        }
    }
    //! display
    else if (kv.first == "display")
    {
        if (kv.second == "false" || kv.second == "0")
        {
            iter->second._display = false;
        }
        else
        {
            iter->second._display = true;
        }
    }
    //! output to file
    else if (kv.first == "outfile")
    {
        if (kv.second == "false" || kv.second == "0")
        {
            iter->second._outfile = false;
        }
        else
        {
            iter->second._outfile = true;
        }
    }
    //! monthdir
    else if (kv.first == "monthdir")
    {
        if (kv.second == "false" || kv.second == "0")
        {
            iter->second._monthdir = false;
        }
        else
        {
            iter->second._monthdir = true;
        }
    }
    //! limit file size
    else if (kv.first == "limitsize")
    {
        iter->second._limitsize = atoi(kv.second.c_str());
    }
    //! display log in file line
    else if (kv.first == "fileline")
    {
        if (kv.second == "false" || kv.second == "0")
        {
            iter->second._fileLine = false;
        }
        else
        {
            iter->second._fileLine = true;
        }
    }
    //! enable/disable one logger
    else if (kv.first == "enable")
    {
        if (kv.second == "false" || kv.second == "0")
        {
            iter->second._enable = false;
        }
        else
        {
            iter->second._enable = true;
        }
    }
    //! set reserve time
    else if (kv.first == "reserve")
    {
        iter->second._logReserveTime = atoi(kv.second.c_str());
    }

#endif
    return 0;
}

int
log4x::start()
{
    return  thread::start();
}

void
log4x::stop()
{
    thread::stop();
}

void
log4x::run()
{

    while (_active)
    {
        std::cout << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
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